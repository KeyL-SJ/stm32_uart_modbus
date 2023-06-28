# stm32_uart_modbus
基于stm32的uart串口modbus通讯协议
# modbus-RTU协议
**Modbus报文帧结构**
一个报文就是一帧数据，一个数据帧就一个报文： 指的是一串完整的指令数据，本质就是一串数据
Modbus协议在串行链路上的报文格式如下所示：
![image](https://github.com/KeyL-SJ/stm32_uart_modbus/assets/78483846/bcec2fe4-3a28-42b3-b2aa-8d75f8fe1f36)

| 从机地址 | 功能码 |  数据   | CRC校验 |
| :------: | :----: | :-----: | :-----: |
|  1 byte  | 1 byte | N bytes | 2 bytes |

**帧结构 = 从机地址 + 功能吗 + 数据 + 校验**

- 从机地址: 每个从机都有唯一地址，占用一个字节,范围0-255,其中有效范围是1-247,其中255是广播地址(广播就是对所有从机发送应答)

- 功能码: 占用一个字节,功能码的意义就是,知道这个指令是干啥的,比如你可以查询从机的数据,也可以修改从机的数据,所以不同功能码对应不同功能.
- 数据: 根据功能码不同,有不同功能，比方说功能码是查询从机的数据，这里就是查询数据的地址和查询字节数等。
- 校验: 在数据传输过程中可能数据会发生错误，CRC检验检测接收的数据是否正确
# Modbus功能码
**Modbus规定了多个功能，那么为了方便的使用这些功能，我们给每个功能都设定一个功能码，也就是指代码。**

Modbus协议同时规定了二十几种功能码，但是常用的只有8种，用于对存储区的读写，如下表所示：

| 功能码 |    功能说明    |
| :----: | :------------: |
|  01H   |  读取输出线圈  |
|  02H   |  读取输入线圈  |
|  03H   | 读取保持寄存器 |
|  04H   | 读取输入寄存器 |
|  05H   |   写入单线圈   |
|  06H   |  写入单寄存器  |
|  0FH   |   写入多线圈   |
|  10H   |  写入多寄存器  |

当然我们用的最多的就是03和06 一个是读取数据，一个是修改数据。

# CRC校验
**错误校验（CRC）域占用两个字节包含了一个16位的二进制值。CRC值由传输设备计算出来，然后附加到数据帧上，接收设备在接收数据时重新计算CRC值，然后与接收到的CRC域中的值进行比较，如果这两个值不相等，就发生了错误。**

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
    }
# 详细的发送和接收数据：
## 1、主机对从机读数据操作
**主机发送报文格式如下：**

| 从机地址 | 功能码 | 起始地址（高） | 起始地址（低） | 寄存器数量（高） | 寄存器数量（低） |   校验    |
| :------: | :----: | :------------: | :------------: | :--------------: | :--------------: | :-------: |
|   0x01   |  0x03  |      0x00      |      0x01      |       0x00       |       0x01       | 0xD5 0xCA |

**含义：从地址为0x01的从机的0x0001寄存器开始读取数量为1的数据**

- 0x01:从机地址
- 0x03: 查询功能码，读取单个指定从机寄存器的数据
- 0x00 0x01: 要从从机读取数据的寄存器起始地址，表示从从机0x0001开始读取数据
- 0x00 0x01: 要读取的寄存器数量，表示读取一个寄存器的数据
- 0XD5 0XCA:  循环冗余校验 CRC

假设从机0x01的数据如下：

| 寄存器地址 |  数据  |
| :--------: | :----: |
|   0x0000   | 0x0000 |
|   0x0001   | 0x0017 |
|   0x0002   | 0x0020 |
|   0x0003   | 0x0040 |

**那么从机的回复报文格式如下：**

| 从机地址 | 功能码 | 字节数量 | 数据1（高） | 数据1（低） |   校验    |
| :------: | :----: | :------: | :---------: | :---------: | :-------: |
|   0x01   |  0x03  |   0x02   |    0x00     |    0x17     | 0xF8 0x4A |

**含义**：

- 0x01：从机的地址
- 0x03：查询功能，读取从机寄存器的数据
- 0x02： 返回字节数为2个 一个寄存器2个字节
- 0x00 0x17：寄存器的值是0017
- 0xF8 0x4A： 循环冗余校验 CRC

## 2、主机对从机写数据操作

#### 2.1、一次写一个寄存器的数据（0x06）

**主机发送报文格式如下：**

| 从机地址 | 功能码 | 寄存器地址（高） | 寄存器地址（低） | 数据（高） | 数据（低） |   校验    |
| :------: | :----: | :--------------: | :--------------: | :--------: | :--------: | :-------: |
|   0x01   |  0x06  |       0x00       |       0x00       |    0x00    |    0x01    | 0x48 0x0A |

**含义：在地址为0x01的从机的0x0000寄存器写入数据0x0001**

