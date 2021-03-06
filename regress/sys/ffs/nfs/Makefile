# $OpenBSD: Makefile,v 1.3 2017/01/18 11:56:10 bluhm Exp $

TESTS=		chflags chmod chown link mkdir mkfifo open rename rmdir \
		symlink truncate unlink
CLEANFILES=	diskimage stamp-*

.PHONY: disk nfs mount unconfig clean

disk: unconfig
	dd if=/dev/zero of=diskimage bs=512 count=4k
	vnconfig vnd0 diskimage
	newfs vnd0c

nfs:
	grep '/mnt/regress-nfs-server\>' /etc/exports || \
	    echo /mnt/regress-nfs-server -maproot=0:0 127.0.0.1 >>/etc/exports
	rcctl -f start portmap
	rcctl -f start nfsd
	rcctl -f start mountd

mount: disk nfs
	mkdir -p /mnt/regress-nfs-server
	mount /dev/vnd0c /mnt/regress-nfs-server
	mkdir -p /mnt/regress-nfs-client
	mount -t nfs 127.0.0.1:/mnt/regress-nfs-server /mnt/regress-nfs-client

unconfig:
	-umount -f -t nfs -h 127.0.0.1 -a
	-rmdir /mnt/regress-nfs-client 2>/dev/null || true
	-pkill -KILL mountd || true
	-rcctl -f stop nfsd
	-rcctl -f stop portmap
	-umount -f /dev/vnd0c 2>/dev/null || true
	-rmdir /mnt/regress-nfs-server 2>/dev/null || true
	-vnconfig -u vnd0 2>/dev/null || true
	-rm -f stamp-setup

stamp-setup:
	@echo '\n======== $@ ========'
	${.MAKE} -C ${.CURDIR} mount
	date >$@

.for t in ${TESTS}
REGRESS_TARGETS+=	run-regress-${t}
run-regress-${t}: stamp-setup
	@echo '\n======== $@ ========'
	cd /mnt/regress-nfs-client && env FSTEST=${.OBJDIR}/../fstest \
	    /bin/sh ${.CURDIR}/../run ${.CURDIR}/../tests/${t}/*.t
.endfor

REGRESS_TARGETS+=	run-regress-cleanup
run-regress-cleanup:
	@echo '\n======== $@ ========'
	umount /mnt/regress-nfs-client
	${.MAKE} -C ${.CURDIR} unconfig

.include <bsd.regress.mk>
