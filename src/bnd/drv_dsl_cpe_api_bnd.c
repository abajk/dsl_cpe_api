/******************************************************************************

         Copyright (c) 2020, MaxLinear, Inc.
         Copyright 2016 - 2020 Intel Corporation
         Copyright 2015 - 2016 Lantiq Beteiligungs-GmbH & Co. KG
         Copyright 2009 - 2014 Lantiq Deutschland GmbH
         Copyright 2007 - 2008 Infineon Technologies AG

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
#define DSL_INTERN

#include "drv_dsl_cpe_api.h"
#ifdef __LINUX__
#include <linux/capability.h>
#endif /* __LINUX__ */

#ifdef __cplusplus
   extern "C" {
#endif


#undef DSL_DBG_BLOCK
#define DSL_DBG_BLOCK DSL_DBG_CPE_API

typedef enum {
   DSL_BND_INFO_STATUS = 0,
   DSL_BND_INFO_REMOTE_PAF,
   DSL_BND_INFO_REMOTE_IMAP
} DSL_BND_BndInfoType_t;

/*
   For a detailed description please refer to the equivalent ioctl
   \ref DSL_FIO_BND_HW_INIT
*/
DSL_Error_t DSL_DRV_BND_HwInit(
   DSL_IN DSL_Context_t *pContext,
   DSL_IN DSL_BND_HwInit_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;

#ifdef __LINUX__
   if (!capable(CAP_NET_ADMIN))
   {
      printk("Permission denied");
      return -EPERM;
   }
#endif /* __LINUX__ */

   DSL_CHECK_POINTER(pContext, pData);
   DSL_CHECK_ERR_CODE();

   DSL_DEBUG(DSL_DBG_MSG,
      (pContext, SYS_DBG_MSG"DSL[%02d]: IN - DSL_DRV_BND_HwInit"DSL_DRV_CRLF,
      DSL_DEV_NUM(pContext)));

   /* Call device specific implementation*/
   nErrCode = DSL_DRV_BND_DEV_HwInit(pContext, pData);

   DSL_DEBUG(DSL_DBG_MSG,
      (pContext, SYS_DBG_MSG"DSL[%02d]: OUT - DSL_DRV_BND_HwInit, retCode=%d"
      DSL_DRV_CRLF, DSL_DEV_NUM(pContext), nErrCode));

   return nErrCode;
}

#if defined(INCLUDE_DSL_CPE_API_VRX)
/*
   This function does consistency checks for bonding configuration vs
   configured tc-layer and returns appropriate warning/error in case
   of incorrect settings, according to the following specification:
   bPafEnable  |  bImapEnable  |  nDslMode  |  TC-Cfg   |   ErrorCode
   ------------------------------------------------------------------
   TRUE        | FALSE         | ADSL       | AUTO      | DSL_WRN_CONFIG_BND_VS_TCLAYER
               |               |            | ATM       | DSL_ERR_CONFIG_BND_VS_TCLAYER
               |               |            | PTM       | DSL_SUCCESS
               |               | VDSL       | AUTO      | DSL_WRN_CONFIG_BND_VS_TCLAYER
               |               |            | ATM       | DSL_ERR_CONFIG_BND_VS_TCLAYER
               |               |            | PTM       | DSL_SUCCESS
   FALSE       | TRUE          | ADSL       | AUTO      | DSL_WRN_CONFIG_BND_VS_TCLAYER
               |               |            | ATM       | DSL_SUCCESS
               |               |            | PTM       | DSL_ERR_CONFIG_BND_VS_TCLAYER
               |               | VDSL       | AUTO      | DSL_ERR_BND_IMAP_ONLY_SUPPORTED_IN_ADSL
               |               |            | ATM       | DSL_ERR_BND_IMAP_ONLY_SUPPORTED_IN_ADSL
               |               |            | PTM       | DSL_ERR_CONFIG_BND_VS_TCLAYER
   TRUE        | TRUE          | ADSL       | AUTO      | DSL_SUCCESS
               |               |            | ATM       | DSL_SUCCESS
               |               |            | PTM       | DSL_SUCCESS
               |               | VDSL       | AUTO      | DSL_WRN_CONFIG_BND_VS_TCLAYER
               |               |            | ATM       | DSL_ERR_BND_IMAP_ONLY_SUPPORTED_IN_ADSL
               |               |            | PTM       | DSL_SUCCESS

   \param bPafEnable    ptm bonding
   \param bImapEnable   atm bonding

   \return Return values
   - DSL_SUCCESS in case of fully supported configuration
   - DSL_WRN_CONFIG_BND_VS_TCLAYER in case of partly supported configuration
   - DSL_ERR_CONFIG_BND_VS_TCLAYER in case of unsupported configuration
*/
static DSL_Error_t DSL_DRV_BND_ConfigCheck(
   DSL_IN DSL_Context_t *pContext,
   DSL_IN const DSL_DslModeSelection_t nDslMode,
   DSL_IN DSL_boolean_t *bPafEnable,
   DSL_IN DSL_boolean_t *bImapEnable)
{
   DSL_Error_t nErrCode = DSL_SUCCESS, nRetCode = DSL_SUCCESS,
      nBndErrCode = DSL_ERR_CONFIG_BND_VS_TCLAYER;
   DSL_uint8_t nLine;
   DSL_int_t nAutoTcCount = 0, nAtmTcCount = 0;
   DSL_TcLayerSelection_t nTcMode = DSL_TC_UNKNOWN;
   DSL_boolean_t bErrCase = DSL_FALSE, bWrnCase = DSL_FALSE;

   /* Check if one of the following configurations are done for one of
      the available devices/lines. If yes, return appropriate warning.
      - Auto TC-Layer is configured for VDSL/ADSL
      - ATM TC-Layer is configured for VDSL/ADSL */
   for (nLine = 0;
        nLine < DSL_DRV_LINES_PER_DEVICE * DSL_DRV_DEVICE_NUMBER;
        nLine++)
   {
      DSL_CTX_READ_SCALAR(pContext->pXDev[nLine].pContext, nRetCode,
         pDevCtx->data.deviceCfg.sysCIF[nDslMode].nTcLayer,
         nTcMode);
      DSL_SET_ERROR_CODE(nRetCode, nErrCode);

      if (nTcMode == DSL_TC_AUTO)
      {
         nAutoTcCount++;
      }

      if (nTcMode == DSL_TC_ATM)
      {
         nAtmTcCount++;
      }
   }

   /* check erroneous cases */
   if (*bPafEnable == DSL_TRUE && *bImapEnable == DSL_FALSE)
   {
      /* Set error/warning return codes */
      if (nAtmTcCount > 0)
      {
         bErrCase = DSL_TRUE;
      }
      else if (nAutoTcCount > 0)
      {
         bWrnCase = DSL_TRUE;
      }
   }
   else if (*bPafEnable == DSL_FALSE && *bImapEnable == DSL_TRUE)
   {
      if (nDslMode == DSL_MODE_VDSL)
      {
         bErrCase = DSL_TRUE;
         /* Set specific error code for AUTO/ATM configs only */
         if (nAutoTcCount > 0 || nAtmTcCount > 0)
         {
            nBndErrCode = DSL_ERR_BND_IMAP_ONLY_SUPPORTED_IN_ADSL;
         }
      }
      else
      {
         /* Set error/warning return codes */
         if (nAtmTcCount == 0) /* PTM */
         {
            bErrCase = DSL_TRUE;
         }
         else if (nAutoTcCount > 0)
         {
            bWrnCase = DSL_TRUE;
         }
      }
   }
   else if (*bPafEnable == DSL_TRUE && *bImapEnable == DSL_TRUE &&
               nDslMode == DSL_MODE_VDSL)
   {
      /* Set error/warning return codes in order of priority */
      if (nAtmTcCount > 0)
      {
         bErrCase = DSL_TRUE;
         nBndErrCode = DSL_ERR_BND_IMAP_ONLY_SUPPORTED_IN_ADSL;
      }
      else if (nAutoTcCount > 0)
      {
         bWrnCase = DSL_TRUE;
      }
   }

   /* handle error or warning case if detected */
   if (bErrCase == DSL_TRUE)
   {
      /* Reject bonding enable in this case */
      *bPafEnable = DSL_FALSE;
      *bImapEnable = DSL_FALSE;
      DSL_SET_ERROR_CODE(nBndErrCode, nErrCode);
   }
   else if (bWrnCase == DSL_TRUE)
   {
      /* Accept bonding enable in this case (do not reset user enable) */
      DSL_SET_ERROR_CODE(DSL_WRN_CONFIG_BND_VS_TCLAYER, nErrCode);
   }

   return nErrCode;
}
#endif /* defined(INCLUDE_DSL_CPE_API_VRX) */

/*
   For a detailed description please refer to the equivalent ioctl
   \ref DSL_FIO_BND_CONFIG_SET
*/
DSL_Error_t DSL_DRV_BND_ConfigSet(
   DSL_IN DSL_Context_t *pContext,
   DSL_IN DSL_BND_Config_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS, nRetCode = DSL_SUCCESS;
   DSL_uint8_t nLine;
   DSL_Context_t *pCurrCtx;

#ifdef __LINUX__
   if (!capable(CAP_NET_ADMIN))
   {
      printk("Permission denied");
      return -EPERM;
   }
#endif /* __LINUX__ */

   DSL_CHECK_POINTER(pContext, pData);
   DSL_CHECK_ERR_CODE();

   DSL_CHECK_DSLMODE(pData->nDslMode);
   DSL_CHECK_ERR_CODE();

   DSL_DEBUG(DSL_DBG_MSG,
      (pContext, SYS_DBG_MSG"DSL[%02d]: IN - DSL_DRV_BND_ConfigSet"DSL_DRV_CRLF,
      DSL_DEV_NUM(pContext)));

#if defined(INCLUDE_DSL_CPE_API_VRX)
   /* check if bonding is supported by the firmware */
   if (DSL_DRV_LINES_PER_DEVICE == 2 &&  DSL_DRV_DEV_FirmwareFeatureCheck(pContext,
         DSL_FW_XDSLFEATURE_DUALPORT) == DSL_FALSE)
   {
      DSL_DEBUG(DSL_DBG_WRN, (pContext, SYS_DBG_WRN
         "DSL[%02d]: Bonding could not be enabled because of firmware "
         "doesn't support it"DSL_DRV_CRLF, DSL_DEV_NUM(pContext)));

      return DSL_WRN_BND_NOT_SUPPORTED_BY_FIRMWARE;
   }

   /* check selected bonding configuration vs current tc-layer settings
      and update it to default values if there are errors */
   nErrCode = DSL_DRV_BND_ConfigCheck(pContext, pData->nDslMode,
      &pData->data.bPafEnable, &pData->data.bImapEnable);
#endif /* defined(INCLUDE_DSL_CPE_API_VRX) */

   if (DSL_DRV_BONDING_ENABLED)
   {
      for (nLine = 0; nLine < DSL_DRV_LINES_PER_DEVICE * DSL_DRV_DEVICE_NUMBER; nLine++)
      {
         pCurrCtx = pContext->pXDev[nLine].pContext;

         /* Set Bonding configuration within CPE API Context*/
         DSL_CTX_WRITE(pCurrCtx, nRetCode, BndConfig[pData->nDslMode], pData->data);
         DSL_SET_ERROR_CODE(nRetCode, nErrCode);
      }
   }
   DSL_DEBUG(DSL_DBG_MSG,
      (pContext, SYS_DBG_MSG"DSL[%02d]: OUT - DSL_DRV_BND_ConfigSet, retCode=%d"
      DSL_DRV_CRLF, DSL_DEV_NUM(pContext), nErrCode));

   return nErrCode;
}

/*
   For a detailed description please refer to the equivalent ioctl
   \ref DSL_FIO_BND_CONFIG_GET
*/
#ifdef INCLUDE_DSL_CONFIG_GET
DSL_Error_t DSL_DRV_BND_ConfigGet(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_BND_Config_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;

#ifdef __LINUX__
   if (!capable(CAP_NET_ADMIN))
   {
      printk("Permission denied");
      return -EPERM;
   }
#endif /* __LINUX__ */

   DSL_CHECK_POINTER(pContext, pData);
   DSL_CHECK_ERR_CODE();

   DSL_CHECK_DSLMODE(pData->nDslMode);
   DSL_CHECK_ERR_CODE();

   DSL_DEBUG(DSL_DBG_MSG,
      (pContext, SYS_DBG_MSG"DSL[%02d]: IN - DSL_DRV_BND_ConfigGet"DSL_DRV_CRLF,
      DSL_DEV_NUM(pContext)));

   /* Get Bonding configuration from pContext*/
   DSL_CTX_READ(pContext, nErrCode, BndConfig[pData->nDslMode], pData->data);

   DSL_DEBUG(DSL_DBG_MSG,
      (pContext, SYS_DBG_MSG"DSL[%02d]: OUT - DSL_DRV_BND_ConfigGet, retCode=%d"
      DSL_DRV_CRLF, DSL_DEV_NUM(pContext), nErrCode));

   return nErrCode;
}
#endif /* INCLUDE_DSL_CONFIG_GET*/

/*
   For a detailed description please refer to the equivalent ioctl
   \ref DSL_FIO_BND_STATUS_GET
*/
DSL_Error_t DSL_DRV_BND_StatusGet(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_BND_StatusGet_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;
   DSL_BondingMode_t eBndStatus = DSL_BONDING_USED_NO_BOND;
   DSL_uint32_t nRemotePafSupported = 0, nRemoteImapSupported = 0;
   ACK_ADSL_FeatureMapGet_t nFeatureMapGetAck = { 0 };

#ifdef __LINUX__
   if (!capable(CAP_NET_ADMIN))
   {
      printk("Permission denied");
      return -EPERM;
   }
#endif /* __LINUX__ */

   DSL_CHECK_POINTER(pContext, pData);
   DSL_CHECK_ERR_CODE();

   DSL_DEBUG(DSL_DBG_MSG,
      (pContext, SYS_DBG_MSG"DSL[%02d]: IN - DSL_DRV_BND_StatusGet"DSL_DRV_CRLF,
      DSL_DEV_NUM(pContext)));

   /* Get Bonding status */
   if (DSL_DRV_BND_BondingStatusCheck(pContext, &eBndStatus) == DSL_SUCCESS)
   {
      if (eBndStatus == DSL_BONDING_USED_PAF)
      {
         pData->data.nPafEnable = DSL_BND_ENABLE_ON;
      }
      else if (eBndStatus == DSL_BONDING_USED_IMAP)
      {
         pData->data.nImapEnable = DSL_BND_ENABLE_ON;
      }
   }

   /* Get Remote PAF Bonding status */
   if (DSL_DRV_BND_RemotePafAvailableCheck(
         pContext, &nRemotePafSupported) == DSL_SUCCESS &&
         nRemotePafSupported > 0)
   {
      pData->data.nRemotePafSupported = DSL_BND_SUPPORT_ON;
   }

   /* Get Remote IMA+ Bonding status */
   nErrCode = DSL_DRV_VRX_SendMsgFeatureMapGet(pContext,
      (DSL_uint8_t *) &nFeatureMapGetAck);
   if (nErrCode == DSL_SUCCESS && nFeatureMapGetAck.W1F01 == VRX_ENABLE)
   {
      if (DSL_DRV_BND_RemoteImapAvailableCheck(
            pContext, &nRemoteImapSupported) == DSL_SUCCESS &&
            nRemoteImapSupported > 0)
      {
         pData->data.nRemoteImapSupported = DSL_BND_SUPPORT_ON;
      }
   }

   DSL_DEBUG(DSL_DBG_MSG,
      (pContext, SYS_DBG_MSG"DSL[%02d]: OUT - DSL_DRV_BND_StatusGet, retCode=%d"
      DSL_DRV_CRLF, DSL_DEV_NUM(pContext), nErrCode));

   return nErrCode;
}

/*
   For a detailed description please refer to the equivalent ioctl
   \ref DSL_FIO_BND_HS_STATUS_GET
*/
DSL_Error_t DSL_DRV_BND_HsStatusGet(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_BND_HsStatusGet_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;

#ifdef __LINUX__
   if (!capable(CAP_NET_ADMIN))
   {
      printk("Permission denied");
      return -EPERM;
   }
#endif /* __LINUX__ */

   DSL_CHECK_POINTER(pContext, pData);
   DSL_CHECK_ERR_CODE();

   DSL_DEBUG(DSL_DBG_MSG,
      (pContext, SYS_DBG_MSG"DSL[%02d]: IN - DSL_DRV_BND_HsStatusGet"DSL_DRV_CRLF,
      DSL_DEV_NUM(pContext)));

   if (DSL_DRV_LINES_PER_DEVICE == 2 && DSL_DRV_DEV_FirmwareFeatureCheck(pContext,
      DSL_FW_XDSLFEATURE_DUALPORT) == DSL_FALSE)
   {
      /* on-chip bonding: ignore if feature is not supported */
      DSL_DEBUG(DSL_DBG_WRN, (pContext, SYS_DBG_WRN"DSL[%02d]: "
         "WARNING - DSL_DRV_BND_HsStatusGet(): Firmware does not support bonding"
         DSL_DRV_CRLF, DSL_DEV_NUM(pContext)));
   }
   else
   {
      /* Call VRX device specific implementation*/
      nErrCode = DSL_DRV_BND_DEV_HsStatusGet(pContext, pData);
   }

   DSL_DEBUG(DSL_DBG_MSG,
      (pContext, SYS_DBG_MSG"DSL[%02d]: OUT - DSL_DRV_BND_HsStatusGet, retCode=%d"
      DSL_DRV_CRLF, DSL_DEV_NUM(pContext), nErrCode));

   return nErrCode;
}

static DSL_Error_t DSL_DRV_BND_BondingModeGet(
   DSL_IN DSL_Context_t *pContext,
   DSL_IN const DSL_void_t *pTcStatusData,
   DSL_OUT DSL_BondingMode_t *pBndMode
)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;
   const ACK_TC_StatusGet_t *pData = DSL_NULL;
   DSL_BondingMode_t eBndMode = DSL_BONDING_USED_NO_BOND;
   DSL_BND_StatusData_t status;

   if (pTcStatusData == DSL_NULL)
   {
      return DSL_ERROR;
   }

   pData = (ACK_TC_StatusGet_t*)pTcStatusData; /* or EVT_TC_StatusGet_t */

   /* Backward compatibility: if CMD/EVT_TC_STATUSGET provides
      the Bonding enable status (number of items is 2),
      use it, otherwise take the PAF Bonding configuration from
      remote (means: current FW does not support IMA+ Bonding yet
      and PAF Bonding configuration can be only taken from
      remote) */
   if (pData->Length == 2)
   {
      switch (pData->Bonding)
      {
      case ACK_TC_StatusGet_PAF: /* or EVT_TC_StatusGet_PAF */
         eBndMode = DSL_BONDING_USED_PAF;
         break;
      case ACK_TC_StatusGet_IMAP: /* or EVT_TC_StatusGet_IMAP */
         eBndMode = DSL_BONDING_USED_IMAP;
         break;
      case ACK_TC_StatusGet_NO_BOND: /* or EVT_TC_StatusGet_NO_BOND */
      default:
         eBndMode = DSL_BONDING_USED_NO_BOND;
         break;
      }
   }
   else
   {
      /* IMA+ Bonding enable status "FALSE" by definition,
         check only if PAF Bonding supported */
      DSL_DRV_MemSet(&status, 0x0, sizeof(DSL_BND_StatusData_t));
      nErrCode = DSL_DRV_BND_DEV_PafBndStatusGet(pContext, &status);
      if (nErrCode != DSL_SUCCESS)
      {
         DSL_DEBUG(DSL_DBG_ERR,
            (pContext, SYS_DBG_ERR "DSL[%02d]: ERROR - "
            "DSL_DRV_BND_BondingModeGet: BND status get failed!"
            DSL_DRV_CRLF, DSL_DEV_NUM(pContext)));

         return nErrCode;
      }

      if (status.nRemotePafSupported)
      {
         eBndMode = DSL_BONDING_USED_PAF;
      }

      DSL_DEBUG( DSL_DBG_WRN,(pContext, SYS_DBG_WRN"DSL[%02d]: "
         "WARNING - Bonding information not supported by current FW, "
         "taking value from Remote PAF Bonding status (%s)"
         DSL_DRV_CRLF, DSL_DEV_NUM(pContext), status.nRemotePafSupported ?
            "ENABLED" : "DISABLED"));
   }

   *pBndMode = eBndMode;

   return nErrCode;
}

