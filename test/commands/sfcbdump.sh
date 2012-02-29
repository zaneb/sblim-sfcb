#!/bin/sh
RC=0
if ! sfcbdump $SRCDIR/classSchemas | grep Linux_CSProcessor > /dev/null
then
    RC=1 
fi

exit $RC
