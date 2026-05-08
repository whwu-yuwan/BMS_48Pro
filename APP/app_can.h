#ifndef __APP_CAN_H
#define __APP_CAN_H

#include <stdint.h>
#include "bsp_can.h"

/**
 * @brief APP 层 CAN 二次封装
 *
 * 说明：
 * - HAL 层初始化/启动由 MX_CAN_Init() 完成（滤波与 HAL_CAN_Start 已在 Core/Src/can.c 中处理）
 * - APP 层只关心：协议 ID、数据打包/解包、收发调用
 *
 * 目前仅提供“最简单可用”的测试/基础状态帧，便于快速验证是否能收发 CAN 包。
 */

/**
 * @brief CAN 报文 ID 定义（标准帧）
 */
#define APP_CAN_ID_TEST_STD      0x123u // 测试帧
#define APP_CAN_ID_BMS_BASIC_STD 0x321u // BMS 基础状态帧
#define APP_CAN_ID_MOS_CTRL_STD  0x322u // MOS 控制帧
#define APP_CAN_ID_MOS_ACK_STD   0x323u // MOS 控制确认帧
#define APP_CAN_ID_ALARM_STD     0x320u // 报警状态帧

/**
 * @brief CAN 接收报文 ID ,上位机发送给下位机的控制帧
 * */
#define APP_CAN_ID_RX_MOS_CTRL_STD 0x324u // 充放电MOS控制帧
#define APP_CAN_ID_RX_ALARM_STD    0x325u // 取消报警状态帧




/**
 * @brief BMS 基础状态帧（StdID=0x321，DLC=6）
 *
 * Byte0-1: pack_mV   (uint16，小端，单位 mV)
 * Byte2-3: current_mA(int16，小端，单位 mA，充/放电正负号由项目定义)
 * Byte4  : soc       (uint8，0-100)
 * Byte5  : cell_cnt  (uint8，当前有效串数)
 */
typedef struct
{
	uint16_t pack_mV;
	int16_t current_mA;
	uint8_t soc;
	uint8_t cell_count;
} APP_CAN_BmsBasic_t;

typedef struct
{
	uint8_t charge_mos;
	uint8_t discharge_mos;
} APP_CAN_MosCtrl_t;

/**
 * @brief APP 层初始化（最简版可为空）
 * @return 0 成功；非 0 失败
 */
uint8_t APP_CAN_Init(void);

/**
 * @brief 发送测试帧（StdID=0x123，DLC=1）
 * @param counter 测试计数
 * @return 0 成功；非 0 失败
 */
uint8_t APP_CAN_SendTest(uint8_t counter);

/**
 * @brief 发送 BMS 基础状态帧（StdID=0x321，DLC=6）
 * @param basic BMS 基础状态
 * @return 0 成功；非 0 失败
 */
uint8_t APP_CAN_SendBmsBasic(const APP_CAN_BmsBasic_t *basic);

/**
 * @brief 尝试接收一帧（轮询 FIFO0）
 * @param frame 输出帧
 * @return 0 成功收到；1 当前无数据；2 参数/硬件错误；3 CAN 未处于 LISTENING
 */
uint8_t APP_CAN_TryReceive(BSP_CAN_Frame_t *frame);

/**
 * @brief 解包基础状态帧（StdID=0x321，DLC>=6）
 * @param frame 输入帧
 * @param out 输出结构体
 * @return 0 解包成功；非 0 不匹配/失败
 */
uint8_t APP_CAN_DecodeBmsBasic(const BSP_CAN_Frame_t *frame, APP_CAN_BmsBasic_t *out);

uint8_t APP_CAN_DecodeMosCtrl(const BSP_CAN_Frame_t *frame, APP_CAN_MosCtrl_t *out);

#endif
