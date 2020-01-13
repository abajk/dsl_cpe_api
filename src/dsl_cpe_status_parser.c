/******************************************************************************

         Copyright (c) 2019 Intel Corporation

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*******************************************************************************/

#if defined(INCLUDE_DSL_JSON_PARSING) && (INCLUDE_DSL_JSON_PARSING == 1)

#include "json.h"
#include "dsl_cpe_status_parser.h"
#include "drv_dsl_cpe_api_ioctl.h"
#include "drv_dsl_cpe_api.h"

#undef DSL_CCA_DBG_BLOCK
#define DSL_CCA_DBG_BLOCK DSL_CCA_DBG_APP

#define STATUS_FILE_PATH_TMP "/tmp/dsl_cpe_control.status.tmp"
#define STATUS_FILE_PATH "/var/dsl_cpe_control.status"

#define SECTION_PATH_LENGTH 256

/*
   Definitions for status file parameters' update types,
   to be used in different update functions in order to
   determine which parameters need to be updated
*/
typedef enum
{
   DSL_UPDATE_TYPE_NONE, /* only for nodes with JSON object type */
   DSL_UPDATE_TYPE_INIT, /* parameters to be updated during initialization */
   DSL_UPDATE_TYPE_POLL, /* parameters to be updated on polling timer */
   DSL_UPDATE_TYPE_EVENT /* parameters to be updated by events only */
} DSL_CPE_StatusNodeUpdateType_t;

/*
   JSON Status File node definition
*/
typedef struct
{
   DSL_CPE_StatusNodeIdx_t eNodeIdx;
   DSL_char_t jsonPath[SECTION_PATH_LENGTH];
   json_type jsonType;
   DSL_CPE_StatusNodeUpdateType_t eUpdateType;
} DSL_CPE_JsonStatusNode_t;

/*
   JSON Status File entire structure definition (metadata),
   to be used for creation of physical Status File as well as
   for further operations
*/
static const DSL_CPE_JsonStatusNode_t jsonStatusNodes[] =
{
   { DSL_ROOT, "dsl",
      json_type_object, DSL_UPDATE_TYPE_NONE },
   /* common section */
   { DSL_LINE_ENTRIES, "dsl.line_number_of_entries",
      json_type_int, DSL_UPDATE_TYPE_INIT },
   { DSL_CHANNEL_ENTRIES, "dsl.channel_number_of_entries",
      json_type_int, DSL_UPDATE_TYPE_INIT },
   { DSL_BND_GROUP_ENTRIES, "dsl.bonding_group_number_of_entries",
      json_type_int, DSL_UPDATE_TYPE_INIT },
   { DSL_API_VERSION, "dsl.api_version",
      json_type_string, DSL_UPDATE_TYPE_INIT },
   { DSL_MEI_VERSION, "dsl.mei_driver_version",
      json_type_string, DSL_UPDATE_TYPE_INIT },
   { DSL_HW_VERSION, "dsl.hardware_version",
      json_type_string, DSL_UPDATE_TYPE_INIT },
   { DSL_CHIPSET_TYPE, "dsl.chipset_type",
      json_type_string, DSL_UPDATE_TYPE_INIT },
   /* <dsl.status> section */
   { DSL_STATUS, "dsl.status",
      json_type_object, DSL_UPDATE_TYPE_NONE },
   { DSL_STATUS_LINE, "dsl.status.line",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      /* <dsl.status.line.0> section */
      { DSL_STATUS_LINE_0, "dsl.status.line.0",
         json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_0_STATUS, "dsl.status.line.0.status",
         json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_UPSTREAM, "dsl.status.line.0.upstream",
         json_type_boolean, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_FW_VERSION, "dsl.status.line.0.firmware_version",
         json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_LINK_STATUS, "dsl.status.line.0.link_status",
         json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_STD_SUPPORTED, "dsl.status.line.0.standards_supported",
         json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_XTSE, "dsl.status.line.0.xtse",
         json_type_array, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_STD_USED, "dsl.status.line.0.standard_used",
         json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_XTSE_USED, "dsl.status.line.0.xtse_used",
         json_type_array, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_ENCODING, "dsl.status.line.0.line_encoding",
         json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_ALLOWED_PROFILES, "dsl.status.line.0.allowed_profiles",
         json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_CURRENT_PROFILE, "dsl.status.line.0.current_profile",
         json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_PWR_MGMT_STATE, "dsl.status.line.0.power_management_state",
         json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_SUCC_FAIL_CAUSE, "dsl.status.line.0.success_failure_cause",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_LAST_STATE_TRANSM, "dsl.status.line.0.last_state_transmitted",
         json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_0_LAST_STATE_TRANSM_US, "dsl.status.line.0.last_state_transmitted.us",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_LAST_STATE_TRANSM_DS, "dsl.status.line.0.last_state_transmitted.ds",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_US0_MASK, "dsl.status.line.0.us0_mask",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_TRELLIS, "dsl.status.line.0.trellis",
         json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_0_TRELLIS_US, "dsl.status.line.0.trellis.us",
         json_type_boolean, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_TRELLIS_DS, "dsl.status.line.0.trellis.ds",
         json_type_boolean, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_ACT_SNR_MODE, "dsl.status.line.0.act_snr_mode",
         json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_0_ACT_SNR_MODE_US, "dsl.status.line.0.act_snr_mode.us",
         json_type_boolean, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_ACT_SNR_MODE_DS, "dsl.status.line.0.act_snr_mode.ds",
         json_type_boolean, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_LINE_NR, "dsl.status.line.0.line_number",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_MAX_BIT_RATE, "dsl.status.line.0.upstream_max_bit_rate",
         json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_0_MAX_BIT_RATE_US, "dsl.status.line.0.upstream_max_bit_rate.us",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_MAX_BIT_RATE_DS, "dsl.status.line.0.upstream_max_bit_rate.ds",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_NOISE_MARGIN, "dsl.status.line.0.upstream_noise_margin",
         json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_0_NOISE_MARGIN_US, "dsl.status.line.0.upstream_noise_margin.us",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_NOISE_MARGIN_DS, "dsl.status.line.0.upstream_noise_margin.ds",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_SNR_MPB, "dsl.status.line.0.snr_mpb",
         json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_0_SNR_MPB_US, "dsl.status.line.0.snr_mpb.us",
         json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_SNR_MPB_DS, "dsl.status.line.0.snr_mpb.ds",
         json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_POWER, "dsl.status.line.0.upstream_power",
         json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_0_POWER_US, "dsl.status.line.0.upstream_power.us",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_POWER_DS, "dsl.status.line.0.upstream_power.ds",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_XTUR_VENDOR, "dsl.status.line.0.xtur_vendor",
         json_type_array, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_XTUR_COUNTRY, "dsl.status.line.0.xtur_country",
         json_type_array, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_XTUR_ANSI_STD, "dsl.status.line.0.xtur_ansi_std",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_XTUR_ANSI_REV, "dsl.status.line.0.xtur_ansi_rev",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_XTUC_VENDOR, "dsl.status.line.0.xtuc_vendor",
         json_type_array, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_XTUC_COUNTRY, "dsl.status.line.0.xtuc_country",
         json_type_array, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_XTUC_ANSI_STD, "dsl.status.line.0.xtuc_ansi_std",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_XTUC_ANSI_REV, "dsl.status.line.0.xtuc_ansi_rev",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_UPBOKLER_PB, "dsl.status.line.0.upbokler_pb",
         json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_RXTHRSH_DS, "dsl.status.line.0.rxthrsh_ds",
         json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_ACT_RA_MODE, "dsl.status.line.0.act_ra_mode",
         json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_0_ACT_RA_MODE_US, "dsl.status.line.0.act_ra_mode.us",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_ACT_RA_MODE_DS, "dsl.status.line.0.act_ra_mode.ds",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_SNR_MROC, "dsl.status.line.0.snr_mroc",
         json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_0_SNR_MROC_US, "dsl.status.line.0.snr_mroc.us",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_SNR_MROC_DS, "dsl.status.line.0.snr_mroc.ds",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_ATTENUATION, "dsl.status.line.0.attenuation",
         json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_0_ATTENUATION_US, "dsl.status.line.0.attenuation.us",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_0_ATTENUATION_DS, "dsl.status.line.0.attenuation.ds",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         /* <dsl.status.line.0.channel> section */
         { DSL_STATUS_LINE_0_CHANNEL, "dsl.status.line.0.channel",
            json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATUS_LINE_0_CHANNEL_STATUS, "dsl.status.line.0.channel.status",
            json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_0_CHANNEL_LINKENC_SUPP, "dsl.status.line.0.channel.link_encapsulation_supported",
            json_type_string, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_0_CHANNEL_LINKENC_USED, "dsl.status.line.0.channel.link_encapsulation_used",
            json_type_string, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_0_CHANNEL_LPATH, "dsl.status.line.0.channel.lpath",
            json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_0_CHANNEL_INTLVDEPTH, "dsl.status.line.0.channel.intlvdepth",
            json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_0_CHANNEL_INTLVBLOCK, "dsl.status.line.0.channel.intlvblock",
            json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_0_CHANNEL_ACT_INTERL_DELAY, "dsl.status.line.0.channel.actual_interleaving_delay",
            json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_0_CHANNEL_ACTINP, "dsl.status.line.0.channel.actinp",
            json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_0_CHANNEL_INPREPORT, "dsl.status.line.0.channel.inpreport",
            json_type_boolean, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_0_CHANNEL_NFEC, "dsl.status.line.0.channel.nfec",
            json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_0_CHANNEL_RFEC, "dsl.status.line.0.channel.rfec",
            json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_0_CHANNEL_LSYMB, "dsl.status.line.0.channel.lsymb",
            json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_0_CHANNEL_DATARATE, "dsl.status.line.0.channel.curr_rate",
            json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATUS_LINE_0_CHANNEL_DATARATE_US, "dsl.status.line.0.channel.curr_rate.us",
            json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_0_CHANNEL_DATARATE_DS, "dsl.status.line.0.channel.curr_rate.ds",
            json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_0_CHANNEL_ACTNDR, "dsl.status.line.0.channel.actndr",
            json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATUS_LINE_0_CHANNEL_ACTNDR_US, "dsl.status.line.0.channel.actndr.us",
            json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_0_CHANNEL_ACTNDR_DS, "dsl.status.line.0.channel.actndr.ds",
            json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_0_CHANNEL_ACTINPREIN, "dsl.status.line.0.channel.actinprein",
            json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATUS_LINE_0_CHANNEL_ACTINPREIN_US, "dsl.status.line.0.channel.actinprein.us",
            json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_0_CHANNEL_ACTINPREIN_DS, "dsl.status.line.0.channel.actinprein.ds",
            json_type_int, DSL_UPDATE_TYPE_POLL },
      /* <dsl.status.line.1> section */
      { DSL_STATUS_LINE_1, "dsl.status.line.1",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_1_STATUS, "dsl.status.line.1.status",
      json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_UPSTREAM, "dsl.status.line.1.upstream",
      json_type_boolean, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_FW_VERSION, "dsl.status.line.1.firmware_version",
      json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_LINK_STATUS, "dsl.status.line.1.link_status",
      json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_STD_SUPPORTED, "dsl.status.line.1.standards_supported",
      json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_XTSE, "dsl.status.line.1.xtse",
      json_type_array, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_STD_USED, "dsl.status.line.1.standard_used",
      json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_XTSE_USED, "dsl.status.line.1.xtse_used",
      json_type_array, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_ENCODING, "dsl.status.line.1.line_encoding",
      json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_ALLOWED_PROFILES, "dsl.status.line.1.allowed_profiles",
      json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_CURRENT_PROFILE, "dsl.status.line.1.current_profile",
      json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_PWR_MGMT_STATE, "dsl.status.line.1.power_management_state",
      json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_SUCC_FAIL_CAUSE, "dsl.status.line.1.success_failure_cause",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_LAST_STATE_TRANSM, "dsl.status.line.1.last_state_transmitted",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_1_LAST_STATE_TRANSM_US, "dsl.status.line.1.last_state_transmitted.us",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_LAST_STATE_TRANSM_DS, "dsl.status.line.1.last_state_transmitted.ds",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_US0_MASK, "dsl.status.line.1.us0_mask",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_TRELLIS, "dsl.status.line.1.trellis",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_1_TRELLIS_US, "dsl.status.line.1.trellis.us",
      json_type_boolean, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_TRELLIS_DS, "dsl.status.line.1.trellis.ds",
      json_type_boolean, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_ACT_SNR_MODE, "dsl.status.line.1.act_snr_mode",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_1_ACT_SNR_MODE_US, "dsl.status.line.1.act_snr_mode.us",
      json_type_boolean, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_ACT_SNR_MODE_DS, "dsl.status.line.1.act_snr_mode.ds",
      json_type_boolean, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_LINE_NR, "dsl.status.line.1.line_number",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_MAX_BIT_RATE, "dsl.status.line.1.upstream_max_bit_rate",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_1_MAX_BIT_RATE_US, "dsl.status.line.1.upstream_max_bit_rate.us",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_MAX_BIT_RATE_DS, "dsl.status.line.1.upstream_max_bit_rate.ds",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_NOISE_MARGIN, "dsl.status.line.1.upstream_noise_margin",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_1_NOISE_MARGIN_US, "dsl.status.line.1.upstream_noise_margin.us",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_NOISE_MARGIN_DS, "dsl.status.line.1.upstream_noise_margin.ds",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_SNR_MPB, "dsl.status.line.1.snr_mpb",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_1_SNR_MPB_US, "dsl.status.line.1.snr_mpb.us",
      json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_SNR_MPB_DS, "dsl.status.line.1.snr_mpb.ds",
      json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_POWER, "dsl.status.line.1.upstream_power",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_1_POWER_US, "dsl.status.line.1.upstream_power.us",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_POWER_DS, "dsl.status.line.1.upstream_power.ds",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_XTUR_VENDOR, "dsl.status.line.1.xtur_vendor",
      json_type_array, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_XTUR_COUNTRY, "dsl.status.line.1.xtur_country",
      json_type_array, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_XTUR_ANSI_STD, "dsl.status.line.1.xtur_ansi_std",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_XTUR_ANSI_REV, "dsl.status.line.1.xtur_ansi_rev",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_XTUC_VENDOR, "dsl.status.line.1.xtuc_vendor",
      json_type_array, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_XTUC_COUNTRY, "dsl.status.line.1.xtuc_country",
      json_type_array, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_XTUC_ANSI_STD, "dsl.status.line.1.xtuc_ansi_std",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_XTUC_ANSI_REV, "dsl.status.line.1.xtuc_ansi_rev",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_UPBOKLER_PB, "dsl.status.line.1.upbokler_pb",
      json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_RXTHRSH_DS, "dsl.status.line.1.rxthrsh_ds",
      json_type_string, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_ACT_RA_MODE, "dsl.status.line.1.act_ra_mode",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_1_ACT_RA_MODE_US, "dsl.status.line.1.act_ra_mode.us",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_ACT_RA_MODE_DS, "dsl.status.line.1.act_ra_mode.ds",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_SNR_MROC, "dsl.status.line.1.snr_mroc",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_1_SNR_MROC_US, "dsl.status.line.1.snr_mroc.us",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_SNR_MROC_DS, "dsl.status.line.1.snr_mroc.ds",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_ATTENUATION, "dsl.status.line.1.attenuation",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATUS_LINE_1_ATTENUATION_US, "dsl.status.line.1.attenuation.us",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATUS_LINE_1_ATTENUATION_DS, "dsl.status.line.1.attenuation.ds",
      json_type_int, DSL_UPDATE_TYPE_POLL },
         /* <dsl.status.line.1.channel> section */
         { DSL_STATUS_LINE_1_CHANNEL, "dsl.status.line.1.channel",
         json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATUS_LINE_1_CHANNEL_STATUS, "dsl.status.line.1.channel.status",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_1_CHANNEL_LINKENC_SUPP, "dsl.status.line.1.channel.link_encapsulation_supported",
         json_type_string, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_1_CHANNEL_LINKENC_USED, "dsl.status.line.1.channel.link_encapsulation_used",
         json_type_string, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_1_CHANNEL_LPATH, "dsl.status.line.1.channel.lpath",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_1_CHANNEL_INTLVDEPTH, "dsl.status.line.1.channel.intlvdepth",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_1_CHANNEL_INTLVBLOCK, "dsl.status.line.1.channel.intlvblock",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_1_CHANNEL_ACT_INTERL_DELAY, "dsl.status.line.1.channel.actual_interleaving_delay",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_1_CHANNEL_ACTINP, "dsl.status.line.1.channel.actinp",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_1_CHANNEL_INPREPORT, "dsl.status.line.1.channel.inpreport",
         json_type_boolean, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_1_CHANNEL_NFEC, "dsl.status.line.1.channel.nfec",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_1_CHANNEL_RFEC, "dsl.status.line.1.channel.rfec",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_1_CHANNEL_LSYMB, "dsl.status.line.1.channel.lsymb",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_1_CHANNEL_DATARATE, "dsl.status.line.1.channel.curr_rate",
         json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATUS_LINE_1_CHANNEL_DATARATE_US, "dsl.status.line.1.channel.curr_rate.us",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_1_CHANNEL_DATARATE_DS, "dsl.status.line.1.channel.curr_rate.ds",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_1_CHANNEL_ACTNDR, "dsl.status.line.1.channel.actndr",
         json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATUS_LINE_1_CHANNEL_ACTNDR_US, "dsl.status.line.1.channel.actndr.us",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_1_CHANNEL_ACTNDR_DS, "dsl.status.line.1.channel.actndr.ds",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_1_CHANNEL_ACTINPREIN, "dsl.status.line.1.channel.actinprein",
         json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATUS_LINE_1_CHANNEL_ACTINPREIN_US, "dsl.status.line.1.channel.actinprein.us",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATUS_LINE_1_CHANNEL_ACTINPREIN_DS, "dsl.status.line.1.channel.actinprein.ds",
         json_type_int, DSL_UPDATE_TYPE_POLL },
   { DSL_STATS, "dsl.stats",
   json_type_object, DSL_UPDATE_TYPE_NONE },
   { DSL_STATS_LINE, "dsl.stats.line",
   json_type_object, DSL_UPDATE_TYPE_NONE },
      /* <dsl.stats.line.0> section */
      { DSL_STATS_LINE_0, "dsl.stats.line.0",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATS_LINE_0_TOTAL_START, "dsl.stats.line.0.total_start",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_0_SHOWTIME_START, "dsl.stats.line.0.showtime_start",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_0_LAST_SHOWTIME_START, "dsl.stats.line.0.last_showtime_start",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_0_CURRENT_DAY_START, "dsl.stats.line.0.current_day_start",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_0_QUARTER_HOUR_START, "dsl.stats.line.0.quarter_hour_start",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_0_TOTAL, "dsl.stats.line.0.total",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATS_LINE_0_TOTAL_ERR_SECS, "dsl.stats.line.0.total.errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_0_TOTAL_SEV_ERR_SECS, "dsl.stats.line.0.total.severely_errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_0_SHOWTIME, "dsl.stats.line.0.showtime",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATS_LINE_0_SHOWTIME_ERR_SECS, "dsl.stats.line.0.showtime.errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_0_SHOWTIME_SEV_ERR_SECS, "dsl.stats.line.0.showtime.severely_errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_0_LAST_SHOWTIME, "dsl.stats.line.0.last_showtime",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATS_LINE_0_LAST_SHOWTIME_ERR_SECS, "dsl.stats.line.0.last_showtime.errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_0_LAST_SHOWTIME_SEV_ERR_SECS, "dsl.stats.line.0.last_showtime.severely_errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_0_CURRENT_DAY, "dsl.stats.line.0.current_day",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATS_LINE_0_CURRENT_DAY_ERR_SECS, "dsl.stats.line.0.current_day.errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_0_CURRENT_DAY_SEV_ERR_SECS, "dsl.stats.line.0.current_day.severely_errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_0_QUARTER_HOUR, "dsl.stats.line.0.quarter_hour",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATS_LINE_0_QUARTER_HOUR_ERR_SECS, "dsl.stats.line.0.quarter_hour.errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_0_QUARTER_HOUR_SEV_ERR_SECS, "dsl.stats.line.0.quarter_hour.severely_errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
         /* <dsl.stats.line.0.channel> section */
         { DSL_STATS_LINE_0_CHANNEL, "dsl.stats.line.0.channel",
         json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATS_LINE_0_CHANNEL_TOTAL_START, "dsl.stats.line.0.channel.total_start",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_SHOWTIME_START, "dsl.stats.line.0.channel.showtime_start",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_START, "dsl.stats.line.0.channel.last_showtime_start",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_START, "dsl.stats.line.0.channel.current_day_start",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_START, "dsl.stats.line.0.channel.quarter_hour_start",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_TOTAL, "dsl.stats.line.0.channel.total",
         json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATS_LINE_0_CHANNEL_TOTAL_XTUR_FEC_ERRORS, "dsl.stats.line.0.channel.total.xtur_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_TOTAL_XTUR_CRC_ERRORS, "dsl.stats.line.0.channel.total.xtur_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_TOTAL_XTUC_FEC_ERRORS, "dsl.stats.line.0.channel.total.xtuc_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_TOTAL_XTUC_CRC_ERRORS, "dsl.stats.line.0.channel.total.xtuc_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_TOTAL_XTUR_HEC_ERRORS, "dsl.stats.line.0.channel.total.xtur_hex_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_TOTAL_XTUC_HEC_ERRORS, "dsl.stats.line.0.channel.total.xtuc_hex_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_SHOWTIME, "dsl.stats.line.0.channel.showtime",
         json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATS_LINE_0_CHANNEL_SHOWTIME_XTUR_FEC_ERRORS, "dsl.stats.line.0.channel.showtime.xtur_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_SHOWTIME_XTUR_CRC_ERRORS, "dsl.stats.line.0.channel.showtime.xtur_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_SHOWTIME_XTUC_FEC_ERRORS, "dsl.stats.line.0.channel.showtime.xtuc_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_SHOWTIME_XTUC_CRC_ERRORS, "dsl.stats.line.0.channel.showtime.xtuc_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_SHOWTIME_XTUR_HEC_ERRORS, "dsl.stats.line.0.channel.showtime.xtur_hec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_SHOWTIME_XTUC_HEC_ERRORS, "dsl.stats.line.0.channel.showtime.xtuc_hec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME, "dsl.stats.line.0.channel.last_showtime",
         json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_XTUR_FEC_ERRORS, "dsl.stats.line.0.channel.last_showtime.xtur_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_XTUR_CRC_ERRORS, "dsl.stats.line.0.channel.last_showtime.xtur_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_XTUC_FEC_ERRORS, "dsl.stats.line.0.channel.last_showtime.xtuc_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_XTUC_CRC_ERRORS, "dsl.stats.line.0.channel.last_showtime.xtuc_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_XTUR_HEC_ERRORS, "dsl.stats.line.0.channel.last_showtime.xtur_hec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_XTUC_HEC_ERRORS, "dsl.stats.line.0.channel.last_showtime.xtuc_hec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY, "dsl.stats.line.0.channel.current_day",
         json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_XTUR_FEC_ERRORS, "dsl.stats.line.0.channel.current_day.xtur_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_XTUR_CRC_ERRORS, "dsl.stats.line.0.channel.current_day.xtur_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_XTUC_FEC_ERRORS, "dsl.stats.line.0.channel.current_day.xtuc_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_XTUC_CRC_ERRORS, "dsl.stats.line.0.channel.current_day.xtuc_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_XTUR_HEC_ERRORS, "dsl.stats.line.0.channel.current_day.xtur_hec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_XTUC_HEC_ERRORS, "dsl.stats.line.0.channel.current_day.xtuc_hec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR, "dsl.stats.line.0.channel.quarter_hour",
         json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_XTUR_FEC_ERRORS, "dsl.stats.line.0.channel.quarter_hour.xtur_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_XTUR_CRC_ERRORS, "dsl.stats.line.0.channel.quarter_hour.xtur_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_XTUC_FEC_ERRORS, "dsl.stats.line.0.channel.quarter_hour.xtuc_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_XTUC_CRC_ERRORS, "dsl.stats.line.0.channel.quarter_hour.xtuc_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_XTUR_HEC_ERRORS, "dsl.stats.line.0.channel.quarter_hour.xtur_hec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_XTUC_HEC_ERRORS, "dsl.stats.line.0.channel.quarter_hour.xtuc_hec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
      /* <dsl.stats.line.1> section */
      { DSL_STATS_LINE_1, "dsl.stats.line.1",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATS_LINE_1_TOTAL_START, "dsl.stats.line.1.total_start",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_1_SHOWTIME_START, "dsl.stats.line.1.showtime_start",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_1_LAST_SHOWTIME_START, "dsl.stats.line.1.last_showtime_start",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_1_CURRENT_DAY_START, "dsl.stats.line.1.current_day_start",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_1_QUARTER_HOUR_START, "dsl.stats.line.1.quarter_hour_start",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_1_TOTAL, "dsl.stats.line.1.total",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATS_LINE_1_TOTAL_ERR_SECS, "dsl.stats.line.1.total.errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_1_TOTAL_SEV_ERR_SECS, "dsl.stats.line.1.total.severely_errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_1_SHOWTIME, "dsl.stats.line.1.showtime",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATS_LINE_1_SHOWTIME_ERR_SECS, "dsl.stats.line.1.showtime.errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_1_SHOWTIME_SEV_ERR_SECS, "dsl.stats.line.1.showtime.severely_errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_1_LAST_SHOWTIME, "dsl.stats.line.1.last_showtime",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATS_LINE_1_LAST_SHOWTIME_ERR_SECS, "dsl.stats.line.1.last_showtime.errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_1_LAST_SHOWTIME_SEV_ERR_SECS, "dsl.stats.line.1.last_showtime.severely_errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_1_CURRENT_DAY, "dsl.stats.line.1.current_day",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATS_LINE_1_CURRENT_DAY_ERR_SECS, "dsl.stats.line.1.current_day.errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_1_CURRENT_DAY_SEV_ERR_SECS, "dsl.stats.line.1.current_day.severely_errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_1_QUARTER_HOUR, "dsl.stats.line.1.quarter_hour",
      json_type_object, DSL_UPDATE_TYPE_NONE },
      { DSL_STATS_LINE_1_QUARTER_HOUR_ERR_SECS, "dsl.stats.line.1.quarter_hour.errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
      { DSL_STATS_LINE_1_QUARTER_HOUR_SEV_ERR_SECS, "dsl.stats.line.1.quarter_hour.severely_errored_secs",
      json_type_int, DSL_UPDATE_TYPE_POLL },
         /* <dsl.stats.line.1.channel> section */
         { DSL_STATS_LINE_1_CHANNEL, "dsl.stats.line.1.channel",
         json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATS_LINE_1_CHANNEL_TOTAL_START, "dsl.stats.line.1.channel.total_start",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_SHOWTIME_START, "dsl.stats.line.1.channel.showtime_start",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_START, "dsl.stats.line.1.channel.last_showtime_start",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_START, "dsl.stats.line.1.channel.current_day_start",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_START, "dsl.stats.line.1.channel.quarter_hour_start",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_TOTAL, "dsl.stats.line.1.channel.total",
         json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATS_LINE_1_CHANNEL_TOTAL_XTUR_FEC_ERRORS, "dsl.stats.line.1.channel.total.xtur_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_TOTAL_XTUR_CRC_ERRORS, "dsl.stats.line.1.channel.total.xtur_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_TOTAL_XTUC_FEC_ERRORS, "dsl.stats.line.1.channel.total.xtuc_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_TOTAL_XTUC_CRC_ERRORS, "dsl.stats.line.1.channel.total.xtuc_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_TOTAL_XTUR_HEC_ERRORS, "dsl.stats.line.1.channel.total.xtur_hec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_TOTAL_XTUC_HEC_ERRORS, "dsl.stats.line.1.channel.total.xtuc_hec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_SHOWTIME, "dsl.stats.line.1.channel.showtime",
         json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATS_LINE_1_CHANNEL_SHOWTIME_XTUR_FEC_ERRORS, "dsl.stats.line.1.channel.showtime.xtur_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_SHOWTIME_XTUR_CRC_ERRORS, "dsl.stats.line.1.channel.showtime.xtur_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_SHOWTIME_XTUC_FEC_ERRORS, "dsl.stats.line.1.channel.showtime.xtuc_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_SHOWTIME_XTUC_CRC_ERRORS, "dsl.stats.line.1.channel.showtime.xtuc_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_SHOWTIME_XTUR_HEC_ERRORS, "dsl.stats.line.1.channel.showtime.xtur_hec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_SHOWTIME_XTUC_HEC_ERRORS, "dsl.stats.line.1.channel.showtime.xtuc_hec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME, "dsl.stats.line.1.channel.last_showtime",
         json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_XTUR_FEC_ERRORS, "dsl.stats.line.1.channel.last_showtime.xtur_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_XTUR_CRC_ERRORS, "dsl.stats.line.1.channel.last_showtime.xtur_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_XTUC_FEC_ERRORS, "dsl.stats.line.1.channel.last_showtime.xtuc_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_XTUC_CRC_ERRORS, "dsl.stats.line.1.channel.last_showtime.xtuc_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_XTUR_HEC_ERRORS, "dsl.stats.line.1.channel.last_showtime.xtur_hec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_XTUC_HEC_ERRORS, "dsl.stats.line.1.channel.last_showtime.xtuc_hec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY, "dsl.stats.line.1.channel.current_day",
         json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_XTUR_FEC_ERRORS, "dsl.stats.line.1.channel.current_day.xtur_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_XTUR_CRC_ERRORS, "dsl.stats.line.1.channel.current_day.xtur_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_XTUC_FEC_ERRORS, "dsl.stats.line.1.channel.current_day.xtuc_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_XTUC_CRC_ERRORS, "dsl.stats.line.1.channel.current_day.xtuc_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_XTUR_HEC_ERRORS, "dsl.stats.line.1.channel.current_day.xtur_hec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_XTUC_HEC_ERRORS, "dsl.stats.line.1.channel.current_day.xtuc_hec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR, "dsl.stats.line.1.channel.quarter_hour",
         json_type_object, DSL_UPDATE_TYPE_NONE },
         { DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_XTUR_FEC_ERRORS, "dsl.stats.line.1.channel.quarter_hour.xtur_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_XTUR_CRC_ERRORS, "dsl.stats.line.1.channel.quarter_hour.xtur_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_XTUC_FEC_ERRORS, "dsl.stats.line.1.channel.quarter_hour.xtuc_fec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_XTUC_CRC_ERRORS, "dsl.stats.line.1.channel.quarter_hour.xtuc_crc_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_XTUR_HEC_ERRORS, "dsl.stats.line.1.channel.quarter_hour.xtur_hec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL },
         { DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_XTUC_HEC_ERRORS, "dsl.stats.line.1.channel.quarter_hour.xtuc_hec_errors",
         json_type_int, DSL_UPDATE_TYPE_POLL }
};
static const DSL_uint32_t nNodesNum =
   sizeof(jsonStatusNodes) / sizeof(jsonStatusNodes[0]);

/*
   main buffer to keep the status file in a json format
   (this buffer will be kept for whole parser lifetime
   in order to be updated and written down to the
   destination file only in case of need - with this
   approach no status file read operation is needed)
*/
static json_object *pJsonStatusRootObj = DSL_NULL;

/*
   This separator has to be defined as an array, because
   it is used differently by some JSON functions
*/
static const char NODES_SEPARATOR[] = ".";

/*
   Ioctl flag struct declaration
   (needed to avoid redundand IOCTL sending
   for grouped parameters)
*/
typedef struct {
   const DSL_int_t nIoctl;
   DSL_boolean_t bIsSet;
} DSL_CPE_IoctlFlag_t;

/*
   Ioctl flags definitions (separate flag for each IOCTL
   that provides information for more then one parameter),
   initially set to FALSE
*/
static DSL_CPE_IoctlFlag_t ioctlFlags[] =
{
   { DSL_FIO_LINE_STATE_GET, DSL_FALSE},
   { DSL_FIO_G997_XTU_SYSTEM_ENABLING_CONFIG_GET, DSL_FALSE },
   { DSL_FIO_G997_XTU_SYSTEM_ENABLING_STATUS_GET, DSL_FALSE },
   { DSL_FIO_LINE_FEATURE_STATUS_GET, DSL_FALSE },
   { DSL_FIO_G997_LINE_STATUS_GET, DSL_FALSE },
   { DSL_FIO_G997_LINE_INVENTORY_GET, DSL_FALSE },
   { DSL_FIO_PM_CHANNEL_COUNTERS_TOTAL_GET, DSL_FALSE },
   { DSL_FIO_PM_CHANNEL_COUNTERS_SHOWTIME_GET, DSL_FALSE},
   { DSL_FIO_PM_CHANNEL_COUNTERS_15MIN_GET, DSL_FALSE},
   { DSL_FIO_PM_LINE_SEC_COUNTERS_TOTAL_GET, DSL_FALSE },
   { DSL_FIO_PM_LINE_SEC_COUNTERS_SHOWTIME_GET, DSL_FALSE },
   { DSL_FIO_PM_LINE_SEC_COUNTERS_1DAY_GET, DSL_FALSE },
   { DSL_FIO_PM_LINE_SEC_COUNTERS_15MIN_GET, DSL_FALSE },
   { DSL_FIO_SYSTEM_INTERFACE_CONFIG_GET, DSL_FALSE },
   { DSL_FIO_G997_FRAMING_PARAMETER_STATUS_GET, DSL_FALSE },
   { DSL_FIO_G997_CHANNEL_STATUS_GET, DSL_FALSE },
   { DSL_FIO_PM_DATA_PATH_COUNTERS_TOTAL_GET, DSL_FALSE },
   { DSL_FIO_PM_DATA_PATH_COUNTERS_SHOWTIME_GET, DSL_FALSE },
   { DSL_FIO_PM_CHANNEL_COUNTERS_1DAY_GET, DSL_FALSE },
   { DSL_FIO_PM_DATA_PATH_COUNTERS_1DAY_GET, DSL_FALSE },
   { DSL_FIO_PM_DATA_PATH_COUNTERS_15MIN_GET, DSL_FALSE }
};
static const DSL_uint32_t nFlagsNum =
   sizeof(ioctlFlags) / sizeof(ioctlFlags[0]);

/*
   Standards struct declaration for standard parameters
*/
typedef struct {
   DSL_uint8_t nOctet;
   DSL_uint8_t nBit;
   DSL_char_t *sName;
} DSL_CPE_Standarts_t;

/*
   Available standards definition
*/
static const DSL_CPE_Standarts_t standartsTable[] = {
   { 0, XTSE_1_01_A_T1_413, ",T1.413" },
   { 0, XTSE_1_02_C_TS_101388, ",ETSI_101_388" },
   { 0, XTSE_1_03_A_1_NO | XTSE_1_04_A_1_O, ",G.992.1_Annex_A" },
   { 0, XTSE_1_05_B_1_NO | XTSE_1_06_B_1_O, ",G.992.1_Annex_B" },
   { 0, XTSE_1_07_C_1_NO | XTSE_1_08_C_1_O, ",G.992.1_Annex_C" },

   { 1, XTSE_2_01_A_2_NO, ",G.992.2_Annex_A" },
   { 1, XTSE_2_02_B_2_O, ",G.992.2_Annex_B" },
   { 1, XTSE_2_03_C_2_NO | XTSE_2_04_C_2_O, ",G.992.2_Annex_C" },

   { 2, XTSE_3_03_A_3_NO | XTSE_3_04_A_3_O, ",G.992.3_Annex_A" },
   { 2, XTSE_3_05_B_3_NO | XTSE_3_06_B_3_O, ",G.992.3_Annex_B" },

   { 3, XTSE_4_01_A_4_NO | XTSE_4_02_A_4_O, ",G.992.4_Annex_A" },
   { 3, XTSE_4_05_I_3_NO | XTSE_4_06_I_3_O, ",G.992.3_Annex_I" },
   { 3, XTSE_4_07_J_3_NO | XTSE_4_08_J_3_O, ",G.992.3_Annex_J" },

   { 4, XTSE_5_01_I_4_NO | XTSE_5_02_I_4_O, ",G.992.4_Annex_I" },
   { 4, XTSE_5_03_L_3_NO | XTSE_5_04_L_3_NO, ",G.992.3_Annex_L" },
   { 4, XTSE_5_05_L_3_O | XTSE_5_06_L_3_O, ",G.992.3_Annex_L" },
   { 4, XTSE_5_07_M_3_NO | XTSE_5_08_M_3_O, ",G.992.3_Annex_M" },

   { 5, XTSE_6_01_A_5_NO | XTSE_6_02_A_5_O, ",G.992.5_Annex_A" },
   { 5, XTSE_6_03_B_5_NO | XTSE_6_04_B_5_O, ",G.992.5_Annex_B" },
   { 5, XTSE_6_07_I_5_NO | XTSE_6_08_I_5_O, ",G.992.5_Annex_I" },

   { 6, XTSE_7_01_J_5_NO | XTSE_7_02_J_5_O, ",G.992.5_Annex_J" },
   { 6, XTSE_7_03_M_5_NO | XTSE_7_04_M_5_O, ",G.992.5_Annex_M" },

   { 7, XTSE_8_01_A, ",G.993.2_Annex_A" },
   { 7, XTSE_8_02_B, ",G.993.2_Annex_B" },
   { 7, XTSE_8_03_C, ",G.993.2_Annex_C" }
};
static const DSL_uint32_t nStandardsNum =
   sizeof(standartsTable) / sizeof(standartsTable[0]);

/*
   Profiles definitions
*/
static const DSL_char_t *profilesTable[] =
   { ",8a", ",8b", ",8c", ",8d", ",12a", ",12b", ",17a", ",30a", ",35b" };

/*
   Definitions for vendor and country id
*/
#define MAX_LEN_VENDOR_ID 4
#define MAX_LEN_COUNTRY_ID 2

/* Definitions for PM counters */
#define DSL_PM_HISTORY_INTERVALS_NUM 2

/**********************************************/
/* start local function declarations          */
/**********************************************/

static DSL_void_t DSL_CPE_JsonStatusFileCreate();

static json_object* DSL_CPE_JsonNodeParentGet(
   json_object *pRoot,
   const DSL_char_t *pSectionPath
);

static const DSL_char_t* DSL_CPE_JsonNodePathGet(
   const DSL_CPE_StatusNodeIdx_t eNodeIdx
);

static const DSL_char_t* DSL_CPE_JsonNodeNameGet(
   const DSL_char_t* jsonPath
);

static DSL_boolean_t DSL_CPE_IsWithinSection(
   const DSL_char_t* jsonPath,
   const DSL_char_t* jsonSectionPath
);

static DSL_Error_t DSL_CPE_JsonInitParamsUpdate(
   DSL_CPE_Control_Context_t *pContext
);

static DSL_Error_t DSL_CPE_JsonEvtParamsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx
);

static DSL_Error_t DSL_CPE_JsonPollParamsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eSectionIdx
);

static DSL_Error_t DSL_CPE_JsonParamsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx
);

static DSL_Error_t DSL_CPE_JsonStatusFileWrite(
);

static DSL_void_t DSL_CPE_JsonLineNrOfEntriesUpdate(
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonChannelNrOfEntriesUpdate(
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonBndGroupNrOfEntriesUpdate(
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonApiVersionUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonMeiVersionUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonHwVersionUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonChipsetTypeUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonLineStatusUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonUpstreamUpdate(
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonFwVersionUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonLinkStatusUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonStdSupportedUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonXtseConfigUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonStdUsedUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonXtseStatusUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonLineEncodingUpdate(
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonAllowedProfilesUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonCurrentProfileUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonPowerMgmtStateUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonSuccFailCauseUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonLastStateTransmUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_AccessDir_t nDirection,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonUs0MaskUpdate(
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonTrellisUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonActSnrModeUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonLineNrUpdate(
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonMaxBitRateUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonNoiseMarginUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonPowerUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonAttenuationUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonSnrMpbUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_AccessDir_t nDirection,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonXturVendorUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonXturCountryUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonXtucVendorUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonXtucCountryUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonXturAnsiStdUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonXturAnsiRevUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonXtucAnsiStdUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonXtucAnsiRevUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonActRaModeUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_AccessDir_t nDirection,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonUpboklerPbUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonRxthrshDsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonSnrMrocUpdate(
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonTotalStartUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonShowtimeStartUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonLastShowtimeStartUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonCurrDayStartUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonQuarterHourStartUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonTotalErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonTotalSevErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonShowtimeErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonShowtimeSevErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonLastShowtimeErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonLastShowtimeSevErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonCurrDayErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonCurrDaySevErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonQuarterHourErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonQuarterHourSevErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonChannelDataRatesUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_AccessDir_t nDirection,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonChannelStatusUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonChannelLinkEncapSupportedUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonChannelLinkEncapUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonChannelFramingUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonChannelActStatusUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_AccessDir_t nAccessDir,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonChannelActINPReportUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonChannelCountersTotalUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nXTUDir,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonChannelDataPathCountersTotalUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nXTUDir,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonChannelCountersShowtimeUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nXTUDir,
   const DSL_uint32_t nHisotryInterval,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonChannelDataPathCountersShowtimeUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nXTUDir,
   const DSL_uint32_t nHistoryInterval,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonChannelCounters1DayUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nXTUDir,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonChannelDataPathCounters1DayUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nXTUDir,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonChannelCounters15MinUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nXTUDir,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath
);

static DSL_void_t DSL_CPE_JsonChannelDataPathCounters15MinUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nXTUDir,
   const DSL_char_t* jsonPath
);

/**********************************************/
/* start global function definitions          */
/**********************************************/

/*
   For a detailed description of the function, please refer to the
   description in the header file 'drv_cpe_status_parser.h'
*/
DSL_Error_t DSL_CPE_StatusFileInitialize(
   DSL_CPE_Control_Context_t *pContext)
{
   DSL_Error_t nErrorCode = DSL_SUCCESS;

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "IN - DSL_CPE_StatusFileInitialize" DSL_CPE_CRLF));

   DSL_CPE_JsonStatusFileCreate();

   nErrorCode = DSL_CPE_JsonInitParamsUpdate(pContext);

   if (nErrorCode == DSL_SUCCESS)
   {
      nErrorCode = DSL_CPE_JsonStatusFileWrite();
   }

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "OUT - DSL_CPE_StatusFileInitialize, retCode=%d"
      DSL_CPE_CRLF, nErrorCode));

   return nErrorCode;
}

/*
   For a detailed description of the function, please refer to the
   description in the header file 'drv_cpe_status_parser.h'
*/
DSL_Error_t DSL_CPE_StatusFileEvtUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx)
{
   DSL_Error_t nErrorCode = DSL_SUCCESS;

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "IN - DSL_CPE_StatusFileEvtUpdate, node=%d"
      DSL_CPE_CRLF, (DSL_int_t)eNodeIdx));

   nErrorCode = DSL_CPE_JsonEvtParamsUpdate(
      pContext, eNodeIdx);

   if (nErrorCode == DSL_SUCCESS)
   {
      nErrorCode = DSL_CPE_JsonStatusFileWrite();
   }

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "OUT - DSL_CPE_StatusFileEvtUpdate, retCode=%d"
      DSL_CPE_CRLF, nErrorCode));

   return nErrorCode;
}

