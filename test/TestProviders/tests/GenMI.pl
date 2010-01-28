#!/usr/bin/perl
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
#   Generate XML that will modify the retry properties of IndicationService
#   while preserving the other properties as they are.
#
#   Requires the xml output of EI IndicationService in the RIEnumIS.result 
#   file and will output MI xml in the RIModIS.XML file.
#  
#   Command line arguments:
#       <retry interval> <retry count> <removal interval> <removal action>
#
#   Returns 0 on success and non-zero on failure.
# ===========================================================================

# Get values, all 4 must always be specified.
($#ARGV == 3) 
    or die "Wrong arguments,\n  usage: <retry interval> <retry count> <removal interval> <removal action>\n";
$ri=$ARGV[0];
$ra=$ARGV[1];
$rmi=$ARGV[2];
$rma=$ARGV[3];

open (EI, "< ./RIEnumIS.result")
    or die "Couldn't open RIEnumIS.result\n";
open (MI, "> ./RIModIS.XML")
    or die "Couldn't open RIModIS.XML\n";

# print the header info
print MI '<?xml version="1.0" encoding="utf-8" ?>'."\n";
print MI '<CIM CIMVERSION="2.0" DTDVERSION="2.0">'."\n";
print MI '<MESSAGE ID="4711" PROTOCOLVERSION="1.0">'."\n";
print MI '<SIMPLEREQ>'."\n";
print MI '<IMETHODCALL NAME="ModifyInstance">'."\n";
print MI '<LOCALNAMESPACEPATH>'."\n";
print MI '<NAMESPACE NAME="root"></NAMESPACE>'."\n";
print MI '<NAMESPACE NAME="interop"></NAMESPACE>'."\n";
print MI '</LOCALNAMESPACEPATH>'."\n";
print MI '<IPARAMVALUE NAME="ModifiedInstance"><VALUE.NAMEDINSTANCE>'."\n";

# start reading through EI output
$mode=0;
while (<EI>) {
    if ($mode==0) {
        # Looking for start of the instance
        if (/<INSTANCENAME/) {
            print MI $_;
            $mode=1;
            next;
        }
    }
    if ($mode==1) {
        # In the instance, write out properties until the end 
        if (/\/INSTANCE>/){
            last;
        }
        # except the ones we want to change
        if ((/SubscriptionRemovalTimeInterval/) || (/SubscriptionRemovalAction/) || (/DeliveryRetryInterval/) || (/DeliveryRetryAttempts/) ) {
            $mode=2;
            next;
        }
        print MI $_;
    }
    if ($mode==2) {
        # Skipping over one of the properties we're changing
        if (/<\/PROPERTY>/) {
            $mode=1;
            next;
        }
    }
}

# Now write all our modified properties 
print MI '<PROPERTY NAME="DeliveryRetryInterval" TYPE="uint32">'."\n";
print MI '<VALUE>'.$ri.'</VALUE>'."\n";
print MI '</PROPERTY>'."\n";
print MI '<PROPERTY NAME="DeliveryRetryAttempts" TYPE="uint16">'."\n";
print MI '<VALUE>'.$ra.'</VALUE>'."\n";
print MI '</PROPERTY>'."\n";
print MI '<PROPERTY NAME="SubscriptionRemovalTimeInterval" TYPE="uint32">'."\n";
print MI '<VALUE>'.$rmi.'</VALUE>'."\n";
print MI '</PROPERTY>'."\n";
print MI '<PROPERTY NAME="SubscriptionRemovalAction" TYPE="uint16">'."\n";
print MI '<VALUE>'.$rma.'</VALUE>'."\n";
print MI '</PROPERTY>'."\n";

# and the footer
print MI "</INSTANCE>\n";
print MI "</VALUE.NAMEDINSTANCE>\n";
print MI "</IPARAMVALUE>\n";
print MI "</IMETHODCALL>\n";
print MI "</SIMPLEREQ>\n";
print MI "</MESSAGE>\n";
print MI "</CIM>\n";

close(EI);
close(MI);

exit 0;
