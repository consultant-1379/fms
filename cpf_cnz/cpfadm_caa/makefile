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
# **********************************************************************

SOFTWARE_TYPE ?= app

CNZ_NAME ?= cpf_cnz
CAA_NAME ?= cpfadm_caa
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
APP_FILENAME = fms_cpfd
APP_FILENAME_DBG = fms_cpfd_dbg

## Source directories. Here you can list your source directories. Example: SRCDIR src1 src2 src3 (use spaces)
SRCDIRS ?= src

## Source files
SRCFILES = $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
SRCFILES += $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.cpp))

## Object files
MODULE_OBJS =	\
	$(OBJDIR)/fms_cpf_common.obj \
	$(OBJDIR)/fms_cpf_main.obj \
	$(OBJDIR)/fms_cpf_service.obj \
	$(OBJDIR)/fms_cpf_server.obj \
	$(OBJDIR)/fms_cpf_immhandler.obj \
	$(OBJDIR)/fms_cpf_oi_cpvolume.obj \
	$(OBJDIR)/fms_cpf_cmdhandler.obj \
	$(OBJDIR)/fms_cpf_cmdhandlerexit.obj \
	$(OBJDIR)/fms_cpf_createvolume.obj \
	$(OBJDIR)/fms_cpf_removevolume.obj \
	$(OBJDIR)/fms_cpf_oi_baseobject.obj \
	$(OBJDIR)/fms_cpf_oi_compositefile.obj \
	$(OBJDIR)/fms_cpf_createfile.obj \
	$(OBJDIR)/fms_cpf_filetable.obj \
	$(OBJDIR)/fms_cpf_filereference.obj \
	$(OBJDIR)/fms_cpf_filehandler.obj \
	$(OBJDIR)/fms_cpf_filedescriptor.obj \
	$(OBJDIR)/fms_cpf_fileaccess.obj \
	$(OBJDIR)/fms_cpf_directorystructuremgr.obj \
	$(OBJDIR)/fms_cpf_basefilemgr.obj \
	$(OBJDIR)/fms_cpf_regularfilemgr.obj \
	$(OBJDIR)/fms_cpf_filemgr.obj \
	$(OBJDIR)/fms_cpf_parameterhandler.obj  \
	$(OBJDIR)/fms_cpf_oi_compositesubfile.obj \
	$(OBJDIR)/fms_cpf_cmdlistener.obj  \
	$(OBJDIR)/fms_cpf_clientcmdhandler.obj  \
	$(OBJDIR)/fms_cpf_clientcmd_request.obj  \
	$(OBJDIR)/fms_cpf_deletefile.obj \
	$(OBJDIR)/fms_cpf_changefileattribute.obj \
	$(OBJDIR)/fms_cpf_copyfile.obj \
	$(OBJDIR)/fms_cpf_importfile.obj \
	$(OBJDIR)/fms_cpf_exportfile.obj \
	$(OBJDIR)/fms_cpf_oi_infinitefile.obj \
	$(OBJDIR)/fms_cpf_infinitefilemgr.obj \
	$(OBJDIR)/fms_cpf_message.obj \
	$(OBJDIR)/fms_cpf_datafile.obj \
	$(OBJDIR)/fms_cpf_cpchannelmgr.obj \
	$(OBJDIR)/fms_cpf_cpopenchannels.obj \
	$(OBJDIR)/fms_cpf_cpmsgfactory.obj \
	$(OBJDIR)/fms_cpf_cpchannel.obj \
	$(OBJDIR)/fms_cpf_oc_buffermgr.obj \
	$(OBJDIR)/fms_cpf_sb_ocbuffer.obj \
	$(OBJDIR)/fms_cpf_ex_ocbuffer.obj \
	$(OBJDIR)/fms_cpf_filelock.obj \
	$(OBJDIR)/fms_cpf_cpopenfiles.obj \
	$(OBJDIR)/fms_cpf_cpdopenfilesmgr.obj \
	$(OBJDIR)/fms_cpf_cpdfile.obj \
	$(OBJDIR)/fms_cpf_infinitecpdfile.obj \
	$(OBJDIR)/fms_cpf_regularcpdfile.obj \
	$(OBJDIR)/fms_cpf_infinitefileopenlist.obj \
	$(OBJDIR)/fms_cpf_cpdfilethrd.obj \
	$(OBJDIR)/fms_cpf_cpmsg.obj \
	$(OBJDIR)/fms_cpf_closecpmsg.obj \
	$(OBJDIR)/fms_cpf_echocpmsg.obj \
	$(OBJDIR)/fms_cpf_opencpmsg.obj \
	$(OBJDIR)/fms_cpf_synccpmsg.obj \
	$(OBJDIR)/fms_cpf_readnextcpmsg.obj \
	$(OBJDIR)/fms_cpf_resetcpmsg.obj \
	$(OBJDIR)/fms_cpf_rewritecpmsg.obj \
	$(OBJDIR)/fms_cpf_writenextcpmsg.obj \
	$(OBJDIR)/fms_cpf_readrandcpmsg.obj \
	$(OBJDIR)/fms_cpf_writerandcpmsg.obj \
	$(OBJDIR)/fms_cpf_unknowncpmsg.obj \
	$(OBJDIR)/fms_cpf_connectcpmsg.obj \
	$(OBJDIR)/fms_cpf_releasesubfile.obj \
	$(OBJDIR)/fms_cpf_movefile.obj \
	$(OBJDIR)/fms_cpf_oi_simplefile.obj \
	$(OBJDIR)/fms_cpf_filetqchecker.obj \
	$(OBJDIR)/fms_cpf_tqchecker.obj \
	$(OBJDIR)/fms_cpf_filetransferhndl.obj \
	$(OBJDIR)/fms_cpf_eventalarmhndl.obj \
	$(OBJDIR)/fms_cpf_jtpconnectionsmgr.obj \
	$(OBJDIR)/fms_cpf_jtpconnectionhndl.obj \
	$(OBJDIR)/fms_cpf_jtpcpmsg.obj \
	$(OBJDIR)/fms_cpf_renamefile.obj \
	$(OBJDIR)/fms_cpf_oi_cpfroot.obj \
	$(OBJDIR)/fms_cpf_blocktransfermgr.obj \
	$(OBJDIR)/fms_cpf_blocksender_scheduler.obj \
	$(OBJDIR)/fms_cpf_blocksender.obj \
	$(OBJDIR)/fms_cpf_blocksender_logger.obj \
	
	
