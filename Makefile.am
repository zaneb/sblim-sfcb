# $Id$  
#
#  Makefile.am
# 
#  (C) Copyright IBM Corp. 2005
# 
#  THIS FILE IS PROVIDED UNDER THE TERMS OF THE COMMON PUBLIC LICENSE
#  ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
#  CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
# 
#  You can obtain a current copy of the Common Public License from
#  http://oss.software.ibm.com/developerworks/opensource/license-cpl.html
# 
#  Author:        Viktor Mihajlovski <mihajlov@de.ibm.com>
#  Contributions: Adrian Schuur <schuur@de.ibm.com>
# 
#  Description:
# 
#  Makefile process input for sfcb.
# 
# 

BUILT_SOURCES=queryParser.c queryLexer.c authorizationParser.c \
	authorizationLexer.c

AM_YFLAGS=-d

SUBDIRS=. $(MOFC_DIR)

lib_LTLIBRARIES = \
   libsfcBrokerCore.la \
   libsfcInternalProvider.la \
   libsfcInteropProvider.la \
   libsfcClassProvider.la \
   libsfcAuthorizationProvider.la \
   libsfcCimXmlCodec.la \
   libsfcHttpAdapter.la \
   libsfcBasicAuthentication.la \
   libsfcIndCIMXMLHandler.la

bin_PROGRAMS = \
   sfcBroker 

libsfcBrokerCore_la_SOURCES = \
    args.c \
    array.c \
    brokerEnc.c \
    brokerUpc.c \
    brokerOs.c \
    context.c \
    datetime.c \
    enumeration.c \
    instance.c \
    objectpath.c \
    result.c \
    selectexp.c \
    selectcond.c \
    subcond.c \
    predicate.c \
    string.c \
    value.c \
    support.c \
    providerRegister.c \
    objectImpl.c \
    constClass.c \
    hashtable.c \
    genericlist.c \
    utilHashtable.c \
    utilStringBuffer.c \
    utilFactory.c \
    msgqueue.c \
    providerMgr.c \
    providerDrv.c \
    trace.c \
    control.c \
    queryParser.y \
    queryLexer.l \
    queryOperation.c \
    queryStatement.c

libsfcBrokerCore_la_CFLAGS = @SFCB_CMPI_OS@ @SFCB_CMPI_PLATFORM@ 

queryLexer.c: queryLexer.l queryParser.y
	lex -t queryLexer.l | sed -e "s/yy/sfcQuery/g" > queryLexer.c

queryParser.c: queryLexer.l queryParser.y
	yacc -p sfcQuery -d -o queryParser.c queryParser.y  

libsfcHttpAdapter_la_SOURCES = \
   httpAdapter.c \
   httpComm.c
libsfcHttpAdapter_la_LIBADD=-lsfcBrokerCore -lsfcCimXmlCodec

libsfcBasicAuthentication_la_SOURCES = \
   sfcBasicAuthentication.c

libsfcInternalProvider_la_SOURCES = \
   internalProvider.c \
   fileRepository.c
libsfcInternalProvider_la_LIBADD=-lsfcBrokerCore

libsfcInteropProvider_la_SOURCES = \
   interopProvider.c 
libsfcInteropProvider_la_LIBADD=-lsfcBrokerCore -lsfcInternalProvider

libsfcIndCIMXMLHandler_la_SOURCES = \
   indCIMXMLHandler.c \
   indCIMXMLExport.c 
libsfcIndCIMXMLHandler_la_LIBADD=-lsfcBrokerCore -lsfcInternalProvider -lsfcCimXmlCodec -lsfcHttpAdapter -lcurl

libsfcClassProvider_la_SOURCES = \
   classProvider.c
libsfcClassProvider_la_LIBADD=-lsfcBrokerCore

libsfcAuthorizationProvider_la_SOURCES = \
   authorizationProvider.c \
   authorizationUtil.c \
   authorizationParser.y \
   authorizationLexer.l 
libsfcAuthorizationProvider_la_LIBADD=-lsfcBrokerCore

authorizationLexer.c: authorizationLexer.l authorizationParser.y
	lex -t authorizationLexer.l | sed -e "s/yy/sfcAuthyy/g" > authorizationLexer.c

authorizationParser.c: authorizationLexer.l authorizationParser.y
	yacc -p sfcAuthyy -d -o authorizationParser.c authorizationParser.y 

libsfcCimXmlCodec_la_SOURCES = \
   cimXmlOps.y \
   cimXmlParser.c \
   cimXmlRequest.c 
libsfcCimXmlCodec_la_LIBADD=-lsfcBrokerCore 

sfcBroker_LDADD=-lsfcBrokerCore -lsfcCimXmlCodec -lsfcHttpAdapter

noinst_HEADERS=array.h   httpComm.h     providerMgr.h \
	constClass.h   msgqueue.h     providerRegister.h \
	cimXmlParser.h    native.h       support.h \
	cimXmlRequest.h  genericlist.h  objectImpl.h   trace.h \
	hashtable.h      utilft.h \
	cmpidt.h cmpift.h cmpimacs.h  cmpios.h fileRepository.h \
	selectexp.h queryOperation.h authorizationUtil.h authorizationEnum.c \
	sfcVersion.h

EXTRA_DIST=sfcb.cfg.pre.in sfcb.spec sfcbrepos.sh.in sfcbstage.sh.in

dist_pkgdata_SCRIPTS=genSslCert.sh getSchema.sh

dist_pkgdata_DATA=default.reg

nodist_bin_SCRIPTS=sfcbrepos sfcbstage

sfcbrepos: sfcbrepos.sh
	sed s?$$\{prefix\}?$(prefix)? $< > $@

sfcbstage: sfcbstage.sh
	sed s?$$\{prefix\}?$(prefix)? $< > $@

sfcb.cfg: sfcb.cfg.pre
	sed s?$$\{prefix\}?$(prefix)? $< > $@

getSchema.sh: getSchema.sh.pre
	sed s?$$\{prefix\}?$(prefix)? $< > $@

providerRegister: providerRegister.c
	touch $@

unittest:
	cd test && sh check_all.sh

install-data-local: sfcb.cfg
	test -d $(DESTDIR)$(sysconfdir)/sfcb || $(mkdir_p) $(DESTDIR)$(sysconfdir)/sfcb
	$(INSTALL_DATA) sfcb.cfg $(DESTDIR)$(sysconfdir)/sfcb
	test -d $(localstatedir)/lib/sfcb/registration || $(mkdir_p) $(DESTDIR)$(localstatedir)/lib/sfcb/registration
	test -d $(localstatedir)/lib/sfcb/stage || $(mkdir_p) $(DESTDIR)$(localstatedir)/lib/sfcb/stage

distclean-local:
	rm -f sfcbrepos sfcbstage sfcb.cfg getSchema.sh