- 0x01:从机地址
- 0x06: 写入功能码，在单个指定从机寄存器写入指定数据
- 0x00 0x00: 要在从机写入数据的寄存器地址，表示在从机0x0000寄存器写入数据
- 0x00 0x01: 要写入的数据
- 0x48 0x0A:  循环冗余校验 CRC

**从机回复报文格式：**0x06功能码的从机回报文与主机发送的报文是一致的，表示成功写入数据

#### 2.2、一次写多个寄存器的数据（0x10）

**主机发送报文格式如下：**

| 从机地址 | 功能码 | 起始地址 | 写入数量  | 数据长度 | 数据 1 | 数据2  | 数据3  |  CRC校验  |
| :------: | :----: | :------: | --------- | :------: | :----: | :----: | :----: | :-------: |
|   0x01   |  0x10  |  0x0000  | 0x00 0x03 |   0x06   | 0x0001 | 0x0002 | 0x0003 | 0x3A 0x81 |

**含义：从地址为0x01的从机的0x0000寄存器开始写入3个数据分别为0x0001、0x0002、0x0003**

- 0x01:从机地址
- 0x10: 批量写入功能码，从指定从机寄存器开始写入指定数量的数据
- 0x00 0x00: 开始从从机写入数据的寄存器地址，表示从从机0x0000寄存器开始写入数据
- 0x00 0x03: 要写入的数据的数量
- 0x06: 要写入的数据长度 = 要写入的数据数量 * 2
- 0x0001、0x0002、0x0003: 要写入的数量
- 0x3A 0x81:  循环冗余校验 CRC

**从机的回复报文格式如下：**

| 从机地址 | 功能码 | 起始地址（高） | 起始地址（低） | 寄存器数量 |   校验    |
| :------: | :----: | :------------: | :------------: | :--------: | :-------: |
|   0x01   |  0x10  |      0x00      |      0x00      | 0x00 0x03  | 0x80 0x08 |

**含义**：

- 0x01：从机的地址
- 0x10：批量写入功能码，从指定从机寄存器开始写入指定数量的数据
- 0x00 0x00：开始从从机写入数据的寄存器地址，表示从从机0x0000寄存器开始写入数据
- 0x00 0x03：寄存器数量，表示成功写入3个寄存器
- 0x80 0x08： 循环冗余校验 CRC

# 实战示例

#### 1、Modbus Poll&Modbus Slave

在开始写代码之前可以先通过Modbus Poll和Modbus Slave两个软件来模拟实验，首先通过Virtual Serial Port Driver Pro虚拟串口软件创建两个虚拟串口

![虚拟串口](.\image\虚拟串口.png)

之后在Modbus Poll和Modbuus Slave中连接虚拟出来的串口，注意波特率等参数的配置，二者要相同

![Modbus Slave连接配置](.\image\Modbus Slave连接配置.png)

连接成功之后，Modbus Poll会实时的读取Modbus Slave中的全部数据，也可以通过Modbus Poll修改Modbus Slave中的数据，可以通过Modbus Poll工具栏中的放大镜查看具体的报文

![Modbus Poll数据页](.\image\Modbus Poll数据页.png)

# 2、STM32作为从机，串口调试助手作为主机

本系统中使用STM32作为从机，串口调试助手作为主机模拟modbus通讯功能效果如下：

![串口助手主机STM32从机](.\image\串口助手主机STM32从机.png)

串口调试助手使用**SSCOM**，这款串口调试助手是我用过串口调试助手中唯一一个带有加ModbusCRC16校验位功能的，用于modbus调试非常舒服，在上图的测试中，使用串口调试助手作为主机向STM32发送了01 03 00 00 00 01 84 0A，其中84 0A为CRC16校验位，是串口调试助手自动加上去的。串口调试助手发送的报文含义为从**地址为0x01**的从机的**0x0000寄存器**开始读取**数量为1**的数据，通过keil的Debug界面可以看到STM32的0x0000寄存器的数据为0x0001，而串口调试助手成功接收到了STM32的应答报文。

代码实现如下：

```c
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
    crc = modbus_crc16(&modbus_struct.receive_buffer[0]
                      , modbus_struct.receive_count - 2);
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
```
代码中定义了一个stm32作为从机时候的事件处理函数，接收串口接收的数据，通过CRC校验之后，根据主机发送来的功能码做出响应，以0x03功能码为例，其代码具体如下：

```c
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

    reg_address = modbus_struct.receive_buffer[2] * 256 
        		+ modbus_struct.receive_buffer[3]; /*!< 读取寄存器首地址 */
    reg_length = modbus_struct.receive_buffer[4] * 256 
        	   + modbus_struct.receive_buffer[5];  /*!< 读取寄存器个数 */

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
```
# 3、STM32作为主机，Modbus Slave作为从机

本系统中使用STM32作为主机，Modbus Slave作为从机模拟modbus通讯功能效果如下：

![STM32主机Modbus Slave从机](.\image\STM32主机Modbus Slave从机.png)

以功能码0x10为例，在连接成功之后，可以通过keil debug中修改modbus_host_0x10_buffer数组中的数据来改变Modbus Slave中的值

具体代码实现如下：

```c
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
```