DSL_Error_t DSL_DRV_BND_BondingStatusSet(
   DSL_IN DSL_Context_t *pContext,
   DSL_IN const DSL_void_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;
   DSL_BondingMode_t eBndMode = DSL_BONDING_USED_NO_BOND,
      ePrevBndMode = DSL_BONDING_USED_NO_BOND;
   DSL_boolean_t bBndStatusValid = DSL_FALSE;

   nErrCode = DSL_DRV_BND_BondingModeGet(pContext, &pData, &eBndMode);
   if (nErrCode != DSL_SUCCESS)
   {
      DSL_DEBUG( DSL_DBG_ERR,(pContext, SYS_DBG_ERR"DSL[%02d]: "
         "ERROR - Failed to get bonding information from the FW!"
         DSL_DRV_CRLF, DSL_DEV_NUM(pContext)));
      return nErrCode;
   }

   /* Check if bonding status has changed since previous update -
      it would mean that the values provided by the FW can be inconsistent
      during single startup and it should be indicated with a warning */
   DSL_CTX_READ_SCALAR(pContext, nErrCode,
      bBndStatusValid, bBndStatusValid);
   if(nErrCode == DSL_SUCCESS && bBndStatusValid == DSL_TRUE)
   {
      DSL_CTX_READ(pContext, nErrCode, BndStatus, ePrevBndMode);
      if (ePrevBndMode != eBndMode)
      {
         DSL_DEBUG( DSL_DBG_WRN,(pContext, SYS_DBG_WRN"DSL[%02d]: "
         "WARNING - Bonding status fluctuation observed during "
         "startup" DSL_DRV_CRLF, DSL_DEV_NUM(pContext)));
      }
   }

   /* Store the new bonding status in pContext */
   DSL_CTX_WRITE_SCALAR(pContext, nErrCode, BndStatus, eBndMode);

   /* Set the validity flag */
   if (bBndStatusValid == DSL_FALSE)
   {
      DSL_CTX_WRITE_SCALAR(pContext, nErrCode, bBndStatusValid, DSL_TRUE);
   }

   /* Trigger update of all Bonding information */
   DSL_DRV_BND_BondingStatusUpdate(pContext);

   return nErrCode;
}

