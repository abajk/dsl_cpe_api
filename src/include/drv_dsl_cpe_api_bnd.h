/******************************************************************************

         Copyright 2016 - 2020 Intel Corporation
         Copyright 2015 - 2016 Lantiq Beteiligungs-GmbH & Co. KG
         Copyright 2009 - 2014 Lantiq Deutschland GmbH
         Copyright 2007 - 2008 Infineon Technologies AG

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

#ifndef _DRV_DSL_CPE_API_BND_H
#define _DRV_DSL_CPE_API_BND_H

#ifdef __cplusplus
   extern "C" {
#endif

/** \file
   Bonding interface.
*/

/** \addtogroup DRV_DSL_CPE_BND
 @{ */


#include "drv_dsl_cpe_api.h"


/**
   Data structure used to configure the bonding operation modes.
*/
typedef struct
{
   /**
   Enable/disable PAF (PTM) bonding
   DSL_FALSE - PAF bonding is disabled (Default)
   DSL_TRUE  - PAF bonding is enabled */
   DSL_IN DSL_boolean_t bPafEnable;
   /**
   Enable/disable IMA+ (ATM) bonding
   DSL_FALSE - IMA+ bonding is disabled (Default)
   DSL_TRUE  - IMA+ bonding is enabled
   \note This bonding mode is only supported for ADSL modes. */
   DSL_IN DSL_boolean_t bImapEnable;
} DSL_BND_ConfigData_t;

/**
   Structure for configuring bonding operation modes.
   This structure has to be used for ioctl
   - \ref DSL_FIO_BND_CONFIG_SET
   - \ref DSL_FIO_BND_CONFIG_GET
*/
typedef struct
{
   /**
   Driver control/status structure */
   DSL_IN_OUT DSL_AccessCtl_t accessCtl;
   /**
   Specifies for which DSL flavor the configuration will be applied. */
   DSL_IN DSL_DslModeSelection_t nDslMode;
   /**
   Structure that contains Bonding config data */
   DSL_CFG DSL_BND_ConfigData_t data;
} DSL_BND_Config_t;

/**
   Defines possible values to indicate whether the bonding feature is enabled
   or not.
*/
typedef enum
{
   /**
   Default value for initialization (not updated/initialized so far) */
   DSL_BND_ENABLE_NOT_INITIALIZED = -1,
   /**
   Bonding is disabled. */
   DSL_BND_ENABLE_OFF = 0,
   /**
   Bonding is enabled. */
   DSL_BND_ENABLE_ON = 1,
} DSL_BND_Enable_t;

/**
   Defines possible values to indicate whether the bonding feature is supported
   or not.
*/
typedef enum
{
   /**
   Default value for initialization (not updated/initialized so far) */
   DSL_BND_SUPPORT_NOT_INITIALIZED = -1,
   /**
   Bonding is not supported. */
   DSL_BND_SUPPORT_OFF = 0,
   /**
   Bonding is supported. */
   DSL_BND_SUPPORT_ON = 1,
} DSL_BND_Supported_t;

/**
   Data structure used to retrieve the bonding operation status (for NE and FE).
*/
typedef struct
{
   /**
   Status for PAF (PTM) bonding operation at the near end side. */
   DSL_OUT DSL_BND_Enable_t nPafEnable;
   /**
   Status for IMA+ (ATM) bonding operation at the near end side. */
   DSL_OUT DSL_BND_Enable_t nImapEnable;
   /**
   Status for PAF (PTM) bonding support at the far end side. */
   DSL_OUT DSL_BND_Supported_t nRemotePafSupported;
   /**
   Status for IMA+ (ATM) bonding support at the far end side. */
   DSL_OUT DSL_BND_Supported_t nRemoteImapSupported;
} DSL_BND_StatusData_t;

/**
   Structure to retrieve bonding operation status.
   This structure has to be used for ioctl
   - \ref DSL_FIO_BND_STATUS_GET
*/
typedef struct
{
   /**
   Driver control/status structure */
   DSL_IN_OUT DSL_AccessCtl_t accessCtl;
   /**
   Structure that returns Bonding status data */
   DSL_OUT DSL_BND_StatusData_t data;
} DSL_BND_StatusGet_t;

