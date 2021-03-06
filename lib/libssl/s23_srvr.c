/* $OpenBSD: s23_srvr.c,v 1.55 2017/01/23 08:48:44 beck Exp $ */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 *
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 *
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 *
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */
/* ====================================================================
 * Copyright (c) 1998-2006 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */

#include <stdio.h>

#include "ssl_locl.h"

#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/objects.h>

static int ssl23_get_client_hello(SSL *s);

int
ssl23_accept(SSL *s)
{
	void (*cb)(const SSL *ssl, int type, int val) = NULL;
	int ret = -1;
	int new_state, state;

	ERR_clear_error();
	errno = 0;

	if (s->internal->info_callback != NULL)
		cb = s->internal->info_callback;
	else if (s->ctx->internal->info_callback != NULL)
		cb = s->ctx->internal->info_callback;

	s->internal->in_handshake++;
	if (!SSL_in_init(s) || SSL_in_before(s))
		SSL_clear(s);

	for (;;) {
		state = s->internal->state;

		switch (s->internal->state) {
		case SSL_ST_BEFORE:
		case SSL_ST_ACCEPT:
		case SSL_ST_BEFORE|SSL_ST_ACCEPT:
		case SSL_ST_OK|SSL_ST_ACCEPT:

			s->server = 1;
			if (cb != NULL)
				cb(s, SSL_CB_HANDSHAKE_START, 1);

			/* s->version=SSL3_VERSION; */
			s->internal->type = SSL_ST_ACCEPT;

			if (!ssl3_setup_init_buffer(s)) {
				ret = -1;
				goto end;
			}
			if (!tls1_init_finished_mac(s)) {
				ret = -1;
				goto end;
			}

			s->internal->state = SSL23_ST_SR_CLNT_HELLO_A;
			s->ctx->internal->stats.sess_accept++;
			s->internal->init_num = 0;
			break;

		case SSL23_ST_SR_CLNT_HELLO_A:
		case SSL23_ST_SR_CLNT_HELLO_B:

			s->internal->shutdown = 0;
			ret = ssl23_get_client_hello(s);
			if (ret >= 0)
				cb = NULL;
			goto end;
			/* break; */

		default:
			SSLerr(SSL_F_SSL23_ACCEPT, SSL_R_UNKNOWN_STATE);
			ret = -1;
			goto end;
			/* break; */
		}

		if ((cb != NULL) && (s->internal->state != state)) {
			new_state = s->internal->state;
			s->internal->state = state;
			cb(s, SSL_CB_ACCEPT_LOOP, 1);
			s->internal->state = new_state;
		}
	}

end:
	s->internal->in_handshake--;
	if (cb != NULL)
		cb(s, SSL_CB_ACCEPT_EXIT, ret);

	return (ret);
}


