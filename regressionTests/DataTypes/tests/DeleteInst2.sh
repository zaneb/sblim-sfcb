tf="DeleteInst2"
wbemcli di -dx -nl  'http://localhost/root/tests:TST_DataTypes.CreationClassName=Bad_name,InstanceId=1' &>$tf.rsp
sfcbdiff $tf 