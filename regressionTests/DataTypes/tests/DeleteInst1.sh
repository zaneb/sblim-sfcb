tf="DeleteInst1"
wbemcli di -dx -nl  'http://localhost/root/tests:TST_DataTypes.CreationClassName=TST_DataTypes,InstanceId=10' &>$tf.rsp
sfcbdiff $tf 