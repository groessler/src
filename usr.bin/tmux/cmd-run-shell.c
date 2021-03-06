/* $OpenBSD: cmd-run-shell.c,v 1.43 2016/11/12 19:05:53 nicm Exp $ */

/*
 * Copyright (c) 2009 Tiago Cunha <me@tiagocunha.org>
 * Copyright (c) 2009 Nicholas Marriott <nicm@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <stdlib.h>
#include <string.h>

#include "tmux.h"

/*
 * Runs a command without a window.
 */

static enum cmd_retval	cmd_run_shell_exec(struct cmd *, struct cmdq_item *);

static void	cmd_run_shell_callback(struct job *);
static void	cmd_run_shell_free(void *);
static void	cmd_run_shell_print(struct job *, const char *);

const struct cmd_entry cmd_run_shell_entry = {
	.name = "run-shell",
	.alias = "run",

	.args = { "bt:", 1, 1 },
	.usage = "[-b] " CMD_TARGET_PANE_USAGE " shell-command",

	.tflag = CMD_PANE_CANFAIL,

	.flags = 0,
	.exec = cmd_run_shell_exec
};

struct cmd_run_shell_data {
	char			*cmd;
	struct cmdq_item	*item;
	int			 wp_id;
};

static void
cmd_run_shell_print(struct job *job, const char *msg)
{
	struct cmd_run_shell_data	*cdata = job->data;
	struct window_pane		*wp = NULL;
	struct cmd_find_state		 fs;

	if (cdata->wp_id != -1)
		wp = window_pane_find_by_id(cdata->wp_id);
	if (wp == NULL) {
		if (cdata->item != NULL) {
			cmdq_print(cdata->item, "%s", msg);
			return;
		}
		if (cmd_find_current (&fs, NULL, CMD_FIND_QUIET) != 0)
			return;
		wp = fs.wp;
		if (wp == NULL)
			return;
	}

	if (window_pane_set_mode(wp, &window_copy_mode) == 0)
		window_copy_init_for_output(wp);
	if (wp->mode == &window_copy_mode)
		window_copy_add(wp, "%s", msg);
}

static enum cmd_retval
cmd_run_shell_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args			*args = self->args;
	struct cmd_run_shell_data	*cdata;
	char				*shellcmd;
	struct session			*s = item->state.tflag.s;
	struct winlink			*wl = item->state.tflag.wl;
	struct window_pane		*wp = item->state.tflag.wp;
	struct format_tree		*ft;
	const char			*cwd;

	if (item->client != NULL && item->client->session == NULL)
		cwd = item->client->cwd;
	else if (s != NULL)
		cwd = s->cwd;
	else
		cwd = NULL;

	ft = format_create(item, 0);
	format_defaults(ft, item->state.c, s, wl, wp);
	shellcmd = format_expand(ft, args->argv[0]);
	format_free(ft);

	cdata = xcalloc(1, sizeof *cdata);
	cdata->cmd = shellcmd;

	if (args_has(args, 't') && wp != NULL)
		cdata->wp_id = wp->id;
	else
		cdata->wp_id = -1;

	if (!args_has(args, 'b'))
		cdata->item = item;

	job_run(shellcmd, s, cwd, cmd_run_shell_callback, cmd_run_shell_free,
	    cdata);

	if (args_has(args, 'b'))
		return (CMD_RETURN_NORMAL);
	return (CMD_RETURN_WAIT);
}

static void
cmd_run_shell_callback(struct job *job)
{
	struct cmd_run_shell_data	*cdata = job->data;
	char				*cmd = cdata->cmd, *msg, *line;
	size_t				 size;
	int				 retcode;
	u_int				 lines;

	lines = 0;
	do {
		if ((line = evbuffer_readline(job->event->input)) != NULL) {
			cmd_run_shell_print(job, line);
			free(line);
			lines++;
		}
	} while (line != NULL);

	size = EVBUFFER_LENGTH(job->event->input);
	if (size != 0) {
		line = xmalloc(size + 1);
		memcpy(line, EVBUFFER_DATA(job->event->input), size);
		line[size] = '\0';

		cmd_run_shell_print(job, line);
		lines++;

		free(line);
	}

	msg = NULL;
	if (WIFEXITED(job->status)) {
		if ((retcode = WEXITSTATUS(job->status)) != 0)
			xasprintf(&msg, "'%s' returned %d", cmd, retcode);
	} else if (WIFSIGNALED(job->status)) {
		retcode = WTERMSIG(job->status);
		xasprintf(&msg, "'%s' terminated by signal %d", cmd, retcode);
	}
	if (msg != NULL)
		cmd_run_shell_print(job, msg);
	free(msg);

	if (cdata->item != NULL)
		cdata->item->flags &= ~CMDQ_WAITING;
}

static void
cmd_run_shell_free(void *data)
{
	struct cmd_run_shell_data	*cdata = data;

	free(cdata->cmd);
	free(cdata);
}
