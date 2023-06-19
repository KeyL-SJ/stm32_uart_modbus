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
