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

if SSL
  export SFCB_TEST_PORT=5989
  export SFCB_TEST_PROTOCOL=https
else
  export SFCB_TEST_PORT=5988
  export SFCB_TEST_PROTOCOL=http
endif

TESTS = check_errors.pl
