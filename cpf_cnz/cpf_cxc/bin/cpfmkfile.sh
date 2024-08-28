#!/bin/bash
##
# ------------------------------------------------------------------------
#     Copyright (C) 2012 Ericsson AB. All rights reserved.
# ------------------------------------------------------------------------
##
# Name:
#       cpfmkfile.sh
# Description:
#       A script to wrap the invocation of cpfmkfile from the COM CLI.
# Note:
#	None.
##
# Usage:
#	None.
##
# Output:
#       None.
##
# Changelog:
# - Oct 10 2012 - (teidifc)
#	First version.
##

umask 022
/usr/bin/sudo /opt/ap/fms/bin/cpfmkfile "$@"

exit $?
