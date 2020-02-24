/******************************************************************************

         Copyright (c) 2018 - 2019 Intel Corporation

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*******************************************************************************/

#if defined(INCLUDE_DSL_JSON_PARSING) && (INCLUDE_DSL_JSON_PARSING == 1)

#include "json.h"
#include "dsl_cpe_control.h"
#include "dsl_cpe_configuration_parser.h"

#include "drv_dsl_cpe_api_ioctl.h"
#if defined(INCLUDE_DSL_CPE_API_VRX)
/* MEI CPE driver specific header (e.g. IOCtls) */
#include "drv_mei_cpe_interface.h"
#endif /* defined(INCLUDE_DSL_CPE_API_VRX) */

#undef DSL_CCA_DBG_BLOCK
#define DSL_CCA_DBG_BLOCK DSL_CCA_DBG_APP

#define CONFIG_ROOT_NAME "Root"
#define CONFIG_SECTION_PATH_SEPARATOR "."

/* DSL sections */
#define CONFIG_DSL_SECTION_PATH \
   CONFIG_ROOT_NAME CONFIG_SECTION_PATH_SEPARATOR "dsl"

#define CONFIG_DEBUGGING_SECTION_PATH \
   CONFIG_DSL_SECTION_PATH CONFIG_SECTION_PATH_SEPARATOR "debugging"

#define CONFIG_STARTUPINIT_SECTION_PATH \
   CONFIG_DSL_SECTION_PATH CONFIG_SECTION_PATH_SEPARATOR "startup_init"

#define CONFIG_BITSWAP_SECTION_PATH \
   CONFIG_DSL_SECTION_PATH CONFIG_SECTION_PATH_SEPARATOR "bitswap"

#define CONFIG_RETRANSMISSION_SECTION_PATH \
   CONFIG_DSL_SECTION_PATH CONFIG_SECTION_PATH_SEPARATOR "retransmission"

#define CONFIG_SRA_SECTION_PATH \
   CONFIG_DSL_SECTION_PATH CONFIG_SECTION_PATH_SEPARATOR "sra"

#define CONFIG_VIRTUALNOISE_SECTION_PATH \
   CONFIG_DSL_SECTION_PATH CONFIG_SECTION_PATH_SEPARATOR "virtual_noise"

#define CONFIG_REBOOTCRITERIA_SECTION_PATH \
   CONFIG_DSL_SECTION_PATH CONFIG_SECTION_PATH_SEPARATOR "reboot_criteria"

#define CONFIG_XTSE_SECTION_PATH \
   CONFIG_DSL_SECTION_PATH CONFIG_SECTION_PATH_SEPARATOR "xtse"

#define CONFIG_TCLAYER_SECTION_PATH \
   CONFIG_DSL_SECTION_PATH CONFIG_SECTION_PATH_SEPARATOR "tc_layer"

#define CONFIG_COMMON_SECTION_PATH \
   CONFIG_DSL_SECTION_PATH CONFIG_SECTION_PATH_SEPARATOR "common"

/* DSL Common subsections */
#define CONFIG_LOWLEVELHSTONES_SECTION_PATH \
   CONFIG_COMMON_SECTION_PATH CONFIG_SECTION_PATH_SEPARATOR "low_level_hs_tones"

#define CONFIGURATION_VALUE_UNDEFINED 0x7FFFFFFF

#define PARSE_SECTION_INT(Section, ParameterName, ConfigField) \
   do { \
      if (bFound == DSL_FALSE && \
          strstr(pSectionPath, Section CONFIG_SECTION_PATH_SEPARATOR ParameterName) \
         != NULL) \
      { \
         ConfigField = nIntVal; \
         bFound = DSL_TRUE; \
      } \
   } \
   while(0)

#define PARSE_SECTION_STRING(Section, ParameterName, ConfigField) \
   do { \
      if (bFound == DSL_FALSE && \
          strstr(pSectionPath, Section CONFIG_SECTION_PATH_SEPARATOR ParameterName) \
         != NULL) \
      { \
         cpe_control_strncpy_s( \
            ConfigField, sizeof(ConfigField), nStringVal, sizeof(nStringVal)); \
         bFound = DSL_TRUE; \
      } \
   } \
   while(0)

#define PARSE_SECTION_HEX(Section, ParameterName, ConfigField) \
   do { \
      if (bFound == DSL_FALSE && \
          strstr(pSectionPath, Section CONFIG_SECTION_PATH_SEPARATOR ParameterName) \
         != NULL) \
      { \
         ConfigField = (DSL_uint32_t)(strtol(nStringVal, NULL, 16)); \
         bFound = DSL_TRUE; \
      } \
   } \
   while(0)

