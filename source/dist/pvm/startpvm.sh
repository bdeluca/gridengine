#!/bin/sh -f
#
#
#___INFO__MARK_BEGIN__
##########################################################################
#
#  The Contents of this file are made available subject to the terms of
#  the Sun Industry Standards Source License Version 1.2
#
#  Sun Microsystems Inc., March, 2001
#
#
#  Sun Industry Standards Source License Version 1.2
#  =================================================
#  The contents of this file are subject to the Sun Industry Standards
#  Source License Version 1.2 (the "License"); You may not use this file
#  except in compliance with the License. You may obtain a copy of the
#  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
#
#  Software provided under this License is provided on an "AS IS" basis,
#  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
#  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
#  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
#  See the License for the specific provisions governing your rights and
#  obligations concerning the Software.
#
#  The Initial Developer of the Original Code is: Sun Microsystems, Inc.
#
#  Copyright: 2001 by Sun Microsystems, Inc.
#
#  All Rights Reserved.
#
##########################################################################
#___INFO__MARK_END__

# startup of PVM conforming with Grid Engine
# parallel environment interface
#
# usage: startpvm.sh [options] <pe_hostfile> <master_host> <pvm_root>
#
#        options are:
#                     -ep <path_enhancement>
#                      If this option is given following argument added as 
#
#                         * ep=<path_enhancement>
#
#                      before the first host in the hostfile which is 
#                      generated by starpvm.sh. This causes pvmd to
#                      search binaries also in these pathes. Refer to 
#                      pvmd(1).


# useful to control parameters passed to us  
echo $*


path_enhancement=""
while [ $# -gt 0 ]; do
   case "$1" in
      -ep)
         shift
         path_enhancement=$1
         ;;
      *)
         break;
         ;;
   esac
   shift
done

me=`basename $0`

# test number of args
if [ $# -lt 3 ]; then
   echo "$me: got wrong number of arguments" >&2
   exit 1
fi

# get arguments
pe_hostfile=$1
host=$2
PVM_ROOT=$3
export PVM_ROOT

# get PVM_ARCH by starting $PVM_ROOT/lib/pvmgetarch 
if [ ! -x $PVM_ROOT/lib/pvmgetarch ]; then
   echo "$me: can't execute $PVM_ROOT/lib/pvmgetarch" >&2
   exit 1
fi
PVM_ARCH=`$PVM_ROOT/lib/pvmgetarch`

# ensure we are able to exec our starter
if [ ! -x $SGE_ROOT/pvm/bin/$ARC/start_pvm ]; then
   echo "$me: can't execute $SGE_ROOT/pvm/bin/$ARC/start_pvm" >&2
   exit 1
fi

# ensure we are able to start pvmd
if [ ! -x $PVM_ROOT/lib/pvmd ]; then
   echo "$me: can't execute $PVM_ROOT/lib/pvmd" >&2
   exit 1
fi

# ensure pe_hostfile is readable
if [ ! -r $pe_hostfile ]; then
   echo "$me: can't read $pe_hostfile" >&2
   exit 1
fi

# create pvm_hostfile
# remove column with number of slots per queue
# pvm does not support them in this form
pvm_hostfile="$TMPDIR/hostfile"

# enhance the search path if requested
if [ "x$path_enhancement" != "x" ]; then 
   echo "* ep=$path_enhancement" >> $pvm_hostfile
fi

cut -f1 -d" " $pe_hostfile >> $pvm_hostfile

# PVM 3.4.4 knows a Virtual Machine IDs for supporting 
# overlapping PVM instances of the same user
# PVM_VMID=$JOB_ID; export PVM_VMID

# startup and wait for daemons
$SGE_ROOT/pvm/bin/$ARC/start_pvm -h $NHOSTS $PVM_ROOT/lib/pvmd $pvm_hostfile

# on failure we call our cleanup skript
if [ $? -ne 0 ]; then
   echo "$me: startup failed - invoking cleanup script"
   $SGE_ROOT/pvm/stoppvm.sh $pe_hostfile $host
   exit 1
fi

# signal success to caller
exit 0
