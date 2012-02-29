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

# Indication flood limit (don't set less than 101)
lim=1000


sendxml () {
      # Sends the xml file given as argument 1 to wbemcat with appropriate 
      # credentials and protocol. The output of wbemcat will be directed to 
      # argument 2
      if [ -z $SFCB_TEST_PORT ]
      then
            SFCB_TEST_PORT=5988
      fi
      if [ -z $SFCB_TEST_PROTOCOL ]
      then
          SFCB_TEST_PROTOCOL="http"
      fi
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
    sendxml $SRCDIR/IndTest5DeleteSubscription.xml /dev/null
    sendxml $SRCDIR/IndTest6DeleteHandler.xml /dev/null
    sendxml $SRCDIR/IndTest7DeleteFilter.xml /dev/null
    odir clean
    if [ -f ./RIModIS.XML ]
    then
        rm ./RIModIS.XML 
    fi
}

sendInd () {
    # Sends an indication and checks if it was 
    # delivered. Returns 0 on successful delivery
    # and 1 on failure. The failure of delivery here
    # is not necessarily a test failure depending on the
    # test case.
    if [ $1 = "single" ]
    then
        echo -n " initial ..."
        # Invoke method to generate the indication
        sendxml $SRCDIR/IndTest4CallMethod.xml /dev/null
        sleep 5; # Wait due to deadlock prevention in localmode (indCIMXMLHandler.c)
    else
        sendxml $SRCDIR/IndTest4CallMethod.xml /dev/null
    fi
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
    sendxml $SRCDIR/IndTest1CreateFilter.xml /dev/null
    sendxml $SRCDIR/RICreateHandler.XML /dev/null
    sendxml $SRCDIR/IndTest3CreateSubscription.xml /dev/null
}

###
# Main
###

ODIR=/tmp/SFCBIndTest
RC=0

cleanup
init

# Get the IndicationService that GenMI.pl will use
sendxml $SRCDIR/RIEnumIS.XML ./RIEnumIS.result
if [ $? -ne 0 ] 
then
    echo " Failed to get IndicationService"
    exit 1 
fi

###
# Check that indications don't retry when disabled
###
echo -n  "  Disabled indication retries: "

$SRCDIR/GenMI.pl 1 0 300 1
if [ $? -ne 0 ] 
then
    echo " GenMI.pl FAILED"
    exit 1 
fi
sendxml ./RIModIS.XML /dev/null

# No odir, so initial should fail 
odir clean
sendInd single
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

$SRCDIR/GenMI.pl 1 5 300 1
if [ $? -ne 0 ] 
then
    echo " GenMI.pl FAILED"
    exit 1; 
fi
sendxml ./RIModIS.XML /dev/null

# No odir, so initial should fail 
odir clean
sendInd single
if [ $? -eq 0 ] 
then
    echo " FAILED"
    RC=1
else
    # make odir, should retry and succeed
    echo -n " retry ..."
    odir make
    sleep 10
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

$SRCDIR/GenMI.pl 1 5 2 3
if [ $? -ne 0 ] 
then
    echo " GenMI.pl FAILED"
    exit 1; 
fi
sendxml ./RIModIS.XML /dev/null

# No odir, so initial should fail 
odir clean
sendInd single
if [ $? -eq 0 ] 
then
    RC=1 
    echo " FAILED"
else
    # Still no odir, so keeps failing, and should disable sub
    echo -n " disable ..."
    sleep 10
    sendxml $SRCDIR/RIGetSub.XML ./RIGetSubDisable.result
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

$SRCDIR/GenMI.pl 1 5 2 2
if [ $? -ne 0 ] 
then
    echo " GenMI.pl FAILED"
    exit 1; 
fi
sendxml ./RIModIS.XML /dev/null

# No odir, so initial should fail 
odir clean
sendInd single
if [ $? -eq 0 ] 
then
    RC=1 
    echo " FAILED"
else
    # Still no odir, so keeps failing, and should remove sub
    echo -n " remove ..."
    sleep 10
    sendxml $SRCDIR/RIGetSub.XML ./RIGetSubRemove.result
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

###
# Flood
###
cleanup
init
echo -n  "  Indication flood: "
$SRCDIR/GenMI.pl 10 3 300 1
if [ $? -ne 0 ] 
then
    echo " GenMI.pl FAILED"
    exit 1; 
fi
sendxml ./RIModIS.XML /dev/null

i=0
j=0
while [ $j -lt $lim ]
do
    sendInd flood
    if [ $j -eq 100 ] # Let this many fail
    then
        odir make
    fi
    if [ $i -eq 50 ]
    then 
        echo -n "."
        i=0
    else
        i=$((i+1))
    fi
    j=$((j+1))
done
sleep 20 # Let the retries catch up
count=$(grep IndicationTime $ODIR/SFCB_Listener.txt  | wc -l)
if [ $count -eq $lim ] 
then
    echo " received $count of $lim: PASSED"
else
    echo " received $count of $lim: FAILED"
    RC=1
fi

# Check sequence numbers
echo -n  "  Indication flood sequence numbers: "
i=0
f=0
while [ $i -lt $((lim)) ]
do
    grep -A1 SequenceNumber $ODIR/SFCB_Listener.txt | grep '<VALUE>'$i'</VALUE>' > /dev/null 2>&1
    if [ $? -eq 0 ]
    then
        i=$((i+1))
    else
        f=$((f+1))
        i=$((i+1))
    fi
done

if [ $f -eq 0 ]
then 
    echo "PASSED"
else
    echo "$f missing: FAILED"
    RC=1
fi

###
# Cleanup and exit
###
cleanup
# Set Indication_Service back to the defaults
$SRCDIR/GenMI.pl 20 3 2592000 2
if [ $? -ne 0 ] 
then
    echo " GenMI.pl FAILED"
    exit 1; 
fi
sendxml ./RIModIS.XML /dev/null
rm RIEnumIS.result
exit $RC
