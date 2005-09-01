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

sfcbdocdir=$(datadir)/doc/sfcb-$(VERSION)
sfcbdatadir=$(datadir)/sfcb
sfcbconfdir=$(sysconfdir)/sfcb
sfcbstatedir=$(localstatedir)/lib/sfcb
initdir=$(sysconfdir)/init.d

MANFILES=man/genSslCert.1 man/getSchema.1 man/sfcbd.1 man/sfcbmof.1 \
	man/sfcbrepos.1 man/sfcbstage.1 man/sfcbunstage.1 \
	man/wbemcat.1 man/xmltest.1

BUILT_SOURCES=queryParser.c queryLexer.c sqlParser.c sqlLexer.c $(MANFILES)

AM_YFLAGS=-d

AM_CPPFLAGS=-DSFCB_CONFDIR=\"$(sfcbconfdir)\" \
	-DSFCB_STATEDIR=\"$(sfcbstatedir)\" \
	-DSFCB_LIBDIR=\"$(libdir)\"

SUBDIRS=. $(MOFC_DIR)

if INDICATIONS
lib_LTLIBRARIES = \
   libsfcBrokerCore.la \
   libsfcInternalProvider.la \
   libsfcInteropProvider.la \
   libsfcClassProvider.la \
   libsfcCimXmlCodec.la \
   libsfcHttpAdapter.la \
   libsfcBasicAuthentication.la \
   libsfcIndCIMXMLHandler.la
else
lib_LTLIBRARIES = \
   libsfcBrokerCore.la \
   libsfcInternalProvider.la \
   libsfcInteropProvider.la \
   libsfcClassProvider.la \
   libsfcCimXmlCodec.la \
   libsfcHttpAdapter.la \
   libsfcBasicAuthentication.la
endif

sbin_PROGRAMS = \
   sfcbd 

if JDBC
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
    mlog.c \
    dbpAdapter.c \
    sqlLexer.l \
    sqlParser.y \
    sqlStatement.c \
    avltree.c
 
else
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
endif

libsfcBrokerCore_la_CFLAGS = @SFCB_CMPI_OS@ @SFCB_CMPI_PLATFORM@ 

if JDBC
sqlLexer.c: sqlLexer.l sqlParser.y
	$(LEX) -t sqlLexer.l | sed -e "s/yy/sfcSql/g" > sqlLexer.c

sqlParser.c: sqlLexer.l sqlParser.y
	$(YACC) $(AM_YFLAGS) -p sfcSql -o sqlParser.c sqlParser.y
endif

queryLexer.c: queryLexer.l queryParser.y
	$(LEX) -t queryLexer.l | sed -e "s/yy/sfcQuery/g" > queryLexer.c

queryParser.c: queryLexer.l queryParser.y
	$(YACC) $(AM_YFLAGS) -p sfcQuery -o queryParser.c queryParser.y  

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
libsfcIndCIMXMLHandler_la_LIBADD=-lsfcBrokerCore -lsfcInternalProvider -lsfcCimXmlCodec -lsfcHttpAdapter

libsfcClassProvider_la_SOURCES = \
   classProvider.c
libsfcClassProvider_la_LIBADD=-lsfcBrokerCore

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
	cmpidt.h cmpift.h cmpiftx.h cmpimacs.h cmpimacsx.h cmpios.h \
	fileRepository.h \
	selectexp.h queryOperation.h \
	sfcVersion.h mrwlock.h sqlStatement.h avltree.h

man_MANS=$(MANFILES)

EXTRA_DIST=sfcb.cfg.pre.in sfcb.spec sfcbrepos.sh.in sfcbstage.sh.in \
	sfcbunstage.sh.in sfcb.init-redhat.in sfcb.init-suse.in \
	sfcb.init-none.in

sfcbconf_DATA=sfcb.cfg

dist_sfcbdata_SCRIPTS=genSslCert.sh getSchema.sh

dist_sfcbdata_DATA=default.reg interop.mof

nodist_bin_SCRIPTS=sfcbrepos sfcbstage sfcbunstage 

dist_bin_SCRIPTS=wbemcat xmltest

init_SCRIPTS=sfcb

sfcbdoc_DATA=README AUTHORS COPYING

SUFFIXES = .1 .1.pre

.1.pre.1:
	sed -e s?$$\{prefix\}?$(prefix)?g \
	-e s?$$\{exec_prefix\}?$(prefix)?g $< > $@

sfcbrepos: sfcbrepos.sh
	sed -e s?$$\{prefix\}?$(prefix)?g \
	-e s?$$\{exec_prefix\}?$(prefix)?g $< > $@

sfcbstage: sfcbstage.sh
	sed -e s?$$\{prefix\}?$(prefix)?g \
	-e s?$$\{exec_prefix\}?$(prefix)?g $< > $@

sfcbunstage: sfcbunstage.sh
	sed -e s?$$\{prefix\}?$(prefix)?g \
	-e s?$$\{exec_prefix\}?$(prefix)?g $< > $@

sfcb.cfg: sfcb.cfg.pre
	sed -e s?$$\{prefix\}?$(prefix)?g \
	-e s?$$\{exec_prefix\}?$(prefix)?g $< > $@

getSchema.sh: getSchema.sh.pre
	sed -e s?$$\{prefix\}?$(prefix)?g \
	-e s?$$\{exec_prefix\}?$(prefix)?g $< > $@

sfcb: sfcb.$(INIT_STYLE)
	sed -e s?$$\{prefix\}?$(prefix)?g \
	-e s?$$\{exec_prefix\}?$(prefix)?g $< > $@

providerRegister: providerRegister.c
	touch $@

unittest:
	cd test && sh check_all.sh

install-data-local:
	test -d $(DESTDIR)$(sfcbstatedir)/registration/repository || $(mkdir_p) $(DESTDIR)$(sfcbstatedir)/registration/repository
	test -d $(DESTDIR)$(sfcbstatedir)/stage/mofs || $(mkdir_p) $(DESTDIR)$(sfcbstatedir)/stage/mofs
	test -d $(DESTDIR)$(sfcbstatedir)/stage/regs || $(mkdir_p) $(DESTDIR)$(sfcbstatedir)/stage/regs
	$(INSTALL_DATA) $(srcdir)/default.reg $(DESTDIR)$(sfcbstatedir)/stage
	$(INSTALL_DATA) $(srcdir)/interop.mof $(DESTDIR)$(sfcbstatedir)/stage

uninstall-local:
	rm -f $(DESTDIR)$(sfcbstatedir)/stage/default.reg
	rm -f $(DESTDIR)$(sfcbstatedir)/stage/interop.mof

clean-local:
	rm -f sfcbrepos sfcbstage sfcbunstage sfcb.cfg getSchema.sh sfcb \
		sfcb.init-redhat sfcb.init-suse sfcb.init-none \
		$(MANFILES)

dist-hook:
	test -d "$(distdir)" &&	rm -rf `find $(distdir) -type d -name CVS`

install-cimschema: getSchema.sh
	test -d $(DESTDIR)$(sfcbdatadir) || $(mkdir_p) $(DESTDIR)$(sfcbdatadir)
	sh getSchema.sh -f $(DESTDIR)$(sfcbdatadir)

postinstall: install-cimschema
	test -f $(DESTDIR)$(sfcbstatedir)/registration/providerRegister || $(INSTALL_DATA) $(DESTDIR)$(sfcbstatedir)/stage/default.reg $(DESTDIR)$(sfcbstatedir)/registration/providerRegister 

