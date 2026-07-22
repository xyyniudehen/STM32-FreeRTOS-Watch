#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "LED.h"
#include "KEY.h"
#include "Serial.h"

volatile uint32_t led_count = 0;
volatile BaseType_t create_ret = 0;
volatile uint32_t before_scheduler = 0;
volatile uint32_t after_scheduler = 0;

volatile uint8_t key_value = 0;


static void KeyTask(void *param)
{
    uint8_t state;
    uint8_t last_state = 0;
    uint8_t num;

    while (1)
    {
        Key3_Tick();
        Key_Tick();

        state = kEY_GetNum();

        if (state != last_state)
        {
            Serial_Send_String("state=");
            Serial_SendByte(state + '0');
            Serial_Send_String("\r\n");

            last_state = state;
        }

        num = kEY_GetNum();

        if (num != 0)
        {
            key_value = num;

            Serial_Send_String("key=");
            Serial_SendByte(num + '0');
            Serial_Send_String("\r\n");
        }

        vTaskDelay(1);
    }
}

static void LEDTask(void *param)
{
    while (1)
    {
        led_count++;

        LED0_ON();
        vTaskDelay(500);

        led_count++;

        LED0_OFF();
        vTaskDelay(500);
    }
}

int main(void)
{
    LED_Init();
    KEY_Init();
    Serial_Init();

    xTaskCreate(LEDTask, "LED", 128, NULL, 1, NULL);
    xTaskCreate(KeyTask, "KEY", 128, NULL, 2, NULL);


    vTaskStartScheduler();


    while (1)
    {
    }
}