int
ssl23_get_client_hello(SSL *s)
{
	char buf[11];
	/*
	 * sizeof(buf) == 11, because we'll need to request this many bytes in
	 * the initial read.
	 * We can detect SSL 3.0/TLS 1.0 Client Hellos ('type == 3') correctly
	 * only when the following is in a single record, which is not
	 * guaranteed by the protocol specification:
	 * Byte  Content
	 *  0     type            \
	 *  1/2   version          > record header
	 *  3/4   length          /
	 *  5     msg_type        \
	 *  6-8   length           > Client Hello message
	 *  9/10  client_version  /
	 */
	uint16_t client_version = 0;
	uint16_t shared_version;
	unsigned char *p, *d, *d_len, *dd;
	unsigned int i;
	unsigned int csl, sil, cl;
	int n = 0, j;
	int type = 0;

	if (s->internal->state == SSL23_ST_SR_CLNT_HELLO_A) {
		/* read the initial header */
		if (!ssl3_setup_buffers(s))
			return -1;

		n = ssl23_read_bytes(s, sizeof buf);
		if (n != sizeof buf)
			return(n);

		p = s->internal->packet;

		memcpy(buf, p, n);

		if ((p[0] & 0x80) && (p[2] == SSL2_MT_CLIENT_HELLO)) {
			/*
			 * SSLv2 header
			 */
			client_version = p[3] << 8 | p[4];

			if (!ssl_max_shared_version(s, client_version,
			    &shared_version))
				goto unsupported;

			s->version = shared_version;
			s->internal->state = SSL23_ST_SR_CLNT_HELLO_B;
		} else if ((p[0] == SSL3_RT_HANDSHAKE) &&
		    (p[1] == SSL3_VERSION_MAJOR) &&
		    (p[5] == SSL3_MT_CLIENT_HELLO) &&
		    ((p[3] == 0 && p[4] < 5 /* silly record length? */) ||
		    (p[9] >= p[1]))) {
			/*
			 * SSLv3 or tls1 header
			 */

			/* We must look at client_version inside the Client Hello message
			 * to get the correct minor version.
			 * However if we have only a pathologically small fragment of the
			 * Client Hello message, this would be difficult, and we'd have
			 * to read more records to find out.
			 * No known SSL 3.0 client fragments ClientHello like this,
			 * so we simply reject such connections to avoid
			 * protocol version downgrade attacks. */
			if (p[3] == 0 && p[4] < 6) {
				SSLerr(SSL_F_SSL23_GET_CLIENT_HELLO,
				    SSL_R_RECORD_TOO_SMALL);
				return -1;
			}
			client_version = p[9] << 8 | p[10];

			if (!ssl_max_shared_version(s, client_version,
			    &shared_version)) {
				if (s->options & SSL_OP_NO_TLSv1)
					goto unsupported;
				/*
				 * We won't be able to use TLS of course,
				 * but this will send an appropriate alert.
				 */
				shared_version = TLS1_VERSION;
			}
			s->version = shared_version;
			type = 3;
		} else if ((strncmp("GET ", (char *)p, 4) == 0) ||
		    (strncmp("POST ",(char *)p, 5) == 0) ||
		    (strncmp("HEAD ",(char *)p, 5) == 0) ||
		    (strncmp("PUT ", (char *)p, 4) == 0)) {
			SSLerr(SSL_F_SSL23_GET_CLIENT_HELLO, SSL_R_HTTP_REQUEST);
			return -1;
		} else if (strncmp("CONNECT", (char *)p, 7) == 0) {
			SSLerr(SSL_F_SSL23_GET_CLIENT_HELLO, SSL_R_HTTPS_PROXY_REQUEST);
			return -1;
		}
	}

	if (s->internal->state == SSL23_ST_SR_CLNT_HELLO_B) {
		/* we have SSLv3/TLSv1 in an SSLv2 header
		 * (other cases skip this state) */

		/*
		 * Limit the support of "backward compatible" headers
		 * only to "backward" versions of TLS. If we have moved
		 * on to modernity, just say no.
		 */
		if (s->options & SSL_OP_NO_TLSv1)
			goto unsupported;

		type = 2;
		p = s->internal->packet;
		client_version = p[3] << 8 | p[4];

		/* An SSLv3/TLSv1 backwards-compatible CLIENT-HELLO in an SSLv2
		 * header is sent directly on the wire, not wrapped as a TLS
		 * record. It's format is:
		 * Byte  Content
		 * 0-1   msg_length
		 * 2     msg_type
		 * 3-4   version
		 * 5-6   cipher_spec_length
		 * 7-8   session_id_length
		 * 9-10  challenge_length
		 * ...   ...
		 */
		n = ((p[0] & 0x7f) << 8) | p[1];
		if (n > (1024 * 4)) {
			SSLerr(SSL_F_SSL23_GET_CLIENT_HELLO, SSL_R_RECORD_TOO_LARGE);
			return -1;
		}
		if (n < 9) {
			SSLerr(SSL_F_SSL23_GET_CLIENT_HELLO,
			    SSL_R_RECORD_LENGTH_MISMATCH);
			return -1;
		}

		j = ssl23_read_bytes(s, n + 2);
		if (j != n + 2)
			return -1;

		tls1_finish_mac(s, s->internal->packet + 2, s->internal->packet_length - 2);
		if (s->internal->msg_callback)
			s->internal->msg_callback(0, SSL2_VERSION, 0, s->internal->packet + 2,
			    s->internal->packet_length - 2, s, s->internal->msg_callback_arg);

		p = s->internal->packet;
		p += 5;
		n2s(p, csl);
		n2s(p, sil);
		n2s(p, cl);
		d = (unsigned char *)s->internal->init_buf->data;
		if ((csl + sil + cl + 11) != s->internal->packet_length) {
			/*
			 * We can't have TLS extensions in SSL 2.0 format
			 * Client Hello, can we ? Error condition should be
			 * '>' otherwise
			 */
			SSLerr(SSL_F_SSL23_GET_CLIENT_HELLO,
			    SSL_R_RECORD_LENGTH_MISMATCH);
			return -1;
		}

		/* record header: msg_type ... */
		*(d++) = SSL3_MT_CLIENT_HELLO;
		/* ... and length (actual value will be written later) */
		d_len = d;
		d += 3;

		/* client_version */
		*(d++) = client_version >> 8;
		*(d++) = client_version & 0xff;

		/* lets populate the random area */
		/* get the challenge_length */
		i = (cl > SSL3_RANDOM_SIZE) ? SSL3_RANDOM_SIZE : cl;
		memset(d, 0, SSL3_RANDOM_SIZE);
		memcpy(&(d[SSL3_RANDOM_SIZE - i]), &(p[csl + sil]), i);
		d += SSL3_RANDOM_SIZE;

		/* no session-id reuse */
		*(d++) = 0;

		/* ciphers */
		j = 0;
		dd = d;
		d += 2;
		for (i = 0; i < csl; i += 3) {
			if (p[i] != 0)
				continue;
			*(d++) = p[i + 1];
			*(d++) = p[i + 2];
			j += 2;
		}
		s2n(j, dd);

		/* add in (no) COMPRESSION */
		*(d++) = 1;
		*(d++) = 0;

		i = (d - (unsigned char *)s->internal->init_buf->data) - 4;
		l2n3((long)i, d_len);

		/* get the data reused from the init_buf */
		S3I(s)->tmp.reuse_message = 1;
		S3I(s)->tmp.message_type = SSL3_MT_CLIENT_HELLO;
		S3I(s)->tmp.message_size = i;
	}

	/* imaginary new state (for program structure): */
	/* s->internal->state = SSL23_SR_CLNT_HELLO_C */

	if (type == 2 || type == 3) {
		/* we have SSLv3/TLSv1 (type 2: SSL2 style, type 3: SSL3/TLS style) */

		if (!ssl_init_wbio_buffer(s, 1))
			return -1;

		/* we are in this state */
		s->internal->state = SSL3_ST_SR_CLNT_HELLO_A;

		if (type == 3) {
			/* put the 'n' bytes we have read into the input buffer
			 * for SSLv3 */
			s->internal->rstate = SSL_ST_READ_HEADER;
			s->internal->packet_length = n;
			if (s->s3->rbuf.buf == NULL)
				if (!ssl3_setup_read_buffer(s))
					return -1;

			s->internal->packet = &(s->s3->rbuf.buf[0]);
			memcpy(s->internal->packet, buf, n);
			s->s3->rbuf.left = n;
			s->s3->rbuf.offset = 0;
		} else {
			s->internal->packet_length = 0;
			s->s3->rbuf.left = 0;
			s->s3->rbuf.offset = 0;
		}
		if (s->version == TLS1_2_VERSION)
			s->method = TLSv1_2_server_method();
		else if (s->version == TLS1_1_VERSION)
			s->method = TLSv1_1_server_method();
		else if (s->version == TLS1_VERSION)
			s->method = TLSv1_server_method();
		else
			goto unsupported;
		s->internal->handshake_func = s->method->ssl_accept;
	} else {
		/* bad, very bad */
		SSLerr(SSL_F_SSL23_GET_CLIENT_HELLO, SSL_R_UNKNOWN_PROTOCOL);
		return -1;
	}
	s->internal->init_num = 0;

	return (SSL_accept(s));

 unsupported:
	SSLerr(SSL_F_SSL23_GET_CLIENT_HELLO, SSL_R_UNSUPPORTED_PROTOCOL);
	return -1;
}
