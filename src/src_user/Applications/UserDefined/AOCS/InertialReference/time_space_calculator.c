#pragma section REPRO
/**
* @file   time_space_calculator.c
* @brief  各種慣性系リファレンス計算のうち，時刻と座標の変換を行う
*/

#include "time_space_calculator.h"
#include <src_core/TlmCmd/common_cmd_packet_util.h>
#include <src_core/System/TimeManager/time_manager.h>
#include <src_core/System/TimeManager/obc_time.h>
#include "../aocs_manager.h"
#include "../../../../Library/time_space.h"
#include "../../../../Library/stdint.h"
#include "../../../../Library/c2a_math.h"


static TimeSpaceCalculator        time_space_calculator_;
const  TimeSpaceCalculator* const time_space_calculator = &time_space_calculator_;


static void APP_TIME_SPACE_CALC_init_(void);
static void APP_TIME_SPACE_CALC_exec_(void);

// internal work functions
static double APP_TIME_SPACE_CALC_update_current_jday_ref_(void);
static double APP_TIME_SPACE_CALC_propagate_current_jday_ref_(void);
static void   APP_TIME_SPACE_CALC_update_eci_ecef_trans_(const double j_day_ref);


AppInfo APP_TIME_SPACE_CALC_create_app(void)
{
  return AI_create_app_info("TimeSpaceCalc", APP_TIME_SPACE_CALC_init_, APP_TIME_SPACE_CALC_exec_);
}


static void APP_TIME_SPACE_CALC_init_(void)
{
  time_space_calculator_.offset_sec = 0.0f;

  return;
}


static void APP_TIME_SPACE_CALC_exec_(void)
{
  double reference_jday;
  if (aocs_manager->gps_visibility == AOCS_MANAGER_GPS_VISIBLE)
  {
    reference_jday = APP_TIME_SPACE_CALC_update_current_jday_ref_();
  }
  else
  {
    reference_jday = APP_TIME_SPACE_CALC_propagate_current_jday_ref_();
  }

  APP_TIME_SPACE_CALC_update_eci_ecef_trans_(reference_jday);

  return;
}


static double APP_TIME_SPACE_CALC_update_current_jday_ref_(void)
{
  // TODO_L: 位置情報をobsからestに置き換えるタイミングで，時刻もobsからestに置き換えて，ここではestを用いる方が他との統一性の観点でベターだが，
  // 現状では，GPSR情報は全てobsに入れる流れとなっており，estに値が入らないことから，ここではobsのままにする
  GPS_TIME_OF_WEEK ref_gps_time = aocs_manager->current_gps_time_obs;

  double week_of_msec = PHYSICAL_CONST_EARTH_SOLAR_DAY_s * 7.0 * 1e3;

  // check rollover
  if ((double)(ref_gps_time.msec_of_week) > week_of_msec)
  {
    ref_gps_time.msec_of_week = 0;
    ref_gps_time.week_number += 1;
  }

  // GPSRのTLM更新頻度よりも高頻度にこの関数が呼ばれる場合，以下の処理で補間が必要
  ObcTime current_obct                = TMGR_get_master_clock();
  float elapsed_time_from_observed_s  = time_space_calculator_.offset_sec + (float)OBCT_diff_in_sec(&aocs_manager->obct_gps_time_obs, &current_obct);
  double reference_jday = TIME_SPACE_convert_gpstime_to_julian_day(ref_gps_time, elapsed_time_from_observed_s);

  // TODO_L: 位置情報をobsからestに置き換えるタイミングで，時刻もobsからestに置き換えて，ここではestを用いる
  AOCS_MANAGER_set_reference_jday(reference_jday, current_obct);

  return reference_jday;
}


static double APP_TIME_SPACE_CALC_propagate_current_jday_ref_(void)
{
  ObcTime obct_now     = TMGR_get_master_clock();
  float elapsed_ti_sec = (float)OBCT_diff_in_sec(&aocs_manager->obct_reference, &obct_now);

  double reference_jday = (double)(elapsed_ti_sec) / (PHYSICAL_CONST_EARTH_SOLAR_DAY_s) + aocs_manager->reference_jday;

  AOCS_MANAGER_set_reference_jday(reference_jday, obct_now);

  return reference_jday;
}


static void APP_TIME_SPACE_CALC_update_eci_ecef_trans_(const double reference_jday)
{
  double ref_gmst_rad = TIME_SPACE_calc_gmst_rad(reference_jday);
  double dcm_eci_to_ecef_temp[PHYSICAL_CONST_THREE_DIM][PHYSICAL_CONST_THREE_DIM];

  C2A_MATH_ERROR error = TIME_SPACE_trans_eci_to_ecef(reference_jday, ref_gmst_rad, dcm_eci_to_ecef_temp);

  if (error != C2A_MATH_ERROR_OK)
  {
    error = TIME_SPACE_trans_eci_to_ecef_rotation_only(ref_gmst_rad, dcm_eci_to_ecef_temp);
  }

  if (error == C2A_MATH_ERROR_OK)
  {
    AOCS_MANAGER_set_dcm_eci_to_ecef(dcm_eci_to_ecef_temp);
  }

  return;
}

CCP_CmdRet Cmd_APP_TIME_SPACE_CALC_SET_OFFSET_TIME(const CommonCmdPacket* packet)
{
  const float offset_range_limit_sec = 60 * 60; // 1時間に相当する秒数
  const uint8_t* param = CCP_get_param_head(packet);
  int param_id = 0;

  float time_offset_cmd_sec;
  ENDIAN_memcpy(&time_offset_cmd_sec, param + param_id, sizeof(float));
  param_id += (int)sizeof(float);

  if (C2A_MATH_check_abs_range_violation(time_offset_cmd_sec, offset_range_limit_sec) != C2A_MATH_ERROR_OK)
  {
    return CCP_make_cmd_ret_without_err_code(CCP_EXEC_ILLEGAL_PARAMETER);
  }

  time_space_calculator_.offset_sec = time_offset_cmd_sec;

  return CCP_make_cmd_ret_without_err_code(CCP_EXEC_SUCCESS);
}

#pragma section
