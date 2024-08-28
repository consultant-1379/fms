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

CNZ_NAME ?= cpf_cnz
CAA_NAME ?= cpfcmd_caa
CXC_NAME ?= cpf_cxc

# Basic paths
VOB_PATH = $(FMS_ROOT)
CNZ_PATH = $(VOB_PATH)/$(CNZ_NAME)
CAA_PATH = $(CNZ_PATH)/$(CAA_NAME)
CXC_PATH = $(CNZ_PATH)/$(CXC_NAME)
                                  
include $(COMMON_ROOT)/common.mk
                                 
DOXYGENFILECONFIG := $(COMMON_ROOT)/doxygen_common.conf

MODULE_NAME = CFP_DAEMON
MODULE_VERSION = 1_0
MODULE_SEARCH = ./src ./inc
export DOXYGEN_OUTPUT
export MODULE_NAME
export MODULE_VERSION
export MODULE_SEARCH

# Setting the application name
CPFMKFILE_APP_FILENAME = cpfmkfile
CPFMKVOL_APP_FILENAME = cpfmkvol
CPFLS_APP_FILENAME = cpfls
CPFRM_APP_FILENAME = cpfrm
CPFRENAME_APP_FILENAME = cpfrename
CPFPORT_APP_FILENAME = cpfport
CPFCP_APP_FILENAME = cpfcp

## Source directories. Here you can list your source directories. Example: SRCDIR src1 src2 src3 (use spaces)
SRCDIRS ?= src

#CAA_API_DIR = ../cpfapi_caa

## Source files
SRCFILES = $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
SRCFILES += $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.cpp))

## Object files
CPFMKFILE_MODULE_OBJS =	\
	$(OBJDIR)/FMS_CPF_CommandInterface.obj \
	$(OBJDIR)/FMS_CPF_ServerStub.obj \
	$(OBJDIR)/FMS_CPF_command.obj \
	$(OBJDIR)/FMS_CPF_mkfile.obj 

## Object files
CPFMKVOL_MODULE_OBJS =	\
	$(OBJDIR)/FMS_CPF_CommandInterface.obj \
	$(OBJDIR)/FMS_CPF_ServerStub.obj \
	$(OBJDIR)/FMS_CPF_command.obj \
	$(OBJDIR)/FMS_CPF_mkvolume.obj 

## Object files
CPFLS_MODULE_OBJS =	\
	$(OBJDIR)/FMS_CPF_CommandInterface.obj \
	$(OBJDIR)/FMS_CPF_ServerStub.obj \
	$(OBJDIR)/FMS_CPF_command.obj \
	$(OBJDIR)/fms_cpf_trace.obj \
	$(OBJDIR)/fms_cpf_ls.obj 
	
## Object files
CPFRM_MODULE_OBJS =	\
	$(OBJDIR)/FMS_CPF_CommandInterface.obj \
	$(OBJDIR)/FMS_CPF_ServerStub.obj \
	$(OBJDIR)/FMS_CPF_command.obj \
	$(OBJDIR)/fms_cpf_trace.obj \
	$(OBJDIR)/fms_cpf_rm.obj 
	
## Object files
CPFRENAME_MODULE_OBJS =	\
	$(OBJDIR)/FMS_CPF_command.obj \
	$(OBJDIR)/fms_cpf_trace.obj \
	$(OBJDIR)/fms_cpf_rename.obj 

## Object files
CPFPORT_MODULE_OBJS =	\
	$(OBJDIR)/FMS_CPF_command.obj \
	$(OBJDIR)/fms_cpf_trace.obj \
	$(OBJDIR)/fms_cpf_port.obj 

## Object files
CPFCP_MODULE_OBJS =	\
	$(OBJDIR)/FMS_CPF_command.obj \
	$(OBJDIR)/fms_cpf_trace.obj \
	$(OBJDIR)/fms_cpf_cp.obj 

## here you can add own lib paths
LIBSDIR += -L"$(OUTDIR)/lib_ext"
LIBSDIR += -L"$(AP_SDK_LIB)"
LIBSDIR += -L"$(ACE_SDK_LIB)"
LIBSDIR += -L"$(COREMW_SDK_LIB)"
LIBSDIR += -L"$(SEC_CREDU_SDK_LIB)"
LIBSDIR += -L"$(BOOST_SDK_LIB)"

## here you can add own compiler flags
CFLAGS += -O3

# here you can add own linker flags
LDFLAGS += -Wl,-rpath-link,$(AP_SDK_LIB) -Wl,-rpath-link,$(COREMW_SDK_LIB) -Wl,-rpath-link,$(SEC_SDK_LIB) -Wl,-rpath-link,$(ACE_SDK_LIB) 
LDFLAGS += -Wl,-rpath-link,$(SEC_CREDU_SDK_LIB) -Wl,-rpath-link,$(BOOST_SDK_LIB) -Wl,-rpath-link,$(LIB_XERCES_SDK_LIB)

