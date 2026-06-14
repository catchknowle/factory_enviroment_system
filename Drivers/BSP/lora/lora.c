#include "./BSP/lora/lora.h"
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "FreeRTOS.h"
#include "task.h"

extern UART_HandleTypeDef g_uart1_handle; /* UART句柄 */
extern UART_HandleTypeDef g_uart3_handle;

/**
 * @brief 初始化LoRa模块
 * @param baudrate 串口通信波特率
 * @note 该函数完成LoRa模块的GPIO引脚配置和串口初始化
 */
void lora_init(void)
{
    // 配置LoRa控制引脚和状态引脚为推挽输出模式
    LORA_KEY_CLOCK_ENABLE();
    GPIO_InitTypeDef gpio_handle = {0};
    gpio_handle.Pin = LORA_KEY_PIN;
    gpio_handle.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_handle.Pull = GPIO_PULLUP;
    gpio_handle.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LORA_KEY_PORT, &gpio_handle);

    // 初始化LORA_STATE_PIN
    LORA_STATE_CLOCK_ENABLE();
    gpio_handle.Pin = LORA_STATE_PIN;
    HAL_GPIO_Init(LORA_STATE_PORT, &gpio_handle);
}

/**
 * @brief 进入AT模式函数
 * @details 该函数通过控制LoRa模块的KEY引脚电平，使模块进入AT配置模式
 * @param 无
 * @return 无
 */
static void EnterATMode(void)
{
    /* 等待并确保LoRa模块KEY引脚为低电平状态 */
    while (HAL_GPIO_ReadPin(LORA_KEY_PORT, LORA_KEY_PIN) == GPIO_PIN_SET)
    {
        HAL_GPIO_WritePin(LORA_KEY_PORT, LORA_KEY_PIN, GPIO_PIN_RESET);
        delay_ms(10);
    }
}

/**
 * @brief 退出AT模式函数
 * @param  无
 * @retval 无
 *
 * 该函数通过控制LoRa模块的KEY引脚来退出AT模式，
 * 确保引脚处于高电平状态并保持一定延时以完成模式切换。
 */
static void ExitATMode(void)
{
    /* 等待并确保LoRa模块KEY引脚处于高电平状态 */
    if (HAL_GPIO_ReadPin(LORA_KEY_PORT, LORA_KEY_PIN) == GPIO_PIN_RESET)
    {
        HAL_GPIO_WritePin(LORA_KEY_PORT, LORA_KEY_PIN, GPIO_PIN_SET);
        vTaskDelay(10);
    }
}

/**
 * 发送AT命令函数
 *
 * 该函数用于构建并发送AT命令字符串，将命令类型和命令值组合成完整的AT指令格式
 *
 * @param pCmdType 指向命令类型字符串的指针
 * @param cmdTypeLen 命令类型字符串的长度
 * @param cmdValue 命令值参数
 * @param cmdValueLen 命令值的长度
 *
 * @return 无返回值
 */
static void SendATCommand(const char *pCmdType, uint8_t cmdTypeLen, uint32_t cmdValue, uint8_t cmdValueLen)
{
    char cmd[USART_SEND_LEN] = {0};

    // 将命令类型复制到命令缓冲区
    memcpy(cmd, pCmdType, cmdTypeLen);

    // 在命令类型后追加命令值和结束符，格式化为AT命令格式
    snprintf(cmd + cmdTypeLen, cmdValueLen + 1, "%d\r\n", cmdValue); // 1代表\0，添加\r\n结束符
    // 打印构建好的命令字符串用于调试
    // printf("cmd: %s", cmd);

    HAL_UART_Transmit(&g_uart3_handle, (uint8_t *)cmd, strlen(cmd), 1000);
    while (__HAL_UART_GET_FLAG(&g_uart3_handle, USART_FLAG_TC) == RESET)
        ;
    HAL_UART_Receive_IT(&g_uart3_handle, &g_uart3_rx_byte, 1);
}

/**
 * @brief 设置LoRa模块的通信参数，包括波特率、信道和传输速度。
 *
 * 该函数首先验证输入参数的有效性，若所有参数均有效，则通过发送AT指令设置模块参数。
 * 支持的波特率包括：1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200。
 * 信道范围为1~50，速率范围为1~8。
 *
 * @param baudrate_in 要设置的串口波特率
 * @param channel_in  要设置的通信信道（1~50）
 * @param speed_in    要设置的传输速率（1~8）
 *
 * @return 返回错误码，NO_ERROR表示无错误，其他值表示对应错误类型（如BAUD_ERROR等）
 */
