/**
 * @file
 * @brief  BCTのCoreTlmパラメタのオーバーライド用ヘッダー
 */
#ifndef BLOCK_COMMAND_TABLE_PARAMS_H_
#define BLOCK_COMMAND_TABLE_PARAMS_H_

#undef BCT_CMD_MAX_LENGTH
#undef BCT_MAX_CMD_NUM
#undef BCT_MAX_BLOCKS

#undef SIZE_OF_BCT_ID_T


#define BCT_CMD_MAX_LENGTH (32)
#define BCT_MAX_CMD_NUM (16)
#define BCT_MAX_BLOCKS (80)
// ここ変えたら，Cmd_RESET_(RE)STORE_BCT* も変える！

#define SIZE_OF_BCT_ID_T     (2)

#endif
