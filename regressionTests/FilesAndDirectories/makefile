# $Id$

# Build Options
export CFLAGS=-Wall -g -DSIMULATED
CPPFLAGS=-I$(CMPIINCLUDE) -I .
LDFLAGS=-L .
LOADLIBES=

# Targets
SOURCES=CWS_PlainFile.c CWS_Directory.c CWS_DirectoryContainsFile.c CWS_FileUtils.c cwsutil.c
DEPFILES=$(SOURCES:.c=.d)
OBJECTS=$(SOURCES:.c=.o)
LIBTARGETS=libFilesAndDirectories.so 
#libcwsutil.so libCWS_PlainFile.so libCWS_Directory.so \
#	libCWS_DirectoryContainsFile.so 

include env
include rules

.PHONY: all clean distclean install uninstall $(SUBDIRS) 

lib%.so: $(OBJECTS)
	$(LINK.c) -shared $^ $(LOADLIBES) $(LDLIBS)-o $@


all: $(LIBTARGETS)

clean: 
	$(RM) $(OBJECTS) $(LIBTARGETS)

distclean: clean
	$(RM) $(DEPFILES) *~

install: $(LIBTARGETS)
	install -m 755 $(LIBTARGETS) $(LIBINSTALL)

uninstall: 
	cd $(LIBINSTALL) && $(RM) $(LIBTARGETS)

ifeq ($(MAKECMDGOALS),clean)
NODEPS=1
endif

ifeq ($(MAKECMDGOALS),distclean)
NODEPS=1
endif

ifndef NODEPS
include $(DEPFILES)
endif