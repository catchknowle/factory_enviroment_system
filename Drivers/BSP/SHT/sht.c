/**
 * @file sht.c
 * @brief SHT30传感器驱动
 */

#include "sht.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "QueueManage.h"
#include "./BSP/LED/led.h"

#define SINGLE_SHOT_COMMAND_MSB (0x2CU)
#define SINGLE_SHOT_COMMAND_LSB (0x0DU)
#define SHT30_MEASURE_WAIT_MS (6U)
#define SHT30_READ_WAIT_MS (4U)

static void Sht30DelayMs(uint32_t delayMs);
static bool SoftIicSht30Init(void *pBusCtx);
static bool SoftIicSht30Write(void *pBusCtx, uint8_t deviceAddr, const uint8_t *pWriteData, uint16_t writeLen);
static bool SoftIicSht30Read(void *pBusCtx, uint8_t deviceAddr, uint8_t *pReadData, uint16_t readLen);
static bool Sht30IsDeviceValid(const Sht30DeviceStructType *pDevice);
static bool Sht30CalculateChecksum(uint8_t highByte, uint8_t lowByte, uint8_t checksum);
static bool Sht30SendSingleShotCommand(Sht30DeviceStructType *pDevice);

// 默认SCL时钟使能函数
static void Sht30DefaultSclClockEnable(void)
{
    SHT30_CLK_RCC_ENABLE();
}

// 默认SDA时钟使能函数
static void Sht30DefaultSdaClockEnable(void)
{
    SHT30_SDA_RCC_ENABLE();
}

// 默认软件IIC总线对象
static SoftIicBusStructType g_sht30DefaultSoftIicBus =
{
    {SHT30_CLK_PORT, SHT30_CLK_PIN},
    {SHT30_SDA_PORT, SHT30_SDA_PIN},
    Sht30DefaultSclClockEnable,
    Sht30DefaultSdaClockEnable
};

// 默认软件IIC操作对象
const Sht30OpsStructType g_sht30SoftIicOps =
{
    SoftIicSht30Init,
    SoftIicSht30Write,
    SoftIicSht30Read,
    Sht30DelayMs
};

// 默认SHT30设备对象
static Sht30DeviceStructType g_sht30DefaultDevice =
{
    &g_sht30SoftIicOps,
    &g_sht30DefaultSoftIicBus,
    SHT30_ADDR_WRITE,
    SHT30_ADDR_READ
};

/**
 * @brief       毫秒级延时
 * @param       delayMs : 需要延时的毫秒数
 * @retval      无
 */
static void Sht30DelayMs(uint32_t delayMs)
{
    // 校验延时参数是否有效
    if (delayMs == 0U)
    {
        return;
    }

    // 判断调度器是否已经启动
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        vTaskDelay(pdMS_TO_TICKS(delayMs));
        return;
    }

    IicDelayUs(delayMs * 1000U);
}

/**
 * @brief       校验SHT30设备对象是否有效
 * @param       pDevice : SHT30设备对象指针
 * @retval      true    : 设备对象有效
 * @retval      false   : 设备对象无效
 */
static bool Sht30IsDeviceValid(const Sht30DeviceStructType *pDevice)
{
    // 校验设备对象指针
    if (pDevice == NULL)
    {
        return false;
    }

    // 校验底层操作对象
    if (pDevice->pOps == NULL)
    {
        return false;
    }

    // 校验底层操作函数指针
    if ((pDevice->pOps->Init == NULL) || (pDevice->pOps->Write == NULL) || (pDevice->pOps->Read == NULL))
    {
        return false;
    }

    // 校验总线上下文
    if (pDevice->pBusCtx == NULL)
    {
        return false;
    }

    return true;
}

/**
 * @brief       软件IIC方式初始化SHT30总线
 * @param       pBusCtx : 软件IIC总线上下文指针
 * @retval      true    : 初始化成功
 * @retval      false   : 初始化失败
 */
