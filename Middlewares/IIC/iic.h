#ifndef __IIC__H
#define __IIC__H

#include "./SYSTEM/sys/sys.h"
#include <stdbool.h>

// 定义IIC GPIO引脚信息结构体
typedef struct 
{
    GPIO_TypeDef *GPIOx;
    uint32_t Pin;
}gpioPinInfo_s;

// 定义IIC发送信息结构体
typedef struct
{
    uint8_t byte;
    uint8_t deviceAddr;
    uint8_t regAddr;
}iicSendInfo_s;

void IicStart(gpioPinInfo_s clkPin, gpioPinInfo_s sdaPin);
void IicStop(gpioPinInfo_s clkPin, gpioPinInfo_s sdaPin);
void SetSdaPin(gpioPinInfo_s sdaPin, GPIO_PinState state);
void SetSclPin(gpioPinInfo_s sclPin, GPIO_PinState state);
uint8_t GetSdaPin(gpioPinInfo_s sdaPin);
uint8_t GetSclPin(gpioPinInfo_s sclPin);
void IicDelayUs(uint32_t us);
bool CheckAckSignal(gpioPinInfo_s clkPin, gpioPinInfo_s sdaPin);
void SendByte(gpioPinInfo_s clkPin, gpioPinInfo_s sdaPin, uint8_t byte);
void IicSendAck(gpioPinInfo_s clkPin, gpioPinInfo_s sdaPin, bool ack);
uint8_t ReceiveByte(gpioPinInfo_s clkPin, gpioPinInfo_s sdaPin);
#endif
