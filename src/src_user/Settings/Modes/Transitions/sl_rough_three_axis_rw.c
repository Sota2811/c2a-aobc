/**
* @file   sl_rough_three_axis_rw.c
* @brief  粗三軸制御モードのシーケンスリスト
*/

#pragma section REPRO
#include "sl_rough_three_axis_rw.h"

#include <src_user/TlmCmd/block_command_definitions.h>
#include <src_user/TlmCmd/command_definitions.h>
#include <src_user/Applications/UserDefined/AOCS/aocs_manager.h>
#include <src_user/Library/physical_constants.h>
#include <src_user/Applications/UserDefined/AOCS/HardwareDependent/SensorSelectors/magnetometer_selector.h>

#include <src_core/Applications/timeline_command_dispatcher_id_define.h>
#include <src_core/TlmCmd/block_command_loader.h>
#include <src_core/System/TimeManager/obc_time.h>

// Satellite Parameters
#include <src_user/Settings/SatelliteParameters/fdir_parameters.h>

void BCL_load_rough_three_axis_to_rough_three_axis_rw(void)
{
  uint32_t timing_sec = 1;
  // 自動モード遷移OFF
  BCL_tool_prepare_param_uint8(0);
  BCL_tool_register_cmd(OBCT_sec2cycle(timing_sec), Cmd_CODE_APP_AOCS_MM_SET_AUTO_MODE_TRANSITION);

  // 自動モード遷移閾値変更
  timing_sec++;
  BCL_tool_prepare_param_float(FDIR_PARAMETERS_rough_three_axis_rw_div_limit_rad);
  BCL_tool_prepare_param_float(FDIR_PARAMETERS_rough_three_axis_rw_div_time_limit_s);
  BCL_tool_register_cmd(OBCT_sec2cycle(timing_sec), Cmd_CODE_APP_AOCS_MM_SET_THREE_AXIS_THRESHOLD);

  // RW X ON
  timing_sec++;
  BCL_tool_prepare_param_uint8(TLCD_ID_DEPLOY_BC);
  BCL_tool_prepare_param_uint16(BC_POWER_ON_RWX);
  BCL_tool_register_cmd(OBCT_sec2cycle(timing_sec), Cmd_CODE_TLCD_DEPLOY_BLOCK);

  // RW Y ON
  timing_sec += 10;
  BCL_tool_prepare_param_uint8(TLCD_ID_DEPLOY_BC);
  BCL_tool_prepare_param_uint16(BC_POWER_ON_RWY);
  BCL_tool_register_cmd(OBCT_sec2cycle(timing_sec), Cmd_CODE_TLCD_DEPLOY_BLOCK);

  // RW Z ON
  timing_sec += 10;
  BCL_tool_prepare_param_uint8(TLCD_ID_DEPLOY_BC);
  BCL_tool_prepare_param_uint16(BC_POWER_ON_RWZ);
  BCL_tool_register_cmd(OBCT_sec2cycle(timing_sec), Cmd_CODE_TLCD_DEPLOY_BLOCK);

  // deactivate MAG Exclusive Control
  timing_sec += 5;
  BCL_tool_prepare_param_uint8(AOCS_MANAGER_MAG_EXCLUSIVE_STATE_IDLE);
  BCL_tool_register_cmd(OBCT_sec2cycle(timing_sec), Cmd_CODE_APP_AOCS_MANAGER_SET_MAG_EXCLUSIVE_STATE);

  // モード遷移完了
  timing_sec += 5;
  BCL_tool_register_cmd(OBCT_sec2cycle(timing_sec), Cmd_CODE_MM_FINISH_TRANSITION);

  // 自動モード遷移ON
  timing_sec += FDIR_PARAMETERS_rough_three_axis_rw_start_mode_manager_time_s;
  BCL_tool_prepare_param_uint8(1);
  BCL_tool_register_cmd(OBCT_sec2cycle(timing_sec), Cmd_CODE_APP_AOCS_MM_SET_AUTO_MODE_TRANSITION);
}

#pragma section
