#include "sht.h"
#include "iic.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

gpioPinInfo_s g_clkInfo = {SHT_CLK_PORT, SHT_CLK_PIN};
gpioPinInfo_s g_sdaInfo = {SHT_SDA_PORT, SHT_SDA_PIN};

// 初始化SHT传感器
void ShtInit(void)
{
    // 使能SHT时钟
    SHT_CLK_RCC_ENABLE();
    SHT_SDA_RCC_ENABLE();
    // 初始化SHT传感器
    GPIO_InitTypeDef shtHandle= {0};
    shtHandle.Pin = SHT_CLK_PIN;
    shtHandle.Mode = GPIO_MODE_OUTPUT_OD;
    shtHandle.Pull = GPIO_PULLUP;
    shtHandle.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SHT_CLK_PORT, &shtHandle);

    shtHandle.Pin = SHT_SDA_PIN;
    HAL_GPIO_Init(SHT_SDA_PORT, &shtHandle);

    IicStop(g_clkInfo, g_sdaInfo);
}



/**
 * @brief 计算SHT传感器的校验和
 *
 * @param highByte 高字节
 *
 * @param lowByte 低字节
 *
 * @param checksum 校验和指针
 *
 * @return true 校验和校验通过 false 校验和校验不通过
 */
static bool CalculateChecksum(const uint8_t highByte, const uint8_t lowByte, const uint8_t checksum)
{
    uint8_t crc = 0xFF;
    const uint8_t datalen = 2;
    const uint8_t data[2] = {highByte, lowByte};

    for (uint8_t i = 0; i < datalen; i++)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ 0x31;
            }
            else
            {
                crc = (crc << 1);
            }
        }
    }

    if(crc == checksum)
    {
        return true;
    }

    return false;
}


// 读取SHT传感器的温度湿度数据
static bool ReadTempHumid(gpioPinInfo_s clkPin, gpioPinInfo_s sdaPin, uint8_t *pReceiveData)
{
    // 读取数据成功标志
    bool readSuccess = false;

    if(pReceiveData == NULL)
    {
        return readSuccess;
    }

    // 发送读设备指令
    SendByte(clkPin, sdaPin, SHT_ADDR_READ);
    // 检查应答信号
    if(false == CheckAckSignal(clkPin, sdaPin))
    {
        return false;
    }
    // 使ACK完成后拉低SCL，ReceiveByte能从正确的SCL低电平开始
    SetSclPin(clkPin, GPIO_PIN_RESET);
    IicDelayUs(4000);
    for(int i = 0; i < RECEIVE_INDEX_NUM; i++)
    {
        pReceiveData[i] = ReceiveByte(clkPin, sdaPin);
        // 只有最后一个字节需要发送非应答信号
        if(i == RECEIVE_INDEX_NUM - 1)
        {
            IicSendAck(clkPin, sdaPin, false);
            IicStop(clkPin, sdaPin);
        }
        else
        {
            IicSendAck(clkPin, sdaPin, true);
        }
    }

    readSuccess = true;
    return readSuccess;
}


/**
 * @brief 发送SHT传感器的传输命令字
 *
 * @param clkPin 时钟引脚信息
 *
 * @param sdaPin 数据引脚信息
 *
 * @return true 发送成功(接收到应答信号) false 发送失败（未接收到应答信号）
*/
static bool SendTransmitCommand(gpioPinInfo_s clkPin, gpioPinInfo_s sdaPin)
{
    bool isAck = false;
    // 发送向器件写指令
    SendByte(clkPin, sdaPin, SHT_ADDR_WRITE);
    // 检查应答信号
    if(true == CheckAckSignal(clkPin, sdaPin))
    {
        // 发送单次读写指令
        SendByte(clkPin, sdaPin, SINGLE_SHOT_COMMAND_MSB);
        isAck = CheckAckSignal(clkPin, sdaPin);
        if(true == isAck)
        {
            SendByte(clkPin, sdaPin, SINGLE_SHOT_COMMAND_LSB);
            isAck = CheckAckSignal(clkPin, sdaPin);
            if(true == isAck)
            {
                return true;
            }
        }

    }

    // ACK失败，发送STOP释放总线
    IicStop(clkPin, sdaPin);
    return false;
}


/**
 * @brief 获取SHT传感器的温度湿度数据
 *
 * @param pTempRaw 温度原始数据指针（可为NULL）
 * @param pHumidRaw 湿度原始数据指针（可为NULL）
 *
 * @return true:校验温度湿度数据成功  false:校验温度湿度数据失败
 * */
bool GetTempHumidProcess(uint16_t *pTempRaw, uint16_t *pHumidRaw)
{
    bool returnResult = false;
    uint8_t receiveData[RECEIVE_INDEX_NUM] = {0};

    // 发送起始信号
    IicStart(g_clkInfo, g_sdaInfo);
    // 发送命令字
    returnResult = SendTransmitCommand(g_clkInfo, g_sdaInfo);
    if(returnResult == true)
    {
        // 拉低SCL完成ACK周期，使从机释放SDA
        // SetSclPin(g_clkInfo, GPIO_PIN_RESET);
        // IicDelayUs(2);
        // 发送停止信号，结束命令阶段
        IicStop(g_clkInfo, g_sdaInfo);
        // 等待测量完成（中重复度最大 6ms）
        IicDelayUs(6000);
        // 发送起始信号，开始读取数据
        IicStart(g_clkInfo, g_sdaInfo);
        returnResult = ReadTempHumid(g_clkInfo, g_sdaInfo, receiveData);
        if(true == returnResult)
        {
            // 计算校验和，检查数据有效性
            bool tempChecksumRes = CalculateChecksum(receiveData[TEMP_HIGH_BYTE_INDEX],
                                                        receiveData[TEMP_LOW_BYTE_INDEX],
                                                        receiveData[TEMP_CHECKSUM_INDEX]);
            bool humidChecksumRes = CalculateChecksum(receiveData[HUMID_HIGH_BYTE_INDEX],
                                                        receiveData[HUMID_LOW_BYTE_INDEX],
                                                        receiveData[HUMID_CHECKSUM_INDEX]);
            if(tempChecksumRes == true && humidChecksumRes == true)
            {
                // 校验通过，输出原始数据
                if(pTempRaw != NULL)
                {
                    *pTempRaw = ((uint16_t)receiveData[TEMP_HIGH_BYTE_INDEX] << 8) | receiveData[TEMP_LOW_BYTE_INDEX];
                }
                if(pHumidRaw != NULL)
                {
                    *pHumidRaw = ((uint16_t)receiveData[HUMID_HIGH_BYTE_INDEX] << 8) | receiveData[HUMID_LOW_BYTE_INDEX];
                }
                return true;
            }
        }

    }
    return false;
}


/**
 * @brief 根据原始数据计算温度值
 *
 * @param tempRaw 温度原始数据（16位）
 *
 * @return float 温度值（单位：°C）
*/
float CalculateTemperature(uint16_t tempRaw)
{
    return -45.0f + 175.0f * ((float)tempRaw / 65535.0f);
}


/**
 * @brief 根据原始数据计算湿度值
 *
 * @param humidRaw 湿度原始数据（16位）
 *
 * @return float 湿度值（单位：%RH）
*/
float CalculateHumidity(uint16_t humidRaw)
{
    return 100.0f * ((float)humidRaw / 65535.0f);
}