/* Macro used to check if configuration parameter is in valid range */
#define CONFIG_PARAM_RANGE_VALIDATION(Param, nMinVal, nMaxVal) \
   do { \
      int val = Param; \
      if (val == CONFIGURATION_VALUE_UNDEFINED) \
      { \
            break; \
      } \
      if ((val > nMaxVal) || (val < nMinVal)) \
      { \
         DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, \
            (DSL_CPE_PREFIX"ERROR - Configuration param %s " \
            "validation failed! (min = %d, max = %d, current = %d)!" \
            DSL_CPE_CRLF, #Param, nMinVal, nMaxVal, val)); \
         return DSL_ERROR; \
      } \
   } \
   while(0)

/* Macro used to check if parameter in DSL_CPE_Configuration_t (configParam)
is undefined, if so use currentParam (from DSL_xxxGet function or any other source) */
#define CONFIG_PARAM_APPLY(currentParam, configParam) \
   do { \
      currentParam = (configParam == CONFIGURATION_VALUE_UNDEFINED) ? currentParam : configParam; \
   } \
   while(0)

#define SECTION_PATH_LENGTH 256

/* Configuration file path */
static const char *g_pJsonConfigPath = "/opt/intel/etc/dsl_cpe_control.conf";
static DSL_Configuration_t g_sConfiguration = { 0 };

/**********************************************/
/* start local function declarations          */
/**********************************************/

static DSL_Error_t DSL_CPE_JsonParseConfigFile(
   const DSL_char_t * pPath,
   DSL_Configuration_t *pData
);

static DSL_void_t DSL_CPE_JsonReadObj(
   json_object * pObj,
   DSL_char_t * pSectionPath,
   DSL_Configuration_t *pData
);

static DSL_void_t DSL_CPE_JsonParse(
   json_object * pObj,
   DSL_char_t * pSectionPath,
   DSL_Configuration_t *pData
);

static DSL_void_t DSL_CPE_JsonParseArray(
   json_object * pObj,
   DSL_char_t * pKey,
   DSL_char_t * pSectionPath,
   DSL_Configuration_t *pData
);

static DSL_Error_t DSL_CPE_ConfigRead();

static DSL_Error_t DSL_CPE_ConfigValidateAndApply(
   DSL_CPE_Control_Context_t *pCtrlCtx
);

static DSL_Error_t DSL_CPE_LowLevelCfgValidateAndApply(
   DSL_CPE_Control_Context_t *pControlContext,
   const DSL_uint32_t nDevNum
);

static DSL_Error_t DSL_CPE_LineFeatureCfgValidateAndApply(
   DSL_CPE_Control_Context_t *pControlContext,
   const DSL_uint32_t nDevNum
);

static DSL_Error_t DSL_CPE_RebootCriteriaCfgValidateAndApply(
   DSL_CPE_Control_Context_t *pControlContext,
   const DSL_uint32_t nDevNum
);

static DSL_Error_t DSL_CPE_VdslProfileCfgValidateAndApply(
   DSL_CPE_Control_Context_t *pControlContext,
   const DSL_uint32_t nDevNum
);

static DSL_Error_t DSL_CPE_G997_RateAdaptationCfgValidateAndApply(
   DSL_CPE_Control_Context_t *pControlContext,
   const DSL_uint32_t nDevNum
);

static DSL_Error_t DSL_CPE_SysIfCfgValidateAndApply(
   DSL_CPE_Control_Context_t *pControlContext,
   const DSL_uint32_t nDevNum
);

static DSL_Error_t DSL_CPE_XtseCfgValidateAndApply(
   DSL_CPE_Control_Context_t *pControlContext,
   const DSL_uint32_t nDevNum
);

static DSL_Error_t DSL_CPE_VectoringEnableCfgApply(
   const DSL_uint32_t nDevNum
);

static DSL_Error_t DSL_CPE_FwMessagePollingCfgValidateAndApply(
   DSL_CPE_Control_Context_t *pControlContext,
   const DSL_uint32_t nDevNum
);

static DSL_Error_t DSL_CPE_OperatorSelectCfgValidateAndApply(
   DSL_CPE_Control_Context_t *pControlContext,
   const DSL_uint32_t nDevNum
);

/**********************************************/
/* start interface function definitions       */
/**********************************************/

/*
   For a detailed description of the function, its arguments and return value
   please refer to the description in the header file 'dsl_cpe_configuration_parser.h'
*/
DSL_Error_t DSL_CPE_ConfigInit()
{
   static DSL_boolean_t bInitDone = DSL_FALSE;

   if (bInitDone == DSL_FALSE)
   {
      if (DSL_CPE_ConfigRead() == DSL_ERROR)
      {
         /* keep default configuration just awhile
            to reduce memory consumption */
         DSL_ConfigurationData_t sDefaultConfigData =
         {
            /* DSL_CFG_Debugging_t */
            { 1 },

            /* DSL_CFG_StartupInit_t */
            { 0, 1, 1, 1 },

            /* DSL_CFG_Bitswap_t */
            { 1, 1, 1, 1 },

            /* DSL_CFG_Retransmission_t */
            { 0, 0, 1, 1 },

            /* DSL_CFG_Sra_t */
            { 0, 0, 1, 1 },

            /* DSL_CFG_VirtualNoise_t */
            { 1, 1 },

            /* DSL_CFG_RebootCriteria_t */
            { 0x4F, 0xF },

            /* DSL_CFG_SystemInterface_t */
            { 0, 0 },

            /* DSL_CFG_Common_t */
            {
               /* OperatorSelect */
               0,

               /* LdAfeShutdown */
               1,

               /* VectoringEnable */
               0,

               /* VdslProfileVal */
               0x1FF,

               /* ActSeq */
               1,

               /* ActMode */
               1,

               /* Remember */
               0,

               /* FWMsgPollingOnly */
               0,

               /* DSL_CFG_LowLevelHsTones_t */
               { 0, 0 }
            },

            /* xtse */
            { 0x4, 0x0, 0x4, 0x0, 0xC, 0x1, 0x0, 0x7 }
         };
         DSL_Configuration_t sDefaultConfig = { 0 };

         cpe_control_memcpy_s(
            &(sDefaultConfig.data), sizeof(DSL_ConfigurationData_t),
            &sDefaultConfigData, sizeof(DSL_ConfigurationData_t));

         cpe_control_memcpy_s(
            &g_sConfiguration, sizeof(DSL_Configuration_t),
            &sDefaultConfig, sizeof(DSL_Configuration_t));

         DSL_CCA_DEBUG(DSL_CCA_DBG_WRN, (DSL_CPE_PREFIX
            "WARNING - Configuration initialized with default values!" DSL_CPE_CRLF));
      }
      bInitDone = DSL_TRUE;

      return DSL_TRUE;
   }
   else
   {
      return DSL_FALSE;
   }
}

/*
   For a detailed description of the function, its arguments and return value
   please refer to the description in the header file 'dsl_cpe_configuration_parser.h'
*/
DSL_Error_t DSL_CPE_ConfigGet(const DSL_Configuration_t **pConfiguration)
{
   *pConfiguration = &g_sConfiguration;

   return DSL_SUCCESS;
}

/*
   For a detailed description of the function, its arguments and return value
   please refer to the description in the header file 'dsl_cpe_configuration_parser.h'
*/
DSL_Error_t DSL_CPE_ConfigUpdate(
   DSL_CPE_Control_Context_t *pCtrlCtx)
{
   DSL_Error_t nRet = DSL_SUCCESS;
   static DSL_boolean_t bReconfig = DSL_FALSE;

   /* on init, ConfigRead is already called in ConfigInit function -
      no need to call it again */
   if (bReconfig == DSL_TRUE)
   {
      nRet = DSL_CPE_ConfigRead();
   }

   /* do not proceed when ConfigRead failed with undefined values */
   if (nRet == DSL_SUCCESS)
   {
      nRet = DSL_CPE_ConfigValidateAndApply(pCtrlCtx);
   }

   bReconfig = DSL_TRUE;

   return nRet;
}

/*********************************************/
/* start local function definitions          */
/*********************************************/

static DSL_Error_t DSL_CPE_JsonParseConfigFile(
   const DSL_char_t * pPath,
   DSL_Configuration_t *pData)
{
   DSL_CPE_File_t *pFile = DSL_NULL;
   DSL_int_t nFileLength = 0;
   DSL_char_t * pBuffer = DSL_NULL;
   DSL_char_t currentSectionPath[SECTION_PATH_LENGTH] = { CONFIG_ROOT_NAME };

   json_object * pRootObj = DSL_NULL;

   pFile = DSL_CPE_FOpen(pPath, "r");
   if (pFile == DSL_NULL)
   {
      return DSL_ERROR;
   }

   /* Get length of file as JSON has strict format
   and we have to read whole file at once */
   fseek(pFile, 0, SEEK_END);
   nFileLength = ftell(pFile);
   fseek(pFile, 0, SEEK_SET);

   /* Alloc buffer for JSON file content */
   pBuffer = (DSL_char_t *)DSL_CPE_Malloc(nFileLength * sizeof(DSL_char_t));
   if (pBuffer == DSL_NULL)
   {
      DSL_CPE_FClose(pFile);
      return DSL_ERROR;
   }

   /* Read file */
   DSL_CPE_FRead(pBuffer, sizeof(DSL_char_t), nFileLength, pFile);
   DSL_CPE_FClose(pFile);

   /* Create JSON Object from string */
   pRootObj = json_tokener_parse(pBuffer);
   DSL_CPE_Free(pBuffer);

   if (pRootObj == DSL_NULL)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
         "Configuration Parser: invalid JSON file syntax" DSL_CPE_CRLF));
      return DSL_ERROR;
   }

   /* Start parsing */
   DSL_CPE_JsonReadObj(pRootObj, currentSectionPath, pData);

   return DSL_SUCCESS;
}

static DSL_void_t DSL_CPE_JsonReadObj(
   json_object * pObj,
   DSL_char_t *pSectionPath,
   DSL_Configuration_t *pData)
{
   enum json_type nType = json_type_null;
   DSL_char_t currentSectionPath[SECTION_PATH_LENGTH];
   json_object *pTmpObj = DSL_NULL;

   json_object_object_foreach(pObj, key, val)
   {
      pTmpObj = pObj;
      DSL_CPE_snprintf(currentSectionPath,
         sizeof(currentSectionPath), "%s.%s", pSectionPath, key);

      nType = json_object_get_type(val);

      switch (nType)
      {
      case json_type_array:
         DSL_CPE_JsonParseArray(pTmpObj, key, currentSectionPath, pData);
         break;
      case json_type_object:
         json_object_object_get_ex(pTmpObj, key, &pTmpObj);

         if (pTmpObj != DSL_NULL)
         {
            DSL_CPE_JsonReadObj(pTmpObj, currentSectionPath, pData);
         }
         break;
      case json_type_null:
      case json_type_boolean:
      case json_type_double:
      case json_type_int:
      case json_type_string:
         DSL_CPE_JsonParse(val, currentSectionPath, pData);
         break;
      }
   }
}

