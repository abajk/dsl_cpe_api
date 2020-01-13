/******************************************************************************

         Copyright 2018 - 2019 Intel Corporation

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*******************************************************************************/

#ifndef _DSL_CPE_CONFIGURATION_PARSER_H
#define _DSL_CPE_CONFIGURATION_PARSER_H

#if defined(INCLUDE_DSL_JSON_PARSING) && (INCLUDE_DSL_JSON_PARSING == 1)

typedef struct
{
   /**
      NextMode (DSL) to be used on API startup
      Only of relevance if xDSL multimode is enabled by using VDSL and ADSL
      G997 XTU octet bits.
      NextMode
         0: use API-default value (if A+V modes are selected VDSL will be used)
         1: use ADSL mode as initial one for multimode handling
         2: use VDSL mode as initial one for multimode handling
   */
   unsigned int nNextMode;

   /** MaxDeviceNumber provided by the MEI Driver */
   unsigned int nMaxDeviceNumber;

   /** LinesPerDevice provided by the MEI Driver */
   unsigned int nLinesPerDevice;

   /** ChannelsPerLine provided by the MEI Driver */
   unsigned int nChannelsPerLine;
} DSL_CFG_StartupInit_t;

typedef struct
{
   /** Bitswap config for US in VDSL */
   bool Us_Vdsl;

   /** Bitswap config for DS in VDSL */
   bool Ds_Vdsl;

   /** Bitswap config for US in ADSL */
   bool Us_Adsl;

   /** Bitswap config for DS in ADSL */
   bool Ds_Adsl;
} DSL_CFG_Bitswap_t;

typedef struct
{
   /** Retransmission config for US in ADSL */
   unsigned int Us_Adsl;

   /** Retransmission config for DS in ADSL */
   unsigned int Ds_Adsl;

   /** Retransmission config for US in VDSL */
   unsigned int Us_Vdsl;

   /** Retransmission config for DS in VDSL */
   unsigned int Ds_Vdsl;
} DSL_CFG_Retransmission_t;

typedef struct
{
   /** SRA config for US in ADSL */
   unsigned int Us_Adsl;

   /** SRA config for DS in ADSL */
   unsigned int Ds_Adsl;

   /** SRA config for US in VDSL */
   unsigned int Us_Vdsl;

   /** SRA config for DS in VDSL */
   unsigned int Ds_Vdsl;
} DSL_CFG_Sra_t;

typedef struct
{
   /** Virtual Noise config for US */
   bool Us;

   /** Virtual Noise config for DS */
   bool Ds;
} DSL_CFG_VirtualNoise_t;

typedef struct
{
   /**
      The value selects the activated reboot criteria's according to the
      following bitmask definitions.
      Please note that some values are used only within ADSL or VDSL. For
      details please refer to the UMPR of the DSL CPE API.
      DEFAULT value for ADSL is "4F"
      DEFAULT value for VDSL is "F"
         0: CLEANED (no reboot criteria active)
         1: LOM              - default ON
         2: LOF              - default ON
         4: LOS              - default ON
         8: ESE              - default ON
         10: ES90             - default OFF
         20: SES30            - default OFF
         40: NEGATIVE_MARGIN  - default ON for ADSL / OFF for VDSL
         80: OOS_BC0          - default OFF
         100: OOS_BC1          - default OFF
         200: NCD_BC0          - default OFF
         400: NCD_BC1          - default OFF
         800: LCD_BC0          - default OFF
         1000: LCD_BC1          - default OFF
   */
   unsigned int Adsl;
   unsigned int Vdsl;
} DSL_CFG_RebootCriteria_t;

typedef struct
{
   /**
      The value selects the activated handshake tones according to the
      following bitmask definitions.
      The default values are related to the DSL operation mode. Please refer to
      the UMPR of the DSL CPE API for all details.
         1: VDSL2 B43
         2: VDSL2 A43
         4: VDSL2 V43
         8: VDSL1 V43POTS
         10: VDSL1 V43ISDN
         20: ADSL1 C43
         40: ADSL2 J43
         80: ADSL2 B43C
         100: ADSL2 A43C
   */
   unsigned int Adsl;
   unsigned int Vdsl;
} DSL_CFG_LowLevelHsTones_t;

typedef struct
{
   /**
      The value selects the system interface configuration for ADSL and VDSL modes
      according to the following bitmask definitions:
         0: Use API internal defaults
         1: ATM-TC
         2: EFM/PTM-TC
         4: Auto-TC
   */
   unsigned int nTcLayer_Adsl;
   unsigned int nTcLayer_Vdsl;
} DSL_CFG_SystemInterface_t;

typedef struct
{
   /**
      Configuration of test and debug interface usage
         0: Do not use debug and test interfaces.
               Independent from feature availability.
         1: Use debug and test interfaces on LAN interfaces only. - DEFAULT
               Dependent on feature availability.
         2: Use debug and test interfaces on ALL available interfaces.
               Dependent on feature availability. Former default value.
   */
   unsigned int nDebugAndTestInterfaces;
} DSL_CFG_Debugging_t;

