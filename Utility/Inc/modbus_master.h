/********************************************************************************
 * @file    modbus_master.h
 * @author  TenPao Software Team / 林世杰
 * @version V0.0.1.0
 * @date    2023/06/12
 * @brief   modbus_master.c配套头文件
 * @note    Chip:STM32F030C8T6
 *
 *          Copyright (C) 1979-2023 TenPao Ltd. All rights reserved.
 *******************************************************************************/

#ifndef __MODBUS_MASTER_H_
#define __MODBUS_MASTER_H_ /*!< header guard */

#ifdef __cplusplus /*!< start of "if cpp file used" */
extern "C"
{
#endif


#include "usart.h"

typedef struct
{
    /*!< 作为从机时使用 */
    uint8_t address;                                /*!< 本机地址 */
    uint8_t receive_buffer[recv_buffer_max_length]; /*!< modbus接收缓冲区 */
    uint8_t send_buffer[recv_buffer_max_length];    /*!< modbus发送缓冲区 */
    uint8_t receive_count;                          /*!< modbus端口接收到的数据个数 */
    uint8_t receive_flag;                           /*!< modbus一帧数据接收完成标志位 */

    /*!< 作为主机添加部分 */
    uint8_t host_send_buffer[recv_buffer_max_length]; /*!< modbus主机发送数据缓冲区 */
    uint8_t slave_address;                            /*!< 要匹配的从机设备地址 */
    uint8_t host_send_flag;                           /*!< 主机设备发送数据完毕标志位 */
} modbus;

extern modbus modbus_struct;

void modbus_host_readdata_0x03(uint8_t slave, uint16_t start_address, uint16_t data_num);
void modbus_host_writedata_0x06(uint8_t slave, uint16_t data_address, uint16_t data);
void modbus_host_weiredata_0x10(uint8_t slave, uint16_t start_address, uint16_t register_num, uint8_t data_length, uint8_t *pbuffer);
void modbus_host_receive_process(void);

void modbus_init(void);
void modbus_slave_event(void);

#ifdef __cplusplus /*!< end of "if cpp file used" */
}
#endif

#endif /*!< __MODBUS_MASTER_H_ END */
