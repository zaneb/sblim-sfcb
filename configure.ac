# $Id$
# 
# configure.ac
#
# (C) Copyright IBM Corp. 2005
#
# THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
# ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
# CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
#
# You can obtain a current copy of the Eclipse Public License from
# http://www.opensource.org/licenses/eclipse-1.0.php
#
# Author:        Viktor Mihajlovski <mihajlov@de.ibm.com>
# Contributions: Adrian Schuur <schuur@de.ibm.com>
#                Sven Schuetz <sven@de.ibm.com>
#
# Description:
#
# Configure process input for sfcb.
# 

# Process this file with autoconf to produce a configure script.

AC_INIT(Small Footprint CIM Broker, 1.2.0a, sblim-devel@lists.sourceforge.net, sblim-sfcb)
AC_CONFIG_SRCDIR([providerDrv.c])

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER([config.h])

AC_ARG_ENABLE(debug,
	[AC_HELP_STRING([--disable-debug],
		[disable debug mode for SFCB.])],
	[enable_debug=$enableval],
	[enable_debug="yes"]
	)	

AC_ARG_ENABLE(ssl,
	[AC_HELP_STRING([--enable-ssl],
		[include ssl support for SFCB.])])

AC_ARG_ENABLE(jdbc,
	[AC_HELP_STRING([--enable-jdbc],
		[include server-side JDBC support for SFCB - EXPERIMENTAL.])])

AC_ARG_ENABLE(slp,
	[AC_HELP_STRING([--enable-slp],
		[include slp agent for SFCB - EXPERIMENTAL.])])
		
AC_ARG_ENABLE(qualifierrep,
	[AC_HELP_STRING([--disable-qualifierrep],
		[disable qualifier repository support.])],
	[enable_qualifierrep=$enableval],
	[enable_qualifierrep="yes"]
	)		

AC_ARG_ENABLE(indications,
	[AC_HELP_STRING([--disable-indications],
		[disable indication support for SFCB - currently not working.])],
	[enable_indications=$enableval],
	[enable_indications="yes"]
	)

AC_ARG_ENABLE(default-properties,
	[AC_HELP_STRING([--disable-default-properties],
		[disable generation of default instance properties for SFCB.])],
	[enable_default_properties=$enableval],
	[enable_default_properties="yes"]
	)

AC_ARG_ENABLE(optimized-enumeration,
	[AC_HELP_STRING([--enable-optimized-enumeration],
		[Enable enumeration call optimization for polymorphic invocations.])],
	)

# Size checks
AC_CHECK_SIZEOF(void*)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(short)

if test "$ac_cv_sizeof_voidp" = 4
then
  if test "$ac_cv_sizeof_short" = 2
  then
     AC_DEFINE(SFCB_HALFWORD,short,[integer od half pointer size])
  else
   AC_ERROR([Unsupported 32-bit platform: short size is $ac_cv_sizeof_short, expected 2.])
  fi
elif  test "$ac_cv_sizeof_voidp" = 8
then
  if test "$ac_cv_sizeof_int" = 4
  then
     AC_DEFINE(SFCB_HALFWORD,int,[integer od half pointer size])
  else
   AC_ERROR([Unsupported 64-bit platform: int size is $ac_cv_sizeof_int, expected 4])
  fi
else
   AC_ERROR([Unsupported platform, invalid pointer size $ac_cv_sizeof_voidp.])
fi 

# Configuration Arguments
AC_ARG_VAR([CIMSCHEMA_SOURCE],[The URL or path name of the CIM Schema ZIP File.])
AC_ARG_VAR([CIMSCHEMA_MOF],[The file name of the top level MOF (CIMSchema.mof)])
AC_ARG_VAR([CIMSCHEMA_SUBDIRS],[Set this to any value if the unpacked CIM Schema directory has subdirectories])
AC_SUBST(CIMSCHEMA_SOURCE)
AC_SUBST(CIMSCHEMA_MOF)
AC_SUBST(CIMSCHEMA_SUBDIRS)


AC_DISABLE_STATIC

if test "$enable_debug" == "yes"; then
   SFCB_CPPFLAGS="-D SFCB_DEBUG $SFCB_CPPFLAGS"
fi

if test "$enable_jdbc" == "yes"; then
   AC_DEFINE(HAVE_JDBC,1,[JDBC support enabled.])
fi

if test "$enable_default_properties" == "yes"; then
   AC_DEFINE(HAVE_DEFAULT_PROPERTIES,1,[Default instance property generation enabled.])
fi

if test "$enable_slp" == "standalone"; then
   AC_DEFINE(HAVE_SLP_ALONE,,[SLP standalone support enabled.])
fi

if test "$enable_slp" == "yes"; then
   AC_DEFINE(HAVE_SLP,,[SLP support enabled.])
fi

if test "$enable_qualifierrep" == "yes"; then
   AC_DEFINE(HAVE_QUALREP,,[Qualifier repository support enabled.])
fi

if test "$enable_optimized_enumeration" == "yes"; then
   AC_DEFINE(HAVE_OPTIMIZED_ENUMERATION,1,[Enumeration optimization enabled.])
fi