typedef struct
{
   /**
      The message selects a DSL operator.
      The information is used to configure operator specific settings inside the DSL firmware.
   */
   unsigned int nOperatorSelect;

   /**
      Configuration of API shutdown handling in case of using autoboot control
      command "acs <nLine> 7" (DSL_CFG_AUTOBOOT_CTRL_STOP_PD)
         LdAfeShutdown
            0: Do not shutdown the Line Driver and the AFE
                  This option should be used for example in case of underlying
                  ISDN service is used and an impedance change of the line should
                  be avoided
            1: Shutdown the Line Driver and the AFE - DEFAULT
                  This option is selected by default and make use of a specific DSL FW
                  messages (if supported) to power down the LD/AFE
   */
   unsigned int LdAfeShutdown;

   /**
      Configuration of test and debug specific vectoring setting
         VectoringEnable
            0: Disable any vectoring operation
            1: Enables the G.993.5 full G.Vector for US and DS)
            2: Enables the G.993.2 Annex Y (G.Vector friendly)
            3: Enables automatic detection for the best fitting configuration
      Notes:
         - Vectoring is only supported for VDSL
         - There are some additional conditions that needs to be fulfilled for
         successful enabling of the full G.Vector functionality (Annex N)
            + DSL Firmware needs to support vectoring, means that the last digit of the
            firmware version has to be "7"
            + The DSL Subsystem has to be compiled to support vectoring
   */
   unsigned int VectoringEnable;

   /**
      VdslProfileVal
      The value selects the activated Vdsl profile according to the
      following bitmask definitions.
      Please note that 12A/12B/17A/30A are not supported for the Dual port mode.
      DEFAULT value is "0xFF" (single port mode), "0xF" (dual port mode).
         1: 8A
         2: 8B
         4: 8C
         8: 8D
         10: 12A
         20: 12B
         40: 17A
         80: 30A
         100: 35B
   */
   unsigned int VdslProfileVal;

   /**
      Configuration of Non-standard activation sequence
         ActSeq
            0: Automatic detection from XTSE configuration (enables Non-standard
                  activation sequence in case of T1.413 is enabled as well)
            1: Standard Handshake (G.HS)  - DEFAULT
            2: Non-standard activation sequence ("Telefonica switching mode")
   */
   unsigned int ActSeq;

   /**
      Configuration of initial activation mode
         ActMode (only used if "ActMode" equals "2" or
         "ActMode" equals "0" and T1.413 mode is included within XTSE octets)
            1: Start with G.HS activation mode  - DEFAULT
            2: Start with ANSI-T1.413 activation mode
   */
   unsigned int ActMode;

   /**
      Configuration of remember functionality used in case of activated xDSL multimode
         Remember
            0: disable remember functionality
            1: enable remember functionality
   */
   unsigned int Remember;

   /**
      Test and debug configuration to enable event based (autonomous FW message
      handling) mode instead of polling (which is default within API and strictly
      recommended to be used!)
         xDSL_Dbg_FwMsgPollingOnly
         0: Use autonomous FW message handling (instead of polling)
         1: Use polling mode only - DEFAULT
   */
   bool FWMsgPollingOnly;

   DSL_CFG_LowLevelHsTones_t sLowLevelHsTonesConfig;
} DSL_CFG_Common_t;

typedef struct
{
   /** Debugging configuration */
   DSL_CFG_Debugging_t sDebuggingConfig;

   /** StartupInit configuration */
   DSL_CFG_StartupInit_t sStartupInitConfig;

   /** Bitswap configuration */
   DSL_CFG_Bitswap_t sBitswapConfig;

   /** Retransmission configuration */
   DSL_CFG_Retransmission_t sRetransmissionConfig;

   /** Sra configuration */
   DSL_CFG_Sra_t sSraConfig;

   /** VirtualNoise configuration */
   DSL_CFG_VirtualNoise_t sVirtualNoiseConfig;

   /** Reboot criteria configuration */
   DSL_CFG_RebootCriteria_t sRebootCriteriaConfig;

   /** SystemInterface configuration */
   DSL_CFG_SystemInterface_t sSystemIfConfig;

   /** Common configuration */
   DSL_CFG_Common_t sCommonConfig;

   /**
      Configuration of test and debug specific DSL activation mode settings
      G997 XTU octet bits.
         G997XtuVal (only used in case of "G997XtuVal" equals "1")
   */
   unsigned char G997XtuVal[DSL_G997_NUM_XTSE_OCTETS];

} DSL_ConfigurationData_t;

typedef struct
{
   /**
   Structure that contains initialization data */
   DSL_ConfigurationData_t data;
} DSL_Configuration_t;

/*
   This function initialize internal structures of Configuration module.

   \remark Should be called only once and at application startup.

   \return  Return values are defined within the \ref DSL_Error_t definition
   - DSL_SUCCESS in case of success
   - DSL_ERROR if operation failed
*/
DSL_Error_t DSL_CPE_ConfigInit();

/*
   This function provides accesss to current configuration params.

   \param pConfiguration      Pointer to pointer to
                              DSL_Configuration_t structure, [O]

   \return  Return values are defined within the \ref DSL_Error_t definition
   - DSL_SUCCESS in case of success
   - DSL_ERROR if operation failed
*/
DSL_Error_t DSL_CPE_ConfigGet(
   const DSL_Configuration_t **pConfiguration
);

/*
   This function executes readout of configuration file then
   validation and appliance of configuration params.

   \param pControlContext     Pointer to DSL CPE Control context structure, [I]

   \return  Return values are defined within the \ref DSL_Error_t definition
   - DSL_SUCCESS in case of success
   - DSL_ERROR if operation failed
*/
DSL_Error_t DSL_CPE_ConfigUpdate(
   DSL_CPE_Control_Context_t *pCtrlCtx
);

#endif /* defined(INCLUDE_DSL_JSON_PARSING) && (INCLUDE_DSL_JSON_PARSING == 1) */

#endif /* _LIB_DSL_CONFIGURATION_PARSER_H */
