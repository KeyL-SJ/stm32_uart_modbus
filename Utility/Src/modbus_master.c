/********************************************************************************
 * @file    modbus_master.c
 * @author  TenPao Software Team / 林世杰
 * @version V0.0.1.6
 * @date    2023/06/12
 * @brief   提供功能控制函数
 * @note    Chip:STM32F030C8T6
 *
 *          Copyright (C) 1979-2023 TenPao Ltd. All rights reserved.
 *******************************************************************************/
#include "modbus_master.h"
#include "modbus_crc.h"

modbus modbus_struct;

uint16_t reg[] = {
    0x0001,
    0x0002,
    0x0003,
    0x0004,
    0x0005,
    0x0006,
    0x0007,
    0x0008,
};

/**
 * @brief         本机作为从机时modbus初始化地址
 * @param[in]     无
 * @param[out]    无
 * @retval        无
 * @note          无
 */
void modbus_init(void)
{
    modbus_struct.address = 0x01;
    modbus_struct.slave_address = 0x01; /*!< 主机要匹配的从机地址 */
}

/**
 * @brief         本机作为从机时， modbus 0x03功能码函数
 *                接收主机的数据之后，返回给主机相应数据
 * @param[in]     无
 * @param[out]    无
 * @retval        无
 * @note          无
 */
static void modbus_slave_function_0x03(void)
{
    uint16_t reg_address, reg_length, crc;
    uint8_t i = 0;

    reg_address = modbus_struct.receive_buffer[2] * 256 + modbus_struct.receive_buffer[3]; /*!< 读取寄存器首地址 */
    reg_length = modbus_struct.receive_buffer[4] * 256 + modbus_struct.receive_buffer[5];  /*!< 读取寄存器个数 */

    /*!< 给主机的应答 */
    modbus_struct.send_buffer[i++] = modbus_struct.address;
    modbus_struct.send_buffer[i++] = 0x03;
    modbus_struct.send_buffer[i++] = ((reg_length * 2) % 256);/*!< 返回字节个数 */

    for (int j = 0; j < reg_length; j++) /*!< 返回主机要读取的数据 */
    {
        modbus_struct.send_buffer[i++] = reg[reg_address + j] / 256;
        modbus_struct.send_buffer[i++] = reg[reg_address + j] % 256;
    }

    crc = modbus_crc16(modbus_struct.send_buffer, i); /*!< 计算CRC校验位 */
    modbus_struct.send_buffer[i++] = crc / 256;
    modbus_struct.send_buffer[i++] = crc % 256;

    HAL_UART_Transmit_DMA(&huart1, modbus_struct.send_buffer,i); /*!< 发送数据 */
}

/**
 * @brief         本机作为从机时， modbus 0x06功能码函数
 *                接收主机的数据之后，根据主机要求修改数据
 * @param[in]     无
 * @param[out]    无
 * @retval        无
 * @note          无
 */
static void modbus_slave_function_0x06(void)
{
    uint16_t reg_address, val, crc;

    reg_address = modbus_struct.receive_buffer[2] * 256 + modbus_struct.receive_buffer[3];
    val = modbus_struct.receive_buffer[4] * 256 + modbus_struct.receive_buffer[5]; /*!< 读取修改的数据值 */
    reg[reg_address] = val; /*!< 数据写入 */

    /*!< 给主机的应答 */
    for (int i = 0; i < 6; i++)
    {
        modbus_struct.send_buffer[i] = modbus_struct.receive_buffer[i];
    }

    crc = modbus_crc16(modbus_struct.send_buffer, 6);
    modbus_struct.send_buffer[6] = crc / 256;
    modbus_struct.send_buffer[7] = crc % 256;

    HAL_UART_Transmit_DMA(&huart1, modbus_struct.send_buffer,8);
}

/**
 * @brief         本机作为从机时， modbus 0x10功能码函数
 *                接收主机的数据之后，根据主机要求修改多个数据
 * @param[in]     无
 * @param[out]    无
 * @retval        无
 * @note          无
 */
