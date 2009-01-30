#!/bin/sh
if ! ps -C sfcbd >/dev/null
then
    echo "  FAILED sfcbd no longer running"
    exit 1
else
    echo "=================================================="
    echo "******** ALL TESTS COMPLETED SUCCESSFULLY ********"
    echo "=================================================="
    exit 0
fi