/*
   For a detailed description of the function, please refer to the
   description in the header file 'drv_cpe_status_parser.h'
*/
DSL_Error_t DSL_CPE_StatusFilePollUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t ePollSectionIdx)
{
   DSL_Error_t nErrorCode = DSL_SUCCESS;

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "IN - DSL_CPE_StatusFilePollUpdate, section=%d"
      DSL_CPE_CRLF, (DSL_int_t)ePollSectionIdx));

   nErrorCode = DSL_CPE_JsonPollParamsUpdate(
      pContext, ePollSectionIdx);

   if (nErrorCode == DSL_SUCCESS)
   {
      nErrorCode = DSL_CPE_JsonStatusFileWrite();
   }

   DSL_CCA_DEBUG(DSL_CCA_DBG_MSG, (DSL_CPE_PREFIX
      "OUT - DSL_CPE_StatusFilePollUpdate, retCode=%d"
      DSL_CPE_CRLF, nErrorCode));

   return nErrorCode;
}

/*********************************************/
/* start local function definitions          */
/*********************************************/

/*
   This function returns ioctl flag
*/
static DSL_Error_t DSL_CPE_IoctlFlagGet(
   DSL_int_t nIoctl,
   const DSL_CPE_IoctlFlag_t **ppIoctlFlag)
{
   DSL_Error_t nErrorCode = DSL_ERROR;

   for (unsigned int i = 0; i < nFlagsNum; ++i)
   {
      if (ioctlFlags[i].nIoctl == nIoctl)
      {
         *ppIoctlFlag = &ioctlFlags[i];

         /* flag found */
         nErrorCode = DSL_SUCCESS;
      }
   }

   return nErrorCode;
}

/*
   This function toggles ioctl flag to TRUE
*/
static DSL_void_t DSL_CPE_IoctlFlagSet(
   DSL_int_t nIoctl)
{
   DSL_CPE_IoctlFlag_t *pIoctlFlag = DSL_NULL;

   if (DSL_CPE_IoctlFlagGet(nIoctl,
         (const DSL_CPE_IoctlFlag_t**)&pIoctlFlag) == DSL_SUCCESS)
   {
      pIoctlFlag->bIsSet = DSL_TRUE;
   }
   else
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_WRN, (DSL_CPE_PREFIX
         "WARNING - Status Parser, DSL_CPE_IoctlFlagSet: flag not found!"
         DSL_CPE_CRLF));
   }
}

/*
   This function returns current "is set" value of related ioctl flag
*/
static DSL_boolean_t DSL_CPE_IsIoctlFlagSet(DSL_int_t nIoctl)
{
   DSL_boolean_t bIsSet = DSL_FALSE;
   const DSL_CPE_IoctlFlag_t *pIoctlFlag = DSL_NULL;

   if (DSL_CPE_IoctlFlagGet(nIoctl, &pIoctlFlag) == DSL_SUCCESS)
   {
      bIsSet = pIoctlFlag->bIsSet;
   }
   else
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_WRN, (DSL_CPE_PREFIX
         "WARNING - Status Parser, DSL_CPE_IsIoctlFlagSet: flag not found!"
         DSL_CPE_CRLF));
   }

   return bIsSet;
}

/*
   This function toggles all ioctl flags back to FALSE
*/
static DSL_void_t DSL_CPE_IoctlFlagsReset()
{
   for (unsigned int i = 0; i < nFlagsNum; ++i)
   {
      ioctlFlags[i].bIsSet = DSL_FALSE;
   }
}

/*
   This function creates JSON status file and stores it in the
   common JSON object for further use
*/
static DSL_void_t DSL_CPE_JsonStatusFileCreate()
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   pJsonStatusRootObj = json_object_new_object();

   for (unsigned int i = 0; i < nNodesNum; ++i)
   {
      pParentObj = DSL_CPE_JsonNodeParentGet(
         pJsonStatusRootObj, jsonStatusNodes[i].jsonPath);

      /* to prevent json_object_object_add function from asserting */
      if (pParentObj != DSL_NULL &&
         json_object_get_type(pParentObj) == json_type_object)
      {
         switch (jsonStatusNodes[i].jsonType)
         {
         case json_type_object:
            pNewObj = json_object_new_object();
            break;
         case json_type_array:
            pNewObj = json_object_new_array();
            break;
         case json_type_string:
            pNewObj = json_object_new_string("");
            break;
         case json_type_int:
            pNewObj = json_object_new_int(0);
            break;
         default:
            break;
         }

         if (pNewObj)
         {
            json_object_object_add(pParentObj,
               DSL_CPE_JsonNodeNameGet(jsonStatusNodes[i].jsonPath), pNewObj);

            pNewObj = DSL_NULL;
         }
      }
   }
}

/*
   This function returns parent node for the given json node
*/
static json_object* DSL_CPE_JsonNodeParentGet(
   json_object *pRoot,
   const DSL_char_t *pSectionPath)
{
   typedef struct
   {
      DSL_char_t *pName;
      json_object *pJsonObj;
   } DSL_CPE_JsonNode_t;
   DSL_char_t inputPath[SECTION_PATH_LENGTH];
   DSL_CPE_JsonNode_t visitedNode;
   DSL_char_t *token = DSL_NULL;

   memset(inputPath, 0, sizeof(inputPath));
   cpe_control_strncpy_s(inputPath, sizeof(inputPath),
      pSectionPath, sizeof(inputPath));

   memset(&visitedNode, 0, sizeof(visitedNode));
   visitedNode.pJsonObj = pRoot;

   token = strtok(inputPath, NODES_SEPARATOR);
   while (token)
   {
      visitedNode.pName = token;
      json_object_object_foreach(visitedNode.pJsonObj, key, val)
      {
         if (strcmp(key, visitedNode.pName) == 0)
         {
            if (json_object_get_type(val) == json_type_object)
            {
               visitedNode.pJsonObj = val;
            }
            break;
         }
      }
      token = strtok(NULL, NODES_SEPARATOR);
   }

   return visitedNode.pJsonObj;
}

/*
   This function returns full json path for the given node index
*/
static const DSL_char_t* DSL_CPE_JsonNodePathGet(
   const DSL_CPE_StatusNodeIdx_t eNodeIdx)
{
   const DSL_char_t *sOutputPath = DSL_NULL;

   for (unsigned int i = 0; i < nNodesNum; ++i)
   {
      if (jsonStatusNodes[i].eNodeIdx == eNodeIdx)
      {
         sOutputPath = jsonStatusNodes[i].jsonPath;
         break;
      }
   }

   return sOutputPath;
}

