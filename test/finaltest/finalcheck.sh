#!/bin/sh
if ! ps -C sfcbd >/dev/null
then
    if ps aux | grep valgrind | grep sfcbd > /dev/null
    then
        echo "  assuming sfcbd is running under valgrind"
    else
        echo "  FAILED sfcbd no longer running"
        exit 1
    fi
fi
echo "=================================================="
echo "******** ALL TESTS COMPLETED SUCCESSFULLY ********"
echo "=================================================="
exit 0
