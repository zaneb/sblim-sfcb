#!/bin/sh

# /*
#  * clean.sh
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


rm -rf bin lib repository
rm -f providerRegister
rm -f indication.log
find -name "*.d" -exec rm {} \;
find -name "*.o" -exec rm {} \;
find -name "*.so" -exec rm {} \;
