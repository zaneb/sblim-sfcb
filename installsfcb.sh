#!/bin/sh

clear
echo "This script installs the Small-Footprint CIM Broker (sfcb) and supporting"
echo "packages from CVS. You will be prompted for some settings; the default response"
echo "is shown in square brackets; e.g. [Y]. Press ENTER to select the default."
echo
echo "To log the output of this installation re-run this script using the command:"
echo "   sh $0 | tee /tmp/installsfcb.log"

# Check if running as root
if [[ $USER = "root" ]]; then
   echo
   echo "WARNING: It is not recommended to run this script as root!"
   echo "Please re-run this script under a regular userid; you will be prompted"
   echo "to enter the root password for operations that require root permission."
fi  

# Check if we have correct versions of required packages
echo
echo "The following packages are required to build the sfcb packages:"
echo "   automake >= 1.8. Your automake version is: $(rpm -q automake)"
echo "   autoconf >= 2.5.8. Your autoconf version is: $(rpm -q autoconf)"
echo "   libtool >= 1.5.8. Your libtool version is: $(rpm -q libtool)"
echo "   bison >= 1.85. Your bison version is: $(rpm -q bison)"
echo "   curl >= 7.11.1-1. Your curl version is: $(rpm -q curl)"
echo "   curl-devel >= 7.11.1-1. Your curl-devel version is: $(rpm -q curl-devel)"
echo "Older/missing versions of packages may cause build failures."

echo
echo -n "Continue with sfcb install? [Y]/n "; read _ANSWER
if [[ $_ANSWER = "N" || $_ANSWER = "n" ]]; then exit 1; fi

# Check if there is an existing install script anywhere that will break autoconf
for _FILE in ../install-sh ../install.sh; do
   if [[ -e $_FILE ]]; then
      echo "Existing $_FILE will break autoconf! Please remove/rename it and re-run $0"
      exit 1 
   fi
done

# Setup CVS access
unset _ANSWER
if [[ -n $CVSROOT ]]; then
   echo "Your CVSROOT is currently set to:"
   echo "   $CVSROOT"
   echo -n "Change CVSROOT to SourceForge SBLIM project anonymous access? [Y]/n "; read _ANSWER
fi
if [[ -z $_ANSWER || $_ANSWER = "Y" || $_ANSWER = "y" ]]; then
   export CVSROOT=":pserver:anonymous:@cvs.sourceforge.net:/cvsroot/sblim"
fi

# If using CVS pserver method then CVS login now
if [[ $( echo $CVSROOT | cut -f2 -d':' ) == "pserver" ]]; then 
   cvs login
   if [[ $? -ne 0 ]]; then exit 1; fi
fi

# Where to download and build packages
_SRCROOT=$PWD
echo -n "Location to download sfcb packages into? [$_SRCROOT] "; read _ANSWER
if [[ -n $_ANSWER ]]; then _SRCROOT=$_ANSWER; fi
mkdir -p $_SRCROOT
if [[ $? -ne 0 ]]; then exit 1; fi

# Where do we want to install to? The default install target is /usr/local/...
export _PREFIX=/usr/local
echo -n "Location prefix to install local sfcb packages under? [$_PREFIX] "; read _ANSWER
if [[ -n $_ANSWER ]]; then _PREFIX=$_ANSWER; fi

