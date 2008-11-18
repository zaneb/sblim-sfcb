#!/usr/bin/perl
$rc=0;

$output=`wbemcli ei http://localhost:5988/root/cimv2:bogusclass 2>&1 1>/dev/null`;
unless ($output =~ /CIM_ERR_INVALID_CLASS: Class not found/ ) {
    print "\tIncorrect error message for: \"Invalid class\".\n"; 
    $rc=1;
}

$output=`wbemcli ei http://localhost:5988/root/bogus:CIM_LogicalElement 2>&1 1>/dev/null`;
unless ($output =~ /CIM_ERR_INVALID_NAMESPACE: Invalid namespace/ ) {
    print "\tIncorrect error message for: \"Invalid namespace\".\n"; 
    $rc=1;
}


$output=`wbemcli bogus http://localhost:5988/root/cimv2:CIM_LogicalElement 2>&1 1>/dev/null`;
unless ($output =~ /Cmd Exception: Invalid operation/ ) {
    print "\tIncorrect error message for: \"Invalid operation\".\n"; 
    $rc=1;
}

exit $rc;


