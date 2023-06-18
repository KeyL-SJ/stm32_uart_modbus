/********************************************************************************
  * @file    modbus_crc.h
  * @author  TenPao Software Team / 林世杰
  * @version V0.0.1.0
  * @date    2023/06/12
  * @brief   modbus_crc.c配套头文件
  * @note    Chip:STM32F030C8T6
  *
  *          Copyright (C) 1979-2023 TenPao Ltd. All rights reserved.
  *******************************************************************************/
	
#ifndef __MODBUS_CRC_H_
#define __MODBUS_CRC_H_ /*!< header guard */

#ifdef __cplusplus   /*!< start of "if cpp file used" */
extern "C" {
#endif


#include "usart.h"

uint16_t modbus_crc16(uint8_t *pbuffer, uint16_t length);
#ifdef __cplusplus   /*!< end of "if cpp file used" */
}
#endif

#endif               /*!< __MODBUS_CRC_H_ END */