static DSL_void_t DSL_CPE_JsonParse(
   json_object * pObj,
   DSL_char_t * pSectionPath,
   DSL_Configuration_t *pData)
{
   enum json_type nType = json_type_null;
   DSL_int_t nIntVal = 0;
   DSL_char_t nStringVal[64] = { 0 };
   DSL_boolean_t bFound = DSL_FALSE;

   DSL_CFG_Debugging_t *pDebuggingConfig = &pData->data.sDebuggingConfig;
   DSL_CFG_StartupInit_t *pStartupInitConfig = &pData->data.sStartupInitConfig;
   DSL_CFG_Bitswap_t *pBitswapConfig = &pData->data.sBitswapConfig;
   DSL_CFG_Common_t *pCommonConfig = &pData->data.sCommonConfig;
   DSL_CFG_Retransmission_t *pRetransmissionConfig =
      &pData->data.sRetransmissionConfig;
   DSL_CFG_Sra_t *pSraConfig = &pData->data.sSraConfig;
   DSL_CFG_VirtualNoise_t *pVirtualNoiseConfig =
      &pData->data.sVirtualNoiseConfig;
   DSL_CFG_RebootCriteria_t *pRebootCriteriaConfig =
      &pData->data.sRebootCriteriaConfig;
   DSL_CFG_SystemInterface_t *pSystemIfConfig =
      &pData->data.sSystemIfConfig;

   /* We are only interested in Root.DSL section */
   if (strstr(pSectionPath, CONFIG_DSL_SECTION_PATH) == DSL_NULL)
   {
      return;
   }

   nType = json_object_get_type(pObj);

   switch (nType)
   {
      case json_type_boolean:
      case json_type_int:
         nIntVal = (DSL_int_t)json_object_get_int(pObj);

         DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
            "Configuration Parser: %s: %d" DSL_CPE_CRLF,
            pSectionPath, nIntVal));

         /* Debugging */
         PARSE_SECTION_INT(
            CONFIG_DEBUGGING_SECTION_PATH, "debug_and_test_interfaces", pDebuggingConfig->nDebugAndTestInterfaces);

         /* StartupInit */
         PARSE_SECTION_INT(
            CONFIG_STARTUPINIT_SECTION_PATH, "next_mode", pStartupInitConfig->nNextMode);
         PARSE_SECTION_INT(
            CONFIG_STARTUPINIT_SECTION_PATH, "max_device_number", pStartupInitConfig->nMaxDeviceNumber);
         PARSE_SECTION_INT(
            CONFIG_STARTUPINIT_SECTION_PATH, "lines_per_device", pStartupInitConfig->nLinesPerDevice);
         PARSE_SECTION_INT(
            CONFIG_STARTUPINIT_SECTION_PATH, "channels_per_line", pStartupInitConfig->nChannelsPerLine);

         /* Bitswap */
         PARSE_SECTION_INT(
            CONFIG_BITSWAP_SECTION_PATH, "vdsl_us", pBitswapConfig->Us_Vdsl);
         PARSE_SECTION_INT(
            CONFIG_BITSWAP_SECTION_PATH, "vdsl_ds", pBitswapConfig->Ds_Vdsl);
         PARSE_SECTION_INT(
            CONFIG_BITSWAP_SECTION_PATH, "adsl_us", pBitswapConfig->Us_Adsl);
         PARSE_SECTION_INT(
            CONFIG_BITSWAP_SECTION_PATH, "adsl_ds", pBitswapConfig->Ds_Adsl);

         /* Retransmission */
         PARSE_SECTION_INT(
            CONFIG_RETRANSMISSION_SECTION_PATH, "vdsl_us", pRetransmissionConfig->Us_Vdsl);
         PARSE_SECTION_INT(
            CONFIG_RETRANSMISSION_SECTION_PATH, "vdsl_ds", pRetransmissionConfig->Ds_Vdsl);
         PARSE_SECTION_INT(
            CONFIG_RETRANSMISSION_SECTION_PATH, "adsl_us", pRetransmissionConfig->Us_Adsl);
         PARSE_SECTION_INT(
            CONFIG_RETRANSMISSION_SECTION_PATH, "adsl_ds", pRetransmissionConfig->Ds_Adsl);

         /* SRA */
         PARSE_SECTION_INT(
            CONFIG_SRA_SECTION_PATH, "vdsl_us", pSraConfig->Us_Vdsl);
         PARSE_SECTION_INT(
            CONFIG_SRA_SECTION_PATH, "vdsl_ds", pSraConfig->Ds_Vdsl);
         PARSE_SECTION_INT(
            CONFIG_SRA_SECTION_PATH, "adsl_us", pSraConfig->Us_Adsl);
         PARSE_SECTION_INT(
            CONFIG_SRA_SECTION_PATH, "adsl_ds", pSraConfig->Ds_Adsl);

         /* Virtualnoise */
         PARSE_SECTION_INT(
            CONFIG_VIRTUALNOISE_SECTION_PATH, "us", pVirtualNoiseConfig->Us);
         PARSE_SECTION_INT(
            CONFIG_VIRTUALNOISE_SECTION_PATH, "ds", pVirtualNoiseConfig->Ds);

         /* TcLayer */
         PARSE_SECTION_INT(
            CONFIG_TCLAYER_SECTION_PATH, "adsl", pSystemIfConfig->nTcLayer_Adsl);
         PARSE_SECTION_INT(
            CONFIG_TCLAYER_SECTION_PATH, "vdsl", pSystemIfConfig->nTcLayer_Vdsl);

         /* Common */
         PARSE_SECTION_INT(
            CONFIG_COMMON_SECTION_PATH, "operator_select", pCommonConfig->nOperatorSelect);
         PARSE_SECTION_INT(
            CONFIG_COMMON_SECTION_PATH, "ld_afe_shutdown", pCommonConfig->LdAfeShutdown);
         PARSE_SECTION_INT(
            CONFIG_COMMON_SECTION_PATH, "vectoring_enable", pCommonConfig->VectoringEnable);
         PARSE_SECTION_INT(
            CONFIG_COMMON_SECTION_PATH, "activation_sequence", pCommonConfig->ActSeq);
         PARSE_SECTION_INT(
            CONFIG_COMMON_SECTION_PATH, "activation_mode", pCommonConfig->ActMode);
         PARSE_SECTION_INT(
            CONFIG_COMMON_SECTION_PATH, "remember", pCommonConfig->Remember);
         PARSE_SECTION_INT(
            CONFIG_COMMON_SECTION_PATH, "fw_msg_polling_only", pCommonConfig->FWMsgPollingOnly);

         if (bFound == DSL_FALSE)
         {
            DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
               "Configuration Parser: Unknown parameter %s" DSL_CPE_CRLF,
               pSectionPath));
         }
         break;
      case json_type_string:
         DSL_CPE_snprintf(nStringVal, sizeof(nStringVal), "%s",
            json_object_get_string(pObj));
         DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
            "Configuration Parser: %s: %s" DSL_CPE_CRLF, pSectionPath, nStringVal));

         /* Reboot criteria */
         PARSE_SECTION_HEX(
            CONFIG_REBOOTCRITERIA_SECTION_PATH, "adsl", pRebootCriteriaConfig->Adsl);
         PARSE_SECTION_HEX(
            CONFIG_REBOOTCRITERIA_SECTION_PATH, "vdsl", pRebootCriteriaConfig->Vdsl);

         /* Common */
         PARSE_SECTION_HEX(
            CONFIG_COMMON_SECTION_PATH, "vdsl_profile", pCommonConfig->VdslProfileVal);
         PARSE_SECTION_HEX(
            CONFIG_LOWLEVELHSTONES_SECTION_PATH, "adsl", pCommonConfig->sLowLevelHsTonesConfig.Adsl);
         PARSE_SECTION_HEX(
            CONFIG_LOWLEVELHSTONES_SECTION_PATH, "vdsl", pCommonConfig->sLowLevelHsTonesConfig.Vdsl);

         if (bFound == DSL_FALSE)
         {
            DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
               "Configuration Parser: Unknown parameter %s" DSL_CPE_CRLF,
               pSectionPath));
         }
         break;
      default:
         break;
   }
}

static DSL_void_t DSL_CPE_JsonParseArray(
   json_object * pObj,
   DSL_char_t * pKey,
   DSL_char_t * pSectionPath,
   DSL_Configuration_t *pData)
{
   enum json_type nType = json_type_null;
   json_object * pArrayObj = pObj;
   json_object * pValue = DSL_NULL;
   DSL_char_t currentSectionPath[SECTION_PATH_LENGTH];
   DSL_int_t nArrayLength = 0;
   DSL_char_t nStringVal[64] = { 0 };
   unsigned char *pG997XtuVal = pData->data.G997XtuVal;
   DSL_boolean_t bFound = DSL_FALSE;

   if (pKey != DSL_NULL)
   {
      json_object_object_get_ex(pObj, pKey, &pArrayObj);
   }

   nArrayLength = json_object_array_length(pArrayObj);

   for (DSL_int_t i = 0; i < nArrayLength; i++)
   {
      DSL_CPE_snprintf(currentSectionPath,
         sizeof(currentSectionPath), "%s[%d]", pSectionPath, i);

      pValue = json_object_array_get_idx(pArrayObj, i);
      nType = json_object_get_type(pValue);

      bFound = DSL_FALSE;

      switch (nType)
      {
      case json_type_array:
         DSL_CPE_JsonParseArray(pValue, DSL_NULL, currentSectionPath, pData);
         break;
      case json_type_object:
         DSL_CPE_JsonReadObj(pValue, currentSectionPath, pData);
         break;
      case json_type_string:
         DSL_CPE_snprintf(nStringVal, sizeof(nStringVal), "%s",
            json_object_get_string(pValue));

         /* XTSE */
         if (strstr(pSectionPath, CONFIG_XTSE_SECTION_PATH) != DSL_NULL)
         {
            if (i >= DSL_G997_NUM_XTSE_OCTETS)
            {
               DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
                  "Configuration Array Parser: XTSE, index out of range (%d)"
                  DSL_CPE_CRLF, i));
               break;
            }
            PARSE_SECTION_HEX(CONFIG_DSL_SECTION_PATH, "xtse",
               pG997XtuVal[i]);
         }
         break;
      case json_type_null:
      case json_type_boolean:
      case json_type_double:
      case json_type_int:
      default:
         break;
      }
   }
}

