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
# -----      ----------   --------  --------------------------
#            2018-04-23   xcsrajn   ESM adoptations  
#            2016-03-02   XSATYRE   updated                                                            
#	           2015-11-23   XSATYRE   updated
#	           2015-08-30   XSATYRE   updated
#	     			 2011-06-29   qvincon		updated
#            2010-11-30   esalves		Created
# **********************************************************************

# BEGIN: GIT INTRO.
CURDIR = $(shell pwd)
REPO_NAME = fms
FMS_ROOT = $(shell echo $(CURDIR) | sed 's@'/$(REPO_NAME)'.*@'/$(REPO_NAME)'@g')
COMMON_ROOT = $(FMS_ROOT)/common

include $(COMMON_ROOT)/common.mk 

CXC_NAME ?=FMS_CPFBIN
CXC_NR ?=CXC1371438_9
CXC_VER ?=R1U
BASE_SW_VER ?= 3.8.0-R1D
CXC_BLK_NAME?=fms_cpf
 
RPM_SPEC = cpf.spec

CXCPATH= $(CURDIR)
CXCDIR=$(CXCPATH)
export CXCDIR
export CXCPATH
FX_CLEAN_TMP := rm -rf /tmp/$(USER)/*

DOXYGEN_OUTPUT=$(DOCDIR)/docgen
BLOCK_NAME=$(CXC_NAME)
BLOCK_VERSION=$(CXC_NAME)_$(CXC_VER)
BLOCK_SEARCH=../cpfapi_caa/src ../cpfapi_caa/inc_ext ../cpfapi_caa/inc_int ../cpfadm_caa/src ../cpfadm_caa/inc ../cpfcmd_caa/src ../cpfcmd_caa/inc
export DOXYGEN_OUTPUT
export BLOCK_NAME
export BLOCK_VERSION
export BLOCK_SEARCH

CPPCKECK_SRCS = ../cpfapi_caa/src/*.cpp ../cpfadm_caa/src/*.cpp ./cpfadm_caa/src/*.c ../cpfcmd_caa/src/*.cpp
CPPCKECK_INCS = -I../cpfapi_caa/inc_int -I../cpfapi_caa/inc_ext -I../cpfadm_caa/inc -I../cpfcmd_caa/inc
CCCC_SRCS = ../cpfapi_caa/src/* ../cpfadm_caa/src/* ../cpfcmd_caa/src/*
SLOCCOUNT_SRCS = ../cpfapi_caa/ ../cpfadm_caa/ ../cpfcmd_caa/

CAA_API_DIR = ../cpfapi_caa
export CAA_API_DIR
			
CAA_ADM_DIR = ../cpfadm_caa
export CAA_ADM_DIR
			
CAA_CMD_DIR = ../cpfcmd_caa
export CAA_CMD_DIR

SUBSYSTEM_DIRS = $(CAA_API_DIR) $(CAA_ADM_DIR) $(CAA_CMD_DIR) 

.PHONY: all
all:
		@for i in $(SUBSYSTEM_DIRS) ; do \
			(echo "+++++++++++++++++" ; \
			echo "cd $$i" ; \
			cd $$i ; \
			$(MAKE) $(MFLAGS) COMMON_ROOT=$(COMMON_ROOT) FMS_ROOT=$(FMS_ROOT) all) ;\
			if [ $$? != 0 ]; \
			then $(ECHO) "Error: please see the printout!!!";\
			exit $$?; \
			fi;\
			echo "+++++++++++++++++" ; \
		done
		$(call FX_CLEAN_TMP)

.PHONY: big
big:
		@for i in $(SUBSYSTEM_DIRS) ; do \
			(echo "+++++++++++++++++" ; \
			echo "cd $$i" ; \
			cd $$i ; \
			$(MAKE) $(MFLAGS)CXCDIR=$(CXCPATH)  big) ;\
			if [ $$? != 0 ]; \
			then $(ECHO) "Error: please see the printout!!!";\
			exit $$?; \
			fi;\
			echo "+++++++++++++++++" ; \
		done
		
.PHONY: release
release: all rpm_package sdp_package tar_package esm_package ap_sdk documentation metrics

.PHONY: rpm_package
rpm_package:
	$(call rpm_file_creation,$(RPM_SPEC),$(BLOCK_NAME),$(CXC_NR),$(CXC_VER))

.PHONY: del_update
del_update:
	$(call rpm_del_update,$(CXCPATH),$(BLOCK_NAME),$(CXC_VER),$(OLD_CXC_VER),$(CXC_NR))
	$(call sdp_del_update,$(CXCPATH),$(BLOCK_NAME),$(CXC_VER),$(OLD_CXC_VER),$(CXC_NR))
	$(call esm_del_update,$(CXCPATH),$(BLOCK_NAME),$(CXC_VER),$(OLD_ESM_VER),$(CXC_NR))

# Perform chain of code analysis tools
######################################################################################
# METRICS
# Perform chain of code analysis tools
######################################################################################
	
.PHONY: metrics
metrics: cppcheck_m cccc_m sloccount_m

######################################################################################
# CPPCHECK
# - analysis tool for C/C++ code
######################################################################################

.PHONY: cppcheck_m
cppcheck_m:
	$(call cppcheck_doc_creation,$(CPPCKECK_SRCS),$(CPPCKECK_INCS))

######################################################################################
# CCCC
# - C and C++ Code Counter
######################################################################################

.PHONY: cccc_m
cccc_m:
	$(call cccc_doc_creation,$(CCCC_SRCS))
   
######################################################################################
# SLOC
# - tools for counting physical Source Lines of Code
######################################################################################

.PHONY: sloccount_m
sloccount_m:
	$(call sloccount_doc_creation,$(SLOCCOUNT_SRCS))

######################################################################################
# DOCUMENTATION
# Generate HTML documentation with Doxygen for each CAA
######################################################################################	

.PHONY: documentation
documentation:
	$(call doc_creation)

.PHONY: doc_package
doc_package: documentation metrics
	$(SILENT)$(ECHO) 'DOC and Metrics packaging ...'
	$(call doc_tarfile_creation,$(CXC_NAME)-$(CXC_NR)-$(CXC_VER)-documentation.tar)

	
.PHONY: clean
clean:
	  @for i in $(SUBSYSTEM_DIRS) ; do \
			(echo "+++++++++++++++++" ; \
			echo "cd $$i" ; \
			cd $$i ; \
			$(MAKE) $(MFLAGS) COMMON_ROOT=$(COMMON_ROOT) FMS_ROOT=$(FMS_ROOT) clean) ;\
			if [ $$? != 0 ]; \
			then $(ECHO) "Error: please see the printout!!!";\
			exit $$?; \
			fi;\
			echo "+++++++++++++++++" ; \
	  done
		$(call FX_CLEAN_TMP)

.PHONY: distclean
distclean:
	  @for i in $(SUBSYSTEM_DIRS) ; do \
			(echo "+++++++++++++++++" ; \
			echo "cd $$i" ; \
			cd $$i ; \
			$(MAKE) $(MFLAGS) COMMON_ROOT=$(COMMON_ROOT) FMS_ROOT=$(FMS_ROOT) distclean) ;\
			if [ $$? != 0 ]; \
			then $(ECHO) "Error: please see the printout!!!";\
			exit $$?; \
			fi;\
			echo "+++++++++++++++++" ; \
	    done
		  $(call FX_CLEAN_TMP)
	    $(SILENT) $(RM) $(RPMDIR)/$(BLOCK_NAME)-$(CXC_NR)-$(CXC_VER).x86_64.rpm
	    echo "+++++++++++++++++" ; \

.PHONY: sdp_package
sdp_package:
	$(SILENT)$(ECHO) 'SDP and UP packaging ...'
	$(SILENT)$(ECHO) 'SDP packaging ...'
	$(call sdp_creation_US3,$(CXCPATH),$(BLOCK_NAME),$(CXC_VER),$(CXC_OLD_VER),$(CXC_NR))
	$(SILENT)$(ECHO) 'UP packaging ...' 
	$(call up_creation,$(CXC_NAME),$(CXC_NR),$(CXC_VER),$(CXC_OLD_VER),$(CXCPATH),$(BASE_SW_VER))
.PHONY: esm_package
esm_package:
	$(call esm_pkg_creation,$(CXCPATH),$(CXC_BLK_NAME),$(CXC_VER),$(BASE_SW_VER),$(CXC_NR),$(CXC_NAME))
                      
.PHONY: upload_package 
upload_package: 
	$(call upload_package,$(CXC_BLK_NAME),$(CXC_NR),$(CXC_NAME),$(CXC_VER))

	
.PHONY: tar_package
tar_package:
	$(SILENT)$(ECHO) 'TAR packaging ...'
	$(SILENT)$(ECHO) '**********************************'
	$(SILENT)$(ECHO) '****** NOT YET IMPLEMENTED *******'
	$(SILENT)$(ECHO) '**********************************'

.PHONY: ap_sdk
ap_sdk:
	$(SILENT)$(ECHO) 'AP SDK updating ...'
	$(SILENT)$(ECHO) '**********************************'
	$(SILENT)$(ECHO) '****** NOT YET IMPLEMENTED *******'
	$(SILENT)$(ECHO) '**********************************'