static DSL_Error_t DSL_DRV_BND_BondingInfoUpdate(
   DSL_IN DSL_Context_t *pContext,
   DSL_IN const DSL_BND_BndInfoType_t eBndInfoType)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;
   DSL_boolean_t bBndStatusValid, bRemotePafBndValid, bRemoteImapBndValid;
   ACK_TC_StatusGet_t sAck;
   DSL_BND_StatusData_t status;
   ACK_ADSL_FeatureMapGet_t nFeatureMapGetAck = { 0 };
   DSL_LineStateValue_t nCurrentState = DSL_LINESTATE_UNKNOWN;

   switch (eBndInfoType)
   {
      case DSL_BND_INFO_STATUS:
      {
         DSL_CTX_READ_SCALAR(pContext, nErrCode,
            bBndStatusValid, bBndStatusValid);
         DSL_CTX_READ_SCALAR(pContext, nErrCode, nLineState, nCurrentState);
         if (nErrCode == DSL_SUCCESS && bBndStatusValid ==  DSL_FALSE &&
            nCurrentState >= DSL_LINESTATE_FULL_INIT)
         {
            /* Get the information from FW */
            memset(&sAck, 0, sizeof(sAck));
            nErrCode = DSL_DRV_VRX_SendMsgTcStatusGet(
               pContext, (DSL_uint8_t*)&sAck);
            if (nErrCode != DSL_SUCCESS)
            {
               DSL_DEBUG(DSL_DBG_ERR,
                     (pContext, SYS_DBG_ERR "DSL[%02d]: ERROR - "
                     "DSL_DRV_BND_BondingInfoUpdate: TC status get failed!"
                     DSL_DRV_CRLF, DSL_DEV_NUM(pContext)));
               return DSL_ERROR;
            }

            /* Update all parameters provided by the msg
               (TC layer and Bonding mode) */
            nErrCode = DSL_DRV_VRX_TcStatusSet(pContext, &sAck);
            if (nErrCode != DSL_SUCCESS)
            {
               DSL_DEBUG( DSL_DBG_ERR,(pContext, SYS_DBG_ERR
                  "DSL[%02d]: ERROR - Failed to set TC status!"
                  DSL_DRV_CRLF, DSL_DEV_NUM(pContext)));
               break;
            }
         }
         break;
      }
      case DSL_BND_INFO_REMOTE_PAF:
      {
         DSL_CTX_READ_SCALAR(pContext, nErrCode,
            bRemotePafBndValid, bRemotePafBndValid);
         DSL_CTX_READ_SCALAR(pContext, nErrCode, nLineState, nCurrentState);
         if(nErrCode == DSL_SUCCESS && bRemotePafBndValid == DSL_FALSE &&
            nCurrentState >= DSL_LINESTATE_FULL_INIT)
         {
            /* Update Remote PAF Bonding Status */
            DSL_DRV_MemSet(&status, 0x0, sizeof(DSL_BND_StatusData_t));
            nErrCode = DSL_DRV_BND_DEV_PafBndStatusGet(pContext, &status);
            if (nErrCode != DSL_SUCCESS)
            {
               DSL_DEBUG(DSL_DBG_ERR,
                  (pContext, SYS_DBG_ERR "DSL[%02d]: ERROR - "
                  "DSL_DRV_BND_BondingInfoUpdate: PAF BND status get failed!"
                  DSL_DRV_CRLF, DSL_DEV_NUM(pContext)));

               return nErrCode;
            }
            DSL_CTX_WRITE_SCALAR(pContext, nErrCode,
               RemoteBndConfig.bPafEnable,
               (DSL_uint32_t)status.nRemotePafSupported);

            /* Set the validity flag as the line state reached FULL_INIT */
            DSL_CTX_WRITE_SCALAR(pContext, nErrCode,
               bRemotePafBndValid, DSL_TRUE);
         }
         break;
      }
      case DSL_BND_INFO_REMOTE_IMAP:
      {
         DSL_CTX_READ_SCALAR(pContext, nErrCode,
            bRemoteImapBndValid, bRemoteImapBndValid);
         DSL_CTX_READ_SCALAR(pContext, nErrCode, nLineState, nCurrentState);
         if (nErrCode == DSL_SUCCESS && bRemoteImapBndValid == DSL_FALSE &&
            nCurrentState >= DSL_LINESTATE_FULL_INIT)
         {
            /* Update Remote IMA+ Bonding Status */
            nErrCode = DSL_DRV_VRX_SendMsgFeatureMapGet(pContext,
               (DSL_uint8_t *) &nFeatureMapGetAck);
            if (nErrCode == DSL_SUCCESS && nFeatureMapGetAck.W1F01 == VRX_ENABLE)
            {
               DSL_DRV_MemSet(&status, 0x0, sizeof(DSL_BND_StatusData_t));
               nErrCode = DSL_DRV_BND_DEV_ImapBndStatusGet(pContext, &status);
               if (nErrCode != DSL_SUCCESS)
               {
                  DSL_DEBUG(DSL_DBG_ERR,
                     (pContext, SYS_DBG_ERR "DSL[%02d]: ERROR - "
                     "DSL_DRV_BND_BondingInfoUpdate: IMA+ BND status get failed!"
                     DSL_DRV_CRLF, DSL_DEV_NUM(pContext)));

                  return nErrCode;
               }
               DSL_CTX_WRITE_SCALAR(pContext, nErrCode,
                  RemoteBndConfig.bImapEnable,
                  (DSL_uint32_t)status.nRemoteImapSupported);
            }

            /* Set the validity flag as the line state reached FULL_INIT */
            DSL_CTX_WRITE_SCALAR(pContext, nErrCode,
               bRemoteImapBndValid, DSL_TRUE);
         }
         break;
      }
      default:
         DSL_DEBUG( DSL_DBG_WRN, (pContext,
            SYS_DBG_WRN"DSL[%02d]: WARNING - DSL_DRV_BND_BondingInfoUpdate: "
            "invalid argument!"DSL_DRV_CRLF, DSL_DEV_NUM(pContext)));
         break;
   }

   return nErrCode;
}