/**
   Read configuration from the config file
*/
static DSL_Error_t DSL_CPE_ConfigRead()
{
   DSL_ConfigurationData_t sUndefinedConfigData =
   {
      /* DSL_CFG_Debugging_t */
      {
         CONFIGURATION_VALUE_UNDEFINED
      },

      /* DSL_CFG_StartupInit_t */
      {
         CONFIGURATION_VALUE_UNDEFINED,
         CONFIGURATION_VALUE_UNDEFINED,
         CONFIGURATION_VALUE_UNDEFINED,
         CONFIGURATION_VALUE_UNDEFINED
      },

      /* DSL_CFG_Bitswap_t */
      {
         CONFIGURATION_VALUE_UNDEFINED,
         CONFIGURATION_VALUE_UNDEFINED,
         CONFIGURATION_VALUE_UNDEFINED,
         CONFIGURATION_VALUE_UNDEFINED
      },

      /* DSL_CFG_Retransmission_t */
      {
         CONFIGURATION_VALUE_UNDEFINED,
         CONFIGURATION_VALUE_UNDEFINED,
         CONFIGURATION_VALUE_UNDEFINED,
         CONFIGURATION_VALUE_UNDEFINED
      },

      /* DSL_CFG_Sra_t */
      {
         CONFIGURATION_VALUE_UNDEFINED,
         CONFIGURATION_VALUE_UNDEFINED,
         CONFIGURATION_VALUE_UNDEFINED,
         CONFIGURATION_VALUE_UNDEFINED
      },

      /* DSL_CFG_VirtualNoise_t */
      {
         CONFIGURATION_VALUE_UNDEFINED,
         CONFIGURATION_VALUE_UNDEFINED
      },

      /* DSL_CFG_RebootCriteria_t */
      {
         CONFIGURATION_VALUE_UNDEFINED,
         CONFIGURATION_VALUE_UNDEFINED
      },

      /* DSL_CFG_SystemInterface_t */
      {
         CONFIGURATION_VALUE_UNDEFINED,
         CONFIGURATION_VALUE_UNDEFINED
      },

      /* DSL_CFG_Common_t */
      {
         /* operator_select */
         CONFIGURATION_VALUE_UNDEFINED,

         /* ld_afe_shutdown */
         CONFIGURATION_VALUE_UNDEFINED,

         /* vectoring_enable */
         CONFIGURATION_VALUE_UNDEFINED,

         /* vdsl_profile */
         CONFIGURATION_VALUE_UNDEFINED,

         /* activation_sequence */
         CONFIGURATION_VALUE_UNDEFINED,

         /* activation_mode */
         CONFIGURATION_VALUE_UNDEFINED,

         /* remember */
         CONFIGURATION_VALUE_UNDEFINED,

         /* fw_msg_polling_only */
         CONFIGURATION_VALUE_UNDEFINED,

         /* low_level_hs_tones */
         {
            CONFIGURATION_VALUE_UNDEFINED,
            CONFIGURATION_VALUE_UNDEFINED
         }
      },

      /* xtse */
      ""
   };

   cpe_control_memcpy_s(
      &(g_sConfiguration.data), sizeof(DSL_ConfigurationData_t),
      &sUndefinedConfigData, sizeof(DSL_ConfigurationData_t));

   if (DSL_CPE_JsonParseConfigFile(g_pJsonConfigPath, &g_sConfiguration) == DSL_ERROR)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - Parsing configuration file failed!" DSL_CPE_CRLF));

      return DSL_ERROR;
   }

   return DSL_SUCCESS;
}

/**
   Execute config update
*/
static DSL_Error_t DSL_CPE_ConfigValidateAndApply(
   DSL_CPE_Control_Context_t *pCtrlCtx)
{
   DSL_Error_t nRet = DSL_SUCCESS, nErrCode = DSL_SUCCESS;

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "IN - DSL_CPE_ConfigValidateAndApply" DSL_CPE_CRLF));

   if (pCtrlCtx == DSL_NULL)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - Invalid context pointer!" DSL_CPE_CRLF));
      return DSL_ERROR;
   }

   for (int nDevice = 0; nDevice < DSL_CPE_DSL_ENTITIES; ++nDevice)
   {
      nRet = DSL_CPE_LowLevelCfgValidateAndApply(pCtrlCtx, nDevice);
      if (nRet != DSL_SUCCESS)
      {
         nErrCode = nRet;
      }

      nRet = DSL_CPE_LineFeatureCfgValidateAndApply(pCtrlCtx, nDevice);
      if (nRet != DSL_SUCCESS)
      {
         nErrCode = nRet;
      }

      nRet = DSL_CPE_RebootCriteriaCfgValidateAndApply(pCtrlCtx, nDevice);
      if (nRet != DSL_SUCCESS)
      {
         nErrCode = nRet;
      }

      nRet = DSL_CPE_VdslProfileCfgValidateAndApply(pCtrlCtx, nDevice);
      if (nRet != DSL_SUCCESS)
      {
         nErrCode = nRet;
      }

      nRet = DSL_CPE_G997_RateAdaptationCfgValidateAndApply(pCtrlCtx, nDevice);
      if (nRet != DSL_SUCCESS)
      {
         nErrCode = nRet;
      }

      nRet = DSL_CPE_SysIfCfgValidateAndApply(pCtrlCtx, nDevice);
      if (nRet != DSL_SUCCESS)
      {
         nErrCode = nRet;
      }

      nRet = DSL_CPE_XtseCfgValidateAndApply(pCtrlCtx, nDevice);
      if (nRet != DSL_SUCCESS)
      {
         nErrCode = nRet;
      }

#if defined(INCLUDE_DSL_CPE_API_VRX)
      nRet = DSL_CPE_VectoringEnableCfgApply(nDevice);
      if (nRet != DSL_SUCCESS)
      {
         nErrCode = nRet;
      }
#endif

      nRet = DSL_CPE_FwMessagePollingCfgValidateAndApply(pCtrlCtx, nDevice);
      if (nRet != DSL_SUCCESS)
      {
         nErrCode = nRet;
      }

      nRet = DSL_CPE_OperatorSelectCfgValidateAndApply(pCtrlCtx, nDevice);
      if (nRet != DSL_SUCCESS)
      {
         nErrCode = nRet;
      }
   }

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "OUT - DSL_CPE_ConfigValidateAndApply, retCode=%d" DSL_CPE_CRLF,
      nErrCode));

   return nErrCode;
}

/*
   This function validates and applies device LowLevel configuration
   taken from the config file

   \param pControlContext     Pointer to DSL CPE Control context structure, [I]
   \param nDevNum             Number of device for configuration, [I]

   \return  Return values are defined within the \ref DSL_Error_t definition
   - DSL_SUCCESS in case of success
   - DSL_ERROR if operation failed
*/
static DSL_Error_t DSL_CPE_LowLevelCfgValidateAndApply(
   DSL_CPE_Control_Context_t *pControlContext,
   const DSL_uint32_t nDevNum)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;
   DSL_LowLevelConfiguration_t sLowLevelCfg = { 0 };
   DSL_CFG_LowLevelHsTones_t sCFGLowLevelHsTones;

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "IN - DSL_CPE_LowLevelCfgValidateAndApply, device<%u>" DSL_CPE_CRLF,
      nDevNum));

   sCFGLowLevelHsTones = g_sConfiguration.data.sCommonConfig.sLowLevelHsTonesConfig;

   CONFIG_PARAM_RANGE_VALIDATION(
      sCFGLowLevelHsTones.Adsl,
      DSL_DEV_HS_TONE_GROUP_CLEANED,
      DSL_DEV_HS_TONE_GROUP_ADSL2_A43C);
   CONFIG_PARAM_RANGE_VALIDATION(
      sCFGLowLevelHsTones.Vdsl,
      DSL_DEV_HS_TONE_GROUP_CLEANED,
      DSL_DEV_HS_TONE_GROUP_ADSL2_A43C);

   if (sCFGLowLevelHsTones.Adsl == DSL_DEV_HS_TONE_GROUP_CLEANED &&
       sCFGLowLevelHsTones.Vdsl == DSL_DEV_HS_TONE_GROUP_CLEANED)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
         "API default handshake tones will be used." DSL_CPE_CRLF));

      return nErrCode;
   }

   nErrCode = DSL_CPE_Ioctl(
                  pControlContext->fd[nDevNum],
                  DSL_FIO_LOW_LEVEL_CONFIGURATION_GET,
                  (int) &sLowLevelCfg);

   if (nErrCode == DSL_SUCCESS)
   {
      if (sCFGLowLevelHsTones.Adsl > DSL_DEV_HS_TONE_GROUP_CLEANED ||
         sCFGLowLevelHsTones.Vdsl > DSL_DEV_HS_TONE_GROUP_CLEANED)
      {
         CONFIG_PARAM_APPLY(
            sLowLevelCfg.data.nHsToneGroupMode,
            DSL_DEV_HS_TONE_GROUP_MODE_MANUAL);
         CONFIG_PARAM_APPLY(
            sLowLevelCfg.data.nHsToneGroup_AV,
            DSL_DEV_HS_TONE_GROUP_CLEANED);
      }
      else
      {
         CONFIG_PARAM_APPLY(
            sLowLevelCfg.data.nHsToneGroupMode,
            DSL_DEV_HS_TONE_GROUP_MODE_AUTO);
      }

      if (sCFGLowLevelHsTones.Adsl > DSL_DEV_HS_TONE_GROUP_CLEANED)
      {
         CONFIG_PARAM_APPLY(
            sLowLevelCfg.data.nHsToneGroup_A,
            sCFGLowLevelHsTones.Adsl);
      }
      if (sCFGLowLevelHsTones.Vdsl > DSL_DEV_HS_TONE_GROUP_CLEANED)
      {
         CONFIG_PARAM_APPLY(
            sLowLevelCfg.data.nHsToneGroup_V,
            sCFGLowLevelHsTones.Vdsl);
      }

      nErrCode = DSL_CPE_Ioctl(
                     pControlContext->fd[nDevNum],
                     DSL_FIO_LOW_LEVEL_CONFIGURATION_SET,
                     (int) &sLowLevelCfg);

      if (nErrCode != DSL_SUCCESS)
      {
         DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
            "ERROR - DSL_FIO_LOW_LEVEL_CONFIGURATION_SET ioctl failed, "
            "error=%d!" DSL_CPE_CRLF,
            nErrCode));
      }
   }
   else
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - DSL_FIO_LOW_LEVEL_CONFIGURATION_GET ioctl failed, "
         "error=%d!" DSL_CPE_CRLF,
         nErrCode));
   }

   /* on failure print additional debug info */
   if (sLowLevelCfg.accessCtl.nReturn)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_WRN, (DSL_CPE_PREFIX
         "WARNING - DSL_FIO_LOW_LEVEL_CONFIGURATION "
         "access control retCode=%d!" DSL_CPE_CRLF,
         sLowLevelCfg.accessCtl.nReturn));
   }

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "OUT - DSL_CPE_LowLevelCfgValidateAndApply, retCode=%d" DSL_CPE_CRLF,
      nErrCode));

   return nErrCode;
}

