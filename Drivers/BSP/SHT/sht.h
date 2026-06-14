#ifndef __SHT__H
#define __SHT__H

#include "./SYSTEM/sys/sys.h"
#include <stdbool.h>

#define SHT_CLK_PORT GPIOB
#define SHT_CLK_PIN GPIO_PIN_6
#define SHT_CLK_RCC_ENABLE() do{__HAL_RCC_GPIOB_CLK_ENABLE();}while(0)

#define SHT_SDA_PORT GPIOB
#define SHT_SDA_PIN GPIO_PIN_7
#define SHT_SDA_RCC_ENABLE() do{__HAL_RCC_GPIOB_CLK_ENABLE();}while(0)

#define SHT_ADDR_WRITE (0x88)                  //sht30写器件地址 (0x44 << 1)
#define SHT_ADDR_READ (0x89)                   //sht30读器件地址 (0x45 << 1)
#define SINGLE_SHOT_COMMAND_MSB (0x2c)         //单次测量命令字(高字节) 时钟拉伸
#define SINGLE_SHOT_COMMAND_LSB (0x0d)         //单次测量命令字(低字节) 中重复度

// 接收数据结构体
typedef enum
{
    TEMP_HIGH_BYTE_INDEX = 0,                           // 温度高字节索引位置
    TEMP_LOW_BYTE_INDEX  = 1,                           // 温度低字节索引位置
    TEMP_CHECKSUM_INDEX  = 2,                           // 温度校验和索引位置

    HUMID_HIGH_BYTE_INDEX  = 3,                           // 湿度高字节索引位置
    HUMID_LOW_BYTE_INDEX   = 4,                           // 湿度低字节索引位置
    HUMID_CHECKSUM_INDEX   = 5,                           // 湿度校验和索引位置
    RECEIVE_INDEX_NUM      = 6,      // 接收数据索引数量
}shtReceiveDataIndexEnum;

// 初始化SHT传感器
void ShtInit(void);
// 获取温度湿度数据（校验通过后通过指针返回温湿度原始值）
bool GetTempHumidProcess(uint16_t *pTempRaw, uint16_t *pHumidRaw);
// 根据原始数据计算温度值（单位：°C）
float CalculateTemperature(uint16_t tempRaw);
// 根据原始数据计算湿度值（单位：%RH）
float CalculateHumidity(uint16_t humidRaw);

#endif