# -----------------------------------------------------------------------------------
# Generic function to download, configure, build and install a SBLIM package from CVS
# Usage: _installpkg <PACKAGE> [PREFIX]
#        <PACKAGE>	package CVS module name
#        [PREFIX]	autoconf install prefix. Optional
function _installpkg {
   typeset _PACKAGE=$1
   if [[ -n $2 ]]; then typeset _PREFIX=$2; fi

   # Check if need to download the package source
   if [[ $_PACKAGE != "sfcb" || ! -e sfcBroker.c ]]; then

      # Check if rebuilding existing package
      _ANSWER="Y"
      if [[ -e $PWD/$_PACKAGE ]]; then
         echo -n "$PWD/$_PACKAGE already exists. Rebuild and reinstall? y/[N] "; read _ANSWER
         if [[ -z $_ANSWER || $_ANSWER = "N" || $_ANSWER = "n" ]]; then return 0; fi

         echo -n "Download latest $_PACKAGE source from CVS? y/[N] "; read _ANSWER
      fi

      # Check if need to download package source from CVS
      if [[ $_ANSWER = "Y" || $_ANSWER = "y" ]]; then
         echo "Downloading $_PACKAGE source from CVS. Please wait..."
         rm -rf $_PACKAGE
         cvs -z3 -d$CVSROOT -q co -P $_PACKAGE
        if [[ $? -ne 0 ]]; then return 1; fi
      fi 

      cd $_PACKAGE
   fi

   # Special case: if sfcb package then download mofc into it too
   if [[ $_PACKAGE = "sfcb" && ! -e $PWD/mofc ]]; then
      echo "Downloading mofc source from CVS. Please wait..."
      cvs -z3 -d$CVSROOT -q co -P mofc
      if [[ $? -ne 0 ]]; then return 1; fi
   fi

   # Configure the package for building 
   echo "Running autoconfiscate.sh ..." &&
   sh autoconfiscate.sh &&
   echo "Running configure ..." &&
   sh configure --prefix=$_PREFIX CIMSERVER=sfcb &&
   echo "Running make clean ..." && 
   make clean
   if [[ $? -ne 0 ]]; then return 1; fi

   # WORKAROUND A BUILD PROBLEM THAT CAN HAPPEN DUE TO MISSING ./lib/.lib/
   (mkdir -p .libs && cd .libs && rm -rf .libs && ln -f -s $PWD .libs && cd ..) &&

   # Build/install the package
   echo "Running make ..." &&
   make &&
   echo -n "Running make install & postinstall. Enter Root " && 
   su --preserve-environment --command \
	"make install && (make -k postinstall || echo No postinstall. Continuing...)"

   # Make sure everything worked
   if [[ $? -ne 0 ]]; then
      echo "Failed to build/install $_PACKAGE"
      return 1
   fi
  
   echo "Finished installing $_PACKAGE" 
   return 0
}
# -----------------------------------------------------------------------------------

# Remove all old stuff for clean rebuild
echo -n "Remove all existing sfcb and SBLIM files under $_PREFIX/? y/[N] "; read _ANSWER
if [[ $_ANSWER = "Y" || $_ANSWER = "y" ]]; then
   echo -n "Removing files. Enter Root "
   su --preserve-environment --command "rm -rf \
	$_PREFIX/var/lib/sfcb* \
        $_PREFIX/var/lib/sblim* \
	$_PREFIX/etc/sfcb* \
        $_PREFIX/etc/init.d/sfcb* \
	$_PREFIX/lib/libsfc* \
        $_PREFIX/lib/libcmpi* \
        $_PREFIX/lib/libind_helper* \
        $_PREFIX/lib/libCWS* \
        $_PREFIX/lib/libTST* \
        $_PREFIX/lib/libcwsutil* \
        $_PREFIX/lib/libdmiinfo* \
        $_PREFIX/lib/libIndTestProvider* \
	$_PREFIX/lib/cmpi* \
        $_PREFIX/include/sblim* \
        $_PREFIX/include/cmpi* \
	$_PREFIX/bin/sfcb* \
        $_PREFIX/bin/wbem* \
	$_PREFIX/sbin/sfcbd \
        $_PREFIX/share/doc/sblim* \
        $_PREFIX/share/doc/sfcb* \
        $_PREFIX/share/sblim* \
	$_PREFIX/share/sfcb*"
fi

# Download, build and install the sfcb package
cd $_SRCROOT
_installpkg "sfcb" $_PREFIX
if [[ $? -ne 0 ]]; then exit 1; fi

if [[ -e $_SRCROOT/sfcBroker.c ]]; then
   export SFCB_HOME=$_SRCROOT
else
   export SFCB_HOME=$_SRCROOT/sfcb
fi

