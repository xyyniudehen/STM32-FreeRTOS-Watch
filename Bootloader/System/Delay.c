#include "stm32f10x.h"

/**
  * @brief  微秒级延时
  * @param  xus 延时时长，范围：0~233015
  * @retval 无
  */
/*
 * 使用 SysTick 做忙等待延时。系统时钟为 72 MHz 时，重装值按 72*xus 计算。
 * 该函数会临时占用 SysTick，所以 Boot 跳转 APP 前必须清理 SysTick 状态。
 */
void Delay_us(uint32_t xus)
{
    /* LOAD 是倒计时的重装值，达到 0 后 COUNTFLAG 置位。 */
	SysTick->LOAD = 72 * xus;				//设置定时器重装值
	SysTick->VAL = 0x00;					//清空当前计数值
    /* CTRL=5：选择 HCLK 作为时钟源并使能 SysTick。 */
	SysTick->CTRL = 0x00000005;				//设置时钟源为HCLK，启动定时器
    /* 轮询 COUNTFLAG，直到倒计时结束；这是阻塞式延时。 */
	while(!(SysTick->CTRL & 0x00010000));	//等待计数到0
	SysTick->CTRL = 0x00000004;				//关闭定时器
}

/**
  * @brief  毫秒级延时
  * @param  xms 延时时长，范围：0~4294967295
  * @retval 无
  */
/* 毫秒延时通过重复调用 1000 微秒延时实现。 */
void Delay_ms(uint32_t xms)
{
	while(xms--)
	{
		Delay_us(1000);
	}
}
 
/**
  * @brief  秒级延时
  * @param  xs 延时时长，范围：0~4294967295
  * @retval 无
  */
/* 秒延时通过重复调用 1000 毫秒延时实现。 */
void Delay_s(uint32_t xs)
{
	while(xs--)
	{
		Delay_ms(1000);
	}
} 
