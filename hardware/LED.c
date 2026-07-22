#include "stm32f10x.h"                  // Device header


void LED_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	GPIO_InitTypeDef GPIO_InitStructureLED;
	GPIO_InitStructureLED.GPIO_Mode = GPIO_Mode_Out_PP ;
	GPIO_InitStructureLED.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_3|GPIO_Pin_2;
	GPIO_InitStructureLED.GPIO_Speed = GPIO_Speed_50MHz ;
	GPIO_Init(GPIOA, &GPIO_InitStructureLED);
	GPIO_SetBits(GPIOA, GPIO_Pin_0);

	GPIO_ResetBits(GPIOA, GPIO_Pin_3);
	GPIO_SetBits(GPIOA, GPIO_Pin_2);
		
	
	// GPIO_InitTypeDef GPIO_InitStructureLED1;
	// GPIO_InitStructureLED1.GPIO_Mode = GPIO_Mode_Out_PP ;
	// GPIO_InitStructureLED1.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13;
	// GPIO_InitStructureLED1.GPIO_Speed = GPIO_Speed_50MHz ;
	// GPIO_Init(GPIOB, &GPIO_InitStructureLED1);
	// GPIO_ResetBits(GPIOB, GPIO_Pin_12);
	// GPIO_SetBits(GPIOB, GPIO_Pin_13);
	
	


}


void LED0_ON(void)
{

		GPIO_ResetBits(GPIOA, GPIO_Pin_0);

}

void LED0_OFF(void)
{

		GPIO_SetBits(GPIOA, GPIO_Pin_0);

}