/*
   This function validates and applies LineFeature configuration
   taken from the config file

   \param pControlContext     Pointer to DSL CPE Control context structure, [I]
   \param nDevNum             Number of device for configuration, [I]

   \return  Return values are defined within the \ref DSL_Error_t definition
   - DSL_SUCCESS in case of success
   - DSL_ERROR if operation failed
*/
static DSL_Error_t DSL_CPE_LineFeatureCfgValidateAndApply(
   DSL_CPE_Control_Context_t *pControlContext,
   const DSL_uint32_t nDevNum)
{
   DSL_Error_t nRet = DSL_SUCCESS, nErrCode = DSL_SUCCESS;
   DSL_LineFeature_t sLineFeature = { 0 };
   DSL_CFG_Bitswap_t sCFGBitswap;
   DSL_CFG_Retransmission_t sCFGRetransmission;
   DSL_CFG_VirtualNoise_t sCFGVirtualNoise;
   DSL_DslModeSelection_t dslMode = DSL_MODE_NA;
   DSL_AccessDir_t accessDir = DSL_ACCESSDIR_NA;
   /* Configuration of retransmission setting:
         0: Disable retransmission
         1: Enable retransmission
         2: Use API internal default configuration */
   const DSL_uint8_t CONFIG_MIN_RETX_VAL = 0;
   const DSL_uint8_t CONFIG_MAX_RETX_VAL = 2;

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "IN - DSL_CPE_LineFeatureCfgValidateAndApply, device<%u>" DSL_CPE_CRLF,
      nDevNum));

   sCFGBitswap = g_sConfiguration.data.sBitswapConfig;
   sCFGRetransmission = g_sConfiguration.data.sRetransmissionConfig;
   sCFGVirtualNoise = g_sConfiguration.data.sVirtualNoiseConfig;

   CONFIG_PARAM_RANGE_VALIDATION(
      sCFGRetransmission.Us_Adsl,
      CONFIG_MIN_RETX_VAL,
      CONFIG_MAX_RETX_VAL);
   CONFIG_PARAM_RANGE_VALIDATION(
      sCFGRetransmission.Ds_Adsl,
      CONFIG_MIN_RETX_VAL,
      CONFIG_MAX_RETX_VAL);
   CONFIG_PARAM_RANGE_VALIDATION(
      sCFGRetransmission.Us_Vdsl,
      CONFIG_MIN_RETX_VAL,
      CONFIG_MAX_RETX_VAL);
   CONFIG_PARAM_RANGE_VALIDATION(
      sCFGRetransmission.Ds_Vdsl,
      CONFIG_MIN_RETX_VAL,
      CONFIG_MAX_RETX_VAL);

   /* Use API internal default configuration */
   if (sCFGRetransmission.Us_Adsl == CONFIG_MAX_RETX_VAL)
      sCFGRetransmission.Us_Adsl = 0;

   if (sCFGRetransmission.Ds_Adsl == CONFIG_MAX_RETX_VAL)
      sCFGRetransmission.Ds_Adsl = 0;

   if (sCFGRetransmission.Us_Vdsl == CONFIG_MAX_RETX_VAL)
      sCFGRetransmission.Us_Vdsl = 1;

   if (sCFGRetransmission.Ds_Vdsl == CONFIG_MAX_RETX_VAL)
      sCFGRetransmission.Ds_Vdsl = 1;

   for (dslMode = DSL_MODE_ADSL; dslMode < DSL_MODE_LAST; ++dslMode)
   {
      for (accessDir = DSL_UPSTREAM; accessDir < DSL_ACCESSDIR_LAST; ++accessDir)
      {
         memset(&sLineFeature, 0x0, sizeof(DSL_LineFeature_t));
         sLineFeature.nDslMode = dslMode;
         sLineFeature.nDirection = accessDir;

         nRet = DSL_CPE_Ioctl(
                     pControlContext->fd[nDevNum],
                     DSL_FIO_LINE_FEATURE_CONFIG_GET,
                     (int) &sLineFeature);

         if (nRet == DSL_SUCCESS)
         {
            if (dslMode == DSL_MODE_ADSL)
            {
               if (accessDir == DSL_UPSTREAM)
               {
                  CONFIG_PARAM_APPLY(
                     sLineFeature.data.bBitswapEnable,
                     (DSL_int32_t)(sCFGBitswap.Us_Adsl));
                  CONFIG_PARAM_APPLY(
                     sLineFeature.data.bReTxEnable,
                     sCFGRetransmission.Us_Adsl);
                  CONFIG_PARAM_APPLY(
                     sLineFeature.data.bVirtualNoiseSupport,
                     (DSL_int32_t)(sCFGVirtualNoise.Us));
               }
               else
               {
                  CONFIG_PARAM_APPLY(
                     sLineFeature.data.bBitswapEnable,
                     (DSL_int32_t)(sCFGBitswap.Ds_Adsl));
                  CONFIG_PARAM_APPLY(
                     sLineFeature.data.bReTxEnable,
                     sCFGRetransmission.Ds_Adsl);
                  CONFIG_PARAM_APPLY(
                     sLineFeature.data.bVirtualNoiseSupport,
                     (DSL_int32_t)(sCFGVirtualNoise.Ds));
               }
            }
#if defined(INCLUDE_DSL_CPE_API_VRX)
            else
            {
               if (accessDir == DSL_UPSTREAM)
               {
                  CONFIG_PARAM_APPLY(
                     sLineFeature.data.bBitswapEnable,
                     (DSL_int32_t)(sCFGBitswap.Us_Vdsl));
                  CONFIG_PARAM_APPLY(
                     sLineFeature.data.bReTxEnable,
                     sCFGRetransmission.Us_Vdsl);
                  CONFIG_PARAM_APPLY(
                     sLineFeature.data.bVirtualNoiseSupport,
                     (DSL_int32_t)(sCFGVirtualNoise.Us));
               }
               else
               {
                  CONFIG_PARAM_APPLY(
                     sLineFeature.data.bBitswapEnable,
                     (DSL_int32_t)(sCFGBitswap.Ds_Vdsl));
                  CONFIG_PARAM_APPLY(
                     sLineFeature.data.bReTxEnable,
                     sCFGRetransmission.Ds_Vdsl);
                  CONFIG_PARAM_APPLY(
                     sLineFeature.data.bVirtualNoiseSupport,
                     (DSL_int32_t)(sCFGVirtualNoise.Ds));
               }
            }
#endif /* INCLUDE_DSL_CPE_API_VRX */

            nRet = DSL_CPE_Ioctl(
                        pControlContext->fd[nDevNum],
                        DSL_FIO_LINE_FEATURE_CONFIG_SET,
                        (int) &sLineFeature);

            if (nRet != DSL_SUCCESS)
            {
               nErrCode = nRet;

               DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
                 "ERROR - DSL_FIO_LINE_FEATURE_CONFIG_SET ioctl failed, "
                  "error=%d!" DSL_CPE_CRLF,
                  nRet));
            }
         }
         else
         {
            nErrCode = nRet;

            DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
               "ERROR - DSL_FIO_LINE_FEATURE_CONFIG_GET ioctl failed, "
               "error=%d!" DSL_CPE_CRLF,
               nRet));
         }

         /* on failure print additional debug info */
         if (sLineFeature.accessCtl.nReturn)
         {
            DSL_CCA_DEBUG(DSL_CCA_DBG_WRN, (DSL_CPE_PREFIX
               "WARNING - DSL_FIO_LINE_FEATURE_CONFIG "
               "access control retCode=%d!" DSL_CPE_CRLF,
               sLineFeature.accessCtl.nReturn));
         }
      }
   }

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "OUT - DSL_CPE_LineFeatureCfgValidateAndApply, retCode=%d" DSL_CPE_CRLF,
      nErrCode));

   return nErrCode;
}

