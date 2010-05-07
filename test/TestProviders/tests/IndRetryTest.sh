#!/bin/sh
# ============================================================================
#
# (C) Copyright IBM Corp. 2010
#
# THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
# ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
# CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
#
# You can obtain a current copy of the Eclipse Public License from
# http://www.opensource.org/licenses/eclipse-1.0.php
#
# Author:       Michael Chase-Salerno <bratac@linux.vnet.ibm.com>
#
# Description:
#   Test program to verify indications get properly retried when they fail
#
# Returns 0 on success an non-zero on failure
#
# Depends on the GenMI.pl script and several XML files
# ===========================================================================

sendxml () {
      # Sends the xml file given as argument 1 to wbemcat with appropriate 
      # credentials and protocol. The output of wbemcat will be directed to 
      # argument 2
      if [ "$SFCB_TEST_USER" != "" ] && [ "$SFCB_TEST_PASSWORD" != "" ]; then
           wbemcat -u $SFCB_TEST_USER -pwd $SFCB_TEST_PASSWORD -p $SFCB_TEST_PORT -t $SFCB_TEST_PROTOCOL $1 2>&1 > $2
       else
           wbemcat -p $SFCB_TEST_PORT -t $SFCB_TEST_PROTOCOL $1 2>&1 > $2
       fi
       if [ $? -ne 0 ]; then
          echo "FAILED to send CIM-XML request $1"
          return 1
       fi
}
odir () {
    # Manages the output dir to control indication failures.
    # If the argument is "clean", deletes the directory
    # if it is "make", creates it
    if [ $1 = "clean" ]
    then
        if [ -d $ODIR ] 
        then
            rm -rf $ODIR
        fi
    else 
        if [ ! -d $ODIR ] 
        then
            mkdir -p $ODIR
        fi
    fi
}
cleanup () {
    # Cleanup created objects and files
    sendxml IndTest5DeleteSubscription.xml /dev/null
    sendxml IndTest6DeleteHandler.xml /dev/null
    sendxml IndTest7DeleteFilter.xml /dev/null
    odir clean
    if [ -f RIModIS.XML ]
    then
        rm RIModIS.XML 
    fi
}

sendInd () {
    # Sends an indication and checks if it was 
    # delivered. Returns 0 on successful delivery
    # and 1 on failure. The failure of delivery here
    # is not necessarily a test failure depending on the
    # test case.
    echo -n " initial ..."
    # Invoke method to generate the indication
    sendxml IndTest4CallMethod.xml /dev/null
    # Check if it was sent
    if [ -f $ODIR/SFCB_Listener.txt ]
    then
        return 0
    else
        return 1
    fi
}
init () {
    # Create Filter, Handler, Sub to setup indication
    sendxml IndTest1CreateFilter.xml /dev/null
    sendxml RICreateHandler.XML /dev/null
    sendxml IndTest3CreateSubscription.xml /dev/null
}

###
# Main
###

ODIR=/tmp/SFCBIndTest
RC=0

cleanup
init

# Get the IndicationService that GenMI.pl will use
sendxml RIEnumIS.XML ./RIEnumIS.result
if [ $? -ne 0 ] 
then
    echo " Failed to get IndicationService"
    exit 1 
fi

###
# Check that indications don't retry when disabled
###
echo -n  "  Disabled indication retries: "

./GenMI.pl 1 0 300 1
if [ $? -ne 0 ] 
then
    echo " GenMI.pl FAILED"
    exit 1 
fi
sendxml RIModIS.XML /dev/null

# No odir, so initial should fail 
odir clean
sendInd 
if [ $? -eq 0 ] 
then
    echo " FAILED"
    RC=1
else
    echo -n " retry ..."
    # make odir, but should still fail because 
    # retry is disabled
    odir make
    sleep 2
    if [ -f $ODIR/SFCB_Listener.txt ]
    then
        echo " FAILED"
        RC=1
    else 
        echo " PASSED"
    fi
fi

###
# Check that they do when enabled
###
echo -n  "  Enabled indication retries: "

./GenMI.pl 1 5 300 1
if [ $? -ne 0 ] 
then
    echo " GenMI.pl FAILED"
    exit 1; 
fi
sendxml RIModIS.XML /dev/null

# No odir, so initial should fail 
odir clean
sendInd 
if [ $? -eq 0 ] 
then
    echo " FAILED"
    RC=1
else
    # make odir, should retry and succeed
    echo -n " retry ..."
    odir make
    sleep 4
    if [ -f $ODIR/SFCB_Listener.txt ]
    then
        echo " PASSED"
    else 
        echo " FAILED"
        RC=1
    fi
fi


###
# Verify subscription gets disabled when the indication keeps failing
###
cleanup
init
echo -n  "  Subscription disable: "

./GenMI.pl 1 5 2 3
if [ $? -ne 0 ] 
then
    echo " GenMI.pl FAILED"
    exit 1; 
fi
sendxml RIModIS.XML /dev/null

# No odir, so initial should fail 
odir clean
sendInd 
if [ $? -eq 0 ] 
then
    RC=1 
    echo " FAILED"
else
    # Still no odir, so keeps failing, and should disable sub
    echo -n " disable ..."
    sleep 5
    sendxml RIGetSub.XML ./RIGetSubDisable.result
    grep -A1 '"SubscriptionState"' ./RIGetSubDisable.result | grep '<VALUE>4</VALUE>' >/dev/null 2>&1
    if [ $? -eq 1 ] 
    then
        RC=1 
        echo " FAILED"
    else
        echo " PASSED"
        rm ./RIGetSubDisable.result
    fi
fi


###
# Verify subscription gets removed when the indication keeps failing
###
cleanup
init
echo -n  "  Subscription Removal: "

./GenMI.pl 1 5 2 2
if [ $? -ne 0 ] 
then
    echo " GenMI.pl FAILED"
    exit 1; 
fi
sendxml RIModIS.XML /dev/null

# No odir, so initial should fail 
odir clean
sendInd 
if [ $? -eq 0 ] 
then
    RC=1 
    echo " FAILED"
else
    # Still no odir, so keeps failing, and should remove sub
    echo -n " remove ..."
    sleep 5
    sendxml RIGetSub.XML ./RIGetSubRemove.result
    grep '<VALUE>' ./RIGetSubRemove.result >/dev/null 2>&1
    if [ $? -eq 0 ] 
    then
        RC=1 
        echo " FAILED"
    else
        echo " PASSED"
        rm ./RIGetSubRemove.result
    fi
fi

cleanup

# Set Indication_Service back to the defaults
./GenMI.pl 20 3 2592000 2
if [ $? -ne 0 ] 
then
    echo " GenMI.pl FAILED"
    exit 1; 
fi
sendxml RIModIS.XML /dev/null

rm RIEnumIS.result

exit $RC