CPFMKFILE_OUT_FILENAME = $(CPFMKFILE_APP_FILENAME)
CPFMKFILE_OUT_FILENAME_DBG = $(CPFMKFILE_OUT_FILENAME)_dbg

CPFMKVOL_OUT_FILENAME = $(CPFMKVOL_APP_FILENAME)
CPFMKVOL_OUT_FILENAME_DBG = $(CPFMKVOL_OUT_FILENAME)_dbg

CPFLS_OUT_FILENAME = $(CPFLS_APP_FILENAME)
CPFLS_OUT_FILENAME_DBG = $(CPFLS_OUT_FILENAME)_dbg

CPFRM_OUT_FILENAME = $(CPFRM_APP_FILENAME)
CPFRM_OUT_FILENAME_DBG = $(CPFRM_OUT_FILENAME)_dbg

CPFRENAME_OUT_FILENAME = $(CPFRENAME_APP_FILENAME)
CPFRENAME_OUT_FILENAME_DBG = $(CPFRENAME_OUT_FILENAME)_dbg

CPFPORT_OUT_FILENAME = $(CPFPORT_APP_FILENAME)
CPFPORT_OUT_FILENAME_DBG = $(CPFPORT_OUT_FILENAME)_dbg

CPFCP_OUT_FILENAME = $(CPFCP_APP_FILENAME)
CPFCP_OUT_FILENAME_DBG = $(CPFCP_OUT_FILENAME)_dbg

# here you can add own Include paths and/or other includes before those included by common.mk file
CINCLUDES += -I"$(CAA_API_DIR)/inc_ext"
CINCLUDES += -I"$(CAA_API_DIR)/inc_int"
CINCLUDES += -I"$(INCDIR)"
CINCLUDES += -I"$(AP_SDK_INC)"
CINCLUDES += -I"$(ACE_SDK_INC)"
CINCLUDES += -I"$(COREMW_SDK_INC)"
CINCLUDES += -I"$(3PP_SDK_PATH)"
CINCLUDES += -I"$(BOOST_SDK_INC)"

# here you can add own libs -lfms_cpf
LIBS += -lrt -lpthread -lACE -lfms_cpf -lacs_tra -lacs_csapi -lacs_apgcc -lboost_filesystem -lboost_system -lboost_thread     

VPATH += $(SRCDIR) $(OUTDIR) $(INCDIR) $(OBJDIR)

.PHONY: all CFLAGS += $(GCOV_FLAGS)
all: $(OUTDIR)/$(CPFMKFILE_OUT_FILENAME) \
	$(OUTDIR)/$(CPFMKVOL_OUT_FILENAME) \
	$(OUTDIR)/$(CPFLS_OUT_FILENAME) \
	$(OUTDIR)/$(CPFRM_OUT_FILENAME) \
	$(OUTDIR)/$(CPFRENAME_OUT_FILENAME) \
	$(OUTDIR)/$(CPFPORT_OUT_FILENAME) \
    $(OUTDIR)/$(CPFCP_OUT_FILENAME)
 

$(OUTDIR)/$(CPFMKFILE_OUT_FILENAME): $(CPFMKFILE_MODULE_OBJS)
	$(NEW_LINE)
	$(SEPARATOR_STR)
ifeq ($(SOFTWARE_TYPE),so)
		$(SILENT)$(ECHO) 'Creating library: $(CPFMKFILE_OUT_FILENAME)'
else
		$(SILENT)$(ECHO) 'Creating Application: $(CPFMKFILE_OUT_FILENAME)'
endif
	$(SILENT)$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $(OUTDIR)/$(CPFMKFILE_OUT_FILENAME) $(CPFMKFILE_MODULE_OBJS) $(LDFLAGS) $(LIBSDIR) $(LIBS) 
	$(call stripp,$(CPFMKFILE_OUT_FILENAME))
	$(SEPARATOR_STR) 
	$(NEW_LINE)

$(OUTDIR)/$(CPFMKVOL_OUT_FILENAME): $(CPFMKVOL_MODULE_OBJS)
	$(NEW_LINE)
	$(SEPARATOR_STR)
ifeq ($(SOFTWARE_TYPE),so)
		$(SILENT)$(ECHO) 'Creating library: $(CPFMKVOL_OUT_FILENAME)'
else
		$(SILENT)$(ECHO) 'Creating Application: $(CPFMKVOL_OUT_FILENAME)'
