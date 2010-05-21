#! /bin/sh

# Testcases for sfcb when slp is enabled using 'slptool'

success=0
failure=0
rc=0

slptool register service:test://localhost "(test-suite=sfcb-test suite)"
if [ "$?" = "0" ]; then
    success=$(($success+1))
    echo "Service registration: Success"
else
    failure=$((failure+1))
    echo "Service registration: Failure"
fi

if  [ "$?" = "0" ] ; then

   slptool unicastfindsrvs 127.0.0.1  service:test
   if [ "$?" = "0" ]; then
       success=$((success+1))
   else
       failure=$((failure+1))
   fi

   slptool unicastfindsrvs 127.0.0.1  service:test "(test-suite=sfcb-test suite)"
   if [ "$?" = "0" ]; then
       success=$((success+1))
   else
       failure=$((failure+1))
   fi

   slptool unicastfindattrs 127.0.0.1 service:test
   if [ "$?" = "0" ]; then
       success=$((success+1))
   else
       failure=$((failure+1))
   fi

   slptool unicastfindattrs 127.0.0.1 service:test://localhost test-suite 
   if [ "$?" = "0" ]; then
       success=$((success+1))
   else
       failure=$((failure+1))
   fi

   slptool deregister service:test://localhost
   if [ "$?" = "0" ]; then
       success=$((success+1))
       echo "Service deregistration: Success"
   else
       failure=$((failure+1))
       echo "Service deregistration: Failure" 
   fi

fi

echo $success tests succeeded : $failure tests failed
echo "***** SLP Tests Completed *****"


if [ $failure -gt 0 ]; then
	rc=1	
fi

exit $rc

