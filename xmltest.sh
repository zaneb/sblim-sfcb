#!/bin/sh

_RC=0

# Check for wbemcat utility
if ! which wbemcat > /dev/null; then
   echo "Cannot find wbemcat. Please check your PATH"
   exit 1
fi

# ------------------------------------------------------------------------------
function _runxmltest {
   typeset _TESTXML=$1

   _TEST=${_TESTXML%.xml}
   _TESTDIR=$( dirname $_TEST)
   _TESTOK=$_TEST.OK
   _TESTRESULT=$_TEST.result

   echo -n "Running test $_TESTXML ... "

   # Make sure we will be able to write the result file
   if [[ ! -w $_TESTDIR ]]; then
      echo "FAILED"; echo -e "\tCannot write to $_TESTDIR"
      continue
   fi

   # Remove any old test result file
   rm -f $_TESTRESULT

   # Send the test CIM-XML to the CIMOM and save the response, stripping off the http header
   wbemcat $_TESTXML | awk "{i++; if (i>7) print}" > $_TESTRESULT
   if [[ $? -ne 0 ]]; then
      echo "FAILED"; echo -e "\twbemcat failed to send CIM-XML request"
      _RC=1
      continue
   fi

   # If we dont yet have the expected result file, then save this response as the (new) expected result
   if [[ ! -f $_TESTOK ]]; then
      echo "OK"; echo -e "\tSaving response as $_TESTOK"
      mv $_TESTRESULT $_TESTOK
 
   # Compare the response XML against the expected XML for differences
   elif ! diff --brief $_TESTOK $_TESTRESULT > /dev/null; then
      echo "FAILED"; echo -e "\tCheck $_TESTRESULT for errors"
      _RC=1;
      continue

   # We got the expected response XML
   else
      echo "Passed"
      rm -f $_TESTRESULT
   fi
}
# ------------------------------------------------------------------------------

if [[ -n $1 ]]; then
   _runxmltest $1
else
   # Look for all *.xml test files and run them in sorted order (hence tests should be numbered)
   find -name "*.xml" | sort | while read _TESTXML; do
      _runxmltest $_TESTXML
      # Wait for the dust to settle before trying the next test...
      sleep 1
   done
fi

exit $_RC