DSL_Error_t DSL_DRV_BND_BondingStatusCheck(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_BondingMode_t *pBndStatus)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;
   DSL_boolean_t bBndStatusValid;
   DSL_BondingMode_t eBndStatus = DSL_BONDING_USED_NO_BOND;

   DSL_CTX_READ_SCALAR(pContext, nErrCode, bBndStatusValid, bBndStatusValid);
   if(nErrCode == DSL_SUCCESS && bBndStatusValid == DSL_FALSE)
   {
      nErrCode = DSL_DRV_BND_BondingInfoUpdate(
         pContext, DSL_BND_INFO_STATUS);
      if (nErrCode != DSL_SUCCESS)
      {
         DSL_DEBUG(DSL_DBG_ERR,
            (pContext, SYS_DBG_ERR "DSL[%02d]: ERROR - "
            "DSL_DRV_BND_BondingStatusCheck: BND info update failed!"
            DSL_DRV_CRLF, DSL_DEV_NUM(pContext)));

         return nErrCode;
      }
   }

   /* Get Bonding status from pContext */
   DSL_CTX_READ_SCALAR(pContext, nErrCode, BndStatus, eBndStatus);
   *pBndStatus = eBndStatus;

   return DSL_SUCCESS;
}

DSL_Error_t DSL_DRV_BND_RemotePafAvailableCheck(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_uint32_t *pRemotePafAvailable)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;
   DSL_boolean_t bRemotePafBndValid, bRemotePafSupported;

   DSL_CTX_READ_SCALAR(pContext, nErrCode,
      bRemotePafBndValid, bRemotePafBndValid);
   if(nErrCode == DSL_SUCCESS && bRemotePafBndValid == DSL_FALSE)
   {
      nErrCode = DSL_DRV_BND_BondingInfoUpdate(
         pContext, DSL_BND_INFO_REMOTE_PAF);
      if (nErrCode != DSL_SUCCESS)
      {
         DSL_DEBUG(DSL_DBG_ERR,
            (pContext, SYS_DBG_ERR "DSL[%02d]: ERROR - "
            "DSL_DRV_BND_RemotePafAvailableCheck: BND info update failed!"
            DSL_DRV_CRLF, DSL_DEV_NUM(pContext)));

         return nErrCode;
      }
   }

   /* Get RemotePafAvailable from pContext */
   DSL_CTX_READ_SCALAR(pContext, nErrCode,
      RemoteBndConfig.bPafEnable, bRemotePafSupported);
   *pRemotePafAvailable = (DSL_uint32_t)bRemotePafSupported;

   return DSL_SUCCESS;
}