/*
   This function returns key (last node) name for the given json path
*/
static const DSL_char_t* DSL_CPE_JsonNodeNameGet(
   const DSL_char_t* jsonPath)
{
   const char *jsonLastNode =
      strrchr(jsonPath, (int)NODES_SEPARATOR[0]);
   return jsonLastNode ? jsonLastNode + 1 : jsonPath;
}

/*
   This function checks if the given path is defined
   withing the given section path
*/
static DSL_boolean_t DSL_CPE_IsWithinSection(
   const DSL_char_t* jsonPath,
   const DSL_char_t* jsonSectionPath)
{
   DSL_boolean_t bIsWithin = DSL_FALSE;

   /* given section path is a substring of the given path */
   if (strncmp(jsonPath, jsonSectionPath, strlen(jsonSectionPath)) == 0)
   {
      bIsWithin = DSL_TRUE;
   }

   return bIsWithin;
}

/*
   This function triggers init type params update
*/
static DSL_Error_t DSL_CPE_JsonInitParamsUpdate(
   DSL_CPE_Control_Context_t *pContext)
{
   DSL_Error_t nRet = DSL_SUCCESS, nErrorCode = DSL_SUCCESS;

   if (pJsonStatusRootObj == DSL_NULL)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - DSL_CPE_JsonInitParamsUpdate: root object is NULL!"
         DSL_CPE_CRLF));
      return DSL_ERROR;
   }

   for (unsigned int i = 0; i < nNodesNum; ++i)
   {
      if (jsonStatusNodes[i].eUpdateType == DSL_UPDATE_TYPE_INIT)
      {
         nRet = DSL_CPE_JsonParamsUpdate(
            pContext, jsonStatusNodes[i].eNodeIdx);

         /* keep error code in case of any
            failure and continue */
         if (nRet != DSL_SUCCESS)
         {
            nErrorCode = nRet;
         }

      }
   }

   /* reset all IOCTL flags to FALSE */
   DSL_CPE_IoctlFlagsReset();

   return nErrorCode;
}

/*
   This function triggers given evt type param update
*/
static DSL_Error_t DSL_CPE_JsonEvtParamsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx)
{
   DSL_Error_t nErrorCode = DSL_SUCCESS;

   if (pJsonStatusRootObj == DSL_NULL)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - DSL_CPE_JsonEvtParamsUpdate: root object is NULL!"
         DSL_CPE_CRLF));
      return DSL_ERROR;
   }

   /* call update function for given node */
   nErrorCode = DSL_CPE_JsonParamsUpdate(pContext, eNodeIdx);

   /* reset all IOCTL flags to FALSE */
   DSL_CPE_IoctlFlagsReset();

   return nErrorCode;
}

/*
   This function triggers polling type params update
*/
static DSL_Error_t DSL_CPE_JsonPollParamsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eSectionIdx)
{
   DSL_Error_t nRet = DSL_SUCCESS, nErrorCode = DSL_SUCCESS;
   const DSL_char_t *jsonSectionPath = DSL_NULL;

   if (pJsonStatusRootObj == DSL_NULL)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - DSL_CPE_JsonPollParamsUpdate: root object is NULL!"
         DSL_CPE_CRLF));
      return DSL_ERROR;
   }

   /* get given section path from the index */
   jsonSectionPath = DSL_CPE_JsonNodePathGet(eSectionIdx);

   /* update all poll type parameters witing given section */
   if (jsonSectionPath != DSL_NULL)
   {
      for (unsigned int i = 0; i < nNodesNum; ++i)
      {
         if (DSL_CPE_IsWithinSection(
               jsonStatusNodes[i].jsonPath, jsonSectionPath) &&
            jsonStatusNodes[i].eUpdateType == DSL_UPDATE_TYPE_POLL)
         {
            nRet = DSL_CPE_JsonParamsUpdate(
               pContext, jsonStatusNodes[i].eNodeIdx);

            /* keep error code in case of any
               failure and continue */
            if (nRet != DSL_SUCCESS)
            {
               nErrorCode = nRet;
            }
         }
      }
   }

   /* reset all IOCTL flags to FALSE */
   DSL_CPE_IoctlFlagsReset();

   return nErrorCode;
}

