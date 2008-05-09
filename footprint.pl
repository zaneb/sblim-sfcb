#!/usr/bin/perl 
# ============================================================================
# footprint.pl
#
# (C) Copyright IBM Corp. 2008
#
# THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
# ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
# CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
#
# You can obtain a current copy of the Eclipse Public License from
# http://www.opensource.org/licenses/eclipse-1.0.php
#
# Author:        Michael Chase-Salerno, <salernom@us.ibm.com>
# Description:
#   Utility to measure disk and memory consumption of SFCB
#   Depends on the existance of a MANIFEST file containing the 
#   filenames of all installed files. This is most easily created 
#   by the Makefile. 
#   Also depends on the mem_test.py script for determining RAM
#   usage, available: http://www.pixelbeat.org/scripts/ps_mem.py
# ============================================================================

use strict;
my @man;
my $totalsize=0;

# Read MANIFEST file
my $file="MANIFEST";
if (-e $file) {
  open(MANFILE,"<$file") || die "Cannot open $file: $!";
  @man = (<MANFILE>);
  close(MANFILE) || warn "Cannot close $file: $!";
} else {
    print "Can't read MANIFEST file, skipping disk usage check.\n";
}

# Process MANIFEST contents
print "Disk footprint\n";
print "==============\n";
print "bytes\t\tfile\n";
foreach (@man){
    chomp;
    my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$mtime,$ctime,$blksize,$blocks) =stat($_);
    print "$size\t\t$_\n";
    $totalsize+=$size;
}
print "\n$totalsize\t\tTotal bytes\n\n";

# Check memory usage, sfcb needs to be running for this.
# We also need the ps_mem.py python script.
print "Memory footprint\n";
print "=================\n";
unless ($> == 0) {
    print "Must be run as root to do memory footprint.\n";
} else {
    my $PSMEM="";
    if (-x "./ps_mem.py") {
        $PSMEM="./ps_mem.py";
    } elsif (`which ps_mem.py 2>/dev/null`) {
        $PSMEM="ps_mem.py";
    } else {
        $PSMEM="";
        print "ps_mem.py not found, go get it.\n";
    }
    if ($PSMEM) {
        my $pid=`ps -aef | grep sfcbd`;
        unless (-z $pid) {
		    print " Private  +   Shared  =  RAM used       Program\n";
            system("$PSMEM | grep sfcbd 2>&1");
        } else {
            print "SFCB doesn't appear to be running, skipping RAM footprint.\n";
        }
    }
}
   