/*
   This function validates and applies Reboot configuration
   taken from the config file

   \param pControlContext     Pointer to DSL CPE Control context structure, [I]
   \param nDevNum             Number of device for configuration, [I]

   \return  Return values are defined within the \ref DSL_Error_t definition
   - DSL_SUCCESS in case of success
   - DSL_ERROR if operation failed
*/
static DSL_Error_t DSL_CPE_RebootCriteriaCfgValidateAndApply(
   DSL_CPE_Control_Context_t *pControlContext,
   const DSL_uint32_t nDevNum)
{
   DSL_Error_t nRet = DSL_SUCCESS, nErrCode = DSL_SUCCESS;
   DSL_RebootCriteriaConfig_t sRebootCriteriaConfig;
   DSL_CFG_RebootCriteria_t sCFGRebootCriteria = { 0 };

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "IN - DSL_CPE_RebootCfgValidateAndApply, device<%u>" DSL_CPE_CRLF,
      nDevNum));

   sCFGRebootCriteria = g_sConfiguration.data.sRebootCriteriaConfig;

   CONFIG_PARAM_RANGE_VALIDATION(
      sCFGRebootCriteria.Adsl,
      DSL_REBOOT_CRITERIA_CLEANED,
      DSL_REBOOT_CRITERIA_LCD_BC1);
   CONFIG_PARAM_RANGE_VALIDATION(
      sCFGRebootCriteria.Vdsl,
      DSL_REBOOT_CRITERIA_CLEANED,
      DSL_REBOOT_CRITERIA_LCD_BC1);

   memset(&sRebootCriteriaConfig, 0x0, sizeof(DSL_RebootCriteriaConfig_t));
   sRebootCriteriaConfig.nDslMode = DSL_MODE_ADSL;

   CONFIG_PARAM_APPLY(
      sRebootCriteriaConfig.data.nRebootCriteria,
      sCFGRebootCriteria.Adsl);

   nRet = DSL_CPE_Ioctl(
               pControlContext->fd[nDevNum],
               DSL_FIO_REBOOT_CRITERIA_CONFIG_SET,
               (int) &sRebootCriteriaConfig);

   /* store error code in case of failure */
   if (nRet != DSL_SUCCESS)
   {
      nErrCode = nRet;

      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - DSL_FIO_REBOOT_CRITERIA_CONFIG_SET ioctl failed, "
         "error=%d!" DSL_CPE_CRLF,
         nRet));
   }

   /* on failure print additional debug info */
   if (sRebootCriteriaConfig.accessCtl.nReturn)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_WRN, (DSL_CPE_PREFIX
         "WARNING - DSL_FIO_REBOOT_CRITERIA_CONFIG "
         "access control retCode=%d!" DSL_CPE_CRLF,
         sRebootCriteriaConfig.accessCtl.nReturn));
   }

#if defined(INCLUDE_DSL_CPE_API_VRX)
   memset(&sRebootCriteriaConfig, 0x0, sizeof(DSL_RebootCriteriaConfig_t));
   sRebootCriteriaConfig.nDslMode = DSL_MODE_VDSL;

   CONFIG_PARAM_APPLY(
      sRebootCriteriaConfig.data.nRebootCriteria,
      sCFGRebootCriteria.Vdsl);

   nRet = DSL_CPE_Ioctl(
               pControlContext->fd[nDevNum],
               DSL_FIO_REBOOT_CRITERIA_CONFIG_SET,
               (int) &sRebootCriteriaConfig);

   /* store error code in case of failure */
   if (nRet != DSL_SUCCESS)
   {
      nErrCode = nRet;

      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - DSL_FIO_REBOOT_CRITERIA_CONFIG_SET ioctl failed, "
         "error=%d!" DSL_CPE_CRLF,
         nRet));
   }

   /* on failure print additional debug info */
   if (sRebootCriteriaConfig.accessCtl.nReturn)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_WRN, (DSL_CPE_PREFIX
         "WARNING - DSL_FIO_REBOOT_CRITERIA_CONFIG "
         "access control retCode=%d!" DSL_CPE_CRLF,
         sRebootCriteriaConfig.accessCtl.nReturn));
   }
#endif /* INCLUDE_DSL_CPE_API_VRX */

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "OUT - DSL_CPE_RebootCfgValidateAndApply, retCode=%d" DSL_CPE_CRLF,
      nErrCode));

   return nErrCode;
}

/*
   This function validates and applies VDSL profile configuration
   taken from the config file

   \param pControlContext     Pointer to DSL CPE Control context structure, [I]
   \param nDevNum             Number of device for configuration, [I]

   \return  Return values are defined within the \ref DSL_Error_t definition
   - DSL_SUCCESS in case of success
   - DSL_ERROR if operation failed
*/
static DSL_Error_t DSL_CPE_VdslProfileCfgValidateAndApply(
   DSL_CPE_Control_Context_t *pControlContext,
   const DSL_uint32_t nDevNum)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;
   DSL_VdslProfileConfig_t sVdslProfile;
   DSL_uint32_t nCFGVdslProfileVal = 0;
   const DSL_uint32_t nMaxMask = 0x1FF;

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "IN - DSL_CPE_VdslProfileCfgValidateAndApply, device<%u>" DSL_CPE_CRLF,
      nDevNum));

   nCFGVdslProfileVal = g_sConfiguration.data.sCommonConfig.VdslProfileVal;

   /* Use API internal default configuration */
   if (nCFGVdslProfileVal == 0)
      nCFGVdslProfileVal = nMaxMask;

   CONFIG_PARAM_RANGE_VALIDATION(
      nCFGVdslProfileVal,
      DSL_BF_PROFILE_8A,
      nMaxMask);

   memset(&sVdslProfile, 0x0, sizeof(DSL_VdslProfileConfig_t));

   CONFIG_PARAM_APPLY(
      sVdslProfile.data.nVdslProfile,
      nCFGVdslProfileVal);

   nErrCode = DSL_CPE_Ioctl(
                  pControlContext->fd[nDevNum],
                  DSL_FIO_VDSL_PROFILE_CONFIG_SET,
                  (int) &sVdslProfile);

   /* store error code in case of failure */
   if (nErrCode != DSL_SUCCESS)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - DSL_FIO_VDSL_PROFILE_CONFIG_SET ioctl failed, "
         "error=%d!" DSL_CPE_CRLF,
         nErrCode));
   }

   /* on failure print additional debug info */
   if (sVdslProfile.accessCtl.nReturn)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_WRN, (DSL_CPE_PREFIX
         "wARNING - DSL_FIO_VDSL_PROFILE_CONFIG "
         "access control retCode=%d!" DSL_CPE_CRLF,
         sVdslProfile.accessCtl.nReturn));
   }

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "OUT - DSL_CPE_VdslProfileCfgValidateAndApply, retCode=%d" DSL_CPE_CRLF,
      nErrCode));

   return nErrCode;
}

/*
   This function validates and applies seamless rate adaptation configuration
   taken from the config file

   \param pControlContext     Pointer to DSL CPE Control context structure, [I]
   \param nDevNum             Number of device for configuration, [I]

   \return  Return values are defined within the \ref DSL_Error_t definition
   - DSL_SUCCESS in case of success
   - DSL_ERROR if operation failed
*/
static DSL_Error_t DSL_CPE_G997_RateAdaptationCfgValidateAndApply(
   DSL_CPE_Control_Context_t *pControlContext,
   const DSL_uint32_t nDevNum)
{
   DSL_Error_t nRet = DSL_SUCCESS, nErrCode = DSL_SUCCESS;
   DSL_G997_RateAdaptationConfig_t sg997RA;
   DSL_CFG_Sra_t sCFGSra;
   DSL_DslModeSelection_t dslMode = DSL_MODE_NA;
   DSL_AccessDir_t accessDir = DSL_ACCESSDIR_NA;
   const DSL_uint8_t g997RAShift = 2;
   /* Configuration of seamless rate adaptation (SRA):
         0: Disable SRA
         1: Enable SRA
         2: Use API internal default configuration
            (disabled for ADSL, enabled for VDSL) */
   const DSL_uint8_t CONFIG_MIN_SRA_VAL = 0;
   const DSL_uint8_t CONFIG_MAX_SRA_VAL = 2;

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "IN - DSL_CPE_G997_RateAdaptationCfgValidateAndApply, device<%u>" DSL_CPE_CRLF,
      nDevNum));

   sCFGSra = g_sConfiguration.data.sSraConfig;

   CONFIG_PARAM_RANGE_VALIDATION(
      sCFGSra.Us_Adsl,
      CONFIG_MIN_SRA_VAL,
      CONFIG_MAX_SRA_VAL);
   CONFIG_PARAM_RANGE_VALIDATION(
      sCFGSra.Ds_Adsl,
      CONFIG_MIN_SRA_VAL,
      CONFIG_MAX_SRA_VAL);
   CONFIG_PARAM_RANGE_VALIDATION(
      sCFGSra.Us_Vdsl,
      CONFIG_MIN_SRA_VAL,
      CONFIG_MAX_SRA_VAL);
   CONFIG_PARAM_RANGE_VALIDATION(
      sCFGSra.Ds_Vdsl,
      CONFIG_MIN_SRA_VAL,
      CONFIG_MAX_SRA_VAL);

   /* Use API internal default configuration */
   if (sCFGSra.Us_Adsl == CONFIG_MAX_SRA_VAL)
      sCFGSra.Us_Adsl = 0;

   if (sCFGSra.Ds_Adsl == CONFIG_MAX_SRA_VAL)
      sCFGSra.Ds_Adsl = 0;

   if (sCFGSra.Us_Vdsl == CONFIG_MAX_SRA_VAL)
      sCFGSra.Us_Vdsl = 1;

   if (sCFGSra.Ds_Vdsl == CONFIG_MAX_SRA_VAL)
      sCFGSra.Ds_Vdsl = 1;

   for (dslMode = DSL_MODE_ADSL; dslMode < DSL_MODE_LAST; ++dslMode)
   {
      for (accessDir = DSL_UPSTREAM; accessDir < DSL_ACCESSDIR_LAST; ++accessDir)
      {
         memset(&sg997RA, 0x0, sizeof(DSL_G997_RateAdaptationConfig_t));

         sg997RA.nDslMode = dslMode;
         sg997RA.nDirection = accessDir;

         if (dslMode == DSL_MODE_ADSL)
         {
            if (accessDir == DSL_UPSTREAM)
            {
               CONFIG_PARAM_APPLY(
                  sg997RA.data.RA_MODE,
                  (sCFGSra.Us_Adsl + g997RAShift));
            }
            else
            {
               CONFIG_PARAM_APPLY(
                  sg997RA.data.RA_MODE,
                  (sCFGSra.Ds_Adsl + g997RAShift));
            }
         }
#if defined(INCLUDE_DSL_CPE_API_VRX)
         else
         {
            if (accessDir == DSL_UPSTREAM)
            {
               CONFIG_PARAM_APPLY(
                  sg997RA.data.RA_MODE,
                  (sCFGSra.Us_Vdsl + g997RAShift));
            }
            else
            {
               CONFIG_PARAM_APPLY(
                  sg997RA.data.RA_MODE,
                  (sCFGSra.Ds_Vdsl + g997RAShift));
            }
         }
#endif /* INCLUDE_DSL_CPE_API_VRX */

         nRet = DSL_CPE_Ioctl(
                     pControlContext->fd[nDevNum],
                     DSL_FIO_G997_RATE_ADAPTATION_CONFIG_SET,
                     (int) &sg997RA);

         /* store error code in case of failure */
         if (nRet != DSL_SUCCESS)
         {
            nErrCode = nRet;

            DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
               "ERROR - DSL_FIO_VDSL_PROFILE_CONFIG_SET ioctl failed, "
               "error=%d!" DSL_CPE_CRLF,
               nRet));
         }

         /* on failure print additional debug info */
         if (sg997RA.accessCtl.nReturn)
         {
            DSL_CCA_DEBUG(DSL_CCA_DBG_WRN, (DSL_CPE_PREFIX
               "WARNING - DSL_FIO_VDSL_PROFILE_CONFIG "
               "access control retCode=%d!" DSL_CPE_CRLF,
               sg997RA.accessCtl.nReturn));
         }
      }
   }

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "OUT - DSL_CPE_G997_RateAdaptationCfgValidateAndApply, retCode=%d"
      DSL_CPE_CRLF, nErrCode));

   return nErrCode;
}

