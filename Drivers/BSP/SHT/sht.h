#ifndef __SHT__H
#define __SHT__H

#include "./SYSTEM/sys/sys.h"
#include "iic.h"
#include <stdbool.h>

#define SHT30_CLK_PORT GPIOB
#define SHT30_CLK_PIN GPIO_PIN_6
#define SHT30_CLK_RCC_ENABLE() do{__HAL_RCC_GPIOB_CLK_ENABLE();}while(0)

#define SHT30_SDA_PORT GPIOB
#define SHT30_SDA_PIN GPIO_PIN_7
#define SHT30_SDA_RCC_ENABLE() do{__HAL_RCC_GPIOB_CLK_ENABLE();}while(0)

#define SHT30_ADDR_WRITE (0x88U)
#define SHT30_ADDR_READ (0x89U)

// SHT30接收数据索引枚举类型
typedef enum
{
    // 温度高字节索引
    TEMP_HIGH_BYTE_INDEX = 0,
    // 温度低字节索引
    TEMP_LOW_BYTE_INDEX = 1,
    // 温度CRC校验字节索引
    TEMP_CHECKSUM_INDEX = 2,
    // 湿度高字节索引
    HUMID_HIGH_BYTE_INDEX = 3,
    // 湿度低字节索引
    HUMID_LOW_BYTE_INDEX = 4,
    // 湿度CRC校验字节索引
    HUMID_CHECKSUM_INDEX = 5,
    // 接收数据总长度
    RECEIVE_INDEX_NUM = 6
} ShtReceiveDataIndexEnumType;

// SHT30时钟使能函数类型
typedef void (*Sht30ClockEnableFuncType)(void);

// 软件IIC总线配置结构体类型
typedef struct
{
    // SCL引脚信息
    gpioPinInfo_s sclPin;
    // SDA引脚信息
    gpioPinInfo_s sdaPin;
    // SCL时钟使能函数
    Sht30ClockEnableFuncType pSclClockEnable;
    // SDA时钟使能函数
    Sht30ClockEnableFuncType pSdaClockEnable;
} SoftIicBusStructType;

// SHT30底层操作接口结构体类型
typedef struct
{
    // 初始化函数指针
    bool (*Init)(void *pBusCtx);
    // 写函数指针
    bool (*Write)(void *pBusCtx, uint8_t deviceAddr, const uint8_t *pWriteData, uint16_t writeLen);
    // 读函数指针
    bool (*Read)(void *pBusCtx, uint8_t deviceAddr, uint8_t *pReadData, uint16_t readLen);
    // 延时函数指针
    void (*DelayMs)(uint32_t delayMs);
} Sht30OpsStructType;

// SHT30设备对象结构体类型
typedef struct
{
    // 底层操作对象指针
    const Sht30OpsStructType *pOps;
    // 总线上下文指针
    void *pBusCtx;
    // 设备写地址
    uint8_t writeAddr;
    // 设备读地址
    uint8_t readAddr;
} Sht30DeviceStructType;

// 温湿度数据结构体类型
typedef struct
{
    // 温度值
    float temperature;
    // 湿度值
    float humidity;
} TempHumidStructType;

// SHT30软件IIC操作对象
extern const Sht30OpsStructType g_sht30SoftIicOps;

Sht30DeviceStructType *Sht30GetDefaultDevice(void);
bool Sht30Init(Sht30DeviceStructType *pDevice);
bool Sht30GetTempHumidRaw(Sht30DeviceStructType *pDevice, uint16_t *pTempRaw, uint16_t *pHumidRaw);
float Sht30CalculateTemperature(uint16_t tempRaw);
float Sht30CalculateHumidity(uint16_t humidRaw);
void DataCollectTask(void);

#endif
