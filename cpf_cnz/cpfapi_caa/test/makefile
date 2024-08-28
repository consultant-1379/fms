# **********************************************************************
#
# Short description:
# Makefile template for XXX library
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
#            2011-01-28   esalves   Created
#                                                            
# **********************************************************************

CNZ_PATH = /root/workspace/cpf_cnz
CURDIR = $(shell pwd)
REPO_NAME = fms
FMS_ROOT = $(shell echo $(CURDIR) | sed 's@'/$(REPO_NAME)'.*@'/$(REPO_NAME)'@g')
COMMON_ROOT = $(FMS_ROOT)/common
 
include $(COMMON_ROOT)/common.mk

CXCDIR = cpf_cxc
CAADIR = cpfapi_caa
CURDIR = $(CNZ_PATH)/$(CAADIR)

BLOCKDIR = $(CURDIR)
SRCDIR   = $(BLOCKDIR)/src
INCDIR   = $(BLOCKDIR)/inc
#TESTDIR  = $(BLOCKDIR)/test
TESTDIR = /root/workspace/UT_CPF
CUTESRCDIR = $(TESTDIR)/src 
OBJDIR = $(TESTDIR)/obj
BINDIR = $(TESTDIR)/bin
#OBJDIR = /root/workspace/UT_CPF/Debug/cpf_api

# Flexelint application
FL    = $(LINT)     # Global wrap-up mode
FL1   = $(LINT) -u  # Single unit mode

# here you can add own lib paths
LIBSDIR += -L$(CNZ_PATH)/$(CXCDIR)/bin
LIBSDIR  += -L$(AP_SDK_LIB)

# here you can add own compiler flags
#CFLAGS += $(CFLAGS_EXTRA) -fprofile-arcs -ftest-coverage 

# here you can add own Assembler flags
ASMFLAGS += 

## # here you can add own Include paths and/or other includes
CINCLUDES += -I"/root/workspace/UT_CPF/cute"
CINCLUDES += -I"/root/workspace/UT_CPF/src" 
#CINCLUDES += -I"$(CNZ_PATH)/$(CXCDIR)/cpfapi_caa/inc" 
CINCLUDES += -I$(CNZ_PATH)"/cpfapi_caa/inc"
CINCLUDES += -I$(AP_SDK_INC)
 
## # here you can add own libs 
LIBS += -lfms_cpf 
LIBS += -lrt -lpthread 
LIBS += -lACS_TRA
LIBS += -lACS_CSAPI
LIBS += -lz 

LDFLAGS      =  -lgcov

## here you can add own File paths
VPATH +=$(SRCDIR) $(CUTESRCDIR) $(DSDSRCDIR)

## Source files
#

L_FILES += $(SRCFILES:%=$(BLOCKDIR)/src/%) 

SRCFILES = Test.cpp \
	cute_fms_cpf_createfile.cpp \
	cute_fms_cpf_readattr.cpp \
	fms_cpf_attribute.cpp \
	fms_cpf_baseattribute.cpp \
	fms_cpf_exception.cpp \
	fms_cpf_infiniteattribute.cpp \
	fms_cpf_privateexception.cpp \
	fms_cpf_regularattribute.cpp \
	fms_cpf_volumeiterator.cpp \
	fms_cpf_configreader.cpp \
	fms_cpf_eventhandler.cpp \
	fms_cpf_fileid.cpp \
	fms_cpf_fileiterator.cpp \
	fcc_thread.cpp \
	fms_cpf_file.cpp		

## Object files
#
OBJS = \
	$(SRCFILES:%.cpp=%.obj)
	
## Build instructions
#
#APNAME  = BLOCKNAME1API_CXC
APNAME = Test

APLIB = $(TESTDIR)/bin/$(APNAME)

.PHONY: all
all: $(APLIB)

.PHONY: clean
clean:		
		$(SILENT)$(ECHO) 'Cleaning file: $@'
		$(RM) -r $(OBJDIR)/*.obj
		$(RM) -r $(OBJDIR)/*.d
		$(RM) -r $(OBJDIR)/*.gcno
		$(RM) -r $(BINDIR)/*.*

$(APLIB): $(OBJS) $(METRICS)
	    $(NEW_LINE)
	    $(SEPARATOR_STR)
	    $(SILENT)$(ECHO) 'Creating CUTE appl: $@'
		$(SILENT)$(RM) $(APLIB)
		$(CC) -o $(APLIB) $(LIBSDIR) $(LIBS) $(wildcard $(OBJDIR)/*.obj) $(LDFLAGS)
		$(SEPARATOR_STR)
	    $(NEW_LINE)

.PHONY: metrics
metrics : cccc lint

cccc:
	    $(NEW_LINE)
	    $(SEPARATOR_STR)
	    $(SILENT)$(ECHO) 'C/C++ Code Counter file: $@'
#		$(CCCC) $(wildcard $(BLOCKDIR)/inc/*) 
		$(CCCC) $(wildcard $(BLOCKDIR)/src/*) 
	    $(SEPARATOR_STR)

.PHONY: lint
lint:
	    $(NEW_LINE)
	    $(SEPARATOR_STR)
	    $(SILENT)$(ECHO) 'LINT file: $@'
#		$(LINT) $(wildcard $(BLOCKDIR)/inc/*) 
		$(LINT) $(wildcard $(BLOCKDIR)/src/*) 
	    $(SEPARATOR_STR)

.PHONY: depend
depend:	$(SRCFILES)
		makedepend $(CINCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it

