tf="EnumerateNames2"
wbemcli ein -dx -nl  'http://localhost/root/cimv22:TST_DataTypes' &>$tf.rsp
sfcbdiff $tf 