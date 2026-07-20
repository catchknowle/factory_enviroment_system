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
#include "QueueManage.h"

/**
 * @brief       FreeRTOS启动任务
 * @param       无
 * @retval      无
 */
void StartTask(void)
{
	// 进入临界区
	portENTER_CRITICAL();
	xTaskCreate((TaskFunction_t)DataCollectTask, "DataCollectTask", DATA_COLLECT_TASK_STACK_SIZE,
				NULL, DATA_COLLECT_TASK_PRIO, &DataCollectTask_Handler);
	xTaskCreate((TaskFunction_t)CommunicationTask, "CommunicationTask", COMMUBICATION_TASK_STACK_SIZE,
				NULL, COMMUBICATION_TASK_PRIO, &CommunicationTask_Handler);

    QueueInit();
	// 删除开始任务
	vTaskDelete(StartTask_Handler);
	// 退出临界区
	portEXIT_CRITICAL();
}

/**
 * @brief       FreeRTOS入口函数
 * @param       无
 * @retval      无
 */
void FreertosEntry(void)
{
	xTaskCreate((TaskFunction_t)StartTask, "StartTask", START_TASK_STACK_SIZE,
				NULL, START_TASK_PRIO, &StartTask_Handler);
	vTaskStartScheduler();
}

/**
 * @brief       主函数
 * @param       无
 * @retval      int : 程序运行状态
 */
int main(void)
{
	HAL_Init();							// 初始化HAL库
	sys_stm32_clock_init(RCC_PLL_MUL9); // 设置时钟,72M
	delay_init(72);						// 初始化延时函数
	led_init();							// 初始化LED
	usart_init(115200);					// 初始化串口1和串口3

	// 初始化SHT30传感器
	if (Sht30Init(Sht30GetDefaultDevice()) == false)
	{
		// 预留初始化失败后的日志处理
	}
	LoraInit();							// 初始化LoRa
	FreertosEntry();
}
