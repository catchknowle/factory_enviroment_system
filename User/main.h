#ifndef __MAIN_H__
#define __MAIN_H__
#include "FreeRTOS.h"
#include "task.h"

#define START_TASK_PRIO 0
#define START_TASK_STACK_SIZE 128
TaskHandle_t StartTask_Handler = NULL;

#define DATA_COLLECT_TASK_PRIO 1
#define DATA_COLLECT_TASK_STACK_SIZE 128
TaskHandle_t DataCollectTask_Handler = NULL;

#define COMMUBICATION_TASK_PRIO 2
#define COMMUBICATION_TASK_STACK_SIZE 128
TaskHandle_t CommunicationTask_Handler = NULL;

#endif
