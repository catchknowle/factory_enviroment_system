#include "iic.h" 
#include "SYSTEM/delay/delay.h"
#include "freeRTOS.h"
#include "task.h"
#include "./SYSTEM/sys/sys.h"
#include "SYSTEM/delay/delay.h"


// 发送IIC起始信号
void IicStart(gpioPinInfo_s clkPin, gpioPinInfo_s sdaPin)
{
    // 拉高scl总线和sda总线
    HAL_GPIO_WritePin(clkPin.GPIOx, clkPin.Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(sdaPin.GPIOx, sdaPin.Pin, GPIO_PIN_SET);
    IicDelayUs(2);
    // sda从高电平变为低电平，发送起始信号
    HAL_GPIO_WritePin(sdaPin.GPIOx, sdaPin.Pin, GPIO_PIN_RESET);
    IicDelayUs(2);
    /* 钳住I2C总线，准备发送或接收数据 */
    HAL_GPIO_WritePin(clkPin.GPIOx, clkPin.Pin, GPIO_PIN_RESET);
    IicDelayUs(2);
}


// 发送IIC停止信号
void IicStop(gpioPinInfo_s clkPin, gpioPinInfo_s sdaPin)
{
    // 先拉低SDA（此时SCL可能为高也可能为低）
    HAL_GPIO_WritePin(sdaPin.GPIOx, sdaPin.Pin, GPIO_PIN_RESET);
    IicDelayUs(2);
    // 拉高SCL
    HAL_GPIO_WritePin(clkPin.GPIOx, clkPin.Pin, GPIO_PIN_SET);
    IicDelayUs(2);
    // SDA从低变高，SCL保持高 → Stop条件
    HAL_GPIO_WritePin(sdaPin.GPIOx, sdaPin.Pin, GPIO_PIN_SET);
    IicDelayUs(2);
}

/**
 * @brief 设置IIC数据引脚状态
 * 
 * @param sdaPin IIC数据引脚信息
 * 
 * @param state 状态
 * 
 */
void SetSdaPin(gpioPinInfo_s sdaPin, GPIO_PinState state)
{
    HAL_GPIO_WritePin(sdaPin.GPIOx, sdaPin.Pin, state);
}



/**
 * @brief 获取IIC数据引脚状态
 * 
 * @param sdaPin IIC数据引脚信息
 * 
 * @return GPIO_PinState 状态
 */
uint8_t GetSdaPin(gpioPinInfo_s sdaPin)
{
    return HAL_GPIO_ReadPin(sdaPin.GPIOx, sdaPin.Pin);
}


/**
 * @brief 设置IIC数据引脚状态
 * 
 * @param sdaPin IIC数据引脚信息
 * 
 * @param state 状态
 * 
 */
void SetSclPin(gpioPinInfo_s sdaPin, GPIO_PinState state)
{
    HAL_GPIO_WritePin(sdaPin.GPIOx, sdaPin.Pin, state);
}



/**
 * @brief 获取IIC数据引脚状态
 * 
 * @param sdaPin IIC数据引脚信息
 * 
 * @return GPIO_PinState 状态
 */
uint8_t GetSclPin(gpioPinInfo_s sdaPin)
{
    return HAL_GPIO_ReadPin(sdaPin.GPIOx, sdaPin.Pin);
}

/**
 * @brief 检查IIC应答信号
 *
 * @param clkPin IIC时钟引脚信息
 *
 * @param sdaPin IIC数据引脚信息
 *
 * @return true 接收到应答信号
 * @return false 未接收到应答信号
 */
bool CheckAckSignal(gpioPinInfo_s clkPin, gpioPinInfo_s sdaPin)
{
   bool ack = true;
   uint16_t cnt = 0;

   SetSclPin(clkPin, GPIO_PIN_SET);
   IicDelayUs(2);

   // 检测ACK信号
   cnt = 0;
   while(GetSdaPin(sdaPin) == GPIO_PIN_SET)
   {
       cnt++;
       if(cnt > 5000)
       {
           ack = false;
           break;
       }
   }

   // 拉低SCL完成ACK时钟周期
   SetSclPin(clkPin, GPIO_PIN_RESET);
   IicDelayUs(2);
   return ack;
}


// IIC延时函数
void IicDelayUs(uint32_t us)
{
    delay_us(us);
}

/**
 * @brief 发送IIC字节
 * 
 * @param clkPin IIC时钟引脚信息
 * 
 * @param sdaPin IIC数据引脚信息
 * 
 * @param byte 要发送的字节
 * 
 */
void SendByte(gpioPinInfo_s clkPin, gpioPinInfo_s sdaPin, uint8_t byte)
{
    uint8_t transmitBit = 0;
    SetSclPin(clkPin, GPIO_PIN_RESET);
    IicDelayUs(2);
    for(int i = 0; i < 8; i++)
    {
        transmitBit = (byte & 0x80) >> 7;
        byte <<= 1;
        SetSdaPin(sdaPin,(transmitBit ? GPIO_PIN_SET : GPIO_PIN_RESET));
        IicDelayUs(2);
        SetSclPin(clkPin, GPIO_PIN_SET);
        IicDelayUs(2);
        SetSclPin(clkPin, GPIO_PIN_RESET);
        IicDelayUs(2);
    }
    // 释放SDA总线，使从机能拉低SDA发送ACK
    SetSdaPin(sdaPin, GPIO_PIN_SET);
    IicDelayUs(2);
}

/**
 * @brief 接收IIC字节
 * 
 * @param clkPin IIC时钟引脚信息
 * 
 * @param sdaPin IIC数据引脚信息
 * 
 * @return uint8_t 接收到的字节
 */
uint8_t ReceiveByte(gpioPinInfo_s clkPin, gpioPinInfo_s sdaPin)
{
    uint8_t byte = 0;
    // 拉低SCL，准备接收数据
    SetSclPin(clkPin, GPIO_PIN_RESET);
    IicDelayUs(2);
    // 释放sda总线，以便从设备操作sda总线
    SetSdaPin(sdaPin, GPIO_PIN_SET);
    IicDelayUs(2);

    for(int i = 0; i < 8; i++)
    {
        SetSclPin(clkPin, GPIO_PIN_SET);
        IicDelayUs(2);
        byte |= (GetSdaPin(sdaPin) << (7 - i));
        SetSclPin(clkPin, GPIO_PIN_RESET);
        IicDelayUs(2);
    }
    return byte;
}


/**
 * @brief 发送IIC应答信号
 * 
 * @param clkPin IIC时钟引脚信息
 * 
 * @param sdaPin IIC数据引脚信息
 * 
 * @param ack 应答信号 True:发送应答信号 False:不发送应答信号
 * 
 */
void IicSendAck(gpioPinInfo_s clkPin, gpioPinInfo_s sdaPin, bool ack)
{
    SetSclPin(clkPin, GPIO_PIN_RESET);
    if(true == ack)
    {
        SetSdaPin(sdaPin, GPIO_PIN_RESET);
    }
    else
    {
        SetSdaPin(sdaPin, GPIO_PIN_SET);
    }
    IicDelayUs(2);
    SetSclPin(clkPin, GPIO_PIN_SET);
    IicDelayUs(2);
    SetSclPin(clkPin, GPIO_PIN_RESET);
}
