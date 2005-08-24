#!/bin/sh
aclocal &&
autoheader &&
libtoolize && 
automake -af &&
autoconf &&
#if test -d test; then cd test && ./autoconfiscate.sh && cd ..; fi
if test -d mofc; then cd mofc && ./autoconfiscate.sh; fi
