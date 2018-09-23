#!/bin/bash

set -x
set -e

if ! autoreconf -i; then
	aclocal
	autoheader
	automake --force-missing --add-missing
	autoconf
fi

./configure $@ && make clean && make -j${BUILDJOBS:-4} all
