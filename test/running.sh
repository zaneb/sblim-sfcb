#!/bin/sh
if ! ps -C sfcbd >/dev/null
then
    if ps aux |grep valgrind |grep sfcbd > /dev/null
    then
	echo "  assuming sfcb is running under valgrind"
        exit 0
    fi
    echo "  FAILED sfcbd not running"
    exit 1
fi
exit 0