/**
   Defines possible values that signals the status whether the bonding feature
   is enabled on the far end side.
   \note This value is equivalent to the one with the same definition, which is
         provided by the ioctl \ref DSL_FIO_BND_STATUS_GET. It remains here for
         backward compatibility reason and reflects the former PAF only
         implementation, together with the other parameters within this
         structure.
*/
typedef DSL_BND_Enable_t DSL_BND_RemotePafSupported_t;

/**
   This type enumerates the Discovery or Aggregation activation modes.
*/
typedef enum
{
   /**
   No command */
   DSL_BND_NO_COMMAND = 0,
   /**
   Discovery set if clear */
   DSL_BND_DISCOVERY_SET_IF_CLEAR = 1,
   /**
   Discovery clear if same */
   DSL_BND_DISCOVERY_CLEAR_IF_SAME = 2,
   /**
   Aggregate set */
   DSL_BND_AGGREGATE_SET = 4,
   /**
   Aggregate clear */
   DSL_BND_AGGREGATE_CLR = 8,
} DSL_BND_ActivationMode_t;

/**
   Structure to retrieve bonding handshake status.
*/
typedef struct
{
   /**
   Indicates whether the bonding feature is enabled at the far end side.
   \note This value is equivalent to the one with the same name, which is
         provided by the ioctl \ref DSL_FIO_BND_STATUS_GET. It remains here for
         backward compatibility reason and reflects the former PAF only
         implementation, together with the other parameters within this
         structure. */
   DSL_OUT DSL_BND_RemotePafSupported_t nRemotePafSupported;
   /**
   Indicates the type of Aggregate or Discovery command received */
   DSL_OUT DSL_BND_ActivationMode_t nActivationMode;
   /**
   Discovery Code received in initial CL during "Set if Clear"
   or "Clear if Same" exchange. */
   DSL_OUT DSL_uint8_t nDiscoveryCode[6];
   /**
   Aggregate Data received in initial CL during "Set" command. */
   DSL_OUT DSL_uint32_t nAggregateData;
} DSL_BND_HsStatusData_t;

/**
   Structure to retrieve bonding handshake status.
   This structure has to be used for ioctl
   - \ref DSL_FIO_BND_HS_STATUS_GET
*/
typedef struct
{
   /**
   Driver control/status structure */
   DSL_IN_OUT DSL_AccessCtl_t accessCtl;
   /**
   Structure that contains Bonding handshake status data */
   DSL_OUT DSL_BND_HsStatusData_t data;
} DSL_BND_HsStatusGet_t;

/**
   Structure to handle the handshake continue process.
*/
typedef struct
{
   /**
   Remote Discovery Register value to be used in CLR */
   DSL_IN DSL_uint8_t nDiscoveryCode[6];
   /**
   Aggregate Data value to be used in CLR */
   DSL_IN DSL_uint32_t nAggregateData;
} DSL_BND_HsContinueData_t;

/**
   Structure to configure the Remote Discovery Register and Aggregate Register
   for Bonding handshake.
   This structure has to be used for ioctl
   - \ref DSL_FIO_BND_HS_CONTINUE
*/
typedef struct
{
   /**
   Driver control/status structure */
   DSL_IN_OUT DSL_AccessCtl_t accessCtl;
   /**
   Structure that contains handshake continue process data */
   DSL_IN DSL_BND_HsContinueData_t data;
} DSL_BND_HsContinue_t;

