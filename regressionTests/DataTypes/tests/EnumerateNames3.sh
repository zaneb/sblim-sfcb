tf="EnumerateNames3"
wbemcli ein -dx -nl  'http://localhost/root/cimv2:TST_BadClass' &>$tf.rsp
sfcbdiff $tf 