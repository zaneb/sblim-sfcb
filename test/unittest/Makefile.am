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

AM_LDFLAGS=-L"$(top_builddir)/.libs"

TESTS_ENVIRONMENT = SFCB_TRACE_FILE="/tmp/sfcbtracetest"

TESTS = xmlUnescape newCMPIInstance EmbeddedTests genericlist newDateTime

check_PROGRAMS = xmlUnescape newCMPIInstance EmbeddedTests genericlist newDateTime

xmlUnescape_SOURCES = xmlUnescape.c
xmlUnescape_LDADD = -lsfcCimXmlCodec -lsfcHttpAdapter

newCMPIInstance_SOURCES = newCMPIInstance.c
newCMPIInstance_LDADD = -lsfcCimXmlCodec -lsfcHttpAdapter

EmbeddedTests_SOURCES = EmbeddedTests.c 
EmbeddedTests_LDADD = -lsfcCimXmlCodec -lsfcHttpAdapter

genericlist_SOURCES = genericlist.c
genericlist_LDADD = -lsfcBrokerCore

newDateTime_SOURCES = newDateTime.c
newDateTime_LDADD = -lsfcBrokerCore