DSL_Error_t DSL_DRV_BND_RemoteImapAvailableCheck(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_uint32_t *pRemoteImapAvailable)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;
   DSL_boolean_t bRemoteImapBndValid, bRemoteImapSupported;

   DSL_CTX_READ_SCALAR(pContext, nErrCode,
      bRemoteImapBndValid, bRemoteImapBndValid);
   if(nErrCode == DSL_SUCCESS && bRemoteImapBndValid == DSL_FALSE)
   {
      nErrCode = DSL_DRV_BND_BondingInfoUpdate(
         pContext, DSL_BND_INFO_REMOTE_IMAP);
      if (nErrCode != DSL_SUCCESS)
      {
         DSL_DEBUG(DSL_DBG_ERR,
            (pContext, SYS_DBG_ERR "DSL[%02d]: ERROR - "
            "DSL_DRV_BND_RemoteImapAvailableCheck: BND info update failed!"
            DSL_DRV_CRLF, DSL_DEV_NUM(pContext)));

         return nErrCode;
      }
   }

   /* Get RemoteImapAvailable from pContext */
   DSL_CTX_READ_SCALAR(pContext, nErrCode,
      RemoteBndConfig.bImapEnable, bRemoteImapSupported);
   *pRemoteImapAvailable = (DSL_uint32_t)bRemoteImapSupported;

   return DSL_SUCCESS;
}

