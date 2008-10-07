#!/bin/sh
if ! ps -C sfcbd >/dev/null
then
    echo "  FAILED sfcbd not running"
    exit 1
fi
exit 0