static bool SoftIicSht30Init(void *pBusCtx)
{
    // 软件IIC总线对象指针
    SoftIicBusStructType *pSoftIicBus = (SoftIicBusStructType *)pBusCtx;
    // GPIO初始化结构体
    GPIO_InitTypeDef gpioInitStruct = {0};

    // 校验软件IIC总线对象
    if (pSoftIicBus == NULL)
    {
        return false;
    }

    // 判断是否配置SCL时钟使能函数
    if (pSoftIicBus->pSclClockEnable != NULL)
    {
        pSoftIicBus->pSclClockEnable();
    }

    // 判断是否配置SDA时钟使能函数
    if (pSoftIicBus->pSdaClockEnable != NULL)
    {
        pSoftIicBus->pSdaClockEnable();
    }

    gpioInitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    gpioInitStruct.Pull = GPIO_PULLUP;
    gpioInitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    gpioInitStruct.Pin = pSoftIicBus->sclPin.Pin;
    HAL_GPIO_Init(pSoftIicBus->sclPin.GPIOx, &gpioInitStruct);

    gpioInitStruct.Pin = pSoftIicBus->sdaPin.Pin;
    HAL_GPIO_Init(pSoftIicBus->sdaPin.GPIOx, &gpioInitStruct);

    IicStop(pSoftIicBus->sclPin, pSoftIicBus->sdaPin);
    return true;
}

/**
 * @brief       软件IIC方式向SHT30写入数据
 * @param       pBusCtx     : 软件IIC总线上下文指针
 * @param       deviceAddr  : 设备写地址
 * @param       pWriteData  : 待写入数据缓冲区
 * @param       writeLen    : 待写入数据长度
 * @retval      true        : 写入成功
 * @retval      false       : 写入失败
 */
static bool SoftIicSht30Write(void *pBusCtx, uint8_t deviceAddr, const uint8_t *pWriteData, uint16_t writeLen)
{
    // 软件IIC总线对象指针
    SoftIicBusStructType *pSoftIicBus = (SoftIicBusStructType *)pBusCtx;
    // 写入索引
    uint16_t writeIndex = 0U;

    // 校验输入参数
    if ((pSoftIicBus == NULL) || (pWriteData == NULL) || (writeLen == 0U))
    {
        return false;
    }

    IicStart(pSoftIicBus->sclPin, pSoftIicBus->sdaPin);
    SendByte(pSoftIicBus->sclPin, pSoftIicBus->sdaPin, deviceAddr);

    // 检查设备地址应答信号
    if (CheckAckSignal(pSoftIicBus->sclPin, pSoftIicBus->sdaPin) == false)
    {
        IicStop(pSoftIicBus->sclPin, pSoftIicBus->sdaPin);
        return false;
    }

    // 循环写入待发送数据
    for (writeIndex = 0U; writeIndex < writeLen; writeIndex++)
    {
        SendByte(pSoftIicBus->sclPin, pSoftIicBus->sdaPin, pWriteData[writeIndex]);

        // 检查当前字节应答信号
        if (CheckAckSignal(pSoftIicBus->sclPin, pSoftIicBus->sdaPin) == false)
        {
            IicStop(pSoftIicBus->sclPin, pSoftIicBus->sdaPin);
            return false;
        }
    }

    IicStop(pSoftIicBus->sclPin, pSoftIicBus->sdaPin);
    return true;
}

/**
 * @brief       软件IIC方式从SHT30读取数据
 * @param       pBusCtx     : 软件IIC总线上下文指针
 * @param       deviceAddr  : 设备读地址
 * @param       pReadData   : 数据接收缓冲区
 * @param       readLen     : 读取数据长度
 * @retval      true        : 读取成功
 * @retval      false       : 读取失败
 */
static bool SoftIicSht30Read(void *pBusCtx, uint8_t deviceAddr, uint8_t *pReadData, uint16_t readLen)
{
    // 软件IIC总线对象指针
    SoftIicBusStructType *pSoftIicBus = (SoftIicBusStructType *)pBusCtx;
    // 读取索引
    uint16_t readIndex = 0U;

    // 校验输入参数
    if ((pSoftIicBus == NULL) || (pReadData == NULL) || (readLen == 0U))
    {
        return false;
    }

    IicStart(pSoftIicBus->sclPin, pSoftIicBus->sdaPin);
    SendByte(pSoftIicBus->sclPin, pSoftIicBus->sdaPin, deviceAddr);

    // 检查设备地址应答信号
    if (CheckAckSignal(pSoftIicBus->sclPin, pSoftIicBus->sdaPin) == false)
    {
        IicStop(pSoftIicBus->sclPin, pSoftIicBus->sdaPin);
        return false;
    }

    Sht30DelayMs(SHT30_READ_WAIT_MS);

    // 循环接收传感器返回数据
    for (readIndex = 0U; readIndex < readLen; readIndex++)
    {
        pReadData[readIndex] = ReceiveByte(pSoftIicBus->sclPin, pSoftIicBus->sdaPin);
        IicSendAck(pSoftIicBus->sclPin, pSoftIicBus->sdaPin, (readIndex + 1U) < readLen);
    }

    IicStop(pSoftIicBus->sclPin, pSoftIicBus->sdaPin);
    return true;
}

