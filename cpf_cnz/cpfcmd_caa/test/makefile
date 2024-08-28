# **********************************************************************
#
# Short description:
# Makefile template for CPF service binary
# **********************************************************************
#
# Ericsson AB 2010 All rights reserved.
# The information in this document is the property of Ericsson.
# Except as specifically authorized in writing by Ericsson, the receiver of this
# document shall keep the information contained herein confidential and shall protect
# the same in whole or in part from disclosure and dissemination to third parties.
# Disclosure and disseminations to the receivers employees shall only be made
# on a strict need to know basis.
#
# **********************************************************************
#
# Rev        Date         Name      What
# -----      -------      --------  --------------------------
#            2011-06-14   qvincon   Created
#            2011-07-25   ecarast   Adjusted
# **********************************************************************

SOFTWARE_TYPE ?= app

CURDIR = $(shell pwd)
REPO_NAME = fms
FMS_ROOT = $(shell echo $(CURDIR) | sed 's@'/$(REPO_NAME)'.*@'/$(REPO_NAME)'@g')
COMMON_ROOT = $(FMS_ROOT)/common
 
include $(COMMON_ROOT)/common.mk

MODULE_NAME = CFP_DAEMON
MODULE_VERSION = 1_0
MODULE_SEARCH = ./

OUTDIR = ./
OBJDIR = ./
CAA_API_DIR = $(FMS_ROOT)/cpf_cnz/cpfapi_caa/

# Setting the application name
API_TEST = cpftest

## Source directories. Here you can list your source directories. Example: SRCDIR src1 src2 src3 (use spaces)
SRCDIRS ?= ./

## Object files
API_TEST_OBJS =	\
	$(OBJDIR)/cpftest_main.obj \
	$(OBJDIR)/cpftest.obj \

## here you can add own lib paths
LIBSDIR += -L"$(FMS_ROOT)/cpf_cnz/cpf_cxc/bin/lib_ext"
LIBSDIR += -L"$(AP_SDK_LIB)"

## here you can add own compiler flags
CFLAGS +=

# here you can add own linker flags
LDFLAGS +=

CPFMKFILE_OUT_FILENAME = $(API_TEST)

# here you can add own Include paths and/or other includes before those included by common.mk file
CINCLUDES += -I"$(CAA_API_DIR)/inc_ext"
CINCLUDES += -I"$(CAA_API_DIR)/inc_int"
CINCLUDES += -I"$(AP_SDK_INC)"
CINCLUDES += -I"$(3PP_SDK_PATH)"

# here you can add own libs -lfms_cpf
LIBS += -lrt -lpthread -lacs_dsd -lacs_csapi -lacs_apgcc -lfms_cpf

VPATH += $(SRCDIR) $(OUTDIR) $(INCDIR) $(OBJDIR)

.PHONY: all CFLAGS += $(GCOV_FLAGS)
all: $(OUTDIR)/$(CPFMKFILE_OUT_FILENAME) \
	 

$(OUTDIR)/$(CPFMKFILE_OUT_FILENAME): $(API_TEST_OBJS)
	$(NEW_LINE)
	$(SEPARATOR_STR)
ifeq ($(SOFTWARE_TYPE),so)
		$(SILENT)$(ECHO) 'Creating library: $(CPFMKFILE_OUT_FILENAME)'
else
		$(SILENT)$(ECHO) 'Creating Application: $(CPFMKFILE_OUT_FILENAME)'
endif
	$(SILENT)$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $(OUTDIR)/$(CPFMKFILE_OUT_FILENAME) $(API_TEST_OBJS) $(LDFLAGS) $(LIBSDIR) $(LIBS) 
	$(SEPARATOR_STR) 
	$(NEW_LINE)

.PHONY: clean
clean:
	$(SILENT)$(RM) $(wildcard $(OBJDIR)/*)
	

.PHONY: distclean
distclean: clean
	$(SILENT)$(RM) -r $(OUTDIR)/$(CPFMKFILE_OUT_FILENAME)
	$(SILENT)$(RM) -r $(OUTDIR)/$(CPFMKVOL_OUT_FILENAME)
	$(SILENT)$(RM) -r $(OUTDIR)/$(CPFLS_OUT_FILENAME)

.PHONY: documentation
documentation:
	$(SILENT)$(ECHO) 'documentation updating ...'
	$(SILENT)$(ECHO) ' Removing $(DOXYGEN_OUTPUT) directory'
	$(SILENT)$(RM) -r -f $(DOXYGEN_OUTPUT)
	$(SILENT)$(ECHO) ' Creating $(DOXYGEN_OUTPUT) directory'
	$(SILENT)mkdir $(DOXYGEN_OUTPUT)
	$(SILENT)$(DOXYGEN) -g $(DOXYGENFILECONFIG)

.PHONY: metrics
metrics:
	$(SILENT)$(ECHO) 'Metrics updating ...'
	$(SILENT)$(ECHO) '**********************************'
	$(SILENT)$(ECHO) '****** NOT YET IMPLEMENTED *******'
	$(SILENT)$(ECHO) '**********************************'

