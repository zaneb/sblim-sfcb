tf="ReferenceDirNames1"
wbemcli rin -dx -nl  'http://localhost/root/tests:CWS_Directory.CreationClassName="CWS_Directory",CSCreationClassName="CIM_UnitaryComputerSystem",CSName="localhost",FSCreationClassName="CIM_FileSystem",FSName="/Simulated/CMPI/tests/",Name="/Simulated/CMPI/tests/Providers"' &>$tf.rsp
diff $tf.rsp $tf.OK >$tf.diff
if [ $? != 0 ]
then
   echo "  diff test failed - see $tf.rsp and $tf.diff"
   exit 1
else 
   rm -f $tf.diff $tf.rsp  
fi
exit 0