# Add sfcb locations to the search paths
export PATH=$_PREFIX/bin:$PATH
#(removed for defect#712784)export LD_LIBRARY_PATH=$_PREFIX/lib:$LD_LIBRARY_PATH

# Setup sfcb tracing
export SFCB_TRACE_FILE=/tmp/sfcb.log
rm -f $SFCB_TRACE_FILE
touch $SFCB_TRACE_FILE
export SFCB_TRACE=65535

# Start up the sfcbd, albeit without any namespaces or registered classes
echo -n "Starting sfcb. Enter Root "
su --preserve-environment --command '
	for _LIB in $( find $_PREFIX/lib -name "libsfc*.0.0.0" ); do
	  _NEWLIB=${_LIB%.0.0}
	  if [[ ! -e $_NEWLIB ]]; then ln -s $_LIB $_NEWLIB; fi
	done;
	killall -q -9 sfcbd; sleep 1; $_PREFIX/etc/init.d/sfcb start'
echo
sleep 1
if ! ps -C sfcbd > /dev/null; then
   echo "sfcb failed to start"
   exit 1
fi

# Run a quick test to make sure the sfcb is running and responding to client requests
cd $SFCB_HOME/test
_TEST=enumerateclasses.FIRSTTIME
echo "Running test $_TEST ..."
$_PREFIX/bin/wbemcat $_TEST.xml > $_TEST.result
if ! diff --brief $_TEST.result $_TEST.OK; then
   echo "Test failed. Check $PWD/$_TEST.result for errors"
   echo "(this test will fail if you reinstall sfcb with the old class repository)"  
   echo -n "Continue with install? y/[N] "; read _ANSWER
   if [[ -z $_ANSWER || $_ANSWER = "N" || $_ANSWER = "n" ]]; then exit 1; fi
else
   echo "Test succeeded."
   rm -f $_TEST.result
fi

echo "-------------------------------------------------------------------------------"
echo "The Small-Footprint CIM Broker (sfcb) is successfully installed and running."
echo "You may now download and install additional SBLIM packages."

# Download, build and install the wbemcli client
echo "-------------------------------------------------------------------------------"
echo -n "Do you want to install the SBLIM wbemcli CIM client? [Y]/n "; read _ANSWER
if [[ -z $_ANSWER || $_ANSWER = "Y" || $_ANSWER = "y" ]]; then
   cd $_SRCROOT
   _installpkg "wbemcli" $_PREFIX
fi

# Download, build and install SBLIM provider packages & development tools to ./sblim subdirectory
cd $_SRCROOT
mkdir -p sblim

# Download, build and install the SBLIM development tools
echo "-------------------------------------------------------------------------------"
echo -n "Do you want to install the SBLIM development tools? [Y]/n "; read _ANSWER
if [[ -z $_ANSWER || $_ANSWER = "Y" || $_ANSWER = "y" ]]; then
   _PACKAGES="cmpi-devel indication_helper testsuite"
   for _PACKAGE in $_PACKAGES; do
      cd $_SRCROOT/sblim
      _installpkg $_PACKAGE $_PREFIX
   done
fi

# Download, build and install the CMPI base instrumentation
echo "-------------------------------------------------------------------------------"
echo -n "Do you want to install the SBLIM base CMPI instrumention? [Y]/n "; read _ANSWER
if [[ -z $_ANSWER || $_ANSWER = "Y" || $_ANSWER = "y" ]]; then
   cd $_SRCROOT/sblim
   _installpkg "cmpi-base" $_PREFIX
fi

# Download, build and install the sample CMPI providers
echo "-------------------------------------------------------------------------------"
echo -n "Do you want to install the SBLIM sample CMPI providers? [Y]/n "; read _ANSWER
if [[ -z $_ANSWER || $_ANSWER="Y" || $_ANSWER="y" ]]; then
   _PACKAGES="cmpi-fad cmpi-processes cmpi-authorization cmpi-instancelist"
   for _PACKAGE in $_PACKAGES; do
      cd $_SRCROOT/sblim
      _installpkg cmpi-samples/$_PACKAGE $_PREFIX
   done
