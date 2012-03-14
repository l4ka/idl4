#!/bin/bash
set -x
aclocal $ACLOCAL_FLAGS
autoheader
automake --add-missing --copy
autoconf
echo "Now create build directory, cd into it, and execute $(pwd)/configure $@"

