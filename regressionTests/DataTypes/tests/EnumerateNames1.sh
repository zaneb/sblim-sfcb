tf="EnumerateNames1"
wbemcli ein -dx -nl  'http://localhost/root/tests:TST_DataTypes' &>$tf.rsp
sfcbdiff $tf 