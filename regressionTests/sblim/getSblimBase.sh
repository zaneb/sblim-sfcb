#!/bin/sh
#Script to fetch sblim-cmpi-base package.

mirror=http://unc.dl.sourceforge.net/sourceforge
mirror=http://belnet.dl.sourceforge.net/sourceforge

trap "rm /tmp/cmpi-base.tar.gz" exit 

rm -r sblim-* testsuite schema/*
cp dummy.mof schema
cp Linux_Base.reg schema
    
dir=sblim-indication_helpers
package=sblim-indication_helper-0.2.tar.bz2

if [ -x /usr/bin/curl ] && [ ! -f $dir/makefile ]
then
  echo "Fetching sblim-indication-helper package from SBLIM homepage ..."
  if /usr/bin/curl -o /tmp/indication-helper.tar.gz $mirror/sblim/$package &&
    tar -xjf /tmp/indication-helper.tar.gz &&
    mv sblim-indication_helper-* $dir &&
    cp ind-helper-makefile $dir/makefile        
  then
    echo "... OK"
  else
    echo "Failed to fetch sblim-indication-helper package" 1>&2 
    exit 1
  fi        
else
    echo "Need curl to get sblim packages" 1>&2
    exit 1
fi
  

dir=sblim-cmpi-base
package=sblim-cmpi-base-1.4.2.tar.gz

if [ -x /usr/bin/curl ] && [ ! -f $dir/makefile ]
then
  echo "Fetching sblim-cmpi-base package from SBLIM homepage ..." &&
  if /usr/bin/curl -o /tmp/cmpi-base.tar.gz $mirror/sblim/$package &&    
    tar -xzf /tmp/cmpi-base.tar.gz &&
    mv sblim-cmpi-base-* $dir &&
    cp cmpi-base-setting $dir/setting.cmpi    
  then
    echo "... OK"
  else
    echo "Failed to fetch sblim-cmpi-base package" 1>&2 
    exit 1
  fi        
else
    echo "Need curl to get sblim packages" 1>&2
    exit 1
fi

dir=sblim-wbemcli
package=sblim-wbemcli-1.4.8b.tar.gz

if [ -x /usr/bin/curl ] && [ ! -f $dir/makefile ]
then
  echo "Fetching sblim-wbemcli package from SBLIM homepage ..." &&
  if /usr/bin/curl -o /tmp/cmpi-wbemcli.tar.gz $mirror/sblim/$package &&    
    tar -xzf /tmp/cmpi-wbemcli.tar.gz &&
    mv sblim-wbemcli-* $dir 
  then
    cd $dir
    echo $dir
    path=`pwd`
    sh configure --prefix=`pwd`/../..
    cd ..
    echo "... OK"
  else
    echo "Failed to fetch sblim-wbemcli package" 1>&2 
    exit 1
  fi        
else
    echo "Need curl to get sblim packages" 1>&2
    exit 1
fi

dir=testsuite
package=sblim-testsuite-1.2.0.tar.gz

if [ -x /usr/bin/curl ] && [ ! -f $dir/makefile ]
then
  echo "Fetching sblim-testsuite package from SBLIM homepage ..." &&
  if /usr/bin/curl -o /tmp/cmpi-testsuite.tar.gz $mirror/sblim/$package &&    
    tar -xzf /tmp/cmpi-testsuite.tar.gz &&
    mv sblim-testsuite-* $dir 
  then
    echo "... OK"
  else
    echo "Failed to fetch sblim-testsuite package" 1>&2 
    exit 1
  fi        
else
    echo "Need curl to get sblim packages" 1>&2
    exit 1
fi