/**
   Structure to handle Bonding Ethernet Debug counters.

   \note These counters are non standardized.
*/
typedef struct
{
   /**
   Count of Rx good fragments.
   This value is calculated as follows
   - nRxGoodFragments = nRxFragmentsLink0 + nRxFragmentsLink1 -
                        nRxFragmentsDrop */
   DSL_OUT DSL_uint32_t nRxGoodFragments;
   /**
   Count of Rx Packets transmitted to Network Processor */
   DSL_OUT DSL_uint16_t nRxPackets;
   /**
   Count of Rx Fragments received from PHY0 */
   DSL_OUT DSL_uint16_t nRxFragmentsLink0;
   /**
   Count of Rx Fragments received from PHY1 */
   DSL_OUT DSL_uint16_t nRxFragmentsLink1;
   /**
   Count of Rx Fragments dropped */
   DSL_OUT DSL_uint16_t nRxFragmentsDrop;
   /**
   Count of Tx Packets received from Network Processor */
   DSL_OUT DSL_uint16_t nTxPackets;
   /**
   Count of Tx Fragments transmitted in PHY0 */
   DSL_OUT DSL_uint16_t nTxFragmentsLink0;
   /**
   Count of Tx Fragments transmitted in PHY1 */
   DSL_OUT DSL_uint16_t nTxFragmentsLink1;
   /**
   Control register includes common configuration settings.*/
   DSL_OUT DSL_uint16_t nControlRegister;
} DSL_BND_EthDbgCountersData_t;

/**
   Structure to return Bonding Ethernet Debug counters
*/
typedef struct
{
   /**
   Driver control/status structure */
   DSL_IN_OUT DSL_AccessCtl_t accessCtl;
   /**
   Structure that contains Bonding Ethernet Debug counters data */
   DSL_OUT DSL_BND_EthDbgCountersData_t data;
} DSL_BND_EthDbgCounters_t;

/**
   Structure to handle Bonding Ethernet counters.
*/
typedef struct
{
   /**
   Accumulated count of Rx Errored fragments PHY 0+1*/
   DSL_OUT DSL_uint32_t nRxErroredFragments;
   /**
   Accumulated count of Rx Small fragments PHY 0+1*/
   DSL_OUT DSL_uint32_t nRxSmallFragments;
   /**
   Accumulated count of Rx Large fragments PHY 0+1*/
   DSL_OUT DSL_uint32_t nRxLargeFragments;
   /**
   Count of Rx Lost Fragment Count */
   DSL_OUT DSL_uint16_t nRxLostFragments;
   /**
   Count of Rx Lost SOP Count*/
   DSL_OUT DSL_uint16_t nRxLostStarts;
   /**
   Count of Rx Lost EOP Count*/
   DSL_OUT DSL_uint16_t nRxLostEnds;
   /**
   Accumulated count of Rx Buffer Overflow PHY 0+1*/
   DSL_OUT DSL_uint32_t nRxOverlfows;
   /**
   Count of Rx Bad Fragment Count */
   DSL_OUT DSL_uint16_t nRxBadFragments;
} DSL_BND_EthCountersData_t;

/**
   Structure to return Bonding Ethernet counters.
   This structure has to be used for ioctl
   - \ref  DSL_FIO_BND_ETH_COUNTERS_GET
*/
typedef struct
{
   /**
   Driver control/status structure */
   DSL_IN_OUT DSL_AccessCtl_t accessCtl;
   /**
   Structure that contains Bonding Ethernet Counter data */
   DSL_OUT DSL_BND_EthCountersData_t data;
} DSL_BND_EthCounters_t;

/**
   Structure to initialize the bonding hardware part.
   This structure has to be used for ioctl
   - \ref  DSL_FIO_BND_HW_INIT
*/
typedef struct
{
   /**
   Driver control/status structure */
   DSL_IN_OUT DSL_AccessCtl_t accessCtl;
} DSL_BND_HwInit_t;


/**
   Structure used to specify bonding link port mode
*/
typedef struct
{
   /**
   Port mode data*/
   DSL_IN DSL_PortMode_t nPortMode;
} DSL_BND_PortModeSyncData_t;

/**
   Structure used to specify bonding port mode.
   This structure has to be used for ioctl
   - \ref  DSL_FIO_BND_PORT_MODE_SYNC_SET
*/
typedef struct
{
   /**
   Driver control/status structure */
   DSL_IN_OUT DSL_AccessCtl_t accessCtl;
   /**
   Structure that contains Bonding link Port Mode data */
   DSL_IN DSL_BND_PortModeSyncData_t data;
} DSL_BND_PortModeSync_t;

