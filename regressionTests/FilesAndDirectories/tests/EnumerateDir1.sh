tf="EnumerateDir1"
wbemcli ei -dx -nl  'http://localhost/root/tests:CWS_Directory' &>$tf.rsp
diff $tf.rsp $tf.OK >$tf.diff
if [ $? != 0 ]
then
   echo "  diff test failed - see $tf.rsp and $tf.diff"
   exit 1
else 
   rm -f $tf.diff $tf.rsp  
fi
exit 0