#!/bin/sh

TOOLCHAIN="/var/toolchain/sys30"
DEST="myTunes/usr/libexec"
BINARY="$DEST/myTunes"

mkdir -p $DEST

gcc -I"$TOOLCHAIN/usr/include" -L"$TOOLCHAIN/usr/lib" -lsqlite3 -O3 -o $BINARY myTunes.c
strip $BINARY
ldid -S $BINARY

dpkg-deb -b myTunes