## here you can add own lib paths
LIBSDIR += -L"$(OUTDIR)/lib_ext"
LIBSDIR += -L"$(AP_SDK_LIB)"
LIBSDIR += -L"$(ACE_SDK_LIB)"
LIBSDIR += -L"$(COREMW_SDK_LIB)"
LIBSDIR += -L"$(BOOST_SDK_LIB)"
LIBSDIR += -L"$(SEC_CREDU_SDK_LIB)"

## here you can add own compiler flags
CFLAGS +=

# here you can add own linker flags
LDFLAGS += -Wl,-rpath-link,$(AP_SDK_LIB) -Wl,-rpath-link,$(COREMW_SDK_LIB) -Wl,-rpath-link,$(SEC_SDK_LIB) -Wl,-rpath-link,$(ACE_SDK_LIB) 
LDFLAGS += -Wl,-rpath-link,$(SEC_CREDU_SDK_LIB) -Wl,-rpath-link,$(BOOST_SDK_LIB) -Wl,-rpath-link,$(LIB_XERCES_SDK_LIB)

OUT_FILENAME = $(APP_FILENAME)

# here you can add own Include paths and/or other includes before those included by common.mk file
CINCLUDES += -I"$(CAA_API_DIR)/inc_ext"
CINCLUDES += -I"$(CAA_API_DIR)/inc_int"
CINCLUDES += -I"$(INCDIR)"
CINCLUDES += -I"$(AP_SDK_INC)"
CINCLUDES += -I"$(ACE_SDK_INC)"
CINCLUDES += -I"$(COREMW_SDK_INC)"
CINCLUDES += -I"$(BOOST_SDK_INC)"
CINCLUDES += -I"$(3PP_SDK_PATH)"

# here you can add own libs -lfms_cpf
LIBS += -lrt -lm -lpthread -lACE -lboost_filesystem -lboost_system -lacs_dsd -lacs_csapi -lacs_apgcc -lacs_tra -lacs_jtp -lfms_cpf -laes_ohi -lcap -lacs_aeh

VPATH += $(SRCDIR) $(OUTDIR) $(INCDIR) $(OBJDIR)

.PHONY: all CFLAGS += $(GCOV_FLAGS)
all: $(OUTDIR)/$(OUT_FILENAME)

$(OUTDIR)/$(OUT_FILENAME): $(MODULE_OBJS)
	$(NEW_LINE)
	$(SEPARATOR_STR)
ifeq ($(SOFTWARE_TYPE),so)
		$(SILENT)$(ECHO) 'Creating library: $(OUT_FILENAME)'
else
		$(SILENT)$(ECHO) 'Creating Application: $(OUT_FILENAME)'
endif
	$(SILENT)$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $(OUTDIR)/$(OUT_FILENAME) $(MODULE_OBJS) $(LDFLAGS) $(LIBSDIR) $(LIBS)
	$(call stripp,$(APP_FILENAME))
	$(SEPARATOR_STR)
	$(NEW_LINE)

.PHONY: clean
clean:
	$(SILENT)$(RM) $(wildcard $(OBJDIR)/*)
	
.PHONY: distclean
distclean: clean
	$(SILENT)$(RM) -r $(OUTDIR)/$(OUT_FILENAME)

	

