#!/bin/sh
RC=0

for cmd in  "sfcbrepos -h" \
    "sfcbmof -h" "sfcbstage -h" \
    "sfcbunstage -h" "sfcbuuid -h" \
    "wbemcat -h"
do
    echo -n "  Testing $cmd..."
    $cmd >tmpout 2>&1
    if [ $? -ne 0 ]; then
        RC=1 
        cat tmpout
    else 
        if ! grep -iq "usage" tmpout; then
            echo " Usage error"
            RC=1 
        fi
    fi
    echo "PASSED"
    rm -f tmpout
done

exit $RC
