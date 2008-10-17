#!/bin/sh
# ============================================================================
# xmltest
#
# (C) Copyright IBM Corp. 2005
#
# THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
# ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
# CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
#
# You can obtain a current copy of the Eclipse Public License from
# http://www.opensource.org/licenses/eclipse-1.0.php
#
# Author:       Dr. Gareth S. Bestor, <bestorga@us.ibm.com>
# Contributors: Adrian Schuur, <schuur@de.ibm.com>
# Description:
#    Simple test program to send CIM-XML test request files to a CIMOM and
#    compare the returned results to an expected response file.
#    If test file is specified then run only that test. If test directory is
#    specified then run all tests in the directory in sorted order. If no test
#    file or directory specified then run all tests in the current directory.
# ============================================================================

_RC=0
TESTDIR=.

# Check for wbemcat utility
if ! which wbemcat > /dev/null; then
   echo "  Cannot find wbemcat. Please check your PATH"
   exit 1
fi
if ! touch ./testfile > /dev/null; then
   echo "  Cannot create files, check permissions"
   exit 1
fi
rm -f ./testfile

for xmlfile in `ls *xml`
do
   _TEST=${xmlfile%.xml}
   _TESTOK=$_TEST.OK
   _TESTLINES=$_TEST.lines
   _TESTRESULT=$_TEST.result
   #_TESTNAME=${_TEST##$TESTDIR/}
   _TESTNAME=$_TEST

   # Remove any old test result file
   rm -f $_TESTRESULT

   # Send the test CIM-XML to the CIMOM and save the response, 
   # stripping off the http header
   echo -n "  Testing $_TESTNAME..."
   wbemcat $xmlfile 2>&1 | awk '/<\?xml.*/{xml=1} {if (xml) print}' > $_TESTRESULT 
   if [ $? -ne 0 ]; then
      echo "FAILED to send CIM-XML request"
      _RC=1
      continue
   fi

   # Compare the response XML against the expected XML for differences
   # Either using a full copy of the expected output (testname.OK)
   if [ -f $_TESTOK ] ; then
        if ! diff --brief $_TESTOK $_TESTRESULT > /dev/null; then
            echo "FAILED output not as expected"
            _RC=1;
            continue

        # We got the expected response XML
        else
            echo "PASSED"
            rm -f $_TESTRESULT
        fi
   fi
   # or a file containing individual lines that must be found (testname.lines)
   if [ -f $_TESTLINES ] ; then
        passed=0
        while read line 
        do
            if ! grep --q "$line" $_TESTRESULT  ; then
                echo "FAILED line not found in result:"
                echo "\t$line"
                passed=1
                _RC=1;
            fi
        done < $_TESTLINES
        if [ $passed -eq 0 ] ;  then
            echo "PASSED"
            rm -f $_TESTRESULT
        fi
            
   fi
done

exit $_RC

