#!/bin/sh

# /*
#  * test.sh
#  *
#  * (C) Copyright IBM Corp. 2005
#  *
#  * THIS FILE IS PROVIDED UNDER THE TERMS OF THE COMMON PUBLIC LICENSE
# * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
#  * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
#  *
#  * You can obtain a current copy of the Common Public License from
#  * http://oss.software.ibm.com/developerworks/opensource/license-cpl.html
#  *
#  * Author:       Adrian Schuur <schuur@de.ibm.com>
#  *
#  * Description:
#  *
# */

#dirs=`find -type d -maxdepth 1 \! -name lib \! -name bin \! -name schema \! -name .`
dirs='FilesAndDirectories BigOutput ProcessIndications LifeCycleIndications'

if [ $# -gt 0 ] 
then
   dirs=$1
fi   

let tests=0
let testsOk=0
let testsFailed=0
ccd=`pwd`

for x in $dirs
do 
   echo - Testing directory $x
   cd $x/tests
   tfs=`ls *.sh`
   for t in $tfs
   do
      echo "  Testing $t"
      let tests=tests+1
      sh $t
      if [ $? != 0 ]
      then
         let testFaile=testsFailed+1
      else
         let testsOk=testsOk+1
      fi
   done
   cd $ccd
done

echo - Tests executed: $tests  OK: $testsOk  Failed: $testsFailed  
exit $testsFailed 
