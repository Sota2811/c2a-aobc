#pragma section REPRO
/**
* @file   three_axis_control_rw.c
* @brief  RWを使った三軸姿勢制御アプリケーション
* @note   三軸姿勢制御のためのPID制御を行い、目標回転トルクを計算する。
*/

#include "three_axis_control_rw.h"

#include <math.h>
#include <string.h>

#include "../../../../Library/vector3.h"
#include "../../../../Library/quaternion.h"
#include "../../../../Library/matrix33.h"
#include "../../../../Library/math_constants.h"
#include "../../../../Library/ControlUtility/gyroscopic_effect.h"
#include "../aocs_manager.h"

#include <src_core/Library/endian_memcpy.h>

static ThreeAxisControlRw        three_axis_control_rw_;
const  ThreeAxisControlRw* const three_axis_control_rw = &three_axis_control_rw_;

static void APP_TAC_RW_init_(void);
static void APP_TAC_RW_exec_(void);

/**
 * @brief  三軸姿勢制御における目標トルク計算関数
 */
static void APP_TAC_RW_calc_target_torque_(void);

/**
 * @brief        機体固定座標系姿勢回転ベクトルの偏差の計算
 * @param[out]   rot_vec_body_rad  : 機体固定座標系姿勢回転ベクトル [rad]
 * @return       void
 */
static void APP_TAC_RW_calc_rot_vec_body_rad_(float rot_vec_body_rad[PHYSICAL_CONST_THREE_DIM]);

AppInfo APP_TAC_RW_create_app(void)
{
  return AI_create_app_info("ThreeAxisControlRw", APP_TAC_RW_init_, APP_TAC_RW_exec_);
}

static void APP_TAC_RW_init_(void)
{
  memset(&three_axis_control_rw_, 0x00, sizeof(ThreeAxisControlRw));

  for (uint8_t axis = 0; axis < PHYSICAL_CONST_THREE_DIM; axis++)
  {
    PID_CONTROL_init(&three_axis_control_rw_.pid_omega[axis]);
    PID_CONTROL_init(&three_axis_control_rw_.pid_att[axis]);
  }

  PidGains gains_omega[PHYSICAL_CONST_THREE_DIM];
  gains_omega[0].p_gain = aocs_manager->inertia_tensor_sc_body_kgm2[0][0] * 7.0e-1f;
  gains_omega[1].p_gain = aocs_manager->inertia_tensor_sc_body_kgm2[1][1] * 7.0e-1f;
  gains_omega[2].p_gain = aocs_manager->inertia_tensor_sc_body_kgm2[2][2] * 7.0e-1f;

  gains_omega[0].i_gain = aocs_manager->inertia_tensor_sc_body_kgm2[0][0] * 0.0e-1f;
  gains_omega[1].i_gain = aocs_manager->inertia_tensor_sc_body_kgm2[1][1] * 0.0e-1f;
  gains_omega[2].i_gain = aocs_manager->inertia_tensor_sc_body_kgm2[2][2] * 0.0e-1f;

  gains_omega[0].d_gain = aocs_manager->inertia_tensor_sc_body_kgm2[0][0] * 2.0e-2f;
  gains_omega[1].d_gain = aocs_manager->inertia_tensor_sc_body_kgm2[1][1] * 2.0e-2f;
  gains_omega[2].d_gain = aocs_manager->inertia_tensor_sc_body_kgm2[2][2] * 2.0e-2f;

  PidGains gains_att[PHYSICAL_CONST_THREE_DIM];
  gains_att[0].p_gain = aocs_manager->inertia_tensor_sc_body_kgm2[0][0] * 1.0e-2f;
  gains_att[1].p_gain = aocs_manager->inertia_tensor_sc_body_kgm2[1][1] * 1.0e-2f;
  gains_att[2].p_gain = aocs_manager->inertia_tensor_sc_body_kgm2[2][2] * 1.0e-2f;

  gains_att[0].i_gain = aocs_manager->inertia_tensor_sc_body_kgm2[0][0] * 0.0e-2f;
  gains_att[1].i_gain = aocs_manager->inertia_tensor_sc_body_kgm2[1][1] * 0.0e-2f;
  gains_att[2].i_gain = aocs_manager->inertia_tensor_sc_body_kgm2[2][2] * 0.0e-2f;

  gains_att[0].d_gain = aocs_manager->inertia_tensor_sc_body_kgm2[0][0] * 1.0e-2f;
  gains_att[1].d_gain = aocs_manager->inertia_tensor_sc_body_kgm2[1][1] * 1.0e-2f;
  gains_att[2].d_gain = aocs_manager->inertia_tensor_sc_body_kgm2[2][2] * 1.0e-2f;

  for (uint8_t axis = 0; axis < PHYSICAL_CONST_THREE_DIM; axis++)
  {
    PID_CONTROL_set_gain(&three_axis_control_rw_.pid_omega[axis], gains_omega[axis]);
    PID_CONTROL_set_gain(&three_axis_control_rw_.pid_att[axis], gains_att[axis]);
  }

  return;
}