static void modbus_slave_function_0x10(void)
{
    uint16_t reg_address, reg_length, crc;

    reg_address = modbus_struct.receive_buffer[2] * 256 + modbus_struct.receive_buffer[3];
    reg_length = modbus_struct.receive_buffer[4] * 256 + modbus_struct.receive_buffer[5];

    for (int i = 0; i < reg_length; i++)
    {
        reg[reg_address + i] = modbus_struct.receive_buffer[7 + i * 2] * 256 
                             + modbus_struct.receive_buffer[8 + i * 2]; /*!< 数据写入 */
    }

    /*!< 给主机的应答 */
    for (int i = 0; i < 6; i++)
    {
        modbus_struct.send_buffer[i] = modbus_struct.receive_buffer[i];
    }

    crc = modbus_crc16(modbus_struct.send_buffer, 6); /*!< 计算crc校验位 */
    modbus_struct.send_buffer[6] = crc / 256;
    modbus_struct.send_buffer[7] = crc % 256;

    HAL_UART_Transmit_DMA(&huart1,modbus_struct.send_buffer,8);
}

/**
 * @brief         本机作为从机时， modbus事件处理
 * @param[in]     无
 * @param[out]    无
 * @retval        无
 * @note          无
 */
void modbus_slave_event(void)
{
    uint16_t crc, receive_crc;

    if (modbus_struct.receive_flag == 0 || modbus_struct.receive_count < 2)
    {
        return;
    }
    /*!< modbus crc校验 */
    crc = modbus_crc16(&modbus_struct.receive_buffer[0], modbus_struct.receive_count - 2);
    receive_crc = modbus_struct.receive_buffer[modbus_struct.receive_count - 2] * 256 
                + modbus_struct.receive_buffer[modbus_struct.receive_count - 1];
    if (crc == receive_crc)
    {
        if (modbus_struct.receive_buffer[0] == modbus_struct.address)
        {
            switch(modbus_struct.receive_buffer[1])
            {
            case 0x03:
                modbus_slave_function_0x03();
                break;
            case 0x06:
                modbus_slave_function_0x06();
                break;
            case 0x10:
                modbus_slave_function_0x10();
                break;
            default:
                break;
            }
        }
    }
    modbus_struct.receive_count = 0;
    modbus_struct.receive_flag = 0;
}

/**
 * @brief         本机作为主机时，modbus 0x03读取从机数据
 * @param[in]     slave:从机地址
 *                start_address:读取数据起始地址
 *                register_num:读取寄存器个数
 * @param[out]    无
 * @retval        无
 * @note          调用该函数时发送一次数据，之后将host_send_flag置1，之后不再发送
 *                直到接收到从机的应答，将host_send_flag置0，之后可再次发送
 */
void modbus_host_readdata_0x03(uint8_t slave, uint16_t start_address, uint16_t register_num)
{
    uint16_t crc;
    if (modbus_struct.host_send_flag == 0)
    {
        modbus_struct.slave_address = slave;
        modbus_struct.host_send_buffer[0] = slave;                 /*!< 匹配从机地址 */
        modbus_struct.host_send_buffer[1] = 0x03;                  /*!< 功能码 */
        modbus_struct.host_send_buffer[2] = start_address / 256;   /*!< 起始地址高八位 */
        modbus_struct.host_send_buffer[3] = start_address % 256;   /*!< 起始地址低八位 */
        modbus_struct.host_send_buffer[4] = register_num / 256;    /*!< 寄存器个数高八位 */
        modbus_struct.host_send_buffer[5] = register_num % 256;    /*!< 寄存器个数低八位 */
        crc = modbus_crc16(&modbus_struct.host_send_buffer[0], 6); /*!< 计算CRC16校验位 */
        modbus_struct.host_send_buffer[6] = crc / 256;             /*!< CRC16高八位 */
        modbus_struct.host_send_buffer[7] = crc % 256;             /*!< CRC16低八位 */

        HAL_UART_Transmit_DMA(&huart1, modbus_struct.host_send_buffer, 8); /*!< 向从机发送数据 */
        modbus_struct.host_send_flag = 1;                                  /*!< 数据发送完成标志位 */
    }
}

/**
 * @brief         本机作为主机时，modbus 0x06向从机写入一个数据
 * @param[in]     slave:从机地址
 *                reg_address:写入数据的寄存器地址
 *                data:要写入的数据
 * @param[out]    无
 * @retval        无
 * @note          调用该函数时发送一次数据，之后将host_send_flag置1，之后不再发送
 *                直到接收到从机的应答，将host_send_flag置0，之后可再次发送
 */
