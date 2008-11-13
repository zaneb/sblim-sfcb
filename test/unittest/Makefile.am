# (C) Copyright IBM Corp 2004
# All rights reserved.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
# file and program are licensed under a BSD style license.  See
# the Copying file included with the OpenHPI distribution for
# full licensing terms.

MAINTAINERCLEANFILES = Makefile.in

AM_LDFLAGS=-L"$(top_builddir)/.libs"

TESTS = xmlUnescape

xmlUnescape_SOURCES = xmlUnescape.c
xmlUnescape_LDADD = -lsfcCimXmlCodec -lsfcHttpAdapter

check_PROGRAMS = xmlUnescape