/*
   This function triggers given param update for any update type
*/
static DSL_Error_t DSL_CPE_JsonParamsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx)
{
   DSL_Error_t nErrorCode = DSL_SUCCESS;
   const DSL_char_t *jsonPath = DSL_NULL;

   jsonPath = DSL_CPE_JsonNodePathGet(eNodeIdx);
   if (jsonPath == DSL_NULL)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - DSL_CPE_JsonParamsUpdate: node index (%d) undefined!"
         DSL_CPE_CRLF, (DSL_int_t)eNodeIdx));
      return DSL_ERROR;
   }

   switch (eNodeIdx)
   {
   case DSL_LINE_ENTRIES:
      DSL_CPE_JsonLineNrOfEntriesUpdate(jsonPath);
      break;
   case DSL_CHANNEL_ENTRIES:
      DSL_CPE_JsonChannelNrOfEntriesUpdate(jsonPath);
      break;
   case DSL_BND_GROUP_ENTRIES:
      DSL_CPE_JsonBndGroupNrOfEntriesUpdate(jsonPath);
      break;
   case DSL_API_VERSION:
      DSL_CPE_JsonApiVersionUpdate(pContext, jsonPath);
      break;
   case DSL_MEI_VERSION:
      DSL_CPE_JsonMeiVersionUpdate(pContext, jsonPath);
      break;
   case DSL_HW_VERSION:
      DSL_CPE_JsonHwVersionUpdate(pContext, jsonPath);
      break;
   case DSL_CHIPSET_TYPE:
      DSL_CPE_JsonChipsetTypeUpdate(pContext, jsonPath);
      break;
   /********************************/
   /* <dsl.status.line.0> section: */
   /********************************/
      case DSL_STATUS_LINE_0_STATUS:
         DSL_CPE_JsonLineStatusUpdate(pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATUS_LINE_0_UPSTREAM:
         DSL_CPE_JsonUpstreamUpdate(jsonPath);
         break;
      case DSL_STATUS_LINE_0_FW_VERSION:
         DSL_CPE_JsonFwVersionUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_0_LINK_STATUS:
         DSL_CPE_JsonLinkStatusUpdate(pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATUS_LINE_0_STD_SUPPORTED:
         DSL_CPE_JsonStdSupportedUpdate(pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATUS_LINE_0_XTSE:
         DSL_CPE_JsonXtseConfigUpdate(pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATUS_LINE_0_STD_USED:
         DSL_CPE_JsonStdUsedUpdate(pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATUS_LINE_0_XTSE_USED:
         DSL_CPE_JsonXtseStatusUpdate(pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATUS_LINE_0_ENCODING:
         DSL_CPE_JsonLineEncodingUpdate(jsonPath);
         break;
      case DSL_STATUS_LINE_0_ALLOWED_PROFILES:
         DSL_CPE_JsonAllowedProfilesUpdate(pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATUS_LINE_0_CURRENT_PROFILE:
         DSL_CPE_JsonCurrentProfileUpdate(pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATUS_LINE_0_PWR_MGMT_STATE:
         DSL_CPE_JsonPowerMgmtStateUpdate(pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATUS_LINE_0_SUCC_FAIL_CAUSE:
         DSL_CPE_JsonSuccFailCauseUpdate(pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATUS_LINE_0_LAST_STATE_TRANSM_US:
         DSL_CPE_JsonLastStateTransmUpdate(
            pContext,/*nDevice*/ 0, DSL_UPSTREAM, jsonPath);
         break;
      case DSL_STATUS_LINE_0_LAST_STATE_TRANSM_DS:
         DSL_CPE_JsonLastStateTransmUpdate(
            pContext,/*nDevice*/ 0, DSL_DOWNSTREAM, jsonPath);
         break;
      case DSL_STATUS_LINE_0_US0_MASK:
         DSL_CPE_JsonUs0MaskUpdate(jsonPath);
         break;
      case DSL_STATUS_LINE_0_TRELLIS_US:
         DSL_CPE_JsonTrellisUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_0_TRELLIS_DS:
         DSL_CPE_JsonTrellisUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_0_ACT_SNR_MODE_US:
         DSL_CPE_JsonActSnrModeUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_0_ACT_SNR_MODE_DS:
         DSL_CPE_JsonActSnrModeUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_0_LINE_NR:
         DSL_CPE_JsonLineNrUpdate(/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATUS_LINE_0_MAX_BIT_RATE_US:
         DSL_CPE_JsonMaxBitRateUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_0_MAX_BIT_RATE_DS:
         DSL_CPE_JsonMaxBitRateUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_0_NOISE_MARGIN_US:
         DSL_CPE_JsonNoiseMarginUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_0_NOISE_MARGIN_DS:
         DSL_CPE_JsonNoiseMarginUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_0_SNR_MPB_US:
         DSL_CPE_JsonSnrMpbUpdate(
            pContext,/*nDevice*/ 0, DSL_UPSTREAM, jsonPath);
         break;
      case DSL_STATUS_LINE_0_SNR_MPB_DS:
         DSL_CPE_JsonSnrMpbUpdate(
            pContext,/*nDevice*/ 0, DSL_DOWNSTREAM, jsonPath);
         break;
      case DSL_STATUS_LINE_0_POWER_US:
         DSL_CPE_JsonPowerUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_0_POWER_DS:
         DSL_CPE_JsonPowerUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_0_XTUR_VENDOR:
         DSL_CPE_JsonXturVendorUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_0_XTUR_COUNTRY:
         DSL_CPE_JsonXturCountryUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_0_XTUR_ANSI_STD:
         DSL_CPE_JsonXturAnsiStdUpdate(pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATUS_LINE_0_XTUR_ANSI_REV:
         DSL_CPE_JsonXturAnsiRevUpdate(pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATUS_LINE_0_XTUC_VENDOR:
         DSL_CPE_JsonXtucVendorUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_0_XTUC_COUNTRY:
         DSL_CPE_JsonXtucCountryUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_0_XTUC_ANSI_STD:
         DSL_CPE_JsonXtucAnsiStdUpdate(pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATUS_LINE_0_XTUC_ANSI_REV:
         DSL_CPE_JsonXtucAnsiRevUpdate(pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATUS_LINE_0_UPBOKLER_PB:
         DSL_CPE_JsonUpboklerPbUpdate(pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATUS_LINE_0_RXTHRSH_DS:
         DSL_CPE_JsonRxthrshDsUpdate(pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATUS_LINE_0_ACT_RA_MODE_US:
         DSL_CPE_JsonActRaModeUpdate(
            pContext,/*nDevice*/ 0, DSL_UPSTREAM, jsonPath);
         break;
      case DSL_STATUS_LINE_0_ACT_RA_MODE_DS:
         DSL_CPE_JsonActRaModeUpdate(
            pContext,/*nDevice*/ 0, DSL_DOWNSTREAM, jsonPath);
         break;
      case DSL_STATUS_LINE_0_SNR_MROC_US:
         DSL_CPE_JsonSnrMrocUpdate(jsonPath);
         break;
      case DSL_STATUS_LINE_0_SNR_MROC_DS:
         DSL_CPE_JsonSnrMrocUpdate(jsonPath);
         break;
      case DSL_STATUS_LINE_0_ATTENUATION_US:
         DSL_CPE_JsonAttenuationUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_0_ATTENUATION_DS:
         DSL_CPE_JsonAttenuationUpdate(pContext, eNodeIdx, jsonPath);
         break;
      /****************************************/
      /* <dsl.status.line.0.channel> section: */
      /****************************************/
         case DSL_STATUS_LINE_0_CHANNEL_STATUS:
            DSL_CPE_JsonChannelStatusUpdate(
               pContext,/*nDevice*/ 0, jsonPath);
            break;
         case DSL_STATUS_LINE_0_CHANNEL_LINKENC_SUPP:
            DSL_CPE_JsonChannelLinkEncapSupportedUpdate(
               pContext,/*nDevice*/ 0, jsonPath);
            break;
         case DSL_STATUS_LINE_0_CHANNEL_LINKENC_USED:
            DSL_CPE_JsonChannelLinkEncapUpdate(
               pContext,/*nDevice*/ 0, jsonPath);
            break;
         case DSL_STATUS_LINE_0_CHANNEL_LPATH:
         case DSL_STATUS_LINE_0_CHANNEL_INTLVDEPTH:
         case DSL_STATUS_LINE_0_CHANNEL_INTLVBLOCK:
            DSL_CPE_JsonChannelFramingUpdate(
               pContext,/*nDevice*/ 0, eNodeIdx, jsonPath);
            break;
         case DSL_STATUS_LINE_0_CHANNEL_ACT_INTERL_DELAY:
         case DSL_STATUS_LINE_0_CHANNEL_ACTINP:
            DSL_CPE_JsonChannelActStatusUpdate(
               pContext,/*nDevice*/ 0, DSL_DOWNSTREAM, eNodeIdx, jsonPath);
            break;
         case DSL_STATUS_LINE_0_CHANNEL_INPREPORT:
            DSL_CPE_JsonChannelActINPReportUpdate(
               pContext,/*nDevice*/ 0, jsonPath);
            break;
         case DSL_STATUS_LINE_0_CHANNEL_NFEC:
         case DSL_STATUS_LINE_0_CHANNEL_RFEC:
         case DSL_STATUS_LINE_0_CHANNEL_LSYMB:
            DSL_CPE_JsonChannelFramingUpdate(
               pContext,/*nDevice*/ 0, eNodeIdx, jsonPath);
            break;
         case DSL_STATUS_LINE_0_CHANNEL_DATARATE:
         case DSL_STATUS_LINE_0_CHANNEL_DATARATE_US:
            /* if entire section selected (DSL_STATUS_LINE_0_CHANNEL_DATARATE)
               adapt jsonPath to current case */
            if (eNodeIdx == DSL_STATUS_LINE_0_CHANNEL_DATARATE)
            {
               jsonPath = DSL_CPE_JsonNodePathGet(
                  DSL_STATUS_LINE_0_CHANNEL_DATARATE_US);
            }
            DSL_CPE_JsonChannelDataRatesUpdate(pContext,
               /*nDevice*/ 0, DSL_UPSTREAM, jsonPath);
            /* if selected only upstream node (instead of entire section
               DSL_STATUS_LINE_0_CHANNEL_DATARATE) then break here, otherwise
               continue and update also DATARATE_DS */
            if (eNodeIdx == DSL_STATUS_LINE_0_CHANNEL_DATARATE_US)
            {
               break;
            }
         case DSL_STATUS_LINE_0_CHANNEL_DATARATE_DS:
            /* if entire section selected (DSL_STATUS_LINE_0_CHANNEL_DATARATE)
               adapt jsonPath to current case */
            if (eNodeIdx == DSL_STATUS_LINE_0_CHANNEL_DATARATE)
            {
               jsonPath = DSL_CPE_JsonNodePathGet(
                  DSL_STATUS_LINE_0_CHANNEL_DATARATE_DS);
            }
            DSL_CPE_JsonChannelDataRatesUpdate(
               pContext, /*nDevice*/ 0, DSL_DOWNSTREAM, jsonPath);
            break;
         case DSL_STATUS_LINE_0_CHANNEL_ACTNDR_US:
         case DSL_STATUS_LINE_0_CHANNEL_ACTINPREIN_US:
            DSL_CPE_JsonChannelActStatusUpdate(
               pContext,/*nDevice*/ 0, DSL_UPSTREAM, eNodeIdx, jsonPath);
            break;
         case DSL_STATUS_LINE_0_CHANNEL_ACTNDR_DS:
         case DSL_STATUS_LINE_0_CHANNEL_ACTINPREIN_DS:
            DSL_CPE_JsonChannelActStatusUpdate(
               pContext,/*nDevice*/ 0, DSL_DOWNSTREAM, eNodeIdx, jsonPath);
            break;
   /********************************/
   /* <dsl.status.line.1> section: */
   /********************************/
      case DSL_STATUS_LINE_1_STATUS:
         DSL_CPE_JsonLineStatusUpdate(pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATUS_LINE_1_UPSTREAM:
         DSL_CPE_JsonUpstreamUpdate(jsonPath);
         break;
      case DSL_STATUS_LINE_1_FW_VERSION:
         DSL_CPE_JsonFwVersionUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_1_LINK_STATUS:
         DSL_CPE_JsonLinkStatusUpdate(pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATUS_LINE_1_STD_SUPPORTED:
         DSL_CPE_JsonStdSupportedUpdate(pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATUS_LINE_1_XTSE:
         DSL_CPE_JsonXtseConfigUpdate(pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATUS_LINE_1_STD_USED:
         DSL_CPE_JsonStdUsedUpdate(pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATUS_LINE_1_XTSE_USED:
         DSL_CPE_JsonXtseStatusUpdate(pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATUS_LINE_1_ENCODING:
         DSL_CPE_JsonLineEncodingUpdate(jsonPath);
         break;
      case DSL_STATUS_LINE_1_ALLOWED_PROFILES:
         DSL_CPE_JsonAllowedProfilesUpdate(pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATUS_LINE_1_CURRENT_PROFILE:
         DSL_CPE_JsonCurrentProfileUpdate(pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATUS_LINE_1_PWR_MGMT_STATE:
         DSL_CPE_JsonPowerMgmtStateUpdate(pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATUS_LINE_1_SUCC_FAIL_CAUSE:
         DSL_CPE_JsonSuccFailCauseUpdate(pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATUS_LINE_1_LAST_STATE_TRANSM_US:
         DSL_CPE_JsonLastStateTransmUpdate(
            pContext,/*nDevice*/ 1, DSL_UPSTREAM, jsonPath);
         break;
      case DSL_STATUS_LINE_1_LAST_STATE_TRANSM_DS:
         DSL_CPE_JsonLastStateTransmUpdate(
            pContext,/*nDevice*/ 1, DSL_DOWNSTREAM, jsonPath);
         break;
      case DSL_STATUS_LINE_1_US0_MASK:
         DSL_CPE_JsonUs0MaskUpdate(jsonPath);
         break;
      case DSL_STATUS_LINE_1_TRELLIS_US:
         DSL_CPE_JsonTrellisUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_1_TRELLIS_DS:
         DSL_CPE_JsonTrellisUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_1_ACT_SNR_MODE_US:
         DSL_CPE_JsonActSnrModeUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_1_ACT_SNR_MODE_DS:
         DSL_CPE_JsonActSnrModeUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_1_LINE_NR:
         DSL_CPE_JsonLineNrUpdate(/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATUS_LINE_1_MAX_BIT_RATE_US:
         DSL_CPE_JsonMaxBitRateUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_1_MAX_BIT_RATE_DS:
         DSL_CPE_JsonMaxBitRateUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_1_NOISE_MARGIN_US:
         DSL_CPE_JsonNoiseMarginUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_1_NOISE_MARGIN_DS:
         DSL_CPE_JsonNoiseMarginUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_1_SNR_MPB_US:
         DSL_CPE_JsonSnrMpbUpdate(
            pContext,/*nDevice*/ 1, DSL_UPSTREAM, jsonPath);
         break;
      case DSL_STATUS_LINE_1_SNR_MPB_DS:
         DSL_CPE_JsonSnrMpbUpdate(
            pContext,/*nDevice*/ 1, DSL_DOWNSTREAM, jsonPath);
         break;
      case DSL_STATUS_LINE_1_POWER_US:
         DSL_CPE_JsonPowerUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_1_POWER_DS:
         DSL_CPE_JsonPowerUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_1_XTUR_VENDOR:
         DSL_CPE_JsonXturVendorUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_1_XTUR_COUNTRY:
         DSL_CPE_JsonXturCountryUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_1_XTUR_ANSI_STD:
         DSL_CPE_JsonXturAnsiStdUpdate(pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATUS_LINE_1_XTUR_ANSI_REV:
         DSL_CPE_JsonXturAnsiRevUpdate(pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATUS_LINE_1_XTUC_VENDOR:
         DSL_CPE_JsonXtucVendorUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_1_XTUC_COUNTRY:
         DSL_CPE_JsonXtucCountryUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_1_XTUC_ANSI_STD:
         DSL_CPE_JsonXtucAnsiStdUpdate(pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATUS_LINE_1_XTUC_ANSI_REV:
         DSL_CPE_JsonXtucAnsiRevUpdate(pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATUS_LINE_1_UPBOKLER_PB:
         DSL_CPE_JsonUpboklerPbUpdate(pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATUS_LINE_1_RXTHRSH_DS:
         DSL_CPE_JsonRxthrshDsUpdate(pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATUS_LINE_1_ACT_RA_MODE_US:
         DSL_CPE_JsonActRaModeUpdate(
            pContext,/*nDevice*/ 1, DSL_UPSTREAM, jsonPath);
         break;
      case DSL_STATUS_LINE_1_ACT_RA_MODE_DS:
         DSL_CPE_JsonActRaModeUpdate(
            pContext,/*nDevice*/ 1, DSL_DOWNSTREAM, jsonPath);
         break;
      case DSL_STATUS_LINE_1_SNR_MROC_US:
         DSL_CPE_JsonSnrMrocUpdate(jsonPath);
         break;
      case DSL_STATUS_LINE_1_SNR_MROC_DS:
         DSL_CPE_JsonSnrMrocUpdate(jsonPath);
         break;
      case DSL_STATUS_LINE_1_ATTENUATION_US:
         DSL_CPE_JsonAttenuationUpdate(pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATUS_LINE_1_ATTENUATION_DS:
         DSL_CPE_JsonAttenuationUpdate(pContext, eNodeIdx, jsonPath);
         break;
      /****************************************/
      /* <dsl.status.line.1.channel> section: */
      /****************************************/
         case DSL_STATUS_LINE_1_CHANNEL_STATUS:
            DSL_CPE_JsonChannelStatusUpdate(
               pContext,/*nDevice*/ 1, jsonPath);
            break;
         case DSL_STATUS_LINE_1_CHANNEL_LINKENC_SUPP:
            DSL_CPE_JsonChannelLinkEncapSupportedUpdate(
               pContext,/*nDevice*/ 1, jsonPath);
            break;
         case DSL_STATUS_LINE_1_CHANNEL_LINKENC_USED:
               DSL_CPE_JsonChannelLinkEncapUpdate(
                  pContext,/*nDevice*/ 1, jsonPath);
               break;
         case DSL_STATUS_LINE_1_CHANNEL_LPATH:
         case DSL_STATUS_LINE_1_CHANNEL_INTLVDEPTH:
         case DSL_STATUS_LINE_1_CHANNEL_INTLVBLOCK:
            DSL_CPE_JsonChannelFramingUpdate(
               pContext,/*nDevice*/ 1, eNodeIdx, jsonPath);
            break;
         case DSL_STATUS_LINE_1_CHANNEL_ACT_INTERL_DELAY:
         case DSL_STATUS_LINE_1_CHANNEL_ACTINP:
            DSL_CPE_JsonChannelActStatusUpdate(
               pContext,/*nDevice*/ 1, DSL_DOWNSTREAM, eNodeIdx, jsonPath);
            break;
         case DSL_STATUS_LINE_1_CHANNEL_INPREPORT:
            DSL_CPE_JsonChannelActINPReportUpdate(
               pContext,/*nDevice*/ 1, jsonPath);
            break;
         case DSL_STATUS_LINE_1_CHANNEL_NFEC:
         case DSL_STATUS_LINE_1_CHANNEL_RFEC:
         case DSL_STATUS_LINE_1_CHANNEL_LSYMB:
            DSL_CPE_JsonChannelFramingUpdate(
               pContext,/*nDevice*/ 1, eNodeIdx, jsonPath);
            break;
         case DSL_STATUS_LINE_1_CHANNEL_DATARATE:
         case DSL_STATUS_LINE_1_CHANNEL_DATARATE_US:
            /* if entire section selected (DSL_STATUS_LINE_1_CHANNEL_DATARATE)
               adapt jsonPath to current case */
            if (eNodeIdx == DSL_STATUS_LINE_1_CHANNEL_DATARATE)
            {
               jsonPath = DSL_CPE_JsonNodePathGet(
                  DSL_STATUS_LINE_1_CHANNEL_DATARATE_US);
            }
            DSL_CPE_JsonChannelDataRatesUpdate(pContext,
               /*nDevice*/ 1, DSL_UPSTREAM, jsonPath);
            /* if requested node is DSL_STATUS_LINE_1_CHANNEL_DATARATE
            (entire section) then continue and update also DATARATE_DS */
            if (eNodeIdx == DSL_STATUS_LINE_1_CHANNEL_DATARATE_US)
            {
               break;
            }
         case DSL_STATUS_LINE_1_CHANNEL_DATARATE_DS:
            /* if entire section selected (DSL_STATUS_LINE_1_CHANNEL_DATARATE)
               adapt jsonPath to current case */
            if (eNodeIdx == DSL_STATUS_LINE_1_CHANNEL_DATARATE)
            {
               jsonPath = DSL_CPE_JsonNodePathGet(
                  DSL_STATUS_LINE_1_CHANNEL_DATARATE_DS);
            }
            DSL_CPE_JsonChannelDataRatesUpdate(pContext,
               /*nDevice*/ 1, DSL_DOWNSTREAM, jsonPath);
            break;
         case DSL_STATUS_LINE_1_CHANNEL_ACTNDR_US:
         case DSL_STATUS_LINE_1_CHANNEL_ACTINPREIN_US:
            DSL_CPE_JsonChannelActStatusUpdate(
               pContext,/*nDevice*/ 1, DSL_UPSTREAM, eNodeIdx, jsonPath);
            break;
         case DSL_STATUS_LINE_1_CHANNEL_ACTNDR_DS:
         case DSL_STATUS_LINE_1_CHANNEL_ACTINPREIN_DS:
            DSL_CPE_JsonChannelActStatusUpdate(
               pContext,/*nDevice*/ 1, DSL_DOWNSTREAM, eNodeIdx, jsonPath);
            break;
   /********************************/
   /* <dsl.stats.line.0> section: */
   /********************************/
      case DSL_STATS_LINE_0_TOTAL_START:
         DSL_CPE_JsonTotalStartUpdate(
            pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATS_LINE_0_SHOWTIME_START:
         DSL_CPE_JsonShowtimeStartUpdate(
            pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATS_LINE_0_LAST_SHOWTIME_START:
         DSL_CPE_JsonLastShowtimeStartUpdate(
            pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATS_LINE_0_CURRENT_DAY_START:
         DSL_CPE_JsonCurrDayStartUpdate(
            pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATS_LINE_0_QUARTER_HOUR_START:
         DSL_CPE_JsonQuarterHourStartUpdate(
            pContext,/*nDevice*/ 0, jsonPath);
         break;
      case DSL_STATS_LINE_0_TOTAL_ERR_SECS:
         DSL_CPE_JsonTotalErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATS_LINE_0_TOTAL_SEV_ERR_SECS:
         DSL_CPE_JsonTotalSevErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATS_LINE_0_SHOWTIME_ERR_SECS:
         DSL_CPE_JsonShowtimeErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATS_LINE_0_SHOWTIME_SEV_ERR_SECS:
         DSL_CPE_JsonShowtimeSevErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATS_LINE_0_LAST_SHOWTIME_ERR_SECS:
         DSL_CPE_JsonLastShowtimeErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATS_LINE_0_LAST_SHOWTIME_SEV_ERR_SECS:
         DSL_CPE_JsonLastShowtimeSevErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATS_LINE_0_CURRENT_DAY_ERR_SECS:
         DSL_CPE_JsonCurrDayErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATS_LINE_0_CURRENT_DAY_SEV_ERR_SECS:
         DSL_CPE_JsonCurrDaySevErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATS_LINE_0_QUARTER_HOUR_ERR_SECS:
         DSL_CPE_JsonQuarterHourErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATS_LINE_0_QUARTER_HOUR_SEV_ERR_SECS:
         DSL_CPE_JsonQuarterHourSevErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      /****************************************/
      /* <dsl.stats.line.0.channel> section: */
      /****************************************/
         case DSL_STATS_LINE_0_CHANNEL_TOTAL_START:
         case DSL_STATS_LINE_0_CHANNEL_TOTAL_XTUR_FEC_ERRORS:
         case DSL_STATS_LINE_0_CHANNEL_TOTAL_XTUR_CRC_ERRORS:
            DSL_CPE_JsonChannelCountersTotalUpdate(
               pContext,/*nDevice*/ 0, DSL_NEAR_END, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_TOTAL_XTUC_FEC_ERRORS:
         case DSL_STATS_LINE_0_CHANNEL_TOTAL_XTUC_CRC_ERRORS:
            DSL_CPE_JsonChannelCountersTotalUpdate(
               pContext,/*nDevice*/ 0, DSL_FAR_END, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_TOTAL_XTUR_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCountersTotalUpdate(
               pContext,/*nDevice*/ 0, DSL_NEAR_END, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_TOTAL_XTUC_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCountersTotalUpdate(
               pContext,/*nDevice*/ 0, DSL_FAR_END, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_SHOWTIME_START:
         case DSL_STATS_LINE_0_CHANNEL_SHOWTIME_XTUR_FEC_ERRORS:
         case DSL_STATS_LINE_0_CHANNEL_SHOWTIME_XTUR_CRC_ERRORS:
            DSL_CPE_JsonChannelCountersShowtimeUpdate(
               pContext,/*nDevice*/ 0, DSL_NEAR_END,
               /*HistoryInterval*/ 0, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_SHOWTIME_XTUC_FEC_ERRORS:
         case DSL_STATS_LINE_0_CHANNEL_SHOWTIME_XTUC_CRC_ERRORS:
            DSL_CPE_JsonChannelCountersShowtimeUpdate(
               pContext,/*nDevice*/ 0, DSL_FAR_END,
               /*HistoryInterval*/ 0, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_START:
         case DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_XTUR_FEC_ERRORS:
         case DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_XTUR_CRC_ERRORS:
            DSL_CPE_JsonChannelCountersShowtimeUpdate(
               pContext,/*nDevice*/ 0, DSL_NEAR_END,
               /*HistoryInterval*/ 1, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_XTUC_FEC_ERRORS:
         case DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_XTUC_CRC_ERRORS:
            DSL_CPE_JsonChannelCountersShowtimeUpdate(
               pContext,/*nDevice*/ 0, DSL_FAR_END,
               /*HistoryInterval*/ 1, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_SHOWTIME_XTUR_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCountersShowtimeUpdate(
               pContext,/*nDevice*/ 0, DSL_NEAR_END,
               /*HistoryInterval*/ 0, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_SHOWTIME_XTUC_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCountersShowtimeUpdate(
               pContext,/*nDevice*/ 0, DSL_FAR_END,
               /*HistoryInterval*/ 0, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_XTUR_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCountersShowtimeUpdate(
               pContext,/*nDevice*/ 0, DSL_NEAR_END,
               /*HistoryInterval*/ 1, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_XTUC_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCountersShowtimeUpdate(
               pContext,/*nDevice*/ 0, DSL_FAR_END,
               /*HistoryInterval*/ 1, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_START:
         case DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_XTUR_FEC_ERRORS:
         case DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_XTUR_CRC_ERRORS:
            DSL_CPE_JsonChannelCounters1DayUpdate(
               pContext,/*nDevice*/ 0, DSL_NEAR_END, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_XTUC_FEC_ERRORS:
         case DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_XTUC_CRC_ERRORS:
            DSL_CPE_JsonChannelCounters1DayUpdate(
               pContext,/*nDevice*/ 0, DSL_FAR_END, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_XTUR_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCounters1DayUpdate(
               pContext,/*nDevice*/ 0, DSL_NEAR_END, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_XTUC_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCounters1DayUpdate(
               pContext,/*nDevice*/ 0, DSL_FAR_END, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_START:
         case DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_XTUR_FEC_ERRORS:
         case DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_XTUR_CRC_ERRORS:
            DSL_CPE_JsonChannelCounters15MinUpdate(
               pContext,/*nDevice*/ 0, DSL_NEAR_END, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_XTUC_FEC_ERRORS:
         case DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_XTUC_CRC_ERRORS:
            DSL_CPE_JsonChannelCounters15MinUpdate(
               pContext,/*nDevice*/ 0, DSL_FAR_END, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_XTUR_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCounters15MinUpdate(
               pContext,/*nDevice*/ 0, DSL_NEAR_END, jsonPath);
            break;
         case DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_XTUC_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCounters15MinUpdate(
               pContext,/*nDevice*/ 0, DSL_FAR_END, jsonPath);
            break;
   /********************************/
   /* <dsl.stats.line.1> section: */
   /********************************/
      case DSL_STATS_LINE_1_TOTAL_START:
         DSL_CPE_JsonTotalStartUpdate(
            pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATS_LINE_1_SHOWTIME_START:
         DSL_CPE_JsonShowtimeStartUpdate(
            pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATS_LINE_1_LAST_SHOWTIME_START:
         DSL_CPE_JsonLastShowtimeStartUpdate(
            pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATS_LINE_1_CURRENT_DAY_START:
         DSL_CPE_JsonCurrDayStartUpdate(
            pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATS_LINE_1_QUARTER_HOUR_START:
         DSL_CPE_JsonQuarterHourStartUpdate(
            pContext,/*nDevice*/ 1, jsonPath);
         break;
      case DSL_STATS_LINE_1_TOTAL_ERR_SECS:
         DSL_CPE_JsonTotalErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATS_LINE_1_TOTAL_SEV_ERR_SECS:
         DSL_CPE_JsonTotalSevErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATS_LINE_1_SHOWTIME_ERR_SECS:
         DSL_CPE_JsonShowtimeErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATS_LINE_1_SHOWTIME_SEV_ERR_SECS:
         DSL_CPE_JsonShowtimeSevErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATS_LINE_1_LAST_SHOWTIME_ERR_SECS:
         DSL_CPE_JsonLastShowtimeErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATS_LINE_1_LAST_SHOWTIME_SEV_ERR_SECS:
         DSL_CPE_JsonLastShowtimeSevErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATS_LINE_1_CURRENT_DAY_ERR_SECS:
         DSL_CPE_JsonCurrDayErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATS_LINE_1_CURRENT_DAY_SEV_ERR_SECS:
         DSL_CPE_JsonCurrDaySevErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATS_LINE_1_QUARTER_HOUR_ERR_SECS:
         DSL_CPE_JsonQuarterHourErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      case DSL_STATS_LINE_1_QUARTER_HOUR_SEV_ERR_SECS:
         DSL_CPE_JsonQuarterHourSevErrSecsUpdate(
            pContext, eNodeIdx, jsonPath);
         break;
      /****************************************/
      /* <dsl.stats.line.1.channel> section: */
      /****************************************/
         case DSL_STATS_LINE_1_CHANNEL_TOTAL_START:
         case DSL_STATS_LINE_1_CHANNEL_TOTAL_XTUR_FEC_ERRORS:
         case DSL_STATS_LINE_1_CHANNEL_TOTAL_XTUR_CRC_ERRORS:
            DSL_CPE_JsonChannelCountersTotalUpdate(
               pContext,/*nDevice*/ 1, DSL_NEAR_END, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_TOTAL_XTUC_FEC_ERRORS:
         case DSL_STATS_LINE_1_CHANNEL_TOTAL_XTUC_CRC_ERRORS:
            DSL_CPE_JsonChannelCountersTotalUpdate(
               pContext,/*nDevice*/ 1, DSL_FAR_END, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_TOTAL_XTUR_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCountersTotalUpdate(
               pContext,/*nDevice*/ 1, DSL_NEAR_END, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_TOTAL_XTUC_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCountersTotalUpdate(
               pContext,/*nDevice*/ 1, DSL_FAR_END, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_SHOWTIME_START:
         case DSL_STATS_LINE_1_CHANNEL_SHOWTIME_XTUR_FEC_ERRORS:
         case DSL_STATS_LINE_1_CHANNEL_SHOWTIME_XTUR_CRC_ERRORS:
            DSL_CPE_JsonChannelCountersShowtimeUpdate(
               pContext,/*nDevice*/ 1, DSL_NEAR_END,
               /*HistoryInterval*/ 0, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_SHOWTIME_XTUC_FEC_ERRORS:
         case DSL_STATS_LINE_1_CHANNEL_SHOWTIME_XTUC_CRC_ERRORS:
            DSL_CPE_JsonChannelCountersShowtimeUpdate(
               pContext,/*nDevice*/ 1, DSL_FAR_END,
               /*HistoryInterval*/ 0, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_START:
         case DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_XTUR_FEC_ERRORS:
         case DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_XTUR_CRC_ERRORS:
            DSL_CPE_JsonChannelCountersShowtimeUpdate(
               pContext,/*nDevice*/ 1, DSL_NEAR_END,
               /*HistoryInterval*/ 1, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_XTUC_FEC_ERRORS:
         case DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_XTUC_CRC_ERRORS:
            DSL_CPE_JsonChannelCountersShowtimeUpdate(
               pContext,/*nDevice*/ 1, DSL_FAR_END,
               /*HistoryInterval*/ 1, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_SHOWTIME_XTUR_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCountersShowtimeUpdate(
               pContext,/*nDevice*/ 1, DSL_NEAR_END,
               /*HistoryInterval*/ 0, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_SHOWTIME_XTUC_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCountersShowtimeUpdate(
               pContext,/*nDevice*/ 1, DSL_FAR_END,
               /*HistoryInterval*/ 0, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_XTUR_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCountersShowtimeUpdate(
               pContext,/*nDevice*/ 1, DSL_NEAR_END,
               /*HistoryInterval*/ 1, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_XTUC_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCountersShowtimeUpdate(
               pContext,/*nDevice*/ 1, DSL_FAR_END,
               /*HistoryInterval*/ 1, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_START:
         case DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_XTUR_FEC_ERRORS:
         case DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_XTUR_CRC_ERRORS:
            DSL_CPE_JsonChannelCounters1DayUpdate(
               pContext,/*nDevice*/ 1, DSL_NEAR_END, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_XTUC_FEC_ERRORS:
         case DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_XTUC_CRC_ERRORS:
            DSL_CPE_JsonChannelCounters1DayUpdate(
               pContext,/*nDevice*/ 1, DSL_FAR_END, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_XTUR_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCounters1DayUpdate(
               pContext,/*nDevice*/ 1, DSL_NEAR_END, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_XTUC_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCounters1DayUpdate(
               pContext,/*nDevice*/ 1, DSL_FAR_END, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_START:
         case DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_XTUR_FEC_ERRORS:
         case DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_XTUR_CRC_ERRORS:
            DSL_CPE_JsonChannelCounters15MinUpdate(
               pContext,/*nDevice*/ 1, DSL_NEAR_END, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_XTUC_FEC_ERRORS:
         case DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_XTUC_CRC_ERRORS:
            DSL_CPE_JsonChannelCounters15MinUpdate(
               pContext,/*nDevice*/ 1, DSL_FAR_END, eNodeIdx, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_XTUR_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCounters15MinUpdate(
               pContext,/*nDevice*/ 1, DSL_NEAR_END, jsonPath);
            break;
         case DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_XTUC_HEC_ERRORS:
            DSL_CPE_JsonChannelDataPathCounters15MinUpdate(
               pContext,/*nDevice*/ 1, DSL_FAR_END, jsonPath);
            break;
   default:
      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - Status Parser (DSL_CPE_JsonParamsUpdate): "
         "unknown node!"
         DSL_CPE_CRLF));
      nErrorCode = DSL_ERROR;
      break;
   }

   return nErrorCode;
}

static DSL_Error_t DSL_CPE_JsonStatusFileWrite()
{
   const DSL_char_t * pBufferOut = DSL_NULL;
   DSL_CPE_File_t *pFile = DSL_NULL;

   /* Write updated data to file */
   pBufferOut = json_object_to_json_string_ext(pJsonStatusRootObj,
      JSON_C_TO_STRING_PRETTY);
   pFile = DSL_CPE_FOpen(STATUS_FILE_PATH_TMP, "w");
   if (pFile == DSL_NULL)
   {
      DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
         "ERROR - DSL_CPE_JsonStatusFileWrite: failed to open status file!"
         DSL_CPE_CRLF));
      return DSL_ERROR;
   }
   DSL_CPE_FWrite(pBufferOut, sizeof(DSL_char_t), strlen(pBufferOut), pFile);
   DSL_CPE_FClose(pFile);

   /* execute atomic rename operation to store the status file in the final
      destination and keep it locked as short as possible */
   rename(STATUS_FILE_PATH_TMP, STATUS_FILE_PATH);

   return DSL_SUCCESS;
}

/* --- Common section --- */

static DSL_void_t DSL_CPE_JsonLineNrOfEntriesUpdate(
   const DSL_char_t* jsonPath)
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   /* update json object */
   pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
   pNewObj = json_object_new_int(DSL_CPE_DSL_ENTITIES);
   json_object_object_add(pParentObj,
      DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
}

static DSL_void_t DSL_CPE_JsonChannelNrOfEntriesUpdate(
   const DSL_char_t* jsonPath)
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   /* update json object */
   pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
   pNewObj = json_object_new_int(DSL_CPE_CHANNELS_PER_LINE);
   json_object_object_add(pParentObj,
      DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
}

static DSL_void_t DSL_CPE_JsonBndGroupNrOfEntriesUpdate(
   const DSL_char_t* jsonPath)
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   /* update json object */
   pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
   pNewObj = json_object_new_int(DSL_CPE_BND_GROUP_ENTRIES);
   json_object_object_add(pParentObj,
      DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
}

/*
   This function collects all version information needed
   for common "version" parameters in the status tree
   and keeps it in a static structure in order to not to
   send IOCTL requests each time
*/
static const DSL_char_t* DSL_CPE_VersionInfoGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx)
{
   DSL_Error_t nRet = DSL_SUCCESS;
   static DSL_VersionInformation_t sVersionInfo = { 0 };
   const char *sVersion = DSL_NULL;
   const DSL_int_t nDevice = 0;

   /* send IOCTL only if struct is zeroed (first execution)
      or some parameters are not yet set (some information
      can be available with delay, e.g. hw version is available
      later, after FW DL) */
   if ((eNodeIdx == DSL_API_VERSION && (
         strlen(sVersionInfo.data.DSL_DriverVersionApi) == 0 ||
         strcmp(sVersionInfo.data.DSL_DriverVersionApi, "n/a") == 0)) ||
      (eNodeIdx == DSL_MEI_VERSION && (
         strlen(sVersionInfo.data.DSL_DriverVersionMeiBsp) == 0 ||
         strcmp(sVersionInfo.data.DSL_DriverVersionMeiBsp, "n/a") == 0)) ||
      (eNodeIdx == DSL_HW_VERSION && (
         strlen(sVersionInfo.data.DSL_ChipSetHWVersion) == 0 ||
         strcmp(sVersionInfo.data.DSL_ChipSetHWVersion, "n/a") == 0)) ||
      (eNodeIdx == DSL_CHIPSET_TYPE && (
         strlen(sVersionInfo.data.DSL_ChipSetType) == 0 ||
         strcmp(sVersionInfo.data.DSL_ChipSetType, "n/a") == 0)) ||
      /* FW version can change in the runtime */
      (eNodeIdx == DSL_STATUS_LINE_0_FW_VERSION ||
            eNodeIdx == DSL_STATUS_LINE_1_FW_VERSION)
      )
   {
      nRet = DSL_CPE_Ioctl(pContext->fd[nDevice],
         DSL_FIO_VERSION_INFORMATION_GET, (int)&sVersionInfo);

      if (nRet < DSL_SUCCESS ||
         sVersionInfo.accessCtl.nReturn < DSL_SUCCESS)
      {
         DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
            "ERROR - StatusParser, DSL_CPE_VersionInfoGet: "
            "DSL_FIO_VERSION_INFORMATION_GET ioctl failed "
            "(device<%d>, nRet = %d, accessCtrl = %d)!" DSL_CPE_CRLF,
            nDevice, nRet, sVersionInfo.accessCtl.nReturn));
      }
   }

   switch (eNodeIdx)
   {
      case DSL_API_VERSION:
         sVersion = sVersionInfo.data.DSL_DriverVersionApi;
         break;
      case DSL_MEI_VERSION:
         sVersion = sVersionInfo.data.DSL_DriverVersionMeiBsp;
         break;
      case DSL_HW_VERSION:
         sVersion = sVersionInfo.data.DSL_ChipSetHWVersion;
         break;
      case DSL_CHIPSET_TYPE:
         sVersion = sVersionInfo.data.DSL_ChipSetType;
         break;
      case DSL_STATUS_LINE_0_FW_VERSION:
      case DSL_STATUS_LINE_1_FW_VERSION:
         sVersion = sVersionInfo.data.DSL_ChipSetFWVersion;
         break;
      default:
         DSL_CCA_DEBUG(DSL_CCA_DBG_WRN, (DSL_CPE_PREFIX
            "WARNING - StatusParser, DSL_CPE_VersionInfoGet: "
            "unknown node!"
            DSL_CPE_CRLF));
         break;
   }

   return sVersion;
}

static DSL_void_t DSL_CPE_JsonApiVersionUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_char_t* jsonPath)
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   /* update json object */
   pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
   pNewObj = json_object_new_string(
      DSL_CPE_VersionInfoGet(pContext, DSL_API_VERSION));
   json_object_object_add(pParentObj,
      DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
}

static DSL_void_t DSL_CPE_JsonMeiVersionUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_char_t* jsonPath)
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   /* update json object */
   pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
   pNewObj = json_object_new_string(
      DSL_CPE_VersionInfoGet(pContext, DSL_MEI_VERSION));
   json_object_object_add(pParentObj,
      DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
}

static DSL_void_t DSL_CPE_JsonHwVersionUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_char_t* jsonPath)
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   /* update json object */
   pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
   pNewObj = json_object_new_string(
      DSL_CPE_VersionInfoGet(pContext, DSL_HW_VERSION));
   json_object_object_add(pParentObj,
      DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
}

static DSL_void_t DSL_CPE_JsonChipsetTypeUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_char_t* jsonPath)
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   /* update json object */
   pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
   pNewObj = json_object_new_string(
      DSL_CPE_VersionInfoGet(pContext, DSL_CHIPSET_TYPE));
   json_object_object_add(pParentObj,
      DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
}

/* --- Line section --- */

/*
   This function collects all data provided by
   DSL_FIO_LINE_STATE_GET
   ioctl for all devices
*/
static DSL_Error_t DSL_CPE_LineStateGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   DSL_LineStateValue_t *pLineState)
{
   DSL_Error_t nErrorCode = DSL_SUCCESS, nRet = DSL_SUCCESS;
   static DSL_LineState_t lineData[DSL_CPE_MAX_DSL_ENTITIES] = { 0 };

   if (pLineState == DSL_NULL ||
         nDevice >= DSL_CPE_DSL_ENTITIES)
   {
      return DSL_ERROR;
   }

   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_LINE_STATE_GET))
   {
      for (DSL_uint32_t nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
            DSL_FIO_LINE_STATE_GET, (int)&lineData[nDev]);

         if (nRet < DSL_SUCCESS ||
            lineData[nDev].accessCtl.nReturn != DSL_SUCCESS)
         {
            DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
               "ERROR - DSL_FIO_LINE_STATE_GET ioctl failed "
               "(device<%d>, nRet=%d, accessCtrl=%d)!" DSL_CPE_CRLF,
               nDev, nRet, lineData[nDev].accessCtl.nReturn));

            nErrorCode = DSL_ERROR;
         }

         /* set ioctl flag only if all IOCTLs succeeded */
         if (nErrorCode == DSL_SUCCESS)
         {
            DSL_CPE_IoctlFlagSet(DSL_FIO_LINE_STATE_GET);
         }
      }
   }

   /* provide data of corresponding IOCtl request */
   *pLineState = lineData[nDevice].data.nLineState;

   return nErrorCode;
}

static DSL_void_t DSL_CPE_JsonLineStatusUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_LineStateValue_t nLineState = DSL_LINESTATE_NOT_INITIALIZED;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_LineStateGet(
         pContext, nDevice, &nLineState) == DSL_SUCCESS)
   {
      switch (nLineState)
      {
      case DSL_LINESTATE_IDLE:
      case DSL_LINESTATE_EXCEPTION:
         pNewObj = json_object_new_string("DOWN");
         break;
      case DSL_LINESTATE_SILENT:
         pNewObj = json_object_new_string("READY");
         break;
      case DSL_LINESTATE_HANDSHAKE:
         pNewObj = json_object_new_string("HANDSHAKE");
         break;
      case DSL_LINESTATE_FULL_INIT:
      case DSL_LINESTATE_SHOWTIME_NO_SYNC:
         pNewObj = json_object_new_string("TRAINING");
         break;
      case DSL_LINESTATE_SHOWTIME_TC_SYNC:
         pNewObj = json_object_new_string("UP");
         break;
      case DSL_LINESTATE_LOOPDIAGNOSTIC_ACTIVE:
         pNewObj = json_object_new_string("LDM_ACTIVE");
         break;
      case DSL_LINESTATE_LOOPDIAGNOSTIC_COMPLETE:
         pNewObj = json_object_new_string("LDM_COMPLETE");
         break;
      default:
         break;
      }

      if (pNewObj != DSL_NULL)
      {
         /* update json object */
         pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
         json_object_object_add(pParentObj,
            DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
      }
   }
}

static DSL_void_t DSL_CPE_JsonUpstreamUpdate(
   const DSL_char_t* jsonPath)
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   /* update json object */
   pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
   pNewObj = json_object_new_boolean(DSL_TRUE); /* always true */
   json_object_object_add(pParentObj,
      DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
}

static DSL_void_t DSL_CPE_JsonFwVersionUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   /* update json object */
   pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
   pNewObj = json_object_new_string(
      DSL_CPE_VersionInfoGet(pContext, eNodeIdx));
   json_object_object_add(pParentObj,
      DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
}

static DSL_void_t DSL_CPE_JsonLinkStatusUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_LineStateValue_t nLineState = DSL_LINESTATE_NOT_INITIALIZED;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_LineStateGet(
         pContext, nDevice, &nLineState) == DSL_SUCCESS)
   {
      switch (nLineState)
      {
      case DSL_LINESTATE_NOT_INITIALIZED:
      case DSL_LINESTATE_NOT_UPDATED:
      case DSL_LINESTATE_DISABLED:
      case DSL_LINESTATE_IDLE_REQUEST:
         pNewObj = json_object_new_string("Initializing");
         break;
      case DSL_LINESTATE_IDLE:
         pNewObj = json_object_new_string("Disabled");
         break;
      case DSL_LINESTATE_SHOWTIME_TC_SYNC:
         pNewObj = json_object_new_string("Up");
         break;
      case DSL_LINESTATE_EXCEPTION:
         pNewObj = json_object_new_string("Error");
         break;
      default:
         pNewObj = json_object_new_string("EstablishingLink");
         break;
      }

      if (pNewObj != DSL_NULL)
      {
         /* update json object */
         pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
         json_object_object_add(pParentObj,
            DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
      }
   }
}

/*
   This function collects all data provided by
   DSL_FIO_G997_XTU_SYSTEM_ENABLING_CONFIG_GET
   ioctl for all devices
*/
static DSL_Error_t DSL_CPE_SystemEnablingConfigGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   DSL_uint8_t *pXTSE)
{
   DSL_Error_t nRet = DSL_SUCCESS, nErrorCode = DSL_SUCCESS;
   static DSL_G997_XTUSystemEnabling_t
      xtseData[DSL_CPE_MAX_DSL_ENTITIES] = { 0 };

   if (pXTSE == DSL_NULL ||
         nDevice >= DSL_CPE_DSL_ENTITIES)
   {
      return DSL_ERROR;
   }

   /* avoid sending IOCTL many times in one status update iteration */
   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_G997_XTU_SYSTEM_ENABLING_CONFIG_GET))
   {
      for (unsigned int nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
            DSL_FIO_G997_XTU_SYSTEM_ENABLING_CONFIG_GET, (int)&xtseData[nDev]);

         if (nRet < DSL_SUCCESS ||
            xtseData[nDev].accessCtl.nReturn < DSL_SUCCESS)
         {
            DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
               "ERROR - DSL_FIO_G997_XTU_SYSTEM_ENABLING_CONFIG_GET ioctl "
               "failed (device<%d>, nRet=%d, accessCtrl=%d)!"DSL_CPE_CRLF,
               nDev, nRet, xtseData[nDev].accessCtl.nReturn));

            nErrorCode = DSL_ERROR;
         }
      }

      /* set ioctl flag only if all IOCTLs succeeded */
      if (nErrorCode == DSL_SUCCESS)
      {
         DSL_CPE_IoctlFlagSet(DSL_FIO_G997_XTU_SYSTEM_ENABLING_CONFIG_GET);
      }
   }

   /* get data for the given device */
   for (unsigned int nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
   {
      if (nDev == nDevice)
      {
         for (unsigned int i = 0; i < DSL_G997_NUM_XTSE_OCTETS; ++i)
         {
            pXTSE[i] = xtseData[nDev].data.XTSE[i];
         }
      }
   }

   return nErrorCode;
}

static DSL_void_t DSL_CPE_JsonStdSupportedUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;
   DSL_uint32_t nOffset = 0, nBufLen = 0, nOctet = 0, nBit = 0;
   DSL_char_t sStandardsSupported[512] = "";
   DSL_char_t *sStandardName = DSL_NULL;
   DSL_uint8_t XTSE[DSL_G997_NUM_XTSE_OCTETS] = { 0 };

   if (DSL_CPE_SystemEnablingConfigGet(
         pContext, nDevice, XTSE) == DSL_SUCCESS)
   {
      nBufLen = sizeof(sStandardsSupported);
      for (unsigned int i = 0; i < nStandardsNum; ++i)
      {
         nOctet = standartsTable[i].nOctet;
         nBit = standartsTable[i].nBit;

         if (XTSE[nOctet] & nBit)
         {
            /* select name from the table and exclude leading comma */
            sStandardName = standartsTable[i].sName + ((nOffset) ? 0 : 1);
            nBufLen = (sizeof(sStandardsSupported) > nOffset) ?
               sizeof(sStandardsSupported) - nOffset : 0;
            cpe_control_strncpy_s(sStandardsSupported + nOffset, nBufLen,
               sStandardName, nBufLen);
            nOffset += strlen(sStandardName);
         }
      }

      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_string(sStandardsSupported);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonXtseConfigUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;
   DSL_uint8_t XTSE[DSL_G997_NUM_XTSE_OCTETS] = { 0 };

   if (DSL_CPE_SystemEnablingConfigGet(
         pContext, nDevice, XTSE) == DSL_SUCCESS)
   {
      /* create json array object */
      pNewObj = json_object_new_array();

      /* update json object */
      for (unsigned int i = 0; i < DSL_G997_NUM_XTSE_OCTETS; i++)
      {
         json_object_array_add(pNewObj,
            json_object_new_int(XTSE[i]));
      }
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

/*
   This function collects all data provided by
   DSL_FIO_G997_XTU_SYSTEM_ENABLING_STATUS_GET
   ioctl for all devices
*/
static DSL_Error_t DSL_CPE_SystemEnablingStatusGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   DSL_uint8_t *pXTSE)
{
   DSL_Error_t nRet = DSL_SUCCESS, nErrorCode = DSL_SUCCESS;
   static DSL_G997_XTUSystemEnabling_t
      xtseData[DSL_CPE_MAX_DSL_ENTITIES] = { 0 };

   if (pXTSE == DSL_NULL ||
         nDevice >= DSL_CPE_DSL_ENTITIES)
   {
      return DSL_ERROR;
   }

   /* avoid sending IOCTL many times in one status update iteration */
   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_G997_XTU_SYSTEM_ENABLING_STATUS_GET))
   {
      for (unsigned int nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
            DSL_FIO_G997_XTU_SYSTEM_ENABLING_STATUS_GET, (int)&xtseData[nDev]);

         if (nRet < DSL_SUCCESS ||
            xtseData[nDev].accessCtl.nReturn < DSL_SUCCESS)
         {
            DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
               "ERROR - DSL_FIO_G997_XTU_SYSTEM_ENABLING_STATUS_GET ioctl "
               "failed (device<%d>, nRet=%d, accessCtrl=%d)!"DSL_CPE_CRLF,
               nDev, nRet, xtseData[nDev].accessCtl.nReturn));

            nErrorCode = DSL_ERROR;
         }
      }

      /* set ioctl flag only if all IOCTLs succeeded */
      if (nErrorCode == DSL_SUCCESS)
      {
         DSL_CPE_IoctlFlagSet(DSL_FIO_G997_XTU_SYSTEM_ENABLING_STATUS_GET);
      }
   }

   /* get data for the given device */
   for (unsigned int nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
   {
      if (nDev == nDevice)
      {
         for (unsigned int i = 0; i < DSL_G997_NUM_XTSE_OCTETS; ++i)
         {
            pXTSE[i] = xtseData[nDev].data.XTSE[i];
         }
      }
   }

   return nErrorCode;
}

static DSL_void_t DSL_CPE_JsonStdUsedUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;
   DSL_uint32_t nOctet = 0, nBits = 0;
   DSL_char_t sStandardUsed[20] = "";
   DSL_char_t *sStandardName = DSL_NULL;
   DSL_uint8_t XTSE[DSL_G997_NUM_XTSE_OCTETS] = { 0 };

   if (DSL_CPE_SystemEnablingStatusGet(
         pContext, nDevice, XTSE) == DSL_SUCCESS)
   {
      for (unsigned int i = 0; i < nStandardsNum; ++i)
      {
         nOctet = standartsTable[i].nOctet;
         nBits = standartsTable[i].nBit;

         if (XTSE[nOctet] & nBits)
         {
            sStandardName = standartsTable[i].sName + 1;
            cpe_control_strncpy_s(sStandardUsed, sizeof(sStandardUsed),
               sStandardName, sizeof(sStandardUsed));
            break;
         }
      }

      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_string(sStandardUsed);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}


static DSL_void_t DSL_CPE_JsonXtseStatusUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;
   DSL_uint8_t XTSE[DSL_G997_NUM_XTSE_OCTETS] = { 0 };

   if (DSL_CPE_SystemEnablingStatusGet(
         pContext, nDevice, XTSE) == DSL_SUCCESS)
   {
      /* create json array object */
      pNewObj = json_object_new_array();

      /* update json object */
      for (unsigned int i = 0; i < DSL_G997_NUM_XTSE_OCTETS; ++i)
      {
         json_object_array_add(pNewObj,
            json_object_new_int(XTSE[i]));
      }
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonLineEncodingUpdate(
   const DSL_char_t* jsonPath)
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   /* update json object */
   pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
   pNewObj = json_object_new_string("DMT"); /* fixed value */
   json_object_object_add(pParentObj,
      DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
}

static DSL_void_t DSL_CPE_JsonAllowedProfilesUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_Error_t nRet = DSL_SUCCESS;
   DSL_BandPlanSupport_t profilesSupport = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;
   DSL_uint32_t nOffset = 0, nBufLen = 0;
   DSL_char_t sAllowedProfiles[40] = "";
   const DSL_char_t *sProfileName = DSL_NULL;

   nRet = DSL_CPE_Ioctl(pContext->fd[nDevice],
      DSL_FIO_BAND_PLAN_SUPPORT_GET, (int)&profilesSupport);
   if (nRet == DSL_SUCCESS)
   {
      nBufLen = sizeof(sAllowedProfiles);
      for (unsigned int i = 0; i < DSL_PROFILE_LAST; i++)
      {
         for (unsigned int j = 0; j < DSL_BANDPLAN_LAST; j++)
         {
            if (profilesSupport.data.bSupported[j][i])
            {
               /* select name from the table and exclude leading comma */
               sProfileName = profilesTable[i] + ((nOffset) ? 0 : 1);
               nBufLen = (sizeof(sAllowedProfiles) > nOffset) ?
                  sizeof(sAllowedProfiles) - nOffset : 0;
               cpe_control_strncpy_s(sAllowedProfiles + nOffset, nBufLen,
                  sProfileName, nBufLen);
               nOffset += strlen(sProfileName);

               break;
            }
         }
      }

      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_string(sAllowedProfiles);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonCurrentProfileUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_Error_t nRet = DSL_SUCCESS;
   DSL_BandPlanStatus_t profilesStatus = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;
   DSL_char_t sCurrentProfile[5] = "";
   const DSL_char_t *sProfileName = DSL_NULL;

   nRet = DSL_CPE_Ioctl(pContext->fd[nDevice],
      DSL_FIO_BAND_PLAN_STATUS_GET, (int)&profilesStatus);
   if (nRet == DSL_SUCCESS)
   {
      for (unsigned int i = 0; i < DSL_PROFILE_LAST; i++)
      {
         if (i == profilesStatus.data.nProfile)
         {
            sProfileName = profilesTable[i] + 1;
            cpe_control_strncpy_s(sCurrentProfile, sizeof(sCurrentProfile),
               sProfileName, sizeof(sCurrentProfile));
            break;
         }
      }

      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_string(sCurrentProfile);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonPowerMgmtStateUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_Error_t nRet = DSL_SUCCESS;
   DSL_G997_PowerManagementStatus_t powerStatus = { 0 };
   DSL_char_t *powerTable[] = { "L0", "L1", "L2", "L3" };
   DSL_char_t sPowerMgmtState[4] = "";
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   nRet = DSL_CPE_Ioctl(pContext->fd[nDevice],
      DSL_FIO_G997_POWER_MANAGEMENT_STATUS_GET, (int)&powerStatus);
   if (nRet == DSL_SUCCESS)
   {
      for (unsigned int i = 0; i < sizeof(powerTable) / sizeof(char *); i++)
      {
         if (i == powerStatus.data.nPowerManagementStatus)
         {
            cpe_control_strncpy_s(sPowerMgmtState, sizeof(sPowerMgmtState),
               powerTable[i], sizeof(sPowerMgmtState));
            break;
         }
      }

      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_string(sPowerMgmtState);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonSuccFailCauseUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_Error_t nRet = DSL_SUCCESS;
   DSL_G997_LineInitStatus_t initStatus = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   nRet = DSL_CPE_Ioctl(pContext->fd[nDevice],
      DSL_FIO_G997_LINE_INIT_STATUS_GET, (int)&initStatus);
   if (nRet == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(initStatus.data.nLineInitStatus);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonLastStateTransmUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_AccessDir_t nDirection,
   const DSL_char_t* jsonPath)
{
   DSL_Error_t nRet = DSL_SUCCESS;
   DSL_G997_LastStateTransmitted_t lastState = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   lastState.nDirection = nDirection;
   nRet = DSL_CPE_Ioctl(pContext->fd[nDevice],
      DSL_FIO_G997_LAST_STATE_TRANSMITTED_GET, (int)&lastState);
   if (nRet == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(lastState.data.nLastStateTransmitted);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonUs0MaskUpdate(
   const DSL_char_t* jsonPath)
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   /* update json object */
   pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
   pNewObj =
      json_object_new_int(0xF3F3); /* all types supported, use fixed value */
   json_object_object_add(pParentObj,
      DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
}

/*
   This function collects all data provided by
   DSL_FIO_LINE_FEATURE_STATUS_GET
   ioctl for all devices
*/
static DSL_Error_t DSL_CPE_LineFeatureStatusGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   DSL_boolean_t *pData)
{
   DSL_Error_t nRet = DSL_SUCCESS, nErrorCode = DSL_SUCCESS;
   static DSL_LineFeature_t lineFeatureUs[DSL_CPE_MAX_DSL_ENTITIES] = { 0 };
   static DSL_LineFeature_t lineFeatureDs[DSL_CPE_MAX_DSL_ENTITIES] = { 0 };

   if (pData == DSL_NULL)
   {
      return DSL_ERROR;
   }

   /* avoid sending IOCTL many times in one status update iteration */
   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_LINE_FEATURE_STATUS_GET))
   {
      for (unsigned int nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         lineFeatureUs[nDev].nDirection = DSL_UPSTREAM;
         nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
            DSL_FIO_LINE_FEATURE_STATUS_GET, (int)&lineFeatureUs[nDev]);

         if (nRet < DSL_SUCCESS ||
            lineFeatureUs[nDev].accessCtl.nReturn < DSL_SUCCESS)
         {
            DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
               "ERROR - DSL_FIO_LINE_FEATURE_STATUS_GET(US) ioctl "
               "failed (device<%d>, nRet=%d, accessCtrl=%d)!"DSL_CPE_CRLF,
               nDev, nRet, lineFeatureUs[nDev].accessCtl.nReturn));

            nErrorCode = DSL_ERROR;
         }

         lineFeatureDs[nDev].nDirection = DSL_DOWNSTREAM;
         nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
            DSL_FIO_LINE_FEATURE_STATUS_GET, (int)&lineFeatureDs[nDev]);

         if (nRet < DSL_SUCCESS ||
            lineFeatureDs[nDev].accessCtl.nReturn < DSL_SUCCESS)
         {
            DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
               "ERROR - DSL_FIO_LINE_FEATURE_STATUS_GET(DS) ioctl "
               "failed (device<%d>, nRet=%d, accessCtrl=%d)!"DSL_CPE_CRLF,
               nDev, nRet, lineFeatureDs[nDev].accessCtl.nReturn));

            nErrorCode = DSL_ERROR;
         }
      }

      /* set ioctl flag only if all IOCTLs succeeded */
      if (nErrorCode == DSL_SUCCESS)
      {
         DSL_CPE_IoctlFlagSet(DSL_FIO_LINE_FEATURE_STATUS_GET);
      }
   }

   /* get data for the given node */
   switch (eNodeIdx)
   {
      case DSL_STATUS_LINE_0_TRELLIS_US:
         *pData = lineFeatureUs[/*nDevice*/0].data.bTrellisEnable;
         break;
      case DSL_STATUS_LINE_0_TRELLIS_DS:
         *pData = lineFeatureDs[/*nDevice*/0].data.bTrellisEnable;
         break;
      case DSL_STATUS_LINE_1_TRELLIS_US:
         *pData = lineFeatureUs[/*nDevice*/1].data.bTrellisEnable;
         break;
      case DSL_STATUS_LINE_1_TRELLIS_DS:
         *pData = lineFeatureDs[/*nDevice*/1].data.bTrellisEnable;
         break;
      case DSL_STATUS_LINE_0_ACT_SNR_MODE_US:
         *pData = lineFeatureUs[/*nDevice*/0].data.bVirtualNoiseSupport;
         break;
      case DSL_STATUS_LINE_0_ACT_SNR_MODE_DS:
         *pData = lineFeatureDs[/*nDevice*/0].data.bVirtualNoiseSupport;
         break;
      case DSL_STATUS_LINE_1_ACT_SNR_MODE_US:
         *pData = lineFeatureUs[/*nDevice*/1].data.bVirtualNoiseSupport;
         break;
      case DSL_STATUS_LINE_1_ACT_SNR_MODE_DS:
         *pData = lineFeatureDs[/*nDevice*/1].data.bVirtualNoiseSupport;
         break;
      default:
         break;
   }

   return nErrorCode;
}

static DSL_void_t DSL_CPE_JsonTrellisUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_boolean_t bTrellis = DSL_FALSE;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_LineFeatureStatusGet(
         pContext, eNodeIdx, &bTrellis) == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_boolean(bTrellis);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonActSnrModeUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_boolean_t bActSnrMode = DSL_FALSE;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_LineFeatureStatusGet(
      pContext, eNodeIdx, &bActSnrMode) == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_boolean(bActSnrMode);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonLineNrUpdate(
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   /* update json object */
   pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
   pNewObj =
      json_object_new_int(nDevice);
   json_object_object_add(pParentObj,
      DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
}

/*
   This function collects all data provided by
   DSL_FIO_G997_LINE_STATUS_GET
   ioctl for all devices
*/
static DSL_Error_t DSL_CPE_G997LineStatusGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   DSL_uint32_t *pData)
{
   DSL_Error_t nRet = DSL_SUCCESS, nErrorCode = DSL_SUCCESS;
   static DSL_G997_LineStatus_t
      g997LineStatusUs[DSL_CPE_MAX_DSL_ENTITIES] = { 0 };
   static DSL_G997_LineStatus_t
      g997LineStatusDs[DSL_CPE_MAX_DSL_ENTITIES] = { 0 };

   if (pData == DSL_NULL)
   {
      return DSL_ERROR;
   }

   /* avoid sending IOCTL many times in one status update iteration */
   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_G997_LINE_STATUS_GET))
   {
      for (unsigned int nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         /*showtime, US */
         g997LineStatusUs[nDev].nDeltDataType = DSL_DELT_DATA_SHOWTIME;
         g997LineStatusUs[nDev].nDirection = DSL_UPSTREAM;
         nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
            DSL_FIO_G997_LINE_STATUS_GET, (int)&g997LineStatusUs[nDev]);

         if (nRet < DSL_SUCCESS ||
            g997LineStatusUs[nDev].accessCtl.nReturn < DSL_SUCCESS)
         {
            DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
               "ERROR - DSL_FIO_G997_LINE_STATUS_GET(US) ioctl "
               "failed (device<%d>, nRet=%d, accessCtrl=%d)!"DSL_CPE_CRLF,
               nDev, nRet, g997LineStatusUs[nDev].accessCtl.nReturn));

            nErrorCode = DSL_ERROR;
         }

         /* showtime, DS */
         g997LineStatusDs[nDev].nDeltDataType = DSL_DELT_DATA_SHOWTIME;
         g997LineStatusDs[nDev].nDirection = DSL_DOWNSTREAM;
         nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
            DSL_FIO_G997_LINE_STATUS_GET, (int)&g997LineStatusDs[nDev]);

         if (nRet < DSL_SUCCESS ||
            g997LineStatusDs[nDev].accessCtl.nReturn < DSL_SUCCESS)
         {
            DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
               "ERROR - DSL_FIO_G997_LINE_STATUS_GET(DS) ioctl "
               "failed (device<%d>, nRet=%d, accessCtrl=%d)!"DSL_CPE_CRLF,
               nDev, nRet, g997LineStatusDs[nDev].accessCtl.nReturn));

            nErrorCode = DSL_ERROR;
         }
      }

      /* set ioctl flag only if all IOCTLs succeeded */
      if (nErrorCode == DSL_SUCCESS)
      {
         DSL_CPE_IoctlFlagSet(DSL_FIO_G997_LINE_STATUS_GET);
      }
   }

   /* get data for the given node */
   switch (eNodeIdx)
   {
   case DSL_STATUS_LINE_0_MAX_BIT_RATE_US:
      *pData = g997LineStatusUs[/*nDevice*/0].data.ATTNDR;
      break;
   case DSL_STATUS_LINE_0_MAX_BIT_RATE_DS:
      *pData = g997LineStatusDs[/*nDevice*/0].data.ATTNDR;
      break;
   case DSL_STATUS_LINE_1_MAX_BIT_RATE_US:
      *pData = g997LineStatusUs[/*nDevice*/1].data.ATTNDR;
      break;
   case DSL_STATUS_LINE_1_MAX_BIT_RATE_DS:
      *pData = g997LineStatusDs[/*nDevice*/1].data.ATTNDR;
      break;
   case DSL_STATUS_LINE_0_NOISE_MARGIN_US:
      *pData = g997LineStatusUs[/*nDevice*/0].data.SNR;
      break;
   case DSL_STATUS_LINE_0_NOISE_MARGIN_DS:
      *pData = g997LineStatusDs[/*nDevice*/0].data.SNR;
      break;
   case DSL_STATUS_LINE_1_NOISE_MARGIN_US:
      *pData = g997LineStatusUs[/*nDevice*/1].data.SNR;
      break;
   case DSL_STATUS_LINE_1_NOISE_MARGIN_DS:
      *pData = g997LineStatusDs[/*nDevice*/1].data.SNR;
      break;
   case DSL_STATUS_LINE_0_POWER_US:
      *pData = g997LineStatusUs[/*nDevice*/0].data.ACTATP;
      break;
   case DSL_STATUS_LINE_0_POWER_DS:
      *pData = g997LineStatusDs[/*nDevice*/0].data.ACTATP;
      break;
   case DSL_STATUS_LINE_1_POWER_US:
      *pData = g997LineStatusUs[/*nDevice*/1].data.ACTATP;
      break;
   case DSL_STATUS_LINE_1_POWER_DS:
      *pData = g997LineStatusDs[/*nDevice*/1].data.ACTATP;
      break;
   case DSL_STATUS_LINE_0_ATTENUATION_US:
      *pData = g997LineStatusUs[/*nDevice*/0].data.LATN;
      break;
   case DSL_STATUS_LINE_0_ATTENUATION_DS:
      *pData = g997LineStatusDs[/*nDevice*/0].data.LATN;
      break;
   case DSL_STATUS_LINE_1_ATTENUATION_US:
      *pData = g997LineStatusUs[/*nDevice*/1].data.LATN;
      break;
   case DSL_STATUS_LINE_1_ATTENUATION_DS:
      *pData = g997LineStatusDs[/*nDevice*/1].data.LATN;
      break;
   default:
      break;
   }

   return nErrorCode;
}

static DSL_void_t DSL_CPE_JsonMaxBitRateUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_uint32_t nMaxBitRate = 0;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_G997LineStatusGet(
         pContext, eNodeIdx, &nMaxBitRate) == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(nMaxBitRate);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonNoiseMarginUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_uint32_t nNoiseMargin = 0;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_G997LineStatusGet(
         pContext, eNodeIdx, &nNoiseMargin) == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(nNoiseMargin);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonPowerUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_uint32_t nPower = 0;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_G997LineStatusGet(
         pContext, eNodeIdx, &nPower) == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(nPower);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonAttenuationUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_uint32_t nAttenuation = 0;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_G997LineStatusGet(
         pContext, eNodeIdx, &nAttenuation) == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(nAttenuation);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonSnrMpbUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_AccessDir_t nDirection,
   const DSL_char_t* jsonPath)
{
   DSL_Error_t nRet = DSL_SUCCESS;
   DSL_uint32_t nOffset = 0, nBufSize = 0;
   DSL_char_t sSnrMpbList[256] = "", sBuf[32] = "";
   DSL_char_t *sSnrMpbName = DSL_NULL;
   DSL_G997_LineStatusPerBand_t bandStatus = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   bandStatus.nDirection = nDirection;
   nRet = DSL_CPE_Ioctl(pContext->fd[nDevice],
      DSL_FIO_G997_LINE_STATUS_PER_BAND_GET, (int)&bandStatus);
   if (nRet == DSL_SUCCESS)
   {
      nBufSize = sizeof(sSnrMpbList);
      for (unsigned int i = 0; i < DSL_G997_MAX_NUMBER_OF_BANDS; ++i)
      {
         DSL_CPE_snprintf(sBuf, sizeof(sBuf), ",%d", bandStatus.data.SNR[i]);
         /* select name from the buf and exclude leading comma */
         sSnrMpbName = sBuf + ((nOffset) ? 0 : 1);
         nBufSize =
            (sizeof(sSnrMpbList) > nOffset) ?
            sizeof(sSnrMpbList) - nOffset : 0;
         cpe_control_strncpy_s(sSnrMpbList + nOffset, nBufSize,
            sSnrMpbName, nBufSize);
         nOffset += strlen(sSnrMpbName);
      }

      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_string(sSnrMpbList);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

/*
   This function collects all data provided by
   DSL_FIO_G997_LINE_INVENTORY_GET
   ioctl for all devices
*/
static DSL_Error_t DSL_CPE_G997LineInventoryGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   DSL_uint32_t *pData)
{
   DSL_Error_t nRet = DSL_SUCCESS, nErrorCode = DSL_SUCCESS;
   static DSL_G997_LineInventory_t
      g997LineInventoryNe[DSL_CPE_MAX_DSL_ENTITIES] = { 0 };
   static DSL_G997_LineInventory_t
      g997LineInventoryFe[DSL_CPE_MAX_DSL_ENTITIES] = { 0 };

   if (pData == DSL_NULL)
   {
      return DSL_ERROR;
   }

   /* avoid sending IOCTL many times in one status update iteration */
   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_G997_LINE_INVENTORY_GET))
   {
      for (unsigned int nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         g997LineInventoryNe[nDev].nDirection = DSL_NEAR_END;
         nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
            DSL_FIO_G997_LINE_INVENTORY_GET, (int)&g997LineInventoryNe[nDev]);

         if (nRet < DSL_SUCCESS ||
            g997LineInventoryNe[nDev].accessCtl.nReturn < DSL_SUCCESS)
         {
            DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
               "ERROR - DSL_FIO_G997_LINE_INVENTORY_GET(NE) ioctl "
               "failed (device<%d>, nRet=%d, accessCtrl=%d)!"DSL_CPE_CRLF,
               nDev, nRet, g997LineInventoryNe[nDev].accessCtl.nReturn));

            nErrorCode = DSL_ERROR;
         }

         g997LineInventoryFe[nDev].nDirection = DSL_FAR_END;
         nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
            DSL_FIO_G997_LINE_INVENTORY_GET, (int)&g997LineInventoryFe[nDev]);

         if (nRet < DSL_SUCCESS ||
            g997LineInventoryFe[nDev].accessCtl.nReturn < DSL_SUCCESS)
         {
            DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
               "ERROR - DSL_FIO_G997_LINE_INVENTORY_GET(FE) ioctl "
               "failed (device<%d>, nRet=%d, accessCtrl=%d)!"DSL_CPE_CRLF,
               nDev, nRet, g997LineInventoryFe[nDev].accessCtl.nReturn));

            nErrorCode = DSL_ERROR;
         }
      }

      /* set ioctl flag only if all IOCTLs succeeded */
      if (nErrorCode == DSL_SUCCESS)
      {
         DSL_CPE_IoctlFlagSet(DSL_FIO_G997_LINE_INVENTORY_GET);
      }
   }

   /* get data for the given node */
   switch (eNodeIdx)
   {
   case DSL_STATUS_LINE_0_XTUR_VENDOR:
      pData[0] = g997LineInventoryNe[/*nDevice*/0].data.G994VendorID[2];
      pData[1] = g997LineInventoryNe[/*nDevice*/0].data.G994VendorID[3];
      pData[2] = g997LineInventoryNe[/*nDevice*/0].data.G994VendorID[4];
      pData[3] = g997LineInventoryNe[/*nDevice*/0].data.G994VendorID[5];
      break;
   case DSL_STATUS_LINE_1_XTUR_VENDOR:
      pData[0] = g997LineInventoryNe[/*nDevice*/1].data.G994VendorID[2];
      pData[1] = g997LineInventoryNe[/*nDevice*/1].data.G994VendorID[3];
      pData[2] = g997LineInventoryNe[/*nDevice*/1].data.G994VendorID[4];
      pData[3] = g997LineInventoryNe[/*nDevice*/1].data.G994VendorID[5];
      break;
   case DSL_STATUS_LINE_0_XTUR_COUNTRY:
      pData[0] = g997LineInventoryNe[/*nDevice*/0].data.G994VendorID[0];
      pData[1] = g997LineInventoryNe[/*nDevice*/0].data.G994VendorID[1];
      break;
   case DSL_STATUS_LINE_1_XTUR_COUNTRY:
      pData[0] = g997LineInventoryNe[/*nDevice*/1].data.G994VendorID[0];
      pData[1] = g997LineInventoryNe[/*nDevice*/1].data.G994VendorID[1];
      break;
   case DSL_STATUS_LINE_0_XTUC_VENDOR:
      pData[0] = g997LineInventoryFe[/*nDevice*/0].data.G994VendorID[2];
      pData[1] = g997LineInventoryFe[/*nDevice*/0].data.G994VendorID[3];
      pData[2] = g997LineInventoryFe[/*nDevice*/0].data.G994VendorID[4];
      pData[3] = g997LineInventoryFe[/*nDevice*/0].data.G994VendorID[5];
      break;
   case DSL_STATUS_LINE_1_XTUC_VENDOR:
      pData[0] = g997LineInventoryFe[/*nDevice*/1].data.G994VendorID[2];
      pData[1] = g997LineInventoryFe[/*nDevice*/1].data.G994VendorID[3];
      pData[2] = g997LineInventoryFe[/*nDevice*/1].data.G994VendorID[4];
      pData[3] = g997LineInventoryFe[/*nDevice*/1].data.G994VendorID[5];
      break;
   case DSL_STATUS_LINE_0_XTUC_COUNTRY:
      pData[0] = g997LineInventoryFe[/*nDevice*/0].data.G994VendorID[0];
      pData[1] = g997LineInventoryFe[/*nDevice*/0].data.G994VendorID[1];
      break;
   case DSL_STATUS_LINE_1_XTUC_COUNTRY:
      pData[0] = g997LineInventoryFe[/*nDevice*/1].data.G994VendorID[0];
      pData[1] = g997LineInventoryFe[/*nDevice*/1].data.G994VendorID[1];
      break;
   default:
      DSL_CCA_DEBUG(DSL_CCA_DBG_WRN, (DSL_CPE_PREFIX
         "WARNING - StatusParser, DSL_CPE_G997LineInventoryGet: "
         "unknown node!"
         DSL_CPE_CRLF));
      break;
   }

   return nErrorCode;
}

static DSL_void_t DSL_CPE_JsonXturVendorUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_uint32_t nXturVendor[MAX_LEN_VENDOR_ID] = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_G997LineInventoryGet(
      pContext, eNodeIdx, nXturVendor) == DSL_SUCCESS)
   {
      /* create json array object */
      pNewObj = json_object_new_array();

      /* update json object */
      for (unsigned int i = 0; i < MAX_LEN_VENDOR_ID; i++)
      {
         json_object_array_add(pNewObj,
            json_object_new_int(nXturVendor[i]));
      }

      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonXturCountryUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_uint32_t nXturCountry[MAX_LEN_COUNTRY_ID] = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_G997LineInventoryGet(
      pContext, eNodeIdx, nXturCountry) == DSL_SUCCESS)
   {
      /* create json array object */
      pNewObj = json_object_new_array();

      /* update json object */
      for (unsigned int i = 0; i < MAX_LEN_COUNTRY_ID; i++)
      {
         json_object_array_add(pNewObj,
            json_object_new_int(nXturCountry[i]));
      }

      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonXtucVendorUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_uint32_t nXtucVendor[MAX_LEN_VENDOR_ID] = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_G997LineInventoryGet(
      pContext, eNodeIdx, nXtucVendor) == DSL_SUCCESS)
   {
      /* create json array object */
      pNewObj = json_object_new_array();

      /* update json object */
      for (unsigned int i = 0; i < MAX_LEN_VENDOR_ID; i++)
      {
         json_object_array_add(pNewObj,
            json_object_new_int(nXtucVendor[i]));
      }

      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonXtucCountryUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_uint32_t nXtucCountry[MAX_LEN_COUNTRY_ID] = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_G997LineInventoryGet(
      pContext, eNodeIdx, nXtucCountry) == DSL_SUCCESS)
   {
      /* create json array object */
      pNewObj = json_object_new_array();

      /* update json object */
      for (unsigned int i = 0; i < MAX_LEN_COUNTRY_ID; i++)
      {
         json_object_array_add(pNewObj,
            json_object_new_int(nXtucCountry[i]));
      }

      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonXturAnsiStdUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_Error_t nRet = DSL_SUCCESS;
   DSL_T1413RevisionStatus_t revisionStatus = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   nRet = DSL_CPE_Ioctl(pContext->fd[nDevice],
      DSL_FIO_T1413_XTUR_REVISION_GET, (int)&revisionStatus);
   if (nRet == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(revisionStatus.data.nT1413RevisionNum);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonXturAnsiRevUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_Error_t nRet = DSL_SUCCESS;
   DSL_T1413VendorRevisionStatus_t vendorRevisionStatus = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   nRet = DSL_CPE_Ioctl(pContext->fd[nDevice],
      DSL_FIO_T1413_XTUR_VENDOR_REVISION_GET, (int)&vendorRevisionStatus);
   if (nRet == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(
         vendorRevisionStatus.data.nT1413VendorRevisionNum);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonXtucAnsiStdUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_Error_t nRet = DSL_SUCCESS;
   DSL_T1413RevisionStatus_t revisionStatus = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   nRet = DSL_CPE_Ioctl(pContext->fd[nDevice],
      DSL_FIO_T1413_XTUO_REVISION_GET, (int)&revisionStatus);
   if (nRet == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(revisionStatus.data.nT1413RevisionNum);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonXtucAnsiRevUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_Error_t nRet = DSL_SUCCESS;
   DSL_T1413VendorRevisionStatus_t vendorRevisionStatus = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   nRet = DSL_CPE_Ioctl(pContext->fd[nDevice],
      DSL_FIO_T1413_XTUO_VENDOR_REVISION_GET, (int)&vendorRevisionStatus);
   if (nRet == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(
         vendorRevisionStatus.data.nT1413VendorRevisionNum);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonUpboklerPbUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_Error_t nRet = DSL_SUCCESS;
   DSL_uint32_t nOffset = 0, nBufSize = 0;
   DSL_char_t sUpboklerPb[26] = "", sBuf[32] = "";
   DSL_char_t *sUpboklerPbName = DSL_NULL;
   DSL_G997_UsPowerBackOffStatus_t powerBackOffStatus = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   nRet = DSL_CPE_Ioctl(pContext->fd[nDevice],
      DSL_FIO_G997_US_POWER_BACK_OFF_STATUS_GET, (int)&powerBackOffStatus);
   if (nRet == DSL_SUCCESS)
   {
      nBufSize = sizeof(sUpboklerPb);
      for (unsigned int i = 0; i < DSL_G997_MAX_NUMBER_OF_BANDS; ++i)
      {
         DSL_CPE_snprintf(sBuf, sizeof(sBuf), ",%hu",
            powerBackOffStatus.data.nKl0EstimRPb[i]);
         sBuf[5] = '\0';
         /* select name from the buf and exclude leading comma */
         sUpboklerPbName = sBuf + ((nOffset) ? 0 : 1);
         nBufSize =
            (sizeof(sUpboklerPb) > nOffset) ?
            sizeof(sUpboklerPb) - nOffset : 0;
         cpe_control_strncpy_s(sUpboklerPb + nOffset, nBufSize,
            sUpboklerPbName, nBufSize);
         nOffset += strlen(sUpboklerPbName);
      }

      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_string(sUpboklerPb);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonRxthrshDsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_Error_t nRet = DSL_SUCCESS;
   DSL_uint32_t nOffset = 0, nBufSize = 0;
   DSL_char_t sRxthrshDs[26] = "", sBuf[32] = "";
   DSL_char_t *sRxthrshDsName = DSL_NULL;
   DSL_G997_UsPowerBackOffStatus_t powerBackOffStatus = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   nRet = DSL_CPE_Ioctl(pContext->fd[nDevice],
      DSL_FIO_G997_US_POWER_BACK_OFF_STATUS_GET, (int)&powerBackOffStatus);
   if (nRet == DSL_SUCCESS)
   {
      nBufSize = sizeof(sRxthrshDs);
      for (unsigned int i = 0; i < DSL_G997_MAX_NUMBER_OF_BANDS; ++i)
      {
         DSL_CPE_snprintf(sBuf, sizeof(sBuf), ",%hu",
            powerBackOffStatus.data.nKl0EstimOPb[i]);
         sBuf[5] = '\0';
         /* select name from the buf and exclude leading comma */
         sRxthrshDsName = sBuf + ((nOffset) ? 0 : 1);
         nBufSize =
            (sizeof(sRxthrshDs) > nOffset) ?
            sizeof(sRxthrshDs) - nOffset : 0;
         cpe_control_strncpy_s(sRxthrshDs + nOffset, nBufSize,
            sRxthrshDsName, nBufSize);
         nOffset += strlen(sRxthrshDsName);
      }

      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_string(sRxthrshDs);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonActRaModeUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_AccessDir_t nDirection,
   const DSL_char_t* jsonPath)
{
   DSL_Error_t nRet = DSL_SUCCESS;
   DSL_G997_RateAdaptationStatus_t rateAdaptationStatus = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   rateAdaptationStatus.nDirection = nDirection;
   nRet = DSL_CPE_Ioctl(pContext->fd[nDevice],
      DSL_FIO_G997_RATE_ADAPTATION_STATUS_GET, (int)&rateAdaptationStatus);
   if (nRet == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(rateAdaptationStatus.data.RA_MODE);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonSnrMrocUpdate(
   const DSL_char_t* jsonPath)
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   /* update json object */
   pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
   pNewObj =
      json_object_new_int(0); /* fixed value */
   json_object_object_add(pParentObj,
      DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
}

/*
   This function collects all data provided by
   DSL_FIO_PM_CHANNEL_COUNTERS_TOTAL_GET
   IOCtl from all available devices and XTU directions.
*/
static DSL_Error_t DSL_CPE_PMCountersTotalGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nDirection,
   DSL_PM_ChannelCountersTotal_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS, nRet = DSL_SUCCESS;
   DSL_uint32_t nDev = 0;
   DSL_XTUDir_t nDir = DSL_NEAR_END;
   static DSL_PM_ChannelCountersTotal_t
      sData[DSL_CPE_MAX_DSL_ENTITIES][(DSL_FAR_END + 1)] = { 0 };

   if (pData == DSL_NULL ||
      nDevice >= DSL_CPE_DSL_ENTITIES ||
      nDirection == DSL_XTUDIR_NA)
   {
      return DSL_ERROR;
   }

   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_PM_CHANNEL_COUNTERS_TOTAL_GET))
   {
      for (nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         for (nDir = DSL_NEAR_END; nDir <= DSL_FAR_END; ++nDir)
         {
            sData[nDev][nDir].nDirection = nDir;

            nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
               DSL_FIO_PM_CHANNEL_COUNTERS_TOTAL_GET,
               (int)&sData[nDev][nDir]);

            if (nRet < DSL_SUCCESS ||
               sData[nDev][nDir].accessCtl.nReturn != DSL_SUCCESS)
            {
               DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
                  "ERROR - DSL_FIO_PM_CHANNEL_COUNTERS_TOTAL_GET ioctl failed "
                  "(device<%d>, nRet=%d, accessCtrl=%d)!" DSL_CPE_CRLF,
                  nDev, nRet, sData[nDev][nDir].accessCtl.nReturn));

               nErrCode = DSL_ERROR;
            }
         }
      }

      /* set ioctl flag only if all IOCTLs succeeded */
      if (nErrCode == DSL_SUCCESS)
      {
         DSL_CPE_IoctlFlagSet(DSL_FIO_PM_CHANNEL_COUNTERS_TOTAL_GET);
      }
   }

   *pData = sData[nDevice][nDirection];

   return nErrCode;
}

static DSL_void_t DSL_CPE_JsonTotalStartUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_PM_ChannelCountersTotal_t data = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_PMCountersTotalGet(
         pContext, nDevice, DSL_NEAR_END, &data) == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(data.total.nElapsedTime);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

/*
   This function collects all data provided by
   DSL_FIO_PM_CHANNEL_COUNTERS_SHOWTIME_GET
   IOCtl from all available devices, history intervals and XTU directions.
*/
static DSL_Error_t DSL_CPE_PMCountersShowtimeGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nDirection,
   const DSL_uint32_t nHistoryInterval,
   DSL_PM_ChannelCounters_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS, nRet = DSL_SUCCESS;
   DSL_uint32_t nDev = 0;
   DSL_XTUDir_t nDir = DSL_NEAR_END;
   DSL_uint32_t nHistInt = 0;
   static DSL_PM_ChannelCounters_t
      sData[DSL_CPE_MAX_DSL_ENTITIES]
      [(DSL_FAR_END + 1)]
   [DSL_PM_HISTORY_INTERVALS_NUM] = { 0 };

   if (pData == DSL_NULL ||
      nDevice >= DSL_CPE_DSL_ENTITIES ||
      nDirection == DSL_XTUDIR_NA ||
      nHistoryInterval >= DSL_PM_HISTORY_INTERVALS_NUM)
   {
      return DSL_ERROR;
   }

   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_PM_CHANNEL_COUNTERS_SHOWTIME_GET))
   {
      for (nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         for (nDir = DSL_NEAR_END; nDir <= DSL_FAR_END; ++nDir)
         {
            for (nHistInt = 0; nHistInt < DSL_PM_HISTORY_INTERVALS_NUM; ++nHistInt)
            {
               sData[nDev][nDir][nHistInt].nDirection = nDir;
               sData[nDev][nDir][nHistInt].nHistoryInterval = nHistInt;

               nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
                  DSL_FIO_PM_CHANNEL_COUNTERS_SHOWTIME_GET,
                  (int)&sData[nDev][nDir][nHistInt]);

               if (nRet < DSL_SUCCESS ||
                  sData[nDev][nDir][nHistInt].accessCtl.nReturn != DSL_SUCCESS)
               {
                  DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
                     "ERROR - DSL_FIO_PM_CHANNEL_COUNTERS_SHOWTIME_GET "
                     "ioctl failed (device<%d>, direction<%s>, "
                     "historyInterval<%d>, nRet=%d, "
                     "accessCtrl=%d)!" DSL_CPE_CRLF,
                     nDev, (nDir == DSL_NEAR_END) ? "near end" : "far end",
                     nHistInt, nRet,
                     sData[nDev][nDir][nHistInt].accessCtl.nReturn));
               }
            }
         }
      }

      /* no check for IOCTL failure here - it may happen that IOCTL failes
         due to request for nHistoryInterval that was not yet reached */
      DSL_CPE_IoctlFlagSet(DSL_FIO_PM_CHANNEL_COUNTERS_SHOWTIME_GET);
   }

   *pData = sData[nDevice][nDirection][nHistoryInterval];

   return nErrCode;
}

static DSL_void_t DSL_CPE_JsonShowtimeStartUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_PM_ChannelCounters_t data = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_PMCountersShowtimeGet(
         pContext, nDevice, DSL_NEAR_END, /*HistoryInterval*/ 0, &data)
      == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(data.interval.nElapsedTime);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonLastShowtimeStartUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_PM_ChannelCounters_t data = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_PMCountersShowtimeGet(
      pContext, nDevice, DSL_NEAR_END, /*HistoryInterval*/ 1, &data)
      == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(data.interval.nElapsedTime);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonCurrDayStartUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_Error_t nRet = DSL_SUCCESS;
   DSL_PM_LineInitCounters_t initCounters = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   nRet = DSL_CPE_Ioctl(pContext->fd[nDevice],
      DSL_FIO_PM_LINE_INIT_COUNTERS_1DAY_GET, (int)&initCounters);
   if (nRet == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(initCounters.interval.nElapsedTime);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

/*
   This function collects all data provided by
   DSL_FIO_PM_CHANNEL_COUNTERS_15MIN_GET
   IOCtl from all available devices and XTU directions.
*/
static DSL_Error_t DSL_CPE_PMCounters15MinGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nDirection,
   DSL_PM_ChannelCounters_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS, nRet = DSL_SUCCESS;
   DSL_uint32_t nDev = 0;
   DSL_XTUDir_t nDir = DSL_NEAR_END;
   static DSL_PM_ChannelCounters_t
      sData[DSL_CPE_MAX_DSL_ENTITIES][(DSL_FAR_END + 1)] = { 0 };

   if (pData == DSL_NULL ||
      nDevice >= DSL_CPE_DSL_ENTITIES ||
      nDirection == DSL_XTUDIR_NA)
   {
      return DSL_ERROR;
   }

   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_PM_CHANNEL_COUNTERS_15MIN_GET))
   {
      for (nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         for (nDir = DSL_NEAR_END; nDir <= DSL_FAR_END; ++nDir)
         {
            sData[nDev][nDir].nHistoryInterval = 0;
            sData[nDev][nDir].nDirection = nDir;

            nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
               DSL_FIO_PM_CHANNEL_COUNTERS_15MIN_GET,
               (int)&sData[nDev][nDir]);

            if (nRet < DSL_SUCCESS ||
               sData[nDev][nDir].accessCtl.nReturn != DSL_SUCCESS)
            {
               DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
                  "ERROR - DSL_FIO_PM_CHANNEL_COUNTERS_15MIN_GET ioctl failed "
                  "(device<%d>, nRet=%d, accessCtrl=%d)!" DSL_CPE_CRLF,
                  nDev, nRet, sData[nDev][nDir].accessCtl.nReturn));

               nErrCode = DSL_ERROR;
            }
         }
      }

      /* set ioctl flag only if all IOCTLs succeeded */
      if (nErrCode == DSL_SUCCESS)
      {
         DSL_CPE_IoctlFlagSet(DSL_FIO_PM_CHANNEL_COUNTERS_15MIN_GET);
      }
   }

   *pData = sData[nDevice][nDirection];

   return nErrCode;
}

static DSL_void_t DSL_CPE_JsonQuarterHourStartUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_PM_ChannelCounters_t data = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_PMCounters15MinGet(
         pContext, nDevice, DSL_NEAR_END, &data)
      == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(data.interval.nElapsedTime);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

/*
   This function collects all data provided by
   DSL_FIO_PM_LINE_SEC_COUNTERS_TOTAL_GET
   ioctl for all devices
*/
static DSL_Error_t DSL_CPE_LineSecCountersTotalGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   DSL_int_t *pData)
{
   DSL_Error_t nErrorCode = DSL_SUCCESS, nRet = DSL_SUCCESS;
   static DSL_PM_LineSecCountersTotal_t
      sData[DSL_CPE_MAX_DSL_ENTITIES] = { 0 };

   if (pData == DSL_NULL)
   {
      return DSL_ERROR;
   }

   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_PM_LINE_SEC_COUNTERS_TOTAL_GET))
   {
      for (DSL_uint32_t nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         sData[nDev].nDirection = DSL_NEAR_END;

         nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
            DSL_FIO_PM_LINE_SEC_COUNTERS_TOTAL_GET,
            (int)&sData[nDev]);

         if (nRet < DSL_SUCCESS ||
            sData[nDev].accessCtl.nReturn != DSL_SUCCESS)
         {
            DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
               "ERROR - DSL_FIO_PM_LINE_SEC_COUNTERS_TOTAL_GET ioctl failed "
               "(device<%d>, nRet=%d, accessCtrl=%d)!" DSL_CPE_CRLF,
               nDev, nRet, sData[nDev].accessCtl.nReturn));

            nErrorCode = DSL_ERROR;
         }
      }

      /* set ioctl flag only if all IOCTLs succeeded */
      if (nErrorCode == DSL_SUCCESS)
      {
         DSL_CPE_IoctlFlagSet(DSL_FIO_PM_LINE_SEC_COUNTERS_TOTAL_GET);
      }
   }

   /* provide data of corresponding IOCtl request */
   switch (eNodeIdx)
   {
   case DSL_STATS_LINE_0_TOTAL_ERR_SECS:
      *pData = sData[/*nDevice*/0].data.nES;
      break;
   case DSL_STATS_LINE_0_TOTAL_SEV_ERR_SECS:
      *pData = sData[/*nDevice*/0].data.nSES;
      break;
   case DSL_STATS_LINE_1_TOTAL_ERR_SECS:
      *pData = sData[/*nDevice*/1].data.nES;
      break;
   case DSL_STATS_LINE_1_TOTAL_SEV_ERR_SECS:
      *pData = sData[/*nDevice*/1].data.nSES;
      break;
   default:
      break;
   }

   return nErrorCode;
}

static DSL_void_t DSL_CPE_JsonTotalErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_int_t nTotalErrSecs = 0;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_LineSecCountersTotalGet(
         pContext, eNodeIdx, &nTotalErrSecs) == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(nTotalErrSecs);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonTotalSevErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_int_t nTotalSevErrSecs = 0;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_LineSecCountersTotalGet(
      pContext, eNodeIdx, &nTotalSevErrSecs) == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(nTotalSevErrSecs);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

/*
   This function collects all data provided by
   DSL_FIO_PM_LINE_SEC_COUNTERS_SHOWTIME_GET
   ioctl for all devices
*/
static DSL_Error_t DSL_CPE_LineSecCountersShowtimeGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   DSL_int_t *pData)
{
   DSL_Error_t nErrorCode = DSL_SUCCESS, nRet = DSL_SUCCESS;
   static DSL_PM_LineSecCounters_t
      sData[DSL_CPE_MAX_DSL_ENTITIES][DSL_PM_HISTORY_INTERVALS_NUM] = { 0 };

   if (pData == DSL_NULL)
   {
      return DSL_ERROR;
   }

   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_PM_LINE_SEC_COUNTERS_SHOWTIME_GET))
   {
      for (DSL_uint32_t nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         for (DSL_uint32_t nHistInt = 0;
            nHistInt < DSL_PM_HISTORY_INTERVALS_NUM; ++nHistInt)
         {
            sData[nDev][nHistInt].nDirection = DSL_NEAR_END;
            sData[nDev][nHistInt].nHistoryInterval = nHistInt;

            nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
               DSL_FIO_PM_LINE_SEC_COUNTERS_SHOWTIME_GET,
               (int)&sData[nDev][nHistInt]);

            if (nRet < DSL_SUCCESS ||
               sData[nDev][nHistInt].accessCtl.nReturn != DSL_SUCCESS)
            {
               DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
                  "ERROR - DSL_FIO_PM_LINE_SEC_COUNTERS_SHOWTIME_GET "
                  "ioctl failed (device<%d>, historyInterval<%d>, "
                  "nRet=%d, accessCtrl=%d)!" DSL_CPE_CRLF,
                  nDev, nHistInt, nRet,
                  sData[nDev][nHistInt].accessCtl.nReturn));
            }
         }
      }

      /* no check for IOCTL failure here - it may happen that IOCTL failes
         due to request for nHistoryInterval that was not yet reached */
      DSL_CPE_IoctlFlagSet(DSL_FIO_PM_LINE_SEC_COUNTERS_SHOWTIME_GET);
   }

   /* provide data of corresponding IOCtl request */
   switch (eNodeIdx)
   {
   case DSL_STATS_LINE_0_SHOWTIME_ERR_SECS:
      *pData = sData[/*nDevice*/0][/*HistoryInterval*/0].data.nES;
      break;
   case DSL_STATS_LINE_0_SHOWTIME_SEV_ERR_SECS:
      *pData = sData[/*nDevice*/0][/*HistoryInterval*/0].data.nSES;
      break;
   case DSL_STATS_LINE_0_LAST_SHOWTIME_ERR_SECS:
      *pData = sData[/*nDevice*/0][/*HistoryInterval*/1].data.nES;
      break;
   case DSL_STATS_LINE_0_LAST_SHOWTIME_SEV_ERR_SECS:
      *pData = sData[/*nDevice*/0][/*HistoryInterval*/1].data.nSES;
      break;
   case DSL_STATS_LINE_1_SHOWTIME_ERR_SECS:
      *pData = sData[/*nDevice*/1][/*HistoryInterval*/0].data.nES;
      break;
   case DSL_STATS_LINE_1_SHOWTIME_SEV_ERR_SECS:
      *pData = sData[/*nDevice*/1][/*HistoryInterval*/0].data.nSES;
      break;
   case DSL_STATS_LINE_1_LAST_SHOWTIME_ERR_SECS:
      *pData = sData[/*nDevice*/1][/*HistoryInterval*/1].data.nES;
      break;
   case DSL_STATS_LINE_1_LAST_SHOWTIME_SEV_ERR_SECS:
      *pData = sData[/*nDevice*/1][/*HistoryInterval*/1].data.nSES;
      break;
   default:
      break;
   }

   return nErrorCode;
}

static DSL_void_t DSL_CPE_JsonShowtimeErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_int_t nShowtimeErrSecs = 0;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_LineSecCountersShowtimeGet(
         pContext, eNodeIdx, &nShowtimeErrSecs) == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(nShowtimeErrSecs);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonShowtimeSevErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_int_t nShowtimeSevErrSecs = 0;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_LineSecCountersShowtimeGet(
      pContext, eNodeIdx, &nShowtimeSevErrSecs) == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(nShowtimeSevErrSecs);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonLastShowtimeErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_int_t nLastShowtimeErrSecs = 0;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_LineSecCountersShowtimeGet(
         pContext, eNodeIdx, &nLastShowtimeErrSecs) == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(nLastShowtimeErrSecs);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonLastShowtimeSevErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_int_t nLastShowtimeSevErrSecs = 0;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_LineSecCountersShowtimeGet(
         pContext, eNodeIdx, &nLastShowtimeSevErrSecs) == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(nLastShowtimeSevErrSecs);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

/*
   This function collects all data provided by
   DSL_FIO_PM_LINE_SEC_COUNTERS_1DAY_GET
   ioctl for all devices
*/
static DSL_Error_t DSL_CPE_LineSecCounters1DayGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   DSL_int_t *pData)
{
   DSL_Error_t nErrorCode = DSL_SUCCESS, nRet = DSL_SUCCESS;
   static DSL_PM_LineSecCounters_t
      sData[DSL_CPE_MAX_DSL_ENTITIES] = { 0 };

   if (pData == DSL_NULL)
   {
      return DSL_ERROR;
   }

   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_PM_LINE_SEC_COUNTERS_1DAY_GET))
   {
      for (DSL_uint32_t nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         sData[nDev].nDirection = DSL_NEAR_END;
         sData[nDev].nHistoryInterval = 0;

         nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
            DSL_FIO_PM_LINE_SEC_COUNTERS_1DAY_GET,
            (int)&sData[nDev]);

         if (nRet < DSL_SUCCESS ||
            sData[nDev].accessCtl.nReturn != DSL_SUCCESS)
         {
            DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
               "ERROR - DSL_FIO_PM_LINE_SEC_COUNTERS_1DAY_GET ioctl failed "
               "(device<%d>, nRet=%d, accessCtrl=%d)!" DSL_CPE_CRLF,
               nDev, nRet, sData[nDev].accessCtl.nReturn));

            nErrorCode = DSL_ERROR;
         }
      }

      /* set ioctl flag only if all IOCTLs succeeded */
      if (nErrorCode == DSL_SUCCESS)
      {
         DSL_CPE_IoctlFlagSet(DSL_FIO_PM_LINE_SEC_COUNTERS_1DAY_GET);
      }
   }

   /* provide data of corresponding IOCtl request */
   switch (eNodeIdx)
   {
   case DSL_STATS_LINE_0_CURRENT_DAY_ERR_SECS:
      *pData = sData[/*nDevice*/0].data.nES;
      break;
   case DSL_STATS_LINE_0_CURRENT_DAY_SEV_ERR_SECS:
      *pData = sData[/*nDevice*/0].data.nSES;
      break;
   case DSL_STATS_LINE_1_CURRENT_DAY_ERR_SECS:
      *pData = sData[/*nDevice*/1].data.nES;
      break;
   case DSL_STATS_LINE_1_CURRENT_DAY_SEV_ERR_SECS:
      *pData = sData[/*nDevice*/1].data.nSES;
      break;
   default:
      break;
   }

   return nErrorCode;
}

static DSL_void_t DSL_CPE_JsonCurrDayErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_int_t nCurrDayErrSecs = 0;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_LineSecCounters1DayGet(
         pContext, eNodeIdx, &nCurrDayErrSecs) == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(nCurrDayErrSecs);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonCurrDaySevErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_int_t nCurrDaySevErrSecs = 0;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_LineSecCounters1DayGet(
         pContext, eNodeIdx, &nCurrDaySevErrSecs) == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(nCurrDaySevErrSecs);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

/*
   This function collects all data provided by
   DSL_FIO_PM_LINE_SEC_COUNTERS_15MIN_GET
   ioctl for all devices
*/
static DSL_Error_t DSL_CPE_LineSecCounters15MinGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   DSL_int_t *pData)
{
   DSL_Error_t nErrorCode = DSL_SUCCESS, nRet = DSL_SUCCESS;
   static DSL_PM_LineSecCounters_t
      sData[DSL_CPE_MAX_DSL_ENTITIES] = { 0 };

   if (pData == DSL_NULL)
   {
      return DSL_ERROR;
   }

   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_PM_LINE_SEC_COUNTERS_15MIN_GET))
   {
      for (DSL_uint32_t nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         sData[nDev].nDirection = DSL_NEAR_END;
         sData[nDev].nHistoryInterval = 0;

         nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
            DSL_FIO_PM_LINE_SEC_COUNTERS_15MIN_GET,
            (int)&sData[nDev]);

         if (nRet < DSL_SUCCESS ||
            sData[nDev].accessCtl.nReturn != DSL_SUCCESS)
         {
            DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
               "ERROR - DSL_FIO_PM_LINE_SEC_COUNTERS_15MIN_GET ioctl failed "
               "(device<%d>, nRet=%d, accessCtrl=%d)!" DSL_CPE_CRLF,
               nDev, nRet, sData[nDev].accessCtl.nReturn));

            nErrorCode = DSL_ERROR;
         }
      }

      /* set ioctl flag only if all IOCTLs succeeded */
      if (nErrorCode == DSL_SUCCESS)
      {
         DSL_CPE_IoctlFlagSet(DSL_FIO_PM_LINE_SEC_COUNTERS_15MIN_GET);
      }
   }

   /* provide data of corresponding IOCtl request */
   switch (eNodeIdx)
   {
   case DSL_STATS_LINE_0_QUARTER_HOUR_ERR_SECS:
      *pData = sData[/*nDevice*/0].data.nES;
      break;
   case DSL_STATS_LINE_0_QUARTER_HOUR_SEV_ERR_SECS:
      *pData = sData[/*nDevice*/0].data.nSES;
      break;
   case DSL_STATS_LINE_1_QUARTER_HOUR_ERR_SECS:
      *pData = sData[/*nDevice*/1].data.nES;
      break;
   case DSL_STATS_LINE_1_QUARTER_HOUR_SEV_ERR_SECS:
      *pData = sData[/*nDevice*/1].data.nSES;
      break;
   default:
      break;
   }

   return nErrorCode;
}