void modbus_host_writedata_0x06(uint8_t slave, uint16_t reg_address, uint16_t data)
{
    uint16_t crc;
    if (modbus_struct.host_send_flag == 0)
    {
        modbus_struct.slave_address = slave;
        modbus_struct.host_send_buffer[0] = slave;
        modbus_struct.host_send_buffer[1] = 0x06;
        modbus_struct.host_send_buffer[2] = reg_address / 256;
        modbus_struct.host_send_buffer[3] = reg_address % 256;
        modbus_struct.host_send_buffer[4] = data / 256;
        modbus_struct.host_send_buffer[5] = data % 256;
        crc = modbus_crc16(&modbus_struct.host_send_buffer[0], 6);
        modbus_struct.host_send_buffer[6] = crc / 256;
        modbus_struct.host_send_buffer[7] = crc % 256;

        HAL_UART_Transmit_DMA(&huart1, modbus_struct.host_send_buffer, 8); /*!< 向从机发送数据 */
        modbus_struct.host_send_flag = 1;                                  /*!< 数据发送完成,等待接收数据处理 */
    }
}

/**
 * @brief         本机作为主机时，modbus 0x10向从机写入多个数据
 * @param[in]     slave:从机地址
 *                start_address:要写入数据的寄存器起始地址
 *                register_num:要写入数据的寄存器数量
 *                data_length:要写入数据的数据长度 = register_num * 2
 *                pbuffer:要写入的数据
 * @param[out]    无
 * @retval        无
 * @note          调用该函数时发送一次数据，之后将host_send_flag置1，之后不再发送
 *                直到接收到从机的应答，将host_send_flag置0，之后可再次发送
 */
void modbus_host_weiredata_0x10(uint8_t slave, uint16_t start_address, uint16_t register_num, uint8_t data_length, uint8_t* pbuffer)
{
    uint16_t crc;
    if (modbus_struct.host_send_flag == 0)
    {
        modbus_struct.slave_address = slave;
        modbus_struct.host_send_buffer[0] = slave;
        modbus_struct.host_send_buffer[1] = 0x10;
        modbus_struct.host_send_buffer[2] = start_address / 256;
        modbus_struct.host_send_buffer[3] = start_address % 256;
        modbus_struct.host_send_buffer[4] = register_num / 256;
        modbus_struct.host_send_buffer[5] = register_num % 256;
        modbus_struct.host_send_buffer[6] = data_length;
        for (int i = 0; i < data_length; i++)
        {
            modbus_struct.host_send_buffer[7 + i] = pbuffer[i];
        }
        crc = modbus_crc16(&modbus_struct.host_send_buffer[0], 7 + data_length);
        modbus_struct.host_send_buffer[7 + data_length] = crc / 256;
        modbus_struct.host_send_buffer[8 + data_length] = crc % 256;

        HAL_UART_Transmit_DMA(&huart1, modbus_struct.host_send_buffer, 9 + data_length);
        modbus_struct.host_send_flag = 1;
    }
}

/**
 * @brief         本机作为主机时，主机发送数据之后，接收从机应答数据处理
 * @param[in]     无
 * @param[out]    无
 * @retval        无
 * @note          接收到从机的应答后，进行CRC16校验，通过校验后
 *                将host_send_flag置0，之后主机可再次发送数据
 */
void modbus_host_receive_process(void)
{
    uint16_t crc, receive_crc;
    if (modbus_struct.host_send_flag == 1)
    {
        if (modbus_struct.receive_flag == 0 || modbus_struct.receive_count < 2)
        {
            return;
        }
        crc = modbus_crc16(&modbus_struct.receive_buffer[0], modbus_struct.receive_count - 2);
        receive_crc = modbus_struct.receive_buffer[modbus_struct.receive_count - 2] * 256 
                    + modbus_struct.receive_buffer[modbus_struct.receive_count - 1];
        if (crc == receive_crc)
        {
            modbus_struct.host_send_flag = 0; /*!< 数据处理完成，可进行下一次数据发送 */
            modbus_struct.receive_count = 0;  /*!< 每次数据处理完成之后，将本次接收计数清零 */
        }
    }
}
