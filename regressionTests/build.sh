#!/bin/sh

# /*
#  * build.sh
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


dirs='FilesAndDirectories BigOutput ProcessIndications LifeCyleIndications'

sfcbdir=/usr/local/share/sfcb
if [ -x /usr/bin/curl ] && [ ! -f $sfcbdir/CIM/CIM_Schema.mof ]
then
    echo --- build.sh cannot continue: $sfcbdir/CIM/CIM_Schema.mof not found 
    echo --- You need to download the CIM Mof files from www.dmtf.org prior to running build.sh.
    echo --- Execute \'sudo sh ../getSchema.sh\' to get the latest DMTF schemas. 
    exit 5
fi

CIMDIR=$sfcbdir/CIM

mkdir -p bin lib repository/root/interop  repository/root/tests

rm -f repository/root/interop/*
cp scripts/wbemcat bin
cp scripts/catdiff bin

if [ $# -gt 0 ] 
then
   dirs=$1
fi   

for x in $dirs
do 
   echo make -C $x install
   make -C $x install
done   

echo mofc -I $CIMDIR -i CIM_Schema.mof -o repository/root/interop/classSchemas schema/interop.mof
mofc -I $CIMDIR -i CIM_Schema.mof -o repository/root/interop/classSchemas schema/interop.mof

mofs=''
for x in $dirs
do 
   mofs=$mofs' '$x'/schema/*.mof'
done   

echo mofc -I $CIMDIR -i CIM_Schema.mof -o repository/root/tests/classSchemas $mofs
mofc -I $CIMDIR -i CIM_Schema.mof -o repository/root/tests/classSchemas $mofs

regs='schema/baseProvider.reg'
for x in $dirs
do 
   regs=$regs' '$x'/schema/*.reg'
done   
echo "cat $regs >providerRegister"
cat $regs >providerRegister
