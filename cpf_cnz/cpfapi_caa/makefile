# **********************************************************************
#
# Short description:
# Makefile for CPF library
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
# -----      ---------    --------  --------------------------
#            2010-11-30   esalves	Created
#  	     2011-06-07   qvincon	Adjusted
#
# ********************************************************************

CNZ_NAME ?= cpf_cnz
CAA_NAME ?= cpfapi_caa
CXC_NAME ?= cpf_cxc

# Basic paths
VOB_PATH = $(FMS_ROOT)
CNZ_PATH = $(VOB_PATH)/$(CNZ_NAME)
CAA_PATH = $(CNZ_PATH)/$(CAA_NAME)
CXC_PATH = $(CNZ_PATH)/$(CXC_NAME)
                                  
include $(COMMON_ROOT)/common.mk
                                 
DOXYGENFILECONFIG := $(COMMON_ROOT)/doxygen_common.conf

# If the product is a shared object library, here you can specify the name and the version numbers of the library
# You can also specify these variable as arguments of make command.
# For example: make LIB_NAME=ACS_DSD2 LIB_RELEASE_BUILD=1
LIB_NAME = fms_cpf
LIB_RELEASE_MAJOR ?= 10
LIB_RELEASE_MINOR ?= 1
LIB_RELEASE_BUILD ?= 3

#OUTDIR = $(FMS_ROOT)/fms/cpf_cnz/cpf_cxc/bin
OUTDIR = $(CXC_PATH)/bin/lib_ext

## Setting the so library compiler information
LIB_VERSION ?= $(LIB_RELEASE_MAJOR).$(LIB_RELEASE_MINOR).$(LIB_RELEASE_BUILD)
LIB_FILENAME = lib$(LIB_NAME).so.$(LIB_VERSION)
LIB_FILENAME_DBG = $(LIB_FILENAME)_dbg

LIB_SONAME = lib$(LIB_NAME).so.$(LIB_RELEASE_MAJOR)
LIB_LINKER_NAME = lib$(LIB_NAME).so

## Object files
CPF_OBJ = $(OBJDIR)/fms_cpf_attribute.obj \
	$(OBJDIR)/fms_cpf_baseattribute.obj \
	$(OBJDIR)/fms_cpf_exception.obj \
	$(OBJDIR)/fms_cpf_infiniteattribute.obj \
	$(OBJDIR)/fms_cpf_privateexception.obj \
	$(OBJDIR)/fms_cpf_regularattribute.obj \
	$(OBJDIR)/fms_cpf_volumeiterator.obj \
	$(OBJDIR)/fms_cpf_configreader.obj \
	$(OBJDIR)/fms_cpf_eventhandler.obj \
	$(OBJDIR)/fms_cpf_eventimplementation.obj \
	$(OBJDIR)/fms_cpf_eventreport.obj \
	$(OBJDIR)/fms_cpf_eventreporthandler.obj \
	$(OBJDIR)/fms_cpf_eventreporthandlerimpl.obj \
	$(OBJDIR)/fms_cpf_fileid.obj \
	$(OBJDIR)/fms_cpf_fileiterator.obj \
	$(OBJDIR)/fms_cpf_file.obj \
	$(OBJDIR)/fms_cpf_omcmdhandler.obj \
	$(OBJDIR)/fms_cpf_client.obj \
	$(OBJDIR)/fms_cpf_api_trace.obj \
	$(OBJDIR)/fms_cpf_adminoperation.obj \
	

VPATH += $(SRCDIR) $(OUTDIR) $(INCDIR)\inc_ext $(INCDIR)\inc_int $(OBJDIR)

# here you can add own lib paths
LIBSDIR += -L"$(AP_SDK_LIB)"
LIBSDIR += -L"$(ACE_SDK_LIB)"
LIBSDIR += -L"$(COREMW_SDK_LIB)"
LIBSDIR += -L"$(SEC_CREDU_SDK_LIB)"

# here you can add own Include paths and/or other includes
CINCLUDES += -I"./inc_ext"
CINCLUDES += -I"./inc_int"
CINCLUDES += -I"$(AP_SDK_INC)"
CINCLUDES += -I"$(COREMW_SDK_INC)"
CINCLUDES += -I"$(ACE_SDK_PATH)"
CINCLUDES += -I"$(3PP_SDK_PATH)"

# here you can add own libs
LIBS += -lrt -lpthread -lACE
LIBS += -lacs_tra
LIBS += -lacs_csapi
LIBS += -lacs_apgcc
LIBS += -laes_ohi

CPF_APLIB = $(OUTDIR)/$(LIB_FILENAME)
CPF_APLIB_SO = $(OUTDIR)/$(LIB_LINKER_NAME)
CPF_APLIB_MAJOR = $(OUTDIR)/$(LIB_SONAME)

## here you can add own compiler flags
CFLAGS += -O2 -fPIC -fmessage-length=0
#CFLAGS += $(GCOV_FLAGS)

# here you can add own linker flags
LDFLAGS += -shared -Wl,-soname=$(LIB_SONAME)
 
 .PHONY: all
all: $(CPF_OBJ)
	$(NEW_LINE)
	$(SILENT)$(ECHO) 'Creating library: $(LIB_FILENAME)'
		$(SILENT)$(CC) -o $(CPF_APLIB) $(CPF_OBJ) $(LDFLAGS) $(LIBSDIR) $(LIBS)
		$(call check_and_remove_symbolic_link,$(CPF_APLIB_SO))
	
		ln -s $(CPF_APLIB) $(CPF_APLIB_SO) 
		$(call check_and_remove_symbolic_link,$(CPF_APLIB_MAJOR))
	
		ln -s $(CPF_APLIB) $(CPF_APLIB_MAJOR) 
		$(call stripp,$(LIB_FILENAME))
	$(SEPARATOR_STR)
	$(NEW_LINE)
	
.PHONY: clean
clean:
	$(SILENT)$(RM) $(wildcard $(OBJDIR)/*)
    
.PHONY: distclean
distclean: clean
	$(RM) -r $(OUTDIR)/$(LIB_FILENAME)

.PHONY: documentation
documentation:
	$(SILENT)$(ECHO) 'documentation updating ...'
	$(SILENT)$(ECHO) '**********************************'
	$(SILENT)$(ECHO) '****** NOT YET IMPLEMENTED *******'
	$(SILENT)$(ECHO) '**********************************' 
	

.PHONY: metrics
metrics:
	$(SILENT)$(ECHO) 'Metrics updating ...'
	$(SILENT)$(ECHO) '**********************************'
	$(SILENT)$(ECHO) '****** NOT YET IMPLEMENTED *******'
	$(SILENT)$(ECHO) '**********************************'

	
# $(call check_and_remove_symbolic_link,abspath_plus_file_name)
define check_and_remove_symbolic_link
$(SILENT)if [ -L $1 ];\
then\
        $(ECHO) "Symbolic link already exist ... I'm removing it !!!";\
        $(RM) $1 ;\
fi
endef
	

