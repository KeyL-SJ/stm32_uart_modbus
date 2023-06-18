# stm32_uart_modbus
基于stm32的uart串口modbus通讯协议
# modbus-RTU协议
Modbus报文帧结构
一个报文就是一帧数据，一个数据帧就一个报文： 指的是一串完整的指令数据，本质就是一串数据
Modbus协议在串行链路上的报文格式如下所示：
![image](https://github.com/KeyL-SJ/stm32_uart_modbus/assets/78483846/bcec2fe4-3a28-42b3-b2aa-8d75f8fe1f36)
![image](https://github.com/KeyL-SJ/stm32_uart_modbus/assets/78483846/08571b8d-8013-4afb-9fe5-393f01fab22f)
从机地址: 每个从机都有唯一地址，占用一个字节,范围0-255,其中有效范围是1-247,其中255是广播地址(广播就是对所有从机发送应答)

#### 功能码: 占用一个字节,功能码的意义就是,知道这个指令是干啥的,比如你可以查询从机的数据,也可以修改从机的数据,所以不同功能码对应不同功能.
#### 数据: 根据功能码不同,有不同功能，比方说功能码是查询从机的数据，这里就是查询数据的地址和查询字节数等。
#### 校验: 在数据传输过程中可能数据会发生错误，CRC检验检测接收的数据是否正确
# Modbus功能码
## Modbus规定了多个功能，那么为了方便的使用这些功能，我们给每个功能都设定一个功能码，也就是指代码。
Modbus协议同时规定了二十几种功能码，但是常用的只有8种，用于对存储区的读写，如下表所示：
![image](https://github.com/KeyL-SJ/stm32_uart_modbus/assets/78483846/5f6a0848-b9b6-4fac-8ccb-8aa1ed3e53f4)
当然我们用的最多的就是03和06 一个是读取数据，一个是修改数据。

# CRC校验
## 错误校验（CRC）域占用两个字节包含了一个16位的二进制值。CRC值由传输设备计算出来，然后附加到数据帧上，接收设备在接收数据时重新计算CRC值，然后与接收到的CRC域中的值进行比较，如果这两个值不相等，就发生了错误。
例如若主机向从机发送报文01 03 00 00 00 01 84 0A 其中， 最后两个字节84 0A就是CRC校验位，从机接收到主机发送的报文之后，根据报文的非校验位01 03 00 00 00 01计算CRC校验位，若从机计算出的校验位与主机发送的校验位相同，则证明数据在发送的过程中没有发生错误，反之，则代表数据传输发生错误。
## CRC校验流程
1、预置一个16位寄存器为0FFFFH（全1），称之为CRC寄存器。
2 、把数据帧中的第一个字节的8位与CRC寄存器中的低字节进行异或运算，结果存回CRC寄存器。
3、将CRC寄存器向右移一位，最高位填以0，最低位移出并检测。
4 、如果最低位为0：重复第三步（下一次移位）；如果最低位为1：将CRC寄存器与一个预设的固定值（0A001H）进行异或运算。
5、重复第三步和第四步直到8次移位。这样处理完了一个完整的八位。
6 、重复第2步到第5步来处理下一个八位，直到所有的字节处理结束。
7、最终CRC寄存器的值就是CRC的值。
看着很复杂哈，其实理解了原理就很简单了，这里贴出本项目中CRC校验的代码
'/**
 * @brief         modbus_crc16校验位计算
 * @param[in]     *pbuffer:待校验数据
 *                length:待校验数据长度
 * @param[out]    16位crc校验位
 * @retval        无
 * @note          无
 */
uint16_t modbus_crc16(uint8_t *pbuffer, uint16_t length)
{
    uint16_t crc_high = 0xff;
    uint16_t crc_low = 0xff;
    unsigned long index;
    
    while (length--)
    {
        index = crc_high ^ *pbuffer++;
        crc_high = crc_low ^ auchCRCHi[index];
        crc_low = auchCRCLo[index];
    }
    return (crc_high << 8 | crc_low);
}'
