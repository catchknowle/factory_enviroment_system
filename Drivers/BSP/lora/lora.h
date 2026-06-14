#ifndef __LORA__H
#define __LORA__H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

typedef enum
{
    AT_QUERY,
    AT_SET,
} AT_mode;

typedef struct
{
    uint8_t baudrate;
    uint8_t channel;
    uint8_t speed;
    uint8_t power;
} s_lora_config;

typedef enum
{
    NO_ERROR = 0,      // 无错误
    BAUD_ERROR = 1,    // 波特率错误
    CHANNEL_ERROR = 2, // 通道错误
    SPEED_ERROR = 4,   // 传输速度错误
} e_lora_error;

#define LORA_KEY_PIN GPIO_PIN_0
#define LORA_KEY_PORT GPIOA
#define LORA_KEY_CLOCK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()

#define LORA_STATE_PIN GPIO_PIN_5
#define LORA_STATE_PORT GPIOE
#define LORA_STATE_CLOCK_ENABLE() __HAL_RCC_GPIOE_CLK_ENABLE()

void lora_init(void);

typedef struct
{
    uint32_t baudrate; // 波特率
    uint8_t channel;   // 通道
    uint8_t speed;     // 传输速度
} loraSettingParamStruct;

typedef enum
{
    LORA_REMOTE_COMMUNICATION_TEST = 0, // LoRa模块远程通信测试
    LORA_PRESENT_UNIT_TEST = 1,         // 检测LoRa模块在位测试
} lora_unit_test_flag;

e_lora_error lora_set_param(uint32_t baudrate_in, uint8_t channel_in, uint8_t speed_in);
void lora_check_module_present(void);             /* 检测LoRa模块在位函数 */
void LoraRemoteCommunicationTest(void);           /* LoRa模块远程通信测试函数 */
void LoraUnitTest(lora_unit_test_flag test_flag); /* LoRa模块单元测试函数 */

#endif
