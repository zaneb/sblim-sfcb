# $Id$  
#
#  Makefile.am
# 
#   © Copyright IBM Corp. 2009
# 
#  THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
#  ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
#  CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
# 
#  You can obtain a current copy of the Eclipse Public License from
#  http://www.opensource.org/licenses/eclipse-1.0.php
# 
#  Author:        Michael Chase-Salerno <bratac@linux.vnet.ibm.com>
#  Contributions: 
# 
#  Description:
# 
#  Makefile process input for sfcb tests.
# 
# 

MAINTAINERCLEANFILES = Makefile.in

if SSL
  export SFCB_TEST_PORT=5989
  export SFCB_TEST_PROTOCOL=https
else
  export SFCB_TEST_PORT=5988
  export SFCB_TEST_PROTOCOL=http
endif

#Some wbemcat tests
TESTS = xmltest.sh 