e_lora_error lora_set_param(uint32_t baudrate_in, uint8_t channel_in, uint8_t speed_in)
{
    e_lora_error res = NO_ERROR;
    uint8_t baudrate_len = 0;
    uint8_t channel_len = 0;
    uint8_t speed_len = 1;
    const uint32_t supported_baudrates[] = {
        1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};
    const uint8_t BAUDRATE_COUNT = sizeof(supported_baudrates) / sizeof(supported_baudrates[0]);

    // 验证波特率是否在支持列表中
    int valid_baud = 0;
    for (int i = 0; i < BAUDRATE_COUNT; ++i)
    {
        if (supported_baudrates[i] == baudrate_in)
        {
            valid_baud = 1;
            break;
        }
    }

    if (!valid_baud)
    {
        res |= BAUD_ERROR;
    }
    else
    {
        // 根据波特率确定其数值所占字符长度，用于后续AT命令构造
        if (baudrate_in < 19200)
        {
            baudrate_len = 4;
        }
        else if (baudrate_in < 115200)
        {
            baudrate_len = 5;
        }
        else
        {
            baudrate_len = 6;
        }
    }

    // 检查信道编号是否合法（1~50）
    if (!(channel_in > 0 && channel_in < 51))
    {
        res |= CHANNEL_ERROR;
    }
    else
    {
        // 计算信道数字的位数（一位或两位）
        channel_len = (channel_in < 10) ? 1 : 2;
    }

    // 检查速率参数是否合法（1~8）
    if (!(speed_in > 0 && speed_in < 9))
    {
        res |= SPEED_ERROR;
    }

    // 若存在任一错误则不再继续执行AT命令
    if (res != NO_ERROR)
    {
        return res;
    }

    static const char at_b_cmd[] = "AT+B";
    static const char at_c_cmd[] = "AT+C";
    static const char at_s_cmd[] = "AT+S";

    EnterATMode();
    //  设置波特率
    SendATCommand(at_b_cmd, strlen(at_b_cmd), baudrate_in, baudrate_len);
    // 设置信道
    SendATCommand(at_c_cmd, strlen(at_c_cmd), channel_in, channel_len);
    // 设置速度
    SendATCommand(at_s_cmd, strlen(at_s_cmd), speed_in, speed_len);
    ExitATMode();
    return res;
}

/**
 * @brief       检测LoRa模块在位函数
 * @param       无
 */
void lora_check_module_present(void)
{
    // 进入AT模式
    EnterATMode();

    // 发送AT指令
    char at_cmd[] = "AT";
    HAL_UART_Transmit(&g_uart3_handle, (uint8_t *)at_cmd, strlen(at_cmd), 1000);
    while (__HAL_UART_GET_FLAG(&g_uart3_handle, USART_FLAG_TC) == RESET);

    // ExitATMode();

    bool flag = GetLoraPresentFlag();
    if (flag == true)
    {
        printf("LoRa connected\r\n");
        vTaskDelay(10);
    }
    else
    {
        printf("LoRa not connected\r\n");
        vTaskDelay(10);
    }
}

/**
 * @brief LoRa模块远程通信测试函数
 * @details 该函数用于测试LoRa模块的远程通信功能，通过串口发送测试消息并启动接收中断
 * @param 无
 * @return 无
 */
void LoraRemoteCommunicationTest(void)
{
    // 变量初始化
    uint8_t cmd[] = "lora module remote communication test!\r\n";

    // 退出AT模式，准备进行正常通信
    ExitATMode();

    // 通过UART3发送测试消息，并等待发送完成
    HAL_UART_Transmit(&g_uart3_handle, cmd, strlen((char *)cmd), 0xFFFF);
    while (__HAL_UART_GET_FLAG(&g_uart3_handle, USART_FLAG_TC) == RESET)
        ;

    // 启动UART3接收中断，准备接收数据
    HAL_UART_Receive_IT(&g_uart3_handle, &g_uart3_rx_byte, 1);
}

/**
 * @brief LoRa模块单元测试函数
 * @param test_flag 测试标志位，用于选择不同的测试模式
 *                  0: 执行LoRa远程通信测试
 *                  1: 执行LoRa模块在位检测
 * @return 无返回值
 */
void LoraUnitTest(lora_unit_test_flag test_flag)
{
    // 根据测试标志位执行相应的测试功能
    if (test_flag == LORA_REMOTE_COMMUNICATION_TEST)
    {
        // 执行LoRa远程通信测试
        LoraRemoteCommunicationTest();
    }
    else if (test_flag == LORA_PRESENT_UNIT_TEST)
    {
        // 检测LoRa模块是否存在
        lora_check_module_present();
    }
}
