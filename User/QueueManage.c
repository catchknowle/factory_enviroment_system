#include "QueueManage.h"
#include "FreeRTOS.h"
#include "queue.h"

#define QUEUE_SIZE 10 // 队列长度，单个成员的大小作为基本单位

static QueueHandle_t g_queueHandle = NULL;

void QueueInit(void)
{
	if (g_queueHandle != NULL)
	{
		return;
	}
	g_queueHandle = xQueueCreate(QUEUE_SIZE, sizeof(queueType));
}

void QueueSendUser(queueType *pSendData)
{
	if(pSendData == NULL)
	{
		return;
	}
	if (g_queueHandle == NULL)
	{
		return;
	}
	// 发送数据到队列
	if(pdFALSE == xQueueSend(g_queueHandle, pSendData, WAIT_QUEUE_SEND_RECEIVE_TIMEOUT))
	{
		 // 日志系统
	}
}


/**
 * @brief 从队列接收数据
 * 
 * @param pReceiveData 指向接收数据的指针
 */
ReturnResType QueueReceiveUser(queueType *pReceiveData)
{
	if(pReceiveData == NULL || g_queueHandle == NULL)
	{
		return RETURN_ERROR;
	}

	// 发送数据到队列
	if(pdFALSE == xQueueReceive(g_queueHandle, pReceiveData, WAIT_QUEUE_SEND_RECEIVE_TIMEOUT))
	{
		 // 日志系统
		return RETURN_ERROR;
	}

	return RETURN_SUCCESS;
}