static DSL_void_t DSL_CPE_JsonQuarterHourErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_int_t nQuarterErrSecs = 0;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_LineSecCounters15MinGet(
         pContext, eNodeIdx, &nQuarterErrSecs) == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(nQuarterErrSecs);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

static DSL_void_t DSL_CPE_JsonQuarterHourSevErrSecsUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_int_t nQuarterSevErrSecs = 0;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_LineSecCounters15MinGet(
      pContext, eNodeIdx, &nQuarterSevErrSecs) == DSL_SUCCESS)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      pNewObj = json_object_new_int(nQuarterSevErrSecs);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

/* --- Channel section ---*/

static DSL_void_t DSL_CPE_JsonChannelDataRatesUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_AccessDir_t nDirection,
   const DSL_char_t* jsonPath)
{
   DSL_Error_t nErrCode = DSL_SUCCESS;
   DSL_G997_ChannelStatus_t channelStatus = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;
   DSL_uint32_t nActualDataRate = 0;
   const DSL_uint8_t nChannel = 0;

   channelStatus.nChannel = nChannel;
   channelStatus.nDirection = nDirection;

   nErrCode = DSL_CPE_Ioctl(pContext->fd[nDevice],
      DSL_FIO_G997_CHANNEL_STATUS_GET, (int)&channelStatus);
   if (nErrCode == DSL_SUCCESS)
   {
      nActualDataRate = channelStatus.data.ActualDataRate;
   }

   /* update json object */
   pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
   pNewObj = json_object_new_int(nActualDataRate);
   json_object_object_add(pParentObj,
      DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
}

static DSL_void_t DSL_CPE_JsonChannelLinkEncapUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_Error_t nRet = DSL_SUCCESS;
   DSL_SystemInterfaceStatus_t sysIfData;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;
   DSL_char_t linkEncapUsed[40];
   DSL_uint8_t XTSE[DSL_G997_NUM_XTSE_OCTETS] = { 0 };
   DSL_DslModeSelection_t dslMode = DSL_MODE_NA;

   memset(linkEncapUsed, 0, sizeof(linkEncapUsed));
   if (DSL_CPE_SystemEnablingStatusGet(
         pContext, nDevice, XTSE) == DSL_SUCCESS)
   {
      if (XTSE[7])
      {
         dslMode = DSL_MODE_VDSL;
      }
      else
      {
         dslMode = DSL_MODE_ADSL;
      }

      memset(&sysIfData, 0x00, sizeof(DSL_SystemInterfaceStatus_t));
      nRet = DSL_CPE_Ioctl(
         pContext->fd[nDevice], DSL_FIO_SYSTEM_INTERFACE_STATUS_GET,
         (int)&sysIfData);
      if (nRet == DSL_SUCCESS &&
            sysIfData.accessCtl.nReturn == DSL_SUCCESS)
      {
         switch (sysIfData.data.nTcLayer)
         {
         case DSL_TC_ATM:
            if (dslMode == DSL_MODE_ADSL)
            {
               cpe_control_strncpy_s(linkEncapUsed, sizeof(linkEncapUsed),
                  "G.992.3_Annex_K_ATM", sizeof("G.992.3_Annex_K_ATM"));
            }
            else
            {
               cpe_control_strncpy_s(linkEncapUsed, sizeof(linkEncapUsed),
                  "G.993.2_Annex_K_ATM", sizeof("G.993.2_Annex_K_ATM"));
            }
            break;
         case DSL_TC_EFM:
         case DSL_TC_EFM_FORCED:
            if (dslMode == DSL_MODE_ADSL)
            {
               cpe_control_strncpy_s(linkEncapUsed, sizeof(linkEncapUsed),
                  "G.992.3_Annex_K_PTM", sizeof("G.992.3_Annex_K_PTM"));
            }
            else
            {
               cpe_control_strncpy_s(linkEncapUsed, sizeof(linkEncapUsed),
                  "G.993.2_Annex_K_PTM", sizeof("G.993.2_Annex_K_PTM"));
            }
            break;
         case DSL_TC_AUTO:
            cpe_control_strncpy_s(linkEncapUsed, sizeof(linkEncapUsed),
               "G.994.1 (Auto)", sizeof("G.994.1 (Auto)"));
            break;
         default:
            break;
         }

         /* update json object */
         pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
         pNewObj = json_object_new_string(linkEncapUsed);
         json_object_object_add(pParentObj,
            DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
      }
   }

}

/*
   This function collects all data provided by
   DSL_FIO_SYSTEM_INTERFACE_CONFIG_GET
   IOCtl from all available devices and DSL modes.
*/
static DSL_Error_t DSL_CPE_SystemInterfaceConfigGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_DslModeSelection_t nDslMode,
   DSL_TcLayerSelection_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS, nRet = DSL_SUCCESS;
   DSL_uint32_t nDev = 0;
   DSL_DslModeSelection_t nDsl = DSL_MODE_ADSL;
   static DSL_SystemInterfaceConfig_t
      sData[DSL_CPE_MAX_DSL_ENTITIES][DSL_MODE_LAST] = { 0 };

   if (pData == DSL_NULL ||
       nDevice >= DSL_CPE_DSL_ENTITIES ||
       (nDslMode == DSL_MODE_NA || nDslMode == DSL_MODE_LAST))
   {
      return DSL_ERROR;
   }

   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_SYSTEM_INTERFACE_CONFIG_GET))
   {
      for (nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         for (nDsl = DSL_MODE_ADSL; nDsl < DSL_MODE_LAST; ++nDsl)
         {
            sData[nDev][nDsl].nDslMode = nDsl;

            nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
                                DSL_FIO_SYSTEM_INTERFACE_CONFIG_GET,
                                 (int) &sData[nDev][nDsl]);

            if (nRet < DSL_SUCCESS ||
                  sData[nDev][nDsl].accessCtl.nReturn != DSL_SUCCESS)
            {
               DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
                  "ERROR - DSL_FIO_SYSTEM_INTERFACE_CONFIG_GET ioctl failed "
                  "(device<%d>, nRet=%d, accessCtrl=%d)!" DSL_CPE_CRLF,
                  nDev, nRet, sData[nDev][nDsl].accessCtl.nReturn));

               nErrCode = DSL_ERROR;
            }
         }
      }

      /* set ioctl flag only if all IOCTLs succeeded */
      if (nErrCode == DSL_SUCCESS)
      {
         DSL_CPE_IoctlFlagSet(DSL_FIO_SYSTEM_INTERFACE_CONFIG_GET);
      }
   }

   *pData = sData[nDevice][nDslMode].data.nTcLayer;

   return nErrCode;
}

