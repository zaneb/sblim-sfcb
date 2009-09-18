#!/bin/sh
# ============================================================================
# xmltest
#
# (C) Copyright IBM Corp. 2009
#
# THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
# ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
# CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
#
# You can obtain a current copy of the Eclipse Public License from
# http://www.opensource.org/licenses/eclipse-1.0.php
#
# Description:
# Adds classes for the test suite to the installed SFCB schema. 
# Also removes those classes when called with the "unstage" argument.
# ============================================================================

TEST_SCHEMA_DIR=$1
op=$2

if [ ! -d "$TEST_SCHEMA_DIR" ]; then
    echo "Usage: stageschema <schema-dir> [unstage]"
    exit 1
fi

if [ "$op" = "unstage" ]; then
    echo "Removing test schema. "
    # Check for sfcbunstage utility
    if ! which sfcbunstage > /dev/null ; then
        echo " Cannot find sfcbunstage. "
        exit 1
    else
        for moffile in `ls $TEST_SCHEMA_DIR/*.mof | cut -d '/' -f3`
        do
            sfcbunstage -s /usr/local/var/lib/sfcb/stage -n root/cimv2 $moffile
        done
        sfcbrepos -f
    fi

else

    echo "Adding test schema. "
    # Check for sfcbstage utility
    if ! which sfcbstage > /dev/null
    then
        echo " Cannot find sfcbstage. "
        exit 1
    else
        # Copy test schema files into the SFCB stage directory for testing.
        for regfile in `ls $TEST_SCHEMA_DIR/*.reg | cut -d '/' -f4`
        do
          moffile=`echo $regfile | sed 's/reg/mof/'`
          if [ -f $TEST_SCHEMA_DIR/$regfile ]; then
            sfcbstage -s /usr/local/var/lib/sfcb/stage -n root/cimv2 -r $TEST_SCHEMA_DIR/$regfile $TEST_SCHEMA_DIR/$moffile
          else
            sfcbstage -s /usr/local/var/lib/sfcb/stage -n root/cimv2 $TEST_SCHEMA_DIR/$moffile
          fi
        done
    fi

    # Check for sfcbrepos utility
    if ! which sfcbrepos > /dev/null ; then
        echo " Cannot find sfcbrepos. "
        exit 1
    else
        # Rebuild the repository
        sfcbrepos -f
    fi
fi

exit 0
