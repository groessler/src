#!/bin/sh
#	$OpenBSD: redirect.sh,v 1.3 2017/01/19 08:41:50 beck Exp $

: ${FTP:=ftp}

: ${rport1:=9000}
: ${rport2:=9001}

req1=$1
loc=$2
req2=$3

echo "Testing $req1 => $loc => $req2"

# Be sure to kill any previous nc running on our port
while pkill -fx "nc -l $rport1" && sleep 1; do done

echo "HTTP/1.0 302 Found\r\nLocation: $loc\r\n\r" | nc -l $rport1 >/dev/null &

# Wait for the "server" to start
until fstat | egrep 'nc[ ]+.*tcp 0x0 \*:9000' > /dev/null; do
sleep .1
done

res=$(${FTP} -o/dev/null $req1 2>&1 | sed '/^Redirected to /{s///;x;};$!d;x')

if [ X"$res" != X"$req2" ]; then
	echo "*** Fail; expected \"$req2\", got \"$res\""
	exit 1
fi