/*
   This function collects all data provided by
   DSL_FIO_G997_FRAMING_PARAMETER_STATUS_GET
   IOCtl from all available devices.
*/
static DSL_Error_t DSL_CPE_G997FramingParameterStatusGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   DSL_G997_FramingParameterStatusData_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS, nRet = DSL_SUCCESS;
   DSL_uint32_t nDev = 0;
   static DSL_G997_FramingParameterStatus_t
      sData[DSL_CPE_MAX_DSL_ENTITIES] = { 0 };

   if (pData == DSL_NULL ||
       nDevice >= DSL_CPE_DSL_ENTITIES)
   {
      return DSL_ERROR;
   }

   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_G997_FRAMING_PARAMETER_STATUS_GET))
   {
      for (nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         sData[nDev].nChannel = 0;
         sData[nDev].nDirection = DSL_DOWNSTREAM;

         nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
                              DSL_FIO_G997_FRAMING_PARAMETER_STATUS_GET,
                              (int) &sData[nDev]);

         if (nRet < DSL_SUCCESS ||
               sData[nDev].accessCtl.nReturn != DSL_SUCCESS)
         {
            DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
               "ERROR - DSL_FIO_G997_FRAMING_PARAMETER_STATUS_GET ioctl failed "
               "(device<%d>, nRet=%d, accessCtrl=%d)!" DSL_CPE_CRLF,
               nDev, nRet, sData[nDev].accessCtl.nReturn));

            nErrCode = DSL_ERROR;
         }
      }

      /* set ioctl flag only if all IOCTLs succeeded */
      if (nErrCode == DSL_SUCCESS)
      {
         DSL_CPE_IoctlFlagSet(DSL_FIO_G997_FRAMING_PARAMETER_STATUS_GET);
      }
   }

   /* provide data of corresponding IOCtl request */
   *pData = sData[nDevice].data;

   return nErrCode;
}

