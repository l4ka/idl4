#!/bin/sh

aclocal $ACLOCAL_FLAGS
autoheader
automake --add-missing --copy
autoconf
./configure $@

