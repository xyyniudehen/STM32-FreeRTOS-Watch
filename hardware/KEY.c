#include "stm32f10x.h"                  // Device header
#include "Delay.h"

uint8_t kEY_Num;

void KEY_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	GPIO_InitTypeDef Init_structure_KEY;
	Init_structure_KEY.GPIO_Mode = GPIO_Mode_IPU;
	Init_structure_KEY.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_15 ;
	Init_structure_KEY.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&Init_structure_KEY);
	
}

uint8_t kEY_GetNum(void)
{
	uint8_t Temp;
	if (kEY_Num)
	{
		Temp=kEY_Num;
		kEY_Num=0;
		return Temp;
	}
	else 
	{
		return 0;
	}
	
}

int press_time;
void Key3_Tick(void)
{
	if (GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15 )==0)
	{
		press_time++;
	}
	if (GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15 )==1)
	{
		press_time=0;
	}
}

uint8_t Key_GetState(void)
{
	if (GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_12 )==0)
	{
		return 1;
	}
	
	else if (GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_13 )==0)
	{
		return 2;
	}
		
	else if (GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15 )==0&&press_time>1000)
	{
		return 4;
	}
	else if ((GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15 )==0))
	{
		return 3;
	}
	else {return 0;}
}

void Key_Tick(void)
{
		static uint8_t Count;
		static uint8_t CurrentState, PreState;
		Count++;
		if (Count >=50)
		{
			Count=0;
			PreState=CurrentState;
			CurrentState=Key_GetState();
			if (PreState !=0 && CurrentState==0)
			{
				kEY_Num=PreState;
			}
		}
}