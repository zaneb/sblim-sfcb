#!/usr/bin/perl
$rc=0;

$port=$ENV{"SFCB_TEST_PORT"};
$protocol=$ENV{"SFCB_TEST_PROTOCOL"};

$user=$ENV{"SFCB_TEST_USER"};
$password=$ENV{"SFCB_TEST_PASSWORD"};

my $cred_host="";

if ( $user && $password )
{
    $cred_host = $user . ":" . $password . "@" . "localhost" ;
}
else
{
    $cred_host = "localhost" ;
}

$output=`wbemcli -noverify ei $protocol://$cred_host:$port/root/cimv2:bogusclass 2>&1 1>/dev/null`;
unless ($output =~ /CIM_ERR_INVALID_CLASS: Class not found/ ) {
    print "\tIncorrect error message for: \"Invalid class\".\n"; 
    $rc=1;
}

$output=`wbemcli -noverify ei $protocol://$cred_host:$port/root/bogus:CIM_LogicalElement 2>&1 1>/dev/null`;
unless ($output =~ /CIM_ERR_INVALID_NAMESPACE: Invalid namespace/ ) {
    print "\tIncorrect error message for: \"Invalid namespace\".\n"; 
    $rc=1;
}


$output=`wbemcli -noverify bogus $protocol://$cred_host:$port/root/cimv2:CIM_LogicalElement 2>&1 1>/dev/null`;
unless ($output =~ /Cmd Exception: Invalid operation/ ) {
    print "\tIncorrect error message for: \"Invalid operation\".\n"; 
    $rc=1;
}

exit $rc;


