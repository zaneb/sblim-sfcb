tf="DeleteInst3"
wbemcli di -dx -nl  'http://localhost/root/cimv22:TST_DataTypes.CreationClassName=TST_DataTypes,InstanceId=1' &>$tf.rsp
sfcbdiff $tfsfcbdiff $tf 