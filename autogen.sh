#!/bin/bash

test -f ./Makefile && make distclean

libtoolize --force
aclocal
autoheader
automake --force-missing --add-missing
autoconf

./configure $@ && make -j${BUILDJOBS:-4} all
