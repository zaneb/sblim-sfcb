tf="BigInstEnumeration"
wbemcli ei 'http://localhost/root/tests:Reef_LogicalVolume' &>$tf.rsp
diff $tf.rsp $tf.OK >$tf.diff
if [ $? != 0 ]
then
   echo "  - diff test failed - see $tf.rsp and $tf.diff"
   exit 1
else 
   rm -f $tf.diff $tf.rsp  
fi
exit 0