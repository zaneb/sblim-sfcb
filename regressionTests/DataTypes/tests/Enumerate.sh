tf="Enumerate"
wbemcli ei -dx -nl  'http://localhost/root/tests:TST_DataTypes' &>$tf.rsp
sfcbdiff $tf 