AC_CANONICAL_HOST
case $host_os in
     openedition*)	SFCB_CMPI_PLATFORM="-D CMPI_PLATFORM_LINUX_GENERIC_GNU"
		SFCB_CMPI_OS="-D CMPI_OS_LINUX"
		SFCB_CPPFLAGS=-W"c,SSCOM"
		SFCB_CPPFLAGS="-D_OPEN_SOURCE=3 -D_ENHANCED_ASCII_EXT=0xFFFFFFFF $SFCB_CPPFLAGS"
		SFCB_CFLAGS=-W"c,ASCII,dll,expo,langlvl(extended),float"
		SFCB_LDFLAGS=-W"l,XPLINK,dll,EDIT=NO"
		CXX="c++ -+"
		;;
     linux*)	SFCB_CMPI_PLATFORM="-D CMPI_PLATFORM_LINUX_GENERIC_GNU"
		SFCB_CMPI_OS="-D CMPI_OS_LINUX"
		SFCB_CFLAGS="-Wall -fno-strict-aliasing -fsigned-char $SFCB_CFLAGS"
		;;
     hpux*)	SFCB_CMPI_PLATFORM="-D CMPI_OS_HPUX"
		case $host_arch in
		     parisc*) SFCB_CMPI_PLATFORM="-D CMPI_PLATFORM_HPUX_PARISC_ACC"
			      ;;
		esac
		;;
     **)	AC_MSG_ERROR([$ac_cv_host is not a supported platform, sorry.])
esac


case $host_cpu in
     i*86)     AC_DEFINE(SFCB_IX86,1,[Intel i386 CPU])
	       ;;     
     powerpc*)  CFLAGS="$CFLAGS -O0"
	       ;;     
esac

AC_SUBST(SFCB_CMPI_PLATFORM)
AC_SUBST(SFCB_CMPI_OS)
AC_SUBST(SFCB_LIBZ)
AC_SUBST(SFCB_CFLAGS)
AC_SUBST(SFCB_CPPFLAGS)
AC_SUBST(SFCB_LDFLAGS)

# Checks for programs.
AC_PROG_CC
AC_PROG_LEX
AC_CHECK_PROG(YACC,bison,[bison -y])
if test -z "${YACC}"
then
	AC_MSG_WARN([GNU bison is required (only for rebuilding the sfcb parser code).])
fi
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB(pthread,main)
AC_CHECK_LIB(dl,main)
AC_CHECK_LIB(z,main,[SFCB_LIBZ=-lz],[AC_MSG_ERROR([Could not find required libz])])
if test "$enable_indications" == "yes"; then
   AC_DEFINE(HAVE_INDICATIONS,1,[Indication support enabled.])
   AC_CHECK_LIB(curl,main)
fi

if test "$enable_ssl" = "yes"; then
   AC_CHECK_LIB(ssl,main)
fi

if test "$enable_slp"; then
	#defined in acinclude.m4
	CHECK_SFCC  
	AC_CHECK_LIB(cmpisfcc,cmciConnect,[],[AC_MSG_ERROR(no. Sorry cannot find libcmpisfcc.)])
	
	AC_CHECK_HEADERS([slp.h],[],[AC_MSG_ERROR(no. Sorry cannot find slp.h.)])
	AC_CHECK_LIB(slp,SLPReg,[],[AC_MSG_ERROR(no. Sorry cannot find libslp.)])
fi

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h limits.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h zlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
#AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_FUNC_WAIT3
AC_CHECK_FUNCS([alarm bzero gettimeofday localtime_r memmove memset socket strcasecmp strchr strdup strncasecmp strpbrk strrchr strspn strstr tzset])

# Check for Linux System Style

if test -f /etc/redhat-release 
then
   INIT_STYLE=init-redhat
elif test -f /etc/SuSE-release
then 
   INIT_STYLE=init-suse
else
   INIT_STYLE=init-none
fi
AC_SUBST(INIT_STYLE)

# Checks for subdirectories
if test -d $srcdir/test; then
   AC_CONFIG_SUBDIRS([test])
fi

if test -d $srcdir/mofc; then
   MOFC_DIR=mofc
   export sfcb_recursed=yes
   export with_sfcb=yes
   export enable_namespace_kludge=yes
   AC_CONFIG_SUBDIRS([mofc])
   AC_SUBST(MOFC_DIR)
fi

AM_CONDITIONAL(INDICATIONS,[test "$enable_indications" == "yes"]) 
AM_CONDITIONAL(JDBC,[test "$enable_jdbc" == "yes"]) 
AM_CONDITIONAL(SLP,[test "$enable_slp" == "yes"]) 
AM_CONDITIONAL(SLP_ALONE,[test "$enable_slp" == "standalone"]) 
AM_CONDITIONAL(QUALREP,[test "$enable_qualifierrep" == "yes"]) 

AC_CONFIG_FILES([Makefile sfcb.spec sfcbrepos.sh sfcbstage.sh sfcbunstage.sh 
	         sfcbuuid.sh sfcb.cfg.pre getSchema.sh.pre 
		 sfcb.init-redhat sfcb.init-suse sfcb.init-none 
		 man/genSslCert.1.pre  man/sfcbmof.1.pre 
                 man/sfcbunstage.1.pre man/sfcbuuid.1.pre man/getSchema.1.pre
		 man/sfcbrepos.1.pre  man/wbemcat.1.pre
		 man/sfcbd.1.pre man/sfcbstage.1.pre man/xmltest.1.pre ])
AC_OUTPUT

echo ======================================================
echo sfcb configuration complete
echo configuration features:
echo -e "debug"\\t"${enable_debug:-no}"
echo -e "indications"\\t"${enable_indications}"
echo -e "ssl"\\t"${enable_ssl:-no}"
echo -e "jdbc"\\t"${enable_jdbc:-no}"
echo -e "slp"\\t"${enable_slp:-no}"
echo -e "qualifierrep"\\t"${enable_qualifierrep:-no}"
echo -e "default instance properties"\\t"${enable_default_properties}"
echo ======================================================