/**
   For a detailed description please refer to the equivalent ioctl
   \ref DSL_FIO_BND_HW_INIT
*/
#ifndef SWIG_TMP
DSL_Error_t DSL_DRV_BND_HwInit(
   DSL_IN DSL_Context_t *pContext,
   DSL_IN DSL_BND_HwInit_t *pData);
#endif

/*
   For a detailed description please refer to the equivalent ioctl
   \ref DSL_FIO_BND_CONFIG_SET
*/
#ifndef SWIG_TMP
DSL_Error_t DSL_DRV_BND_ConfigSet(
   DSL_IN DSL_Context_t *pContext,
   DSL_IN DSL_BND_Config_t *pData);
#endif

/*
   For a detailed description please refer to the equivalent ioctl
   \ref DSL_FIO_BND_CONFIG_GET
*/
#ifdef INCLUDE_DSL_CONFIG_GET
#ifndef SWIG_TMP
DSL_Error_t DSL_DRV_BND_ConfigGet(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_BND_Config_t *pData);
#endif
#endif

/*
   For a detailed description please refer to the equivalent ioctl
   \ref DSL_FIO_BND_STATUS_GET
*/
#ifndef SWIG_TMP
DSL_Error_t DSL_DRV_BND_StatusGet(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_BND_StatusGet_t *pData);
#endif

/*
   For a detailed description please refer to the equivalent ioctl
   \ref DSL_FIO_BND_STATUS_GET
*/
#ifndef SWIG_TMP
DSL_Error_t DSL_DRV_BND_StatusGet(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_BND_StatusGet_t *pData);
#endif

/*
   For a detailed description please refer to the equivalent ioctl
   \ref DSL_FIO_BND_HS_STATUS_GET
*/
#ifndef SWIG_TMP
DSL_Error_t DSL_DRV_BND_HsStatusGet(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_BND_HsStatusGet_t *pData);
#endif

#ifndef SWIG_TMP
DSL_Error_t DSL_DRV_BND_BondingStatusSet(
   DSL_IN DSL_Context_t *pContext,
   DSL_IN const DSL_void_t *pData);
#endif

#ifndef SWIG_TMP
DSL_Error_t DSL_DRV_BND_BondingStatusCheck(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_BondingMode_t *pBndStatus);
#endif

#ifndef SWIG_TMP
DSL_Error_t DSL_DRV_BND_RemotePafAvailableCheck(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_uint32_t *pRemotePafAvailable);
#endif

#ifndef SWIG_TMP
DSL_Error_t DSL_DRV_BND_RemoteImapAvailableCheck(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_uint32_t *pRemoteImapAvailable);
#endif

#ifndef SWIG_TMP
DSL_void_t DSL_DRV_BND_BondingStatusUpdate(
   DSL_IN DSL_Context_t *pContext);
#endif

/*
   For a detailed description please refer to the equivalent ioctl
   \ref DSL_FIO_BND_HS_CONTINUE
*/
#ifndef SWIG_TMP
DSL_Error_t DSL_DRV_BND_HsContinue(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_BND_HsContinue_t *pData);
#endif

/*
   For a detailed description please refer to the equivalent ioctl
   \ref DSL_FIO_BND_ETH_DBG_COUNTERS_GET
*/
#ifndef SWIG_TMP
DSL_Error_t DSL_DRV_BND_EthDbgCountersGet(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_BND_EthDbgCounters_t *pData);
#endif

/*
   For a detailed description please refer to the equivalent ioctl
   \ref DSL_FIO_BND_ETH_COUNTERS_GET
*/
#ifndef SWIG_TMP
DSL_Error_t DSL_DRV_BND_EthCountersGet(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_BND_EthCounters_t *pData);
#endif

/*
   For a detailed description please refer to the equivalent ioctl
   \ref DSL_FIO_BND_PORT_MODE_SYNC_SET
*/
#ifndef SWIG_TMP
DSL_Error_t DSL_DRV_BND_PortModeSyncSet(
   DSL_IN DSL_Context_t *pContext,
   DSL_OUT DSL_BND_PortModeSync_t *pData);
#endif

/** @} */ /*DRV_DSL_CPE_BND */

#ifdef __cplusplus
}
#endif

#endif /** _DRV_DSL_CPE_API_BND_H*/