/*
   This function applies SystemInterface configuration taken from the
   config file

   \param pControlContext     Pointer to DSL CPE Control context structure, [I]
   \param nDevNum             Number of device for configuration, [I]

   \return  Return values are defined within the \ref DSL_Error_t definition
   - DSL_SUCCESS in case of success
   - DSL_ERROR if operation failed
*/
static DSL_Error_t DSL_CPE_SysIfCfgValidateAndApply(
   DSL_CPE_Control_Context_t *pControlContext,
   const DSL_uint32_t nDevNum)
{
   DSL_Error_t nRet = DSL_SUCCESS, nErrCode = DSL_SUCCESS;
   DSL_SystemInterfaceConfig_t sSystemIfConfig = { 0 };
   DSL_CFG_SystemInterface_t sCfgSystemIfConfig;

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "IN - DSL_CPE_SysIfCfgValidateAndApply, device<%u>" DSL_CPE_CRLF,
      nDevNum));

   sCfgSystemIfConfig = g_sConfiguration.data.sSystemIfConfig;

   CONFIG_PARAM_RANGE_VALIDATION(
      sCfgSystemIfConfig.nTcLayer_Adsl,
      DSL_TC_UNKNOWN,
      DSL_TC_EFM_FORCED);
   CONFIG_PARAM_RANGE_VALIDATION(
      sCfgSystemIfConfig.nTcLayer_Vdsl,
      DSL_TC_UNKNOWN,
      DSL_TC_EFM_FORCED);

   memset(&sSystemIfConfig, 0x0, sizeof(DSL_SystemInterfaceConfig_t));
   sSystemIfConfig.nDslMode = DSL_MODE_ADSL;

   nRet = DSL_CPE_Ioctl(
               pControlContext->fd[nDevNum],
               DSL_FIO_SYSTEM_INTERFACE_CONFIG_GET,
               (int) &sSystemIfConfig);

   if (nRet == DSL_SUCCESS)
   {
      /* if TcLayer_Adsl is "0" the API internal default
         configuration value DSL_TC_ATM shall be used */
      if (sCfgSystemIfConfig.nTcLayer_Adsl == 0)
      {
         sSystemIfConfig.data.nTcLayer = DSL_TC_ATM;
      }
      else
      {
         CONFIG_PARAM_APPLY(
            sSystemIfConfig.data.nTcLayer,
            sCfgSystemIfConfig.nTcLayer_Adsl);
      }

      nRet = DSL_CPE_Ioctl(
                  pControlContext->fd[nDevNum],
                  DSL_FIO_SYSTEM_INTERFACE_CONFIG_SET,
                  (int) &sSystemIfConfig);

      /* store error code in case of failure */
      if (nRet != DSL_SUCCESS)
      {
         nErrCode = nRet;

         DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
            "ERROR - DSL_FIO_OPERATOR_CONFIG_SET ioctl failed, "
            "error=%d!" DSL_CPE_CRLF,
            nRet));
      }
   }
   else
   {
      nErrCode = nRet;

      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - DSL_FIO_SYSTEM_INTERFACE_CONFIG_GET ioctl failed, "
         "error=%d!" DSL_CPE_CRLF,
         nRet));
   }

   /* on failure print additional debug info */
   if (sSystemIfConfig.accessCtl.nReturn)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_WRN, (DSL_CPE_PREFIX
         "wARNING - DSL_FIO_SYSTEM_INTERFACE_CONFIG "
         "access control retCode=%d!" DSL_CPE_CRLF,
         sSystemIfConfig.accessCtl.nReturn));
   }

#if defined(INCLUDE_DSL_CPE_API_VRX)
   memset(&sSystemIfConfig, 0x0, sizeof(DSL_SystemInterfaceConfig_t));
   sSystemIfConfig.nDslMode = DSL_MODE_VDSL;

   nRet = DSL_CPE_Ioctl(
               pControlContext->fd[nDevNum],
               DSL_FIO_SYSTEM_INTERFACE_CONFIG_GET,
               (int) &sSystemIfConfig);

   if (nRet == DSL_SUCCESS)
   {
      /* if TcLayer_Vdsl is "0" the API internal default
         configuration value DSL_TC_EFM shall be used */
      if (sCfgSystemIfConfig.nTcLayer_Vdsl == 0)
      {
         sSystemIfConfig.data.nTcLayer = DSL_TC_EFM;
      }
      else
      {
         CONFIG_PARAM_APPLY(
            sSystemIfConfig.data.nTcLayer,
            sCfgSystemIfConfig.nTcLayer_Vdsl);
      }

      nRet = DSL_CPE_Ioctl(
                  pControlContext->fd[nDevNum],
                  DSL_FIO_SYSTEM_INTERFACE_CONFIG_SET,
                  (int) &sSystemIfConfig);

      /* store error code in case of failure */
      if (nRet != DSL_SUCCESS)
      {
         nErrCode = nRet;

         DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
            "ERROR - DSL_FIO_OPERATOR_CONFIG_SET ioctl failed, "
            "error=%d!" DSL_CPE_CRLF,
            nRet));
      }
   }
   else
   {
      nErrCode = nRet;

      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - DSL_FIO_SYSTEM_INTERFACE_CONFIG_GET ioctl failed, "
         "error=%d!" DSL_CPE_CRLF,
         nRet));
   }

   /* on failure print additional debug info */
   if (sSystemIfConfig.accessCtl.nReturn)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_WRN, (DSL_CPE_PREFIX
         "wARNING - DSL_FIO_SYSTEM_INTERFACE_CONFIG "
         "access control retCode=%d!" DSL_CPE_CRLF,
         sSystemIfConfig.accessCtl.nReturn));
   }
#endif

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "OUT - DSL_CPE_SysIfCfgValidateAndApply, retCode=%d" DSL_CPE_CRLF,
      nErrCode));

   return nErrCode;
}