void APP_TAC_RW_exec_(void)
{
  APP_TAC_RW_calc_target_torque_();
  AOCS_MANAGER_set_internal_torque_target_body_Nm(three_axis_control_rw_.target_torque_body_Nm);
  return;
}

void APP_TAC_RW_calc_target_torque_(void)
{
  // 機体固定座標系姿勢角速度の偏差の計算
  float ang_vel_error_body_rad_s[PHYSICAL_CONST_THREE_DIM] = {0.0f, 0.0f, 0.0f};
  VECTOR3_subtract(ang_vel_error_body_rad_s, aocs_manager->ang_vel_target_body_rad_s, aocs_manager->ang_vel_est_body_rad_s);
  AOCS_MANAGER_set_ang_vel_error_body_rad_s(ang_vel_error_body_rad_s);

  // 機体固定座標系姿勢の偏差の計算
  float rot_vec_body_rad[PHYSICAL_CONST_THREE_DIM] = {1.0f, 0.0f, 0.0f};
  Quaternion quaternion_error_b2t = QUATERNION_divide(aocs_manager->quaternion_est_i2b, aocs_manager->quaternion_target_i2t);
  if (quaternion_error_b2t.scalar_part < 0.0f)
  {
    quaternion_error_b2t = QUATERNION_scalar_product(-1.0f, quaternion_error_b2t);
  }
  AOCS_MANAGER_set_quaternion_error_b2t(quaternion_error_b2t);
  APP_TAC_RW_calc_rot_vec_body_rad_(rot_vec_body_rad);

  // PIDによる目標トルクの計算
  for (int i = 0; i < PHYSICAL_CONST_THREE_DIM; i++)
  {
    PID_CONTROL_calc_output(&three_axis_control_rw_.pid_omega[i], aocs_manager->ang_vel_error_body_rad_s[i]);
    PID_CONTROL_calc_output(&three_axis_control_rw_.pid_att[i], rot_vec_body_rad[i]);
    three_axis_control_rw_.target_torque_body_Nm[i] = three_axis_control_rw_.pid_omega[i].control_output +
                                                      three_axis_control_rw_.pid_att[i].control_output;
  }

  // ジャイロ効果トルクの追加
  AOCS_MANAGER_add_gyroscopic_torque_body_nm(three_axis_control_rw_.target_torque_body_Nm,
                                             three_axis_control_rw_.target_torque_body_Nm);
  // 固定トルクの追加
  AOCS_MANAGER_add_constant_torque_body_nm(three_axis_control_rw_.target_torque_body_Nm,
                                           three_axis_control_rw_.target_torque_body_Nm);

  return;
}

void APP_TAC_RW_calc_rot_vec_body_rad_(float rot_vec_body_rad[PHYSICAL_CONST_THREE_DIM])
{
  float rot_vec_body[PHYSICAL_CONST_THREE_DIM] = {1.0f, 0.0f, 0.0f};
  VECTOR3_normalize(rot_vec_body, aocs_manager->quaternion_error_b2t.vector_part);
  float rot_scalar_rad = 2.0f * acosf(aocs_manager->quaternion_error_b2t.scalar_part);
  VECTOR3_scalar_product(rot_vec_body_rad, rot_scalar_rad, rot_vec_body);

  return;
}

CCP_EXEC_STS Cmd_APP_TAC_RW_SET_GAIN(const CommonCmdPacket* packet)
{
  const uint8_t* param = CCP_get_param_head(packet);
  int param_id = 0;
  uint8_t att_omega_flag = param[param_id];
  param_id += (int)sizeof(uint8_t);
  if (att_omega_flag >= 2) return CCP_EXEC_ILLEGAL_PARAMETER;

  uint8_t axis = param[param_id];
  param_id += (int)sizeof(uint8_t);
  if (axis >= PHYSICAL_CONST_THREE_DIM) return CCP_EXEC_ILLEGAL_PARAMETER;

  PidGains gains;
  endian_memcpy(&gains.p_gain, param + param_id, sizeof(float));
  param_id += (int)sizeof(float);
  endian_memcpy(&gains.i_gain, param + param_id, sizeof(float));
  param_id += (int)sizeof(float);
  endian_memcpy(&gains.d_gain, param + param_id, sizeof(float));
  param_id += (int)sizeof(float);

  C2A_MATH_ERROR ret;
  if (att_omega_flag == 0)
  {
    three_axis_control_rw_.gains_omega_cmd_tmp[axis] = gains;
  }
  else
  {
    three_axis_control_rw_.gains_att_cmd_tmp[axis] = gains;
  }

  return CCP_EXEC_SUCCESS;
}

CCP_EXEC_STS Cmd_APP_TAC_RW_SAVE_GAIN(const CommonCmdPacket* packet)
{
  for (uint8_t axis = 0; axis < PHYSICAL_CONST_THREE_DIM; axis++)
  {
    PID_CONTROL_set_gain(&three_axis_control_rw_.pid_omega[axis],
                         three_axis_control_rw_.gains_omega_cmd_tmp[axis]);
    PID_CONTROL_set_gain(&three_axis_control_rw_.pid_att[axis],
                         three_axis_control_rw_.gains_att_cmd_tmp[axis]);
  }
  return CCP_EXEC_SUCCESS;
}

#pragma section