DSL_void_t DSL_DRV_BND_BondingStatusUpdate(
   DSL_IN DSL_Context_t *pContext)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;

   /* Update Bonding status */
   nErrCode = DSL_DRV_BND_BondingInfoUpdate(pContext, DSL_BND_INFO_STATUS);
   if (nErrCode != DSL_SUCCESS)
   {
      DSL_DEBUG(DSL_DBG_WRN,
         (pContext, SYS_DBG_WRN "DSL[%02d]: WARNING - "
         "DSL_DRV_BND_BondingStatusUpdate: BND status update failed!"
         DSL_DRV_CRLF, DSL_DEV_NUM(pContext)));
   }

   /* Update Remote PAF Bonding status */
   nErrCode = DSL_DRV_BND_BondingInfoUpdate(pContext, DSL_BND_INFO_REMOTE_PAF);
   if (nErrCode != DSL_SUCCESS)
   {
      DSL_DEBUG(DSL_DBG_WRN,
         (pContext, SYS_DBG_WRN "DSL[%02d]: WARNING - "
         "DSL_DRV_BND_BondingStatusUpdate: Remote PAF "
         "BND status update failed!"
         DSL_DRV_CRLF, DSL_DEV_NUM(pContext)));
   }

   /* Update Remote IMA+ Bonding status */
   nErrCode = DSL_DRV_BND_BondingInfoUpdate(pContext, DSL_BND_INFO_REMOTE_IMAP);
   if (nErrCode != DSL_SUCCESS)
   {
      DSL_DEBUG(DSL_DBG_WRN,
         (pContext, SYS_DBG_WRN "DSL[%02d]: WARNING - "
         "DSL_DRV_BND_BondingStatusUpdate: Remote IMA+ "
         "BND status update failed!"
         DSL_DRV_CRLF, DSL_DEV_NUM(pContext)));
   }
}

