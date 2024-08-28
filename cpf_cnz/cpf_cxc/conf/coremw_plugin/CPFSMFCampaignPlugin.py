###############################################################
#
#      COPYRIGHT Ericsson 2018
#      All rights reserved.
#
#      The Copyright to the computer program(s) herein
#      is the property of Ericsson 2018.
#      The program(s) may be used and/or copied only with
#      the written permission from Ericsson 2018 or in
#      accordance with the terms and conditions stipulated in
#      the agreement/contract under which the program(s) have
#      been supplied.
#
# Changelog:
# -  12 Nov 2018 -Naveen Kumar G (zgxxnav)
#       First version
#
###############################################################
from tcg.plugin_api.SMFCampaignPlugin import SMFCampaignPlugin
from tcg.plugin_api import SMFConstants
from tcg.plugin_api.SMFCampaignGenerationInfoProvider import SMFCampaignGenerationInfoProvider
from tcg.plugin_api.SMFPluginUtilitiesProvider import SMFPluginUtilitiesProvider
import os

def createSMFCampaignPlugin():
   return SCHSMFCampaignPlugin()

class SCHSMFCampaignPlugin(SMFCampaignPlugin):
   # initialization method
   # self: variable represents the instance of the object itself
   def __init__(self):
      super(SCHSMFCampaignPlugin, self).__init__()
      self.MY_COMPONENT_UID = "fms.cpf"
      self._info = None
      self._utils = None
      self._actionType = None

   def prepare(self, csmModelInformationProvider, pluginUtilitiesProvider):
      # Here we save the references to the providers to be used later
      self._utils = pluginUtilitiesProvider
      self._info = csmModelInformationProvider

#-------------------------------------------------------------------------------------------------------------------------------------------------------------
# CLI actions that need to be executed during campaign initialization phase
# These will be translated to a campInitAction doCLI in the generated campaign
# Second argument in undoCli or doCli represents arguments used with doCliCommand or undoClicommand in generated campaign
# Third argument in the tuple action is representing the Exec Environment, None represents SC-1
#---------------------------------------------------------------------------------------------------------------------------------------------------------------
   def cliAtCampInit(self):
      self._actionType = self._info.getComponentActionType()
      sch = self._info.getComponent("fms.cpf")
      for bundle in sch.swBundles:
         bundleName = bundle[1]
      result = []
      undoCli = ("/bin/true", None)
      if (self._actionType == SMFConstants.CT_INSTALL or self._actionType == SMFConstants.CT_UPGRADE or self._actionType == SMFConstants.CT_MIGRATE):
         cli = os.path.join("$OSAFCAMPAIGNROOT", self.MY_COMPONENT_UID, "scripts", "cpf_helper.sh" )
         args="init"+" "+bundleName
         doCli = (cli, args)
         action =(doCli,undoCli,None)
         result.append(action)
      return result