/*
   This function collects all data provided by
   DSL_FIO_G997_CHANNEL_STATUS_GET
   IOCtl from all available devices and DSL modes.
*/
static DSL_Error_t DSL_CPE_G997ChannelStatusGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_AccessDir_t nDirection,
   DSL_G997_ChannelStatusData_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS, nRet = DSL_SUCCESS;
   DSL_uint32_t nDev = 0;
   DSL_AccessDir_t nDir = DSL_UPSTREAM;
   static DSL_G997_ChannelStatus_t
      sData[DSL_CPE_MAX_DSL_ENTITIES][DSL_ACCESSDIR_LAST] = { 0 };

   if (pData == DSL_NULL ||
       nDevice >= DSL_CPE_DSL_ENTITIES ||
       (nDirection == DSL_ACCESSDIR_NA || nDirection == DSL_ACCESSDIR_LAST))
   {
      return DSL_ERROR;
   }

   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_G997_CHANNEL_STATUS_GET))
   {
      for (nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         for (nDir = DSL_UPSTREAM; nDir < DSL_ACCESSDIR_LAST; ++nDir)
         {
            sData[nDev][nDir].nChannel = 0;
            sData[nDev][nDir].nDirection = nDir;

            nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
                                DSL_FIO_G997_CHANNEL_STATUS_GET,
                                 (int) &sData[nDev][nDir]);

            if (nRet < DSL_SUCCESS ||
                  sData[nDev][nDir].accessCtl.nReturn != DSL_SUCCESS)
            {
               nErrCode = DSL_ERROR;

               DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
                  "ERROR - DSL_FIO_G997_CHANNEL_STATUS_GET ioctl failed "
                  "(device<%d>, nRet=%d, accessCtrl=%d)!" DSL_CPE_CRLF,
                  nDev, nRet, sData[nDev][nDir].accessCtl.nReturn));
            }
         }
      }

      /* set ioctl flag only if all IOCTLs succeeded */
      if (nErrCode == DSL_SUCCESS)
      {
         DSL_CPE_IoctlFlagSet(DSL_FIO_G997_CHANNEL_STATUS_GET);
      }
   }

   *pData = sData[nDevice][nDirection].data;

   return nErrCode;
}

static DSL_void_t DSL_CPE_JsonChannelStatusUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_LineStateValue_t data = DSL_LINESTATE_UNKNOWN;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_LineStateGet(pContext, nDevice, &data) ==
         DSL_SUCCESS)
   {
      switch (data)
      {
      case DSL_LINESTATE_SHOWTIME_TC_SYNC:
         pNewObj = json_object_new_string("Up");
         break;
      case DSL_LINESTATE_EXCEPTION:
         pNewObj = json_object_new_string("Error");
         break;
      default:
         pNewObj = json_object_new_string("Down");
         break;
      }

      if (pNewObj != DSL_NULL)
      {
         /* update json object */
         pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
         json_object_object_add(pParentObj,
            DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
      }
   }
}

static DSL_void_t DSL_CPE_JsonChannelLinkEncapSupportedUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   DSL_Error_t nRet = DSL_ERROR;
   DSL_TcLayerSelection_t data = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;
   const DSL_uint8_t nBufSupportedSize = 82;
   char bufSupported[nBufSupportedSize];

   memset(&bufSupported, 0, nBufSupportedSize);

   data = DSL_TC_UNKNOWN;
   if (DSL_CPE_SystemInterfaceConfigGet(
         pContext, nDevice, DSL_MODE_ADSL, &data) == DSL_SUCCESS)
   {
      switch (data)
      {
      case DSL_TC_ATM:
         nRet = DSL_SUCCESS;
         cpe_control_strncpy_s(bufSupported, nBufSupportedSize,
            "G.992.3_Annex_K_ATM,", sizeof("G.992.3_Annex_K_ATM,"));
         break;
      case DSL_TC_EFM:
      case DSL_TC_EFM_FORCED:
         nRet = DSL_SUCCESS;
         cpe_control_strncpy_s(bufSupported, nBufSupportedSize,
            "G.992.3_Annex_K_PTM,", sizeof("G.992.3_Annex_K_PTM,"));
         break;
      case DSL_TC_AUTO:
         nRet = DSL_SUCCESS;
         cpe_control_strncpy_s(bufSupported, nBufSupportedSize,
            "G.994.1 (Auto),", sizeof("G.994.1 (Auto),"));
         break;
      default:
         break;
      }
   }

   data = DSL_TC_UNKNOWN;
   if (DSL_CPE_SystemInterfaceConfigGet(
         pContext, nDevice, DSL_MODE_VDSL, &data) == DSL_SUCCESS)
   {
      switch (data)
      {
      case DSL_TC_ATM:
         nRet = DSL_SUCCESS;
         cpe_control_strncpy_s(bufSupported + strlen(bufSupported),
            nBufSupportedSize - strlen(bufSupported),
            "G.992.2_Annex_K_ATM", sizeof("G.992.2_Annex_K_ATM"));
         break;
      case DSL_TC_EFM:
      case DSL_TC_EFM_FORCED:
         nRet = DSL_SUCCESS;
         cpe_control_strncpy_s(bufSupported + strlen(bufSupported),
            nBufSupportedSize - strlen(bufSupported),
            "G.992.2_Annex_K_PTM", sizeof("G.992.2_Annex_K_PTM"));
         break;
      case DSL_TC_AUTO:
         nRet = DSL_SUCCESS;
         cpe_control_strncpy_s(bufSupported + strlen(bufSupported),
            nBufSupportedSize - strlen(bufSupported),
            "G.994.1 (Auto)", sizeof("G.994.1 (Auto)"));
         break;
      default:
         break;
      }
   }

   if (nRet == DSL_SUCCESS)
   {
      pNewObj = json_object_new_string(bufSupported);

      if (pNewObj != DSL_NULL)
      {
         /* update json object */
         pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
         json_object_object_add(pParentObj,
            DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
      }
   }
}

