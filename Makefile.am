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

sfcbstatedir=$(localstatedir)/lib/sfcb
sfcbdocdir=$(datadir)/doc/$(PACKAGE)-$(VERSION)
initdir=$(sysconfdir)/init.d

BUILT_SOURCES=queryParser.c queryLexer.c authorizationParser.c \
	authorizationLexer.c

AM_YFLAGS=-d

AM_CPPFLAGS=-DSFCB_CONFDIR=\"$(sysconfdir)/$(PACKAGE)\" \
	-DSFCB_STATEDIR=\"$(sfcbstatedir)\"

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
   sfcbd 

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
    queryStatement.c \
    cimXmlGen.c \
    mrwlock.c \
    mlog.c

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


sfcbd_SOURCES=sfcBroker.c
sfcbd_LDADD=-lsfcBrokerCore -lsfcCimXmlCodec -lsfcHttpAdapter

noinst_HEADERS=array.h   httpComm.h control.h    providerMgr.h \
	constClass.h   msgqueue.h     providerRegister.h \
	cimXmlParser.h    native.h       support.h cimXmlGen.h \
	cimXmlRequest.h  genericlist.h  objectImpl.h   trace.h \
	hashtable.h      utilft.h mlog.h \
	cmpidt.h cmpift.h cmpiftx.h cmpimacs.h cmpimacsx.h cmpios.h fileRepository.h \
	selectexp.h queryOperation.h authorizationUtil.h authorizationEnum.c \
	sfcVersion.h mrwlock.h


EXTRA_DIST=sfcb.cfg.pre.in sfcb.spec sfcbrepos.sh.in sfcbstage.sh.in \
	sfcbunstage.sh.in sfcb.init-redhat sfcb.init-suse sfcb.init-none \
	regressionTests doc

dist_pkgdata_SCRIPTS=genSslCert.sh getSchema.sh

dist_pkgdata_DATA=default.reg

nodist_bin_SCRIPTS=sfcbrepos sfcbstage sfcbunstage 

dist_bin_SCRIPTS=wbemcat

sfcbrepos: sfcbrepos.sh
	sed s?$$\{prefix\}?$(prefix)?g $< > $@

sfcbstage: sfcbstage.sh
	sed s?$$\{prefix\}?$(prefix)?g $< > $@

sfcbunstage: sfcbunstage.sh
	sed s?$$\{prefix\}?$(prefix)?g $< > $@

sfcb.cfg: sfcb.cfg.pre
	sed s?$$\{prefix\}?$(prefix)?g $< > $@

getSchema.sh: getSchema.sh.pre
	sed s?$$\{prefix\}?$(prefix)?g $< > $@

sfcb.init: sfcb.$(INIT_STYLE)
	sed -e "s?@sysconfdir\@?$(sysconfdir)?g" \
		-e "s?@bindir\@?$(bindir)?g" \
		-e "s?@libdir\@?$(libdir)?g" $< > $@

providerRegister: providerRegister.c
	touch $@

unittest:
	cd test && sh check_all.sh

install-data-local: sfcb.cfg
	test -d $(DESTDIR)$(sysconfdir)/sfcb || $(mkdir_p) $(DESTDIR)$(sysconfdir)/sfcb
	$(INSTALL_DATA) sfcb.cfg $(DESTDIR)$(sysconfdir)/sfcb
	test -d $(DESTDIR)$(sfcbstatedir)/registration/repository || $(mkdir_p) $(DESTDIR)$(sfcbstatedir)/registration/repository
	test -d $(DESTDIR)$(sfcbstatedir)/stage/mofs || $(mkdir_p) $(DESTDIR)$(sfcbstatedir)/stage/mofs
	test -d $(DESTDIR)$(sfcbstatedir)/stage/regs || $(mkdir_p) $(DESTDIR)$(sfcbstatedir)/stage/regs
	$(INSTALL_DATA) $(srcdir)/default.reg $(DESTDIR)$(sfcbstatedir)/stage
	test -d $(DESTDIR)$(sfcbdocdir) || $(mkdir_p) $(DESTDIR)$(sfcbdocdir)
	$(INSTALL_DATA) $(srcdir)/README $(srcdir)/AUTHORS $(srcdir)/COPYING $(DESTDIR)$(sfcbdocdir)
	$(INSTALL_DATA) $(srcdir)/doc/sfcb.html $(DESTDIR)$(sfcbdocdir)

install-exec-local:sfcb.init
	test -d $(DESTDIR)$(initdir) || $(mkdir_p) $(DESTDIR)$(initdir)
	$(INSTALL_SCRIPT) sfcb.init $(DESTDIR)$(initdir)/sfcb

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/sfcb/sfcb.cfg
	rm -f $(DESTDIR)$(sfcbstatedir)/stage/default.reg
	rm -f $(DESTDIR)$(sfcbdocdir)/*
	rm -f $(DESTDIR)$(initdir)/sfcb

clean-local:
	rm -f sfcbrepos sfcbstage sfcbunstage sfcb.cfg getSchema.sh sfcb.init

dist-hook:
	test -d "$(distdir)" &&	rm -rf `find $(distdir) -type d -name CVS`

install-cimschema: getSchema.sh
	test -d $(DESTDIR)$(pkgdatadir) || $(mkdir_p) $(DESTDIR)$(pkgdatadir)
	sh getSchema.sh -f $(DESTDIR)$(pkgdatadir)

postinstall: install-cimschema
	test -f $(DESTDIR)$(sfcbstatedir)/registration/providerRegister || $(INSTALL_DATA) $(DESTDIR)$(sfcbstatedir)/stage/default.reg $(DESTDIR)$(sfcbstatedir)/registration/providerRegister 

