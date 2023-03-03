#pragma section REPRO
/**
 * @file
 * @brief  PacketHandlerのコマンドルーター，コマンドアナライザのユーザー定義部分
 */
#include "user_packet_handler.h"
#include <src_core/TlmCmd/packet_list_util.h>

void PH_user_init(void)
{
}


PH_ACK PH_user_analyze_cmd(const CommonCmdPacket* packet)
{
  switch (CCP_get_dest_type(packet))
  {
  default:
    return PH_ACK_UNKNOWN;
  }
}


CCP_EXEC_STS PH_user_cmd_router(const CommonCmdPacket* packet)
{
  switch (CCP_get_apid(packet))
  {
  default:
    // 該当する配送先が定義されていない場合。
    return CCP_EXEC_ROUTING_FAILED;
  }
}

#pragma section