/*
   This function applies XTSE configuration taken from the config file

   \param pControlContext     Pointer to DSL CPE Control context structure, [I]
   \param nDevNum             Number of device for configuration, [I]

   \return  Return values are defined within the \ref DSL_Error_t definition
   - DSL_SUCCESS in case of success
   - DSL_ERROR if operation failed
*/
static DSL_Error_t DSL_CPE_XtseCfgValidateAndApply(
   DSL_CPE_Control_Context_t *pControlContext,
   const DSL_uint32_t nDevNum)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;
   DSL_G997_XTUSystemEnabling_t sXtse = { 0 };
   const unsigned char *pG997XtuVal = DSL_NULL;
   DSL_int_t i;

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "IN - DSL_CPE_XtseCfgValidateAndApply, device<%u>" DSL_CPE_CRLF,
      nDevNum));

   pG997XtuVal = g_sConfiguration.data.G997XtuVal;

   nErrCode = DSL_CPE_Ioctl(
                  pControlContext->fd[nDevNum],
                  DSL_FIO_G997_XTU_SYSTEM_ENABLING_CONFIG_GET,
                  (int) &sXtse);

   if (nErrCode == DSL_SUCCESS)
   {
      for (i = 0; i < DSL_G997_NUM_XTSE_OCTETS; i++)
      {
         sXtse.data.XTSE[i] = pG997XtuVal[i];
      }

      nErrCode = DSL_CPE_Ioctl(
                     pControlContext->fd[nDevNum],
                     DSL_FIO_G997_XTU_SYSTEM_ENABLING_CONFIG_SET,
                     (int) &sXtse);

      /* store error code in case of failure */
      if (nErrCode != DSL_SUCCESS)
      {
         DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
            "ERROR - DSL_FIO_G997_XTU_SYSTEM_ENABLING_CONFIG_SET ioctl failed, "
            "error=%d!" DSL_CPE_CRLF,
            nErrCode));
      }
   }
   else
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - DSL_FIO_G997_XTU_SYSTEM_ENABLING_CONFIG_GET ioctl failed, "
         "error=%d!" DSL_CPE_CRLF,
         nErrCode));
   }

   /* on failure print additional debug info */
   if (sXtse.accessCtl.nReturn)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_WRN, (DSL_CPE_PREFIX
         "wARNING - DSL_FIO_G997_XTU_SYSTEM_ENABLING_CONFIG "
         "access control retCode=%d!" DSL_CPE_CRLF,
         sXtse.accessCtl.nReturn));
   }

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "OUT - DSL_CPE_XtseCfgValidateAndApply, retCode=%d" DSL_CPE_CRLF,
      nErrCode));

   return nErrCode;
}

#if defined(INCLUDE_DSL_CPE_API_VRX)
/*
   This function applies Vectoring configuration taken from the config file

   \note Vectoring is only supported for VDSL

   \param nDevNum    Device number, [I]

   \return  Return values are defined within the \ref DSL_Error_t definition
   - DSL_SUCCESS in case of success
   - DSL_ERROR if operation failed
*/
static DSL_Error_t DSL_CPE_VectoringEnableCfgApply(
   const DSL_uint32_t nDevNum)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;
   unsigned int nVectoringEnable = 0;
   DSL_int_t nMeiFd;
   DSL_char_t meiDevName[32] = {0};
   IOCTL_MEI_dsmConfig_t sDsmConfig;

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "IN - DSL_CPE_VectoringEnableCfgApply, device<%u>" DSL_CPE_CRLF,
      nDevNum));

   nVectoringEnable = g_sConfiguration.data.sCommonConfig.VectoringEnable;

   CONFIG_PARAM_RANGE_VALIDATION(
      nVectoringEnable,
      e_MEI_VECTOR_CTRL_OFF,
      e_MEI_VECTOR_CTRL_AUTO);

   memset(&sDsmConfig, 0, sizeof(IOCTL_MEI_dsmConfig_t));
   sDsmConfig.eVectorControl = nVectoringEnable;

   snprintf(meiDevName, sizeof(meiDevName), "/dev/mei_cpe/%u", nDevNum);
   nMeiFd = DSL_CPE_Open(meiDevName);
   if (nMeiFd <= 0)
   {
      DSL_CPE_Close(nMeiFd);

      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - MEI Driver <%s> does not exist!" DSL_CPE_CRLF,
         meiDevName));
      return DSL_ERROR;
   }

   nErrCode = DSL_CPE_Ioctl(nMeiFd,
                  FIO_MEI_DSM_CONFIG_SET,
                  (int) &sDsmConfig);

   if (nErrCode != DSL_SUCCESS)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - FIO_MEI_DSM_CONFIG_SET ioctl failed, error=%d!" DSL_CPE_CRLF,
         nErrCode));
   }

   /* on failure print additional debug info */
   if (sDsmConfig.ictl.retCode)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_WRN, (DSL_CPE_PREFIX
         "WARNING - FIO_MEI_DSM_CONFIG "
         "access control retCode=%d!" DSL_CPE_CRLF,
         sDsmConfig.ictl.retCode));
   }

   DSL_CPE_Close(nMeiFd);

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "OUT - DSL_CPE_VectoringEnableCfgApply, retCode=%d" DSL_CPE_CRLF,
      nErrCode));

   return nErrCode;
}
#endif /* defined(INCLUDE_DSL_CPE_API_VRX) */

/*
   This function applies FW message polling mode (for API) configuration
   taken from the config file

   \param pControlContext     Pointer to DSL CPE Control context structure, [I]
   \param nDevNum             Number of device for configuration, [I]

   \return  Return values are defined within the \ref DSL_Error_t definition
   - DSL_SUCCESS in case of success
   - DSL_ERROR if operation failed
*/
static DSL_Error_t DSL_CPE_FwMessagePollingCfgValidateAndApply(
   DSL_CPE_Control_Context_t *pControlContext,
   const DSL_uint32_t nDevNum)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;
   DSL_boolean_t bFWMsgPollingOnly;
   DSL_InstanceControl_t sInstanceControl;

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "IN - DSL_CPE_FwMessagePollingCfgValidateAndApply, device<%u>" DSL_CPE_CRLF,
      nDevNum));

   bFWMsgPollingOnly = g_sConfiguration.data.sCommonConfig.FWMsgPollingOnly;

   memset(&sInstanceControl, 0x0, sizeof(DSL_InstanceControl_t));
   sInstanceControl.data.bEventActivation = DSL_TRUE;
   sInstanceControl.data.bFwEventActivation = bFWMsgPollingOnly == DSL_TRUE ?
           DSL_FALSE : DSL_TRUE;

   nErrCode = DSL_CPE_Ioctl(
                  pControlContext->fd[nDevNum],
                  DSL_FIO_INSTANCE_CONTROL_SET,
                  (int) &sInstanceControl);

   /* store error code in case of failure */
   if (nErrCode != DSL_SUCCESS)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - DSL_FIO_INSTANCE_CONTROL_SET ioctl failed, "
         "error=%d!" DSL_CPE_CRLF,
         nErrCode));
   }

   /* on failure print additional debug info */
   if (sInstanceControl.accessCtl.nReturn)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_WRN, (DSL_CPE_PREFIX
         "wARNING - DSL_FIO_INSTANCE_CONTROL "
         "access control retCode=%d!" DSL_CPE_CRLF,
         sInstanceControl.accessCtl.nReturn));
   }

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "OUT - DSL_CPE_FwMessagePollingCfgValidateAndApply, retCode=%d" DSL_CPE_CRLF,
      nErrCode));

   return nErrCode;
}

/*
   This function applies OperatorSelect value taken from the config file

   \note Vectoring is only supported for VDSL

   \param pControlContext     Pointer to DSL CPE Control context structure, [I]
   \param nDevNum             Number of device for configuration, [I]

   \return  Return values are defined within the \ref DSL_Error_t definition
   - DSL_SUCCESS in case of success
   - DSL_ERROR if operation failed
*/
static DSL_Error_t DSL_CPE_OperatorSelectCfgValidateAndApply(
   DSL_CPE_Control_Context_t *pControlContext,
   const DSL_uint32_t nDevNum)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;
   DSL_uint16_t nOperatorSelect;
   DSL_OperatorConfig_t sOperatorConfig;
   /* FW message values defined with MCAT is defined for 0..19
      but FW will block reserved values from within full-scale of 8-bit */
   const DSL_uint8_t CONFIG_MIN_OPERATOR_SELECT_VAL = 0;
   const DSL_uint8_t CONFIG_MAX_OPERATOR_SELECT_VAL = 255;

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "IN - DSL_CPE_OperatorSelectCfgValidateAndApply, device<%u>" DSL_CPE_CRLF,
      nDevNum));

   nOperatorSelect = (DSL_uint16_t)g_sConfiguration.data.sCommonConfig.nOperatorSelect;

   CONFIG_PARAM_RANGE_VALIDATION(
      nOperatorSelect,
      CONFIG_MIN_OPERATOR_SELECT_VAL,
      CONFIG_MAX_OPERATOR_SELECT_VAL);

   memset(&sOperatorConfig, 0, sizeof(DSL_OperatorConfig_t));
   sOperatorConfig.data.nDslOperator = nOperatorSelect;

   nErrCode = DSL_CPE_Ioctl(
                  pControlContext->fd[nDevNum],
                  DSL_FIO_OPERATOR_CONFIG_SET,
                  (int) &sOperatorConfig);

   /* store error code in case of failure */
   if (nErrCode != DSL_SUCCESS)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - DSL_FIO_OPERATOR_CONFIG_SET ioctl failed, "
         "error=%d!" DSL_CPE_CRLF,
         nErrCode));
   }

   /* on failure print additional debug info */
   if (sOperatorConfig.accessCtl.nReturn)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_WRN, (DSL_CPE_PREFIX
         "WARNING - DSL_FIO_OPERATOR_CONFIG "
         "access control retCode=%d!" DSL_CPE_CRLF,
         sOperatorConfig.accessCtl.nReturn));
   }

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "OUT - DSL_CPE_OperatorSelectCfgValidateAndApply, retCode=%d" DSL_CPE_CRLF,
      nErrCode));

   return nErrCode;
}

#endif /* defined(INCLUDE_DSL_JSON_PARSING) && (INCLUDE_DSL_JSON_PARSING == 1) */
