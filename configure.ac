
# 
# /*
#  * configure.ac
#  *
#  * (C) Copyright IBM Corp. 2005
#  *
#  * THIS FILE IS PROVIDED UNDER THE TERMS OF THE COMMON PUBLIC LICENSE
#  * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
#  * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
#  *
#  * You can obtain a current copy of the Common Public License from
#  * http://oss.software.ibm.com/developerworks/opensource/license-cpl.html
#  *
#  * Author:        Viktor Mihajlovski <mihajlov@de.ibm.com>
#  * Contributions: Adrian Schuur <schuur@de.ibm.com>
#  *
# * Description:
#  *
#  * Configure process input for sfcb.
#  *
# */
# 

# Process this file with autoconf to produce a configure script.
AC_INIT(Small Footprint CIM Broker, 0.8.6, schuur@de.ibm.com, sfcb)
AC_CONFIG_SRCDIR([providerDrv.c])

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER([config.h])

AC_ARG_ENABLE(debug,
	[AC_HELP_STRING([--enable-debug],
		[enable debug mode for SFCB.])])

AC_ARG_ENABLE(ssl,
	[AC_HELP_STRING([--enable-ssl],
		[include ssl support for SFCB.])])

if test "$enable_debug" != "no"; then
   CPPFLAGS="-D SFCB_DEBUG $CPPFLAGS"
fi

AC_CANONICAL_HOST
case $host_os in
     openedition*)	SFCB_CMPI_PLATFORM="-D CMPI_PLATFORM_LINUX_GENERIC_GNU"
		SFCB_CMPI_OS="-D CMPI_OS_LINUX"
		CPPFLAGS=-W"c,SSCOM"
		CPPFLAGS="-D_OPEN_SOURCE=3 -D_ENHANCED_ASCII_EXT=0xFFFFFFFF $CPPFLAGS"
		CFLAGS=-W"c,ASCII,dll,expo,langlvl(extended),float"
		LDFLAGS=-W"l,XPLINK,dll,EDIT=NO"
if test "$enable_debug" != "no"; then
		CFLAGS="-g $CFLAGS"
fi
		CXX="c++ -+"
		;;
     linux*)	SFCB_CMPI_PLATFORM="-D CMPI_PLATFORM_LINUX_GENERIC_GNU"
		SFCB_CMPI_OS="-D CMPI_OS_LINUX"
		CFLAGS=-Wall
if test "$enable_debug" != "no"; then
		CFLAGS="-g $CFLAGS"
fi
		;;
     hpux*)	SFCB_CMPI_PLATFORM="-D CMPI_OS_HPUX"
		case $host_arch in
		     parisc*) SFCB_CMPI_PLATFORM="-D CMPI_PLATFORM_HPUX_PARISC_ACC"
			      ;;
		esac
		;;
     **)	AC_MSG_ERROR([$ac_cv_host is not a supported platform, sorry.])
esac
AC_SUBST(SFCB_CMPI_PLATFORM)
AC_SUBST(SFCB_CMPI_OS)

# Checks for programs.
AC_PROG_CC
AC_PROG_LEX
AC_CHECK_PROG(YACC,bison,[bison -y])
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB(pthread,main)
AC_CHECK_LIB(dl,main)

if test "$enable_ssl" = "yes"; then
   AC_CHECK_LIB(ssl,main)
fi

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h limits.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_FUNC_WAIT3
AC_CHECK_FUNCS([alarm bzero gettimeofday localtime_r memmove memset socket strcasecmp strchr strdup strncasecmp strpbrk strrchr strspn strstr tzset])

if test -d test; then
   AC_CONFIG_SUBDIRS([test])
fi

if test -d mofc; then
   MOFC_DIR=mofc
   export sfcb_recursed=yes
   export with_sfcb=yes
   AC_CONFIG_SUBDIRS([mofc])
   AC_SUBST(MOFC_DIR)
fi

AC_CONFIG_FILES([Makefile sfcb.spec sfcbrepos.sh sfcbstage.sh sfcb.cfg.pre getSchema.sh.pre])
AC_OUTPUT
