#ifndef __LORA__H
#define __LORA__H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// LoRa AT模式枚举类型
typedef enum
{
    // AT查询模式
    AT_QUERY,
    // AT设置模式
    AT_SET,
} AtModeEnumType;

// LoRa基础配置结构体类型
typedef struct
{
    // 波特率
    uint8_t baudrate;
    // 通道
    uint8_t channel;
    // 速率
    uint8_t speed;
    // 功率
    uint8_t power;
} LoraConfigStructType;

// LoRa错误码枚举类型
typedef enum
{
    // 无错误
    NO_ERROR = 0,
    // 波特率错误
    BAUD_ERROR = 1,
    // 通道错误
    CHANNEL_ERROR = 2,
    // 传输速度错误
    SPEED_ERROR = 4,
} LoraErrorEnumType;

#define LORA_KEY_PIN GPIO_PIN_0
#define LORA_KEY_PORT GPIOA
#define LORA_KEY_CLOCK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()

#define LORA_STATE_PIN GPIO_PIN_5
#define LORA_STATE_PORT GPIOE
#define LORA_STATE_CLOCK_ENABLE() __HAL_RCC_GPIOE_CLK_ENABLE()

void LoraInit(void);

// LoRa参数设置结构体类型
typedef struct
{
    // 波特率
    uint32_t baudrate;
    // 通道
    uint8_t channel;
    // 传输速度
    uint8_t speed;
} LoraSettingParamStructType;

// LoRa单元测试标志枚举类型
typedef enum
{
    // LoRa模块远程通信测试
    LORA_REMOTE_COMMUNICATION_TEST = 0,
    // 检测LoRa模块在位测试
    LORA_PRESENT_UNIT_TEST = 1,
} LoraUnitTestFlagEnumType;

LoraErrorEnumType LoraSetParam(uint32_t baudrate_in, uint8_t channel_in, uint8_t speed_in);
void CommunicationTask(void);

#endif