/*
   For a detailed description please refer to the equivalent ioctl
   \ref DSL_FIO_BND_HS_CONTINUE
*/
DSL_Error_t DSL_DRV_BND_HsContinue(
   DSL_IN DSL_Context_t *pContext,
   DSL_IN DSL_BND_HsContinue_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;

#ifdef __LINUX__
   if (!capable(CAP_NET_ADMIN))
   {
      printk("Permission denied");
      return -EPERM;
   }
#endif /* __LINUX__ */

   DSL_CHECK_POINTER(pContext, pData);
   DSL_CHECK_ERR_CODE();

   DSL_DEBUG(DSL_DBG_MSG,
      (pContext, SYS_DBG_MSG"DSL[%02d]: IN - DSL_DRV_BND_HsContinue"DSL_DRV_CRLF,
      DSL_DEV_NUM(pContext)));

   /* Call device specific implementation*/
   nErrCode = DSL_DRV_BND_DEV_HsContinue(pContext, pData);

   DSL_DEBUG(DSL_DBG_MSG,
      (pContext, SYS_DBG_MSG"DSL[%02d]: OUT - DSL_DRV_BND_HsContinue, retCode=%d"
      DSL_DRV_CRLF, DSL_DEV_NUM(pContext), nErrCode));

   return nErrCode;
}

