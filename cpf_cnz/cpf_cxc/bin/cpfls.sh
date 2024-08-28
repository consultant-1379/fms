#!/bin/bash
##
# ------------------------------------------------------------------------
#     Copyright (C) 2012 Ericsson AB. All rights reserved.
# ------------------------------------------------------------------------
##
# Name:
#       cpfls.sh
# Description:
#       A script to wrap the invocation of cpfls from the COM CLI.
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

/usr/bin/sudo /opt/ap/fms/bin/cpfls "$@"

exit $?