static DSL_void_t DSL_CPE_JsonChannelFramingUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_G997_FramingParameterStatusData_t data = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_G997FramingParameterStatusGet(pContext, nDevice, &data) ==
         DSL_SUCCESS)
   {
      switch (eNodeIdx)
      {
      case DSL_STATUS_LINE_0_CHANNEL_LPATH:
      case DSL_STATUS_LINE_1_CHANNEL_LPATH:
         pNewObj = json_object_new_int(data.nLPATH);
         break;
      case DSL_STATUS_LINE_0_CHANNEL_INTLVDEPTH:
      case DSL_STATUS_LINE_1_CHANNEL_INTLVDEPTH:
         pNewObj = json_object_new_int(data.nINTLVDEPTH);
         break;
      case DSL_STATUS_LINE_0_CHANNEL_INTLVBLOCK:
      case DSL_STATUS_LINE_1_CHANNEL_INTLVBLOCK:
         pNewObj = json_object_new_int(data.nINTLVBLOCK);
         break;
      case DSL_STATUS_LINE_0_CHANNEL_NFEC:
      case DSL_STATUS_LINE_1_CHANNEL_NFEC:
         pNewObj = json_object_new_int(data.nNFEC);
         break;
      case DSL_STATUS_LINE_0_CHANNEL_RFEC:
      case DSL_STATUS_LINE_1_CHANNEL_RFEC:
         pNewObj = json_object_new_int(data.nRFEC);
         break;
      case DSL_STATUS_LINE_0_CHANNEL_LSYMB:
      case DSL_STATUS_LINE_1_CHANNEL_LSYMB:
         pNewObj = json_object_new_int(data.nLSYMB);
         break;
      default:
         break;
      }

      if (pNewObj != DSL_NULL)
      {
         /* update json object */
         pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
         json_object_object_add(pParentObj,
            DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
      }
   }
}

static DSL_void_t DSL_CPE_JsonChannelActStatusUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_AccessDir_t nAccessDir,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_G997_ChannelStatusData_t data = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_G997ChannelStatusGet(pContext, nDevice, nAccessDir, &data) ==
         DSL_SUCCESS)
   {
      switch (eNodeIdx)
      {
      case DSL_STATUS_LINE_0_CHANNEL_ACT_INTERL_DELAY:
      case DSL_STATUS_LINE_1_CHANNEL_ACT_INTERL_DELAY:
         pNewObj = json_object_new_int(data.ActualInterleaveDelay);
         break;
      case DSL_STATUS_LINE_0_CHANNEL_ACTINP:
      case DSL_STATUS_LINE_1_CHANNEL_ACTINP:
         pNewObj = json_object_new_int(data.ActualImpulseNoiseProtection);
         break;
      case DSL_STATUS_LINE_0_CHANNEL_ACTNDR_DS:
      case DSL_STATUS_LINE_1_CHANNEL_ACTNDR_DS:
      case DSL_STATUS_LINE_0_CHANNEL_ACTNDR_US:
      case DSL_STATUS_LINE_1_CHANNEL_ACTNDR_US:
         pNewObj = json_object_new_int(data.ActualNetDataRate);
         break;
      case DSL_STATUS_LINE_0_CHANNEL_ACTINPREIN_DS:
      case DSL_STATUS_LINE_1_CHANNEL_ACTINPREIN_DS:
      case DSL_STATUS_LINE_0_CHANNEL_ACTINPREIN_US:
      case DSL_STATUS_LINE_1_CHANNEL_ACTINPREIN_US:
         pNewObj = json_object_new_int(data.ActualImpulseNoiseProtectionRein);
         break;
      default:
         break;
      }

      if (pNewObj != DSL_NULL)
      {
         /* update json object */
         pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
         json_object_object_add(pParentObj,
            DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
      }
   }
}

static DSL_void_t DSL_CPE_JsonChannelActINPReportUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_char_t* jsonPath)
{
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   pNewObj = json_object_new_boolean(DSL_FALSE);

   if (pNewObj != DSL_NULL)
   {
      /* update json object */
      pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
      json_object_object_add(pParentObj,
         DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
   }
}

/*
   This function collects all data provided by
   DSL_FIO_PM_DATA_PATH_COUNTERS_TOTAL_GET
   IOCtl from all available devices and XTU directions.
*/
static DSL_Error_t DSL_CPE_PMChannelDataPathCountersTotalGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nDirection,
   DSL_uint32_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS, nRet = DSL_SUCCESS;
   DSL_uint32_t nDev = 0;
   DSL_XTUDir_t nDir = DSL_NEAR_END;
   static DSL_PM_DataPathCountersTotal_t
      sData[DSL_CPE_MAX_DSL_ENTITIES][(DSL_FAR_END+1)] = { 0 };

   if (pData == DSL_NULL ||
       nDevice >= DSL_CPE_DSL_ENTITIES ||
       nDirection == DSL_XTUDIR_NA)
   {
      return DSL_ERROR;
   }

   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_PM_DATA_PATH_COUNTERS_TOTAL_GET))
   {
      for (nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         for (nDir = DSL_NEAR_END; nDir <= DSL_FAR_END; ++nDir)
         {
            sData[nDev][nDir].nDirection = nDir;

            nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
                                 DSL_FIO_PM_DATA_PATH_COUNTERS_TOTAL_GET,
                                 (int) &sData[nDev][nDir]);

            if (nRet < DSL_SUCCESS ||
                  sData[nDev][nDir].accessCtl.nReturn != DSL_SUCCESS)
            {
               DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
                  "ERROR - DSL_FIO_PM_DATA_PATH_COUNTERS_TOTAL_GET ioctl failed "
                  "(device<%d>, nRet=%d, accessCtrl=%d)!" DSL_CPE_CRLF,
                  nDev, nRet, sData[nDev][nDir].accessCtl.nReturn));

               nErrCode = DSL_ERROR;
            }
         }
      }

      /* set ioctl flag only if all IOCTLs succeeded */
      if (nErrCode == DSL_SUCCESS)
      {
         DSL_CPE_IoctlFlagSet(DSL_FIO_PM_DATA_PATH_COUNTERS_TOTAL_GET);
      }
   }

   *pData = sData[nDevice][nDirection].data.nHEC;

   return nErrCode;
}

/*
   This function collects all data provided by
   DSL_FIO_PM_DATA_PATH_COUNTERS_SHOWTIME_GET
   IOCtl from all available devices, history intervals and XTU directions.
*/
static DSL_Error_t DSL_CPE_PMChannelDataPathCountersShowtimeGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nDirection,
   const DSL_uint32_t nHistoryInterval,
   DSL_uint32_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS, nRet = DSL_SUCCESS;
   DSL_uint32_t nDev = 0;
   DSL_XTUDir_t nDir = DSL_NEAR_END;
   DSL_uint32_t nHistInt = 0;
   static DSL_PM_DataPathCounters_t
      sData[DSL_CPE_MAX_DSL_ENTITIES]
            [(DSL_FAR_END+1)]
            [DSL_PM_HISTORY_INTERVALS_NUM] = { 0 };

   if (pData == DSL_NULL ||
       nDevice >= DSL_CPE_DSL_ENTITIES ||
       nDirection == DSL_XTUDIR_NA ||
       nHistoryInterval >= DSL_PM_HISTORY_INTERVALS_NUM)
   {
      return DSL_ERROR;
   }

   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_PM_DATA_PATH_COUNTERS_SHOWTIME_GET))
   {
      for (nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         for (nDir = DSL_NEAR_END; nDir <= DSL_FAR_END; ++nDir)
         {
            for (nHistInt = 0; nHistInt < DSL_PM_HISTORY_INTERVALS_NUM; ++nHistInt)
            {
               sData[nDev][nDir][nHistInt].nDirection = nDir;
               sData[nDev][nDir][nHistInt].nHistoryInterval = nHistInt;

               nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
                                    DSL_FIO_PM_DATA_PATH_COUNTERS_SHOWTIME_GET,
                                    (int) &sData[nDev][nDir][nHistInt]);

               if (nRet < DSL_SUCCESS ||
                     sData[nDev][nDir][nHistInt].accessCtl.nReturn != DSL_SUCCESS)
               {
                  DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
                     "ERROR - DSL_FIO_PM_DATA_PATH_COUNTERS_SHOWTIME_GET "
                     "ioctl failed (device<%d>, direction<%s>, "
                     "historyInterval<%d>, nRet=%d, "
                     "accessCtrl=%d)!" DSL_CPE_CRLF,
                     nDev, (nDir == DSL_NEAR_END) ? "near end" : "far end",
                     nHistInt, nRet,
                     sData[nDev][nDir][nHistInt].accessCtl.nReturn));
               }
            }
         }
      }

      /* no check for IOCTL failure here - it may happen that IOCTL failes
         due to request for nHistoryInterval that was not yet reached */
      DSL_CPE_IoctlFlagSet(DSL_FIO_PM_DATA_PATH_COUNTERS_SHOWTIME_GET);
   }

   *pData = sData[nDevice][nDirection][nHistoryInterval].data.nHEC;

   return nErrCode;
}

static DSL_void_t DSL_CPE_JsonChannelCountersTotalUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nXTUDir,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_PM_ChannelCountersTotal_t data = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_PMCountersTotalGet(
         pContext, nDevice, nXTUDir, &data) == DSL_SUCCESS)
   {
      switch (eNodeIdx)
      {
      case DSL_STATS_LINE_0_CHANNEL_TOTAL_START:
      case DSL_STATS_LINE_1_CHANNEL_TOTAL_START:
         pNewObj = json_object_new_int(data.total.nElapsedTime);
         break;
      case DSL_STATS_LINE_0_CHANNEL_TOTAL_XTUR_FEC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_TOTAL_XTUR_FEC_ERRORS:
      case DSL_STATS_LINE_0_CHANNEL_TOTAL_XTUC_FEC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_TOTAL_XTUC_FEC_ERRORS:
         pNewObj = json_object_new_int(data.data.nFEC);
         break;
      case DSL_STATS_LINE_0_CHANNEL_TOTAL_XTUR_CRC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_TOTAL_XTUR_CRC_ERRORS:
      case DSL_STATS_LINE_0_CHANNEL_TOTAL_XTUC_CRC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_TOTAL_XTUC_CRC_ERRORS:
         pNewObj = json_object_new_int(data.data.nCodeViolations);
         break;
      default:
         break;
      }

      if (pNewObj != DSL_NULL)
      {
         /* update json object */
         pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
         json_object_object_add(pParentObj,
            DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
      }
   }
}

static DSL_void_t DSL_CPE_JsonChannelDataPathCountersTotalUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nXTUDir,
   const DSL_char_t* jsonPath)
{
   DSL_uint32_t data = 0;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_PMChannelDataPathCountersTotalGet(
         pContext, nDevice, nXTUDir, &data) == DSL_SUCCESS)
   {
      pNewObj = json_object_new_int(data);

      if (pNewObj != DSL_NULL)
      {
         /* update json object */
         pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
         json_object_object_add(pParentObj,
            DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
      }
   }
}

static DSL_void_t DSL_CPE_JsonChannelCountersShowtimeUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nXTUDir,
   const DSL_uint32_t nHistoryInterval,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_PM_ChannelCounters_t data = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_PMCountersShowtimeGet(
         pContext, nDevice, nXTUDir, nHistoryInterval, &data) ==
         DSL_SUCCESS)
   {
      switch (eNodeIdx)
      {
      case DSL_STATS_LINE_0_CHANNEL_SHOWTIME_START:
      case DSL_STATS_LINE_1_CHANNEL_SHOWTIME_START:
      case DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_START:
      case DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_START:
         pNewObj = json_object_new_int(data.interval.nElapsedTime);
         break;
      case DSL_STATS_LINE_0_CHANNEL_SHOWTIME_XTUR_FEC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_SHOWTIME_XTUR_FEC_ERRORS:
      case DSL_STATS_LINE_0_CHANNEL_SHOWTIME_XTUC_FEC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_SHOWTIME_XTUC_FEC_ERRORS:
      case DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_XTUR_FEC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_XTUR_FEC_ERRORS:
      case DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_XTUC_FEC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_XTUC_FEC_ERRORS:
         pNewObj = json_object_new_int(data.data.nFEC);
         break;
      case DSL_STATS_LINE_0_CHANNEL_SHOWTIME_XTUR_CRC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_SHOWTIME_XTUR_CRC_ERRORS:
      case DSL_STATS_LINE_0_CHANNEL_SHOWTIME_XTUC_CRC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_SHOWTIME_XTUC_CRC_ERRORS:
      case DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_XTUR_CRC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_XTUR_CRC_ERRORS:
      case DSL_STATS_LINE_0_CHANNEL_LAST_SHOWTIME_XTUC_CRC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_LAST_SHOWTIME_XTUC_CRC_ERRORS:
         pNewObj = json_object_new_int(data.data.nCodeViolations);
         break;
      default:
         break;
      }

      if (pNewObj != DSL_NULL)
      {
         /* update json object */
         pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
         json_object_object_add(pParentObj,
            DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
      }
   }
}

static DSL_void_t DSL_CPE_JsonChannelDataPathCountersShowtimeUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nXTUDir,
   const DSL_uint32_t nHistoryInterval,
   const DSL_char_t* jsonPath)
{
   DSL_uint32_t data = 0;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_PMChannelDataPathCountersShowtimeGet(
         pContext, nDevice, nXTUDir, nHistoryInterval, &data) ==
         DSL_SUCCESS)
   {
      pNewObj = json_object_new_int(data);

      if (pNewObj != DSL_NULL)
      {
         /* update json object */
         pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
         json_object_object_add(pParentObj,
            DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
      }
   }
}

/*
   This function collects all data provided by
   DSL_FIO_PM_CHANNEL_COUNTERS_1DAY_GET
   IOCtl from all available devices and XTU directions.
*/
static DSL_Error_t DSL_CPE_PMChannelCounters1DayGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nDirection,
   DSL_PM_ChannelCounters_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS, nRet = DSL_SUCCESS;
   DSL_uint32_t nDev = 0;
   DSL_XTUDir_t nDir = DSL_NEAR_END;
   static DSL_PM_ChannelCounters_t
      sData[DSL_CPE_MAX_DSL_ENTITIES][(DSL_FAR_END+1)] = { 0 };

   if (pData == DSL_NULL ||
       nDevice >= DSL_CPE_DSL_ENTITIES ||
       nDirection == DSL_XTUDIR_NA)
   {
      return DSL_ERROR;
   }

   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_PM_CHANNEL_COUNTERS_1DAY_GET))
   {
      for (nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         for (nDir = DSL_NEAR_END; nDir <= DSL_FAR_END; ++nDir)
         {
            sData[nDev][nDir].nHistoryInterval = 0;
            sData[nDev][nDir].nDirection = nDir;

            nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
                                 DSL_FIO_PM_CHANNEL_COUNTERS_1DAY_GET,
                                 (int) &sData[nDev][nDir]);

            if (nRet < DSL_SUCCESS ||
                  sData[nDev][nDir].accessCtl.nReturn != DSL_SUCCESS)
            {
               DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
                  "ERROR - DSL_FIO_PM_CHANNEL_COUNTERS_1DAY_GET ioctl failed "
                  "(device<%d>, nRet=%d, accessCtrl=%d)!" DSL_CPE_CRLF,
                  nDev, nRet, sData[nDev][nDir].accessCtl.nReturn));

               nErrCode = DSL_ERROR;
            }
         }
      }

      /* set ioctl flag only if all IOCTLs succeeded */
      if (nErrCode == DSL_SUCCESS)
      {
         DSL_CPE_IoctlFlagSet(DSL_FIO_PM_CHANNEL_COUNTERS_1DAY_GET);
      }
   }

   *pData = sData[nDevice][nDirection];

   return nErrCode;
}

static DSL_void_t DSL_CPE_JsonChannelCounters1DayUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nXTUDir,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_PM_ChannelCounters_t data = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_PMChannelCounters1DayGet(
         pContext, nDevice, nXTUDir, &data) == DSL_SUCCESS)
   {
      switch (eNodeIdx)
      {
      case DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_START:
      case DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_START:
         pNewObj = json_object_new_int(data.interval.nElapsedTime);
         break;
      case DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_XTUR_FEC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_XTUR_FEC_ERRORS:
      case DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_XTUC_FEC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_XTUC_FEC_ERRORS:
         pNewObj = json_object_new_int(data.data.nFEC);
         break;
      case DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_XTUR_CRC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_XTUR_CRC_ERRORS:
      case DSL_STATS_LINE_0_CHANNEL_CURRENT_DAY_XTUC_CRC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_CURRENT_DAY_XTUC_CRC_ERRORS:
         pNewObj = json_object_new_int(data.data.nCodeViolations);
         break;
      default:
         break;
      }

      if (pNewObj != DSL_NULL)
      {
         /* update json object */
         pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
         json_object_object_add(pParentObj,
            DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
      }
   }
}

/*
   This function collects all data provided by
   DSL_FIO_PM_DATA_PATH_COUNTERS_1DAY_GET
   IOCtl from all available devices and XTU directions.
*/
static DSL_Error_t DSL_CPE_PMChannelDataPathCounters1DayGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nDirection,
   DSL_uint32_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS, nRet = DSL_SUCCESS;
   DSL_uint32_t nDev = 0;
   DSL_XTUDir_t nDir = DSL_NEAR_END;
   static DSL_PM_DataPathCounters_t
      sData[DSL_CPE_MAX_DSL_ENTITIES][(DSL_FAR_END+1)] = { 0 };

   if (pData == DSL_NULL ||
       nDevice >= DSL_CPE_DSL_ENTITIES ||
       nDirection == DSL_XTUDIR_NA)
   {
      return DSL_ERROR;
   }

   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_PM_DATA_PATH_COUNTERS_1DAY_GET))
   {
      for (nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         for (nDir = DSL_NEAR_END; nDir <= DSL_FAR_END; ++nDir)
         {
            sData[nDev][nDir].nHistoryInterval = 0;
            sData[nDev][nDir].nDirection = nDir;

            nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
                                 DSL_FIO_PM_DATA_PATH_COUNTERS_1DAY_GET,
                                 (int) &sData[nDev][nDir]);

            if (nRet < DSL_SUCCESS ||
                  sData[nDev][nDir].accessCtl.nReturn != DSL_SUCCESS)
            {
               DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
                  "ERROR - DSL_FIO_PM_DATA_PATH_COUNTERS_1DAY_GET ioctl failed "
                  "(device<%d>, nRet=%d, accessCtrl=%d)!" DSL_CPE_CRLF,
                  nDev, nRet, sData[nDev][nDir].accessCtl.nReturn));

               nErrCode = DSL_ERROR;
            }
         }
      }

      /* set ioctl flag only if all IOCTLs succeeded */
      if (nErrCode == DSL_SUCCESS)
      {
         DSL_CPE_IoctlFlagSet(DSL_FIO_PM_DATA_PATH_COUNTERS_1DAY_GET);
      }
   }

   *pData = sData[nDevice][nDirection].data.nHEC;

   return nErrCode;
}

static DSL_void_t DSL_CPE_JsonChannelDataPathCounters1DayUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nXTUDir,
   const DSL_char_t* jsonPath)
{
   DSL_uint32_t data = 0;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_PMChannelDataPathCounters1DayGet(
         pContext, nDevice, nXTUDir, &data) == DSL_SUCCESS)
   {
      pNewObj = json_object_new_int(data);

      if (pNewObj != DSL_NULL)
      {
         /* update json object */
         pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
         json_object_object_add(pParentObj,
            DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
      }
   }
}

static DSL_void_t DSL_CPE_JsonChannelCounters15MinUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nXTUDir,
   const DSL_CPE_StatusNodeIdx_t eNodeIdx,
   const DSL_char_t* jsonPath)
{
   DSL_PM_ChannelCounters_t data = { 0 };
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_PMCounters15MinGet(
         pContext, nDevice, nXTUDir, &data) == DSL_SUCCESS)
   {
      switch (eNodeIdx)
      {
      case DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_START:
      case DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_START:
         pNewObj = json_object_new_int(data.interval.nElapsedTime);
         break;
      case DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_XTUR_FEC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_XTUR_FEC_ERRORS:
      case DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_XTUC_FEC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_XTUC_FEC_ERRORS:
         pNewObj = json_object_new_int(data.data.nFEC);
         break;
      case DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_XTUR_CRC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_XTUR_CRC_ERRORS:
      case DSL_STATS_LINE_0_CHANNEL_QUARTER_HOUR_XTUC_CRC_ERRORS:
      case DSL_STATS_LINE_1_CHANNEL_QUARTER_HOUR_XTUC_CRC_ERRORS:
         pNewObj = json_object_new_int(data.data.nCodeViolations);
         break;
      default:
         break;
      }

      if (pNewObj != DSL_NULL)
      {
         /* update json object */
         pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
         json_object_object_add(pParentObj,
            DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
      }
   }
}

/*
   This function collects all data provided by
   DSL_FIO_PM_DATA_PATH_COUNTERS_15MIN_GET
   IOCtl from all available devices and XTU directions.
*/
static DSL_Error_t DSL_CPE_PMChannelDataPathCounters15MinGet(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nDirection,
   DSL_uint32_t *pData)
{
   DSL_Error_t nErrCode = DSL_SUCCESS, nRet = DSL_SUCCESS;
   DSL_uint32_t nDev = 0;
   DSL_XTUDir_t nDir = DSL_NEAR_END;
   static DSL_PM_DataPathCounters_t
      sData[DSL_CPE_MAX_DSL_ENTITIES][(DSL_FAR_END+1)] = { 0 };

   if (pData == DSL_NULL ||
       nDevice >= DSL_CPE_DSL_ENTITIES ||
       nDirection == DSL_XTUDIR_NA)
   {
      return DSL_ERROR;
   }

   if (!DSL_CPE_IsIoctlFlagSet(DSL_FIO_PM_DATA_PATH_COUNTERS_15MIN_GET))
   {
      for (nDev = 0; nDev < DSL_CPE_DSL_ENTITIES; ++nDev)
      {
         for (nDir = DSL_NEAR_END; nDir <= DSL_FAR_END; ++nDir)
         {
            sData[nDev][nDir].nHistoryInterval = 0;
            sData[nDev][nDir].nDirection = nDir;

            nRet = DSL_CPE_Ioctl(pContext->fd[nDev],
                                 DSL_FIO_PM_DATA_PATH_COUNTERS_15MIN_GET,
                                 (int) &sData[nDev][nDir]);

            if (nRet < DSL_SUCCESS ||
                  sData[nDev][nDir].accessCtl.nReturn != DSL_SUCCESS)
            {
               DSL_CCA_DEBUG(DSL_CCA_DBG_ERR, (DSL_CPE_PREFIX
                  "ERROR - DSL_FIO_PM_DATA_PATH_COUNTERS_15MIN_GET ioctl failed "
                  "(device<%d>, nRet=%d, accessCtrl=%d)!" DSL_CPE_CRLF,
                  nDev, nRet, sData[nDev][nDir].accessCtl.nReturn));

               nErrCode = DSL_ERROR;
            }
         }
      }

      /* set ioctl flag only if all IOCTLs succeeded */
      if (nErrCode == DSL_SUCCESS)
      {
         DSL_CPE_IoctlFlagSet(DSL_FIO_PM_DATA_PATH_COUNTERS_15MIN_GET);
      }
   }

   *pData = sData[nDevice][nDirection].data.nHEC;

   return nErrCode;
}

static DSL_void_t DSL_CPE_JsonChannelDataPathCounters15MinUpdate(
   DSL_CPE_Control_Context_t *pContext,
   const DSL_int_t nDevice,
   const DSL_XTUDir_t nXTUDir,
   const DSL_char_t* jsonPath)
{
   DSL_uint32_t data = 0;
   json_object *pParentObj = DSL_NULL, *pNewObj = DSL_NULL;

   if (DSL_CPE_PMChannelDataPathCounters15MinGet(
         pContext, nDevice, nXTUDir, &data) == DSL_SUCCESS)
   {
      pNewObj = json_object_new_int(data);

      if (pNewObj != DSL_NULL)
      {
         /* update json object */
         pParentObj = DSL_CPE_JsonNodeParentGet(pJsonStatusRootObj, jsonPath);
         json_object_object_add(pParentObj,
            DSL_CPE_JsonNodeNameGet(jsonPath), pNewObj);
      }
   }
}

#endif /* defined(INCLUDE_DSL_JSON_PARSING) && (INCLUDE_DSL_JSON_PARSING == 1) */
