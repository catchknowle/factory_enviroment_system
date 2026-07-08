#ifndef __QUEUE_MANAGE_H__
#define __QUEUE_MANAGE_H__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define WAIT_QUEUE_SEND_RECEIVE_TIMEOUT 5000
#define QUEUE_PAYLOAD_MAX_LEN 64

typedef struct
{
    uint16_t len;
    uint8_t data[QUEUE_PAYLOAD_MAX_LEN];
}QueuePayloadStructType;

typedef struct
{
    uint8_t messageFrom;                           // 消息来源
    uint8_t messageTo;                             // 消息目标
    uint8_t messageType;                           // 消息类型
    QueuePayloadStructType payload;                // 消息负载
}queueType;

/**
 * @brief       写入消息负载
 * @param       pMsg        : 消息指针
 * @param       pPayload    : 负载数据指针
 * @param       payloadLen  : 负载长度（必须 <= QUEUE_PAYLOAD_MAX_LEN）
 * @retval      true:写入成功 false:写入失败
 */
static inline bool QueuePayloadSet(queueType *pMsg, const void *pPayload, uint16_t payloadLen)
{
    if ((pMsg == NULL) || (pPayload == NULL) || (payloadLen > QUEUE_PAYLOAD_MAX_LEN))
    {
        return false;
    }

    memcpy(pMsg->payload.data, pPayload, payloadLen);
    pMsg->payload.len = payloadLen;
    return true;
}

/**
 * @brief       读取消息负载并拷贝到目标缓冲
 * @param       pMsg        : 消息指针
 * @param       pOut        : 输出缓冲区指针
 * @param       outLen      : 输出缓冲区长度（必须等于消息负载长度）
 * @retval      true:拷贝成功 false:拷贝失败
 */
static inline bool QueuePayloadCopyOut(const queueType *pMsg, void *pOut, uint16_t outLen)
{
    if ((pMsg == NULL) || (pOut == NULL))
    {
        return false;
    }

    if (pMsg->payload.len != outLen)
    {
        return false;
    }

    memcpy(pOut, pMsg->payload.data, outLen);
    return true;
}

typedef enum
{
    DATA_COLLECT_TASK = 0,                         // 数据采集任务
    COMMUNICATION_TASK = 1,                        // 通信任务
}TaskRenameEnum;

typedef enum
{
    TEMP_HUMIDITY_COLLECT_FINISH = 0,              // 温湿度采集完成
}MessageTypeEnum;

typedef enum
{
    RETURN_ERROR = 0,
    RETURN_SUCCESS = 1,
}ReturnResType;

void QueueInit(void);
void QueueSendUser(queueType *pSendData);
ReturnResType QueueReceiveUser(queueType *pReceiveData);


#endif