/*
   For a detailed description please refer to the equivalent ioctl
   \ref DSL_FIO_BND_ETH_DBG_COUNTERS_GET
*/
DSL_Error_t DSL_DRV_BND_EthDbgCountersGet(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_BND_EthDbgCounters_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;

#ifdef __LINUX__
   if (!capable(CAP_NET_ADMIN))
   {
      printk("Permission denied");
      return -EPERM;
   }
#endif /* __LINUX__ */

   DSL_CHECK_POINTER(pContext, pData);
   DSL_CHECK_ERR_CODE();

   DSL_DEBUG(DSL_DBG_MSG,
      (pContext, SYS_DBG_MSG"DSL[%02d]: IN - DSL_DRV_BND_EthDbgCountersGet"DSL_DRV_CRLF,
      DSL_DEV_NUM(pContext)));

   /* Call device specific implementation*/
   nErrCode = DSL_DRV_BND_DEV_EthDbgCountersGet(pContext, pData);

   DSL_DEBUG(DSL_DBG_MSG,
      (pContext, SYS_DBG_MSG"DSL[%02d]: OUT - DSL_DRV_BND_EthDbgCountersGet, retCode=%d"
      DSL_DRV_CRLF, DSL_DEV_NUM(pContext), nErrCode));

   return nErrCode;
}

/*
   For a detailed description please refer to the equivalent ioctl
   \ref DSL_FIO_BND_ETH_COUNTERS_GET
*/
DSL_Error_t DSL_DRV_BND_EthCountersGet(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_BND_EthCounters_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;

#ifdef __LINUX__
   if (!capable(CAP_NET_ADMIN))
   {
      printk("Permission denied");
      return -EPERM;
   }
#endif /* __LINUX__ */

   DSL_CHECK_POINTER(pContext, pData);
   DSL_CHECK_ERR_CODE();

   DSL_DEBUG(DSL_DBG_MSG,
      (pContext, SYS_DBG_MSG"DSL[%02d]: IN - DSL_DRV_BND_EthCountersGet"DSL_DRV_CRLF,
      DSL_DEV_NUM(pContext)));

   /* Call device specific implementation*/
   nErrCode = DSL_DRV_BND_DEV_EthCountersGet(pContext, pData);

   DSL_DEBUG(DSL_DBG_MSG,
      (pContext, SYS_DBG_MSG"DSL[%02d]: OUT - DSL_DRV_BND_EthCountersGet, retCode=%d"
      DSL_DRV_CRLF, DSL_DEV_NUM(pContext), nErrCode));

   return nErrCode;
}

DSL_Error_t DSL_DRV_BND_PortModeSyncSet(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_BND_PortModeSync_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;

#ifdef __LINUX__
   if (!capable(CAP_NET_ADMIN))
   {
      printk("Permission denied");
      return -EPERM;
   }
#endif /* __LINUX__ */

   DSL_CHECK_POINTER(pContext, pData);
   DSL_CHECK_ERR_CODE();

   DSL_DEBUG(DSL_DBG_MSG,
      (pContext, SYS_DBG_MSG"DSL[%02d]: IN - DSL_DRV_BND_PortModeSyncSet"DSL_DRV_CRLF,
      DSL_DEV_NUM(pContext)));

   if (pData->data.nPortMode < DSL_PORT_MODE_NA ||
       pData->data.nPortMode >= DSL_PORT_MODE_LAST)
   {
      nErrCode = DSL_ERR_PARAM_RANGE;
   }
   else
   {
      DSL_CTX_WRITE(pContext, nErrCode, BndPortModeSync, pData->data.nPortMode);
   }

   DSL_DEBUG(DSL_DBG_MSG,
      (pContext, SYS_DBG_MSG"DSL[%02d]: OUT - DSL_DRV_BND_PortModeSyncSet, retCode=%d"
      DSL_DRV_CRLF, DSL_DEV_NUM(pContext), nErrCode));

   return nErrCode;

}


#ifdef __cplusplus
}
#endif