fi

# Download, build and install the test CMPI providers
echo "-------------------------------------------------------------------------------"
echo -n "Do you want to install the SBLIM test CMPI providers? [Y]/n "; read _ANSWER
if [[ -z $_ANSWER || $_ANSWER = "Y" || $_ANSWER = "y" ]]; then
   _PACKAGES="cmpi-instancetest cmpi-methodtest cmpi-processindicationtest cmpi-lifecycleindicationtest cmpi-reef"
   for _PACKAGE in $_PACKAGES; do
      cd $_SRCROOT/sblim
      _installpkg cmpi-tests/$_PACKAGE $_PREFIX
   done
fi

# Add SBLIM provider library location to search path
#(defect 712784)export LD_LIBRARY_PATH=$_PREFIX/lib/cmpi:$LD_LIBRARY_PATH

# BUILD PROBLEM WORKAROUND NEEDED FOR SOME PACKAGES WHEN LIBS DONT GET LINKED CORRECTLY
echo "Workaround for missing <lib>.0 symbolic links"
su -p -c 'for _LIB in $( find $_PREFIX/lib -name "lib*.0.0.0" ); do
  _NEWLIB=${_LIB%.0.0}
  if [[ ! -e $_NEWLIB ]]; then ln -s $_LIB $_NEWLIB; fi
done'

# Restart sfcb
echo "-------------------------------------------------------------------------------"
echo -n "Restarting sfcbd. Enter Root "
su --preserve-environment --command "$_PREFIX/etc/init.d/sfcb restart"
echo
sleep 1
if ! ps -C sfcbd > /dev/null; then
   echo "sfcb failed to start"
   exit 1
fi

# Run some simple tests using wbemcat
if which wbemcat > /dev/null; then
   cd $SFCB_HOME/test

   echo "****************************************"
   _CMD="wbemcat $PWD/enumerateclasses.ALL.xml"
   echo -e "wbemcat test: EnumerateClasses on root/cimv2 namespace ...\n\t$_CMD"
   eval $_CMD

   echo "****************************************"
   _CMD="wbemcat $PWD/getclass.Linux_OperatingSystem.xml"
   echo -e "wbemcat test: GetClass on Linux_OperatingSystem class ...\n\t$_CMD"
   eval $_CMD

   echo "****************************************"
   _CMD="wbemcat $PWD/enumerateinstancenames.Linux_OperatingSystem.xml"
   echo -e "wbemcat test: EnumerateInstanceNames on Linux_OperatingSystem class ...\n\t$_CMD"
   eval $_CMD
                                                                                                                        
   echo "****************************************"
   _CMD="wbemcat $PWD/enumerateinstances.Linux_OperatingSystem.xml"
   echo -e "wbemcat test: EnumerateInstances on Linux_OperatingSystem class ...\n\t$_CMD"
   eval $_CMD
fi

# Run some simple tests using wbemcli
if which wbemcli > /dev/null; then
   echo "****************************************"
   _CMD="wbemcli ec 'http://localhost:5988/root/cimv2'"
   echo -e "wbemcli test: EnumerateClasses on root/cimv2 namespace ...\n\t$_CMD"
   eval $_CMD

   echo "****************************************"
   _CMD="wbemcli gc 'http://localhost:5988/root/cimv2:Linux_OperatingSystem'"
   echo -e "wbemcli test: GetClass on Linux_OperatingSystem class ...\n\t$_CMD"
   eval $_CMD

   echo "****************************************"
   _CMD="wbemcli ein 'http://localhost:5988/root/cimv2:Linux_OperatingSystem'"
   echo -e "wbemcli test: EnumerateInstanceNames on Linux_OperatingSystem class ...\n\t$_CMD"
   eval $_CMD

   echo "****************************************"
   _CMD="wbemcli -nl ei 'http://localhost:5988/root/cimv2:Linux_OperatingSystem'"
   echo -e "wbemcli test: EnumerateInstances on Linux_OperatingSystem class ...\n\t$_CMD"
   eval $_CMD
fi

exit 0
