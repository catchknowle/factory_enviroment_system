/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-20
 * @brief       跑马灯 实验
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 STM32F103开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#include "./stm32f1xx_it.h"
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/lora/lora.h"
#include "main.h"
#include "./BSP/SHT/sht.h"

#define LORA_UNIT_TEST 1

char cmd[20] = "AT+B9600";
uint8_t cmd_flag = 0;
extern UART_HandleTypeDef g_uart3_handle;
extern uint8_t g_lora_rx_buffer[RXBUFFERSIZE];

void DataCollectTask(void)
{
	uint16_t times = 0;
	uint16_t tempRaw = 0;
	uint16_t humidRaw = 0;
	float temperature = 0.0f;
	float humidity = 0.0f;

	while (1)
	{
		if(true == GetTempHumidProcess(&tempRaw, &humidRaw))
		{
			// 计算温度和湿度值
			temperature = CalculateTemperature(tempRaw);
			humidity = CalculateHumidity(humidRaw);
			// 通过串口打印温湿度数据
			printf("温度: %.2f °C, 湿度: %.2f %%RH\r\n", temperature, humidity);
		}
		times++;
		if (times % 30 == 0)
		{
			LED0_TOGGLE();
			times = 0;
		}
		vTaskDelay(10);
	}
}

void CommunicationTask(void)
{
	while (1)
	{
#if LORA_UNIT_TEST
		LoraUnitTest(LORA_REMOTE_COMMUNICATION_TEST);
#endif
		vTaskDelay(10);
	}
}

void StartTask(void)
{
	// 进入临界区
	portENTER_CRITICAL();
	xTaskCreate((TaskFunction_t)DataCollectTask, "DataCollectTask", DATA_COLLECT_TASK_STACK_SIZE,
				NULL, DATA_COLLECT_TASK_PRIO, &DataCollectTask_Handler);
	xTaskCreate((TaskFunction_t)CommunicationTask, "CommunicationTask", COMMUBICATION_TASK_STACK_SIZE,
				NULL, COMMUBICATION_TASK_PRIO, &CommunicationTask_Handler);
	// 删除开始任务
	vTaskDelete(StartTask_Handler);
	// 退出临界区
	portEXIT_CRITICAL();
}

void freertos_entry(void)
{
	xTaskCreate((TaskFunction_t)StartTask, "StartTask", START_TASK_STACK_SIZE,
				NULL, START_TASK_PRIO, &StartTask_Handler);
	vTaskStartScheduler();
}

int main(void)
{
	HAL_Init();							/* 初始化HAL库 */
	sys_stm32_clock_init(RCC_PLL_MUL9); /* 设置时钟,72M */
	delay_init(72);						/* 初始化延时函数 */
	led_init();							/* 初始化LED */
	usart_init(115200);					/* 初始化串口1和串口3 */
	ShtInit();							/* 初始化SHT传感器 */
	lora_init();						/* 初始化LoRa */
	freertos_entry();				
}