endif
	$(SILENT)$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $(OUTDIR)/$(CPFMKVOL_OUT_FILENAME) $(CPFMKVOL_MODULE_OBJS) $(LDFLAGS) $(LIBSDIR) $(LIBS)
	$(call stripp,$(CPFMKVOL_OUT_FILENAME))
	$(SEPARATOR_STR)
	$(NEW_LINE)

$(OUTDIR)/$(CPFLS_OUT_FILENAME): $(CPFLS_MODULE_OBJS)
	$(NEW_LINE)
	$(SEPARATOR_STR)
ifeq ($(SOFTWARE_TYPE),so)
		$(SILENT)$(ECHO) 'Creating library: $(CPFLS_OUT_FILENAME)'
else
		$(SILENT)$(ECHO) 'Creating Application: $(CPFLS_OUT_FILENAME)'
endif
	$(SILENT)$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $(OUTDIR)/$(CPFLS_OUT_FILENAME) $(CPFLS_MODULE_OBJS) $(LDFLAGS) $(LIBSDIR) $(LIBS)
	$(SEPARATOR_STR)
	$(call stripp,$(CPFLS_OUT_FILENAME))
	$(NEW_LINE)

$(OUTDIR)/$(CPFRM_OUT_FILENAME): $(CPFRM_MODULE_OBJS)
	$(NEW_LINE)
	$(SEPARATOR_STR)
ifeq ($(SOFTWARE_TYPE),so)
		$(SILENT)$(ECHO) 'Creating library: $(CPFRM_OUT_FILENAME)'
else
		$(SILENT)$(ECHO) 'Creating Application: $(CPFRM_OUT_FILENAME)'
endif
	$(SILENT)$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $(OUTDIR)/$(CPFRM_OUT_FILENAME) $(CPFRM_MODULE_OBJS) $(LDFLAGS) $(LIBSDIR) $(LIBS)
	$(call stripp,$(CPFRM_OUT_FILENAME))
	$(SEPARATOR_STR)
	$(NEW_LINE)
	
$(OUTDIR)/$(CPFRENAME_OUT_FILENAME): $(CPFRENAME_MODULE_OBJS)
	$(NEW_LINE)
	$(SEPARATOR_STR)
ifeq ($(SOFTWARE_TYPE),so)
		$(SILENT)$(ECHO) 'Creating library: $(CPFRENAME_OUT_FILENAME)'
else
		$(SILENT)$(ECHO) 'Creating Application: $(CPFRENAME_OUT_FILENAME)'
endif
	$(SILENT)$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $(OUTDIR)/$(CPFRENAME_OUT_FILENAME) $(CPFRENAME_MODULE_OBJS) $(LDFLAGS) $(LIBSDIR) $(LIBS)
	$(call stripp,$(CPFRENAME_OUT_FILENAME))
	$(SEPARATOR_STR)
	$(NEW_LINE)

$(OUTDIR)/$(CPFPORT_OUT_FILENAME): $(CPFPORT_MODULE_OBJS)
	$(NEW_LINE)
	$(SEPARATOR_STR)
ifeq ($(SOFTWARE_TYPE),so)
		$(SILENT)$(ECHO) 'Creating library: $(CPFPORT_OUT_FILENAME)'
else
		$(SILENT)$(ECHO) 'Creating Application: $(CPFPORT_OUT_FILENAME)'
endif
	$(SILENT)$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $(OUTDIR)/$(CPFPORT_OUT_FILENAME) $(CPFPORT_MODULE_OBJS) $(LDFLAGS) $(LIBSDIR) $(LIBS)
	$(call stripp,$(CPFPORT_OUT_FILENAME))
	$(SEPARATOR_STR)
	$(NEW_LINE)

$(OUTDIR)/$(CPFCP_OUT_FILENAME): $(CPFCP_MODULE_OBJS)
	$(NEW_LINE)
	$(SEPARATOR_STR)
ifeq ($(SOFTWARE_TYPE),so)
		$(SILENT)$(ECHO) 'Creating library: $(CPFCP_OUT_FILENAME)'
else
		$(SILENT)$(ECHO) 'Creating Application: $(CPFCP_OUT_FILENAME)'
endif
	$(SILENT)$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $(OUTDIR)/$(CPFCP_OUT_FILENAME) $(CPFCP_MODULE_OBJS) $(LDFLAGS) $(LIBSDIR) $(LIBS)
	$(call stripp,$(CPFCP_OUT_FILENAME))
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
	$(SILENT)$(RM) -r $(OUTDIR)/$(CPFRM_OUT_FILENAME)
	$(SILENT)$(RM) -r $(OUTDIR)/$(CPFRENAME_OUT_FILENAME)
	$(SILENT)$(RM) -r $(OUTDIR)/$(CPFPORT_OUT_FILENAME)
	$(SILENT)$(RM) -r $(OUTDIR)/$(CPFCP_OUT_FILENAME)

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

