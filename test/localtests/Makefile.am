MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS= \
        @SFCB_CPPFLAGS@ \
        @SFCB_CMPI_PLATFORM@

AM_LDFLAGS=@SFCB_LDFLAGS@

TESTS = test_ec test_ecn test_ei test_ein test_gc test_an test_as test_ci test_gi test_enumNS

check_PROGRAMS = test_ec test_ecn test_ei test_ein test_gc test_an test_as test_ci test_gi test_enumNS

test_ec_SOURCES = test_ec.c show.c
test_ec_LDADD   = -lcmpisfcc

test_ecn_SOURCES = test_ecn.c show.c
test_ecn_LDADD   = -lcmpisfcc

test_ei_SOURCES = test_ei.c show.c
test_ei_LDADD   = -lcmpisfcc

test_ein_SOURCES = test_ein.c show.c
test_ein_LDADD   = -lcmpisfcc

test_gc_SOURCES = test_gc.c show.c
test_gc_LDADD   = -lcmpisfcc

test_an_SOURCES = test_an.c show.c
test_an_LDADD   = -lcmpisfcc

test_as_SOURCES = test_as.c show.c
test_as_LDADD   = -lcmpisfcc

test_ci_SOURCES = test_ci.c show.c
test_ci_LDADD   = -lcmpisfcc

test_gi_SOURCES = test_gi.c show.c
test_gi_LDADD   = -lcmpisfcc

test_enumNS_SOURCES = test_enumNS.c show.c
test_enumNS_LDADD   = -lcmpisfcc
#@INC_AMINCLUDE@