/**
 * @brief       计算SHT30数据CRC校验结果
 * @param       highByte : 数据高字节
 * @param       lowByte  : 数据低字节
 * @param       checksum : 传感器返回的CRC值
 * @retval      true     : CRC校验通过
 * @retval      false    : CRC校验失败
 */
static bool Sht30CalculateChecksum(uint8_t highByte, uint8_t lowByte, uint8_t checksum)
{
    // CRC初始值
    uint8_t crc = 0xFFU;
    // 待计算CRC的数据数组
    uint8_t data[2] = {highByte, lowByte};
    // 数据索引
    uint8_t dataIndex = 0U;
    // 位索引
    uint8_t bitIndex = 0U;

    // 遍历两个数据字节
    for (dataIndex = 0U; dataIndex < 2U; dataIndex++)
    {
        crc ^= data[dataIndex];

        // 逐位执行CRC计算
        for (bitIndex = 0U; bitIndex < 8U; bitIndex++)
        {
            // 判断当前最高位是否为1
            if ((crc & 0x80U) != 0U)
            {
                crc = (uint8_t)((crc << 1U) ^ 0x31U);
            }
            else
            {
                crc <<= 1U;
            }
        }
    }

    return (crc == checksum);
}

/**
 * @brief       发送SHT30单次测量命令
 * @param       pDevice : SHT30设备对象指针
 * @retval      true    : 命令发送成功
 * @retval      false   : 命令发送失败
 */
static bool Sht30SendSingleShotCommand(Sht30DeviceStructType *pDevice)
{
    // 单次测量命令缓存
    uint8_t command[2] = {SINGLE_SHOT_COMMAND_MSB, SINGLE_SHOT_COMMAND_LSB};

    // 校验设备对象
    if (Sht30IsDeviceValid(pDevice) == false)
    {
        return false;
    }

    return pDevice->pOps->Write(pDevice->pBusCtx, pDevice->writeAddr, command, 2U);
}

/**
 * @brief       获取默认SHT30设备对象
 * @param       无
 * @retval      Sht30DeviceStructType * : 默认SHT30设备对象指针
 */
Sht30DeviceStructType *Sht30GetDefaultDevice(void)
{
    return &g_sht30DefaultDevice;
}

/**
 * @brief       初始化SHT30设备
 * @param       pDevice : SHT30设备对象指针
 * @retval      true    : 初始化成功
 * @retval      false   : 初始化失败
 */
bool Sht30Init(Sht30DeviceStructType *pDevice)
{
    // 校验设备对象
    if (Sht30IsDeviceValid(pDevice) == false)
    {
        return false;
    }

    return pDevice->pOps->Init(pDevice->pBusCtx);
}

/**
 * @brief       获取SHT30温湿度原始数据
 * @param       pDevice   : SHT30设备对象指针
 * @param       pTempRaw  : 温度原始值输出指针
 * @param       pHumidRaw : 湿度原始值输出指针
 * @retval      true      : 读取并校验成功
 * @retval      false     : 读取或校验失败
 */
bool Sht30GetTempHumidRaw(Sht30DeviceStructType *pDevice, uint16_t *pTempRaw, uint16_t *pHumidRaw)
{
    // 校验输入参数
    if ((pDevice == NULL) || (pTempRaw == NULL) || (pHumidRaw == NULL))
    {
        return false;
    }

    // 传感器原始接收数据缓存
    uint8_t receiveData[RECEIVE_INDEX_NUM] = {0};
    // 温度CRC校验结果
    bool tempChecksumResult = false;
    // 湿度CRC校验结果
    bool humidChecksumResult = false;

    // 校验设备对象
    if (Sht30IsDeviceValid(pDevice) == false)
    {
        return false;
    }

    // 发送单次测量命令
    if (Sht30SendSingleShotCommand(pDevice) == false)
    {
        return false;
    }

    // 判断是否提供自定义延时函数
    if (pDevice->pOps->DelayMs != NULL)
    {
        pDevice->pOps->DelayMs(SHT30_MEASURE_WAIT_MS);
    }
    else
    {
        Sht30DelayMs(SHT30_MEASURE_WAIT_MS);
    }

    // 读取传感器返回数据
    if (pDevice->pOps->Read(pDevice->pBusCtx, pDevice->readAddr, receiveData, RECEIVE_INDEX_NUM) == false)
    {
        return false;
    }

    tempChecksumResult = Sht30CalculateChecksum(receiveData[TEMP_HIGH_BYTE_INDEX],
                                                receiveData[TEMP_LOW_BYTE_INDEX],
                                                receiveData[TEMP_CHECKSUM_INDEX]);
    humidChecksumResult = Sht30CalculateChecksum(receiveData[HUMID_HIGH_BYTE_INDEX],
                                                 receiveData[HUMID_LOW_BYTE_INDEX],
                                                 receiveData[HUMID_CHECKSUM_INDEX]);

    // 判断CRC校验结果是否通过
    if ((tempChecksumResult == false) || (humidChecksumResult == false))
    {
        return false;
    }

    *pTempRaw = ((uint16_t)receiveData[TEMP_HIGH_BYTE_INDEX] << 8U) | receiveData[TEMP_LOW_BYTE_INDEX];

    *pHumidRaw = ((uint16_t)receiveData[HUMID_HIGH_BYTE_INDEX] << 8U) | receiveData[HUMID_LOW_BYTE_INDEX];

    return true;
}

/**
 * @brief       根据原始数据计算温度值
 * @param       tempRaw : 温度原始值
 * @retval      温度值，单位为摄氏度
 */
float Sht30CalculateTemperature(uint16_t tempRaw)
{
    return -45.0f + 175.0f * ((float)tempRaw / 65535.0f);
}

/**
 * @brief       根据原始数据计算湿度值
 * @param       humidRaw : 湿度原始值
 * @retval      湿度值，单位为%RH
 */
float Sht30CalculateHumidity(uint16_t humidRaw)
{
    return 100.0f * ((float)humidRaw / 65535.0f);
}

/**
 * @brief       温湿度数据采集任务
 * @param       无
 * @retval      无
 */
void DataCollectTask(void)
{
    // 默认SHT30设备对象指针
    Sht30DeviceStructType *pSht30Device = Sht30GetDefaultDevice();
    // LED翻转计数值
    uint16_t times = 0U;
    // 温度原始值
    uint16_t tempRaw = 0U;
    // 湿度原始值
    uint16_t humidRaw = 0U;
    // 待发送温湿度数据
    TempHumidStructType tempHumidSend = {0};
    // 温度累计值
    float tempSum = 0.0f;
    // 湿度累计值
    float humidSum = 0.0f;
    // 有效采样次数
    uint16_t validSampleCnt = 0U;
    // 发送队列对象
    queueType dataCollectToSend = {0};

#if TEST_STACK_WATERMARK
    // 任务剩余栈空间
    UBaseType_t residualStackSize = uxTaskGetStackHighWaterMark(DataCollectTask_Handler);
    printf("DataCollectTask 剩余栈空间: %d\r\n", (int)residualStackSize);
#endif

    // 循环执行温湿度采集任务
    while (1)
    {
        // 等待发送计数器
        static uint8_t waitSendCnt = 0U;
        // 当前温度值
        float temp = 0.0f;
        // 当前湿度值
        float humid = 0.0f;

        // 校验默认设备对象是否有效
        if (pSht30Device != NULL)
        {
            // 采集一次温湿度原始数据
            if (Sht30GetTempHumidRaw(pSht30Device, &tempRaw, &humidRaw) == true)
            {
                temp = Sht30CalculateTemperature(tempRaw);
                humid = Sht30CalculateHumidity(humidRaw);
                tempSum += temp;
                humidSum += humid;
                validSampleCnt++;
            }
        }

        waitSendCnt++;

        // 判断是否达到发送周期
        if (waitSendCnt >= 100U)
        {
            // 判断是否存在有效采样数据
            if (validSampleCnt > 0U)
            {
                tempHumidSend.temperature = tempSum / (float)validSampleCnt;
                tempHumidSend.humidity = humidSum / (float)validSampleCnt;

                dataCollectToSend.messageFrom = DATA_COLLECT_TASK;
                dataCollectToSend.messageTo = COMMUNICATION_TASK;
                dataCollectToSend.messageType = TEMP_HUMIDITY_COLLECT_FINISH;

                // 封装并发送队列数据
                if (QueuePayloadSet(&dataCollectToSend, &tempHumidSend, sizeof(tempHumidSend)) == true)
                {
                    QueueSendUser(&dataCollectToSend);
                }

                tempSum = 0.0f;
                humidSum = 0.0f;
                validSampleCnt = 0U;
            }

            waitSendCnt = 0U;
        }

        times++;

        // 判断是否达到LED翻转周期
        if ((times % 30U) == 0U)
        {
            LED0_TOGGLE();
            times = 0U;
        }

        vTaskDelay(10);
    }
}
