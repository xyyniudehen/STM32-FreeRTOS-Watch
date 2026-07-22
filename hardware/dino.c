#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "dino.h"
#include "stdlib.h"
#include "KEY.h"
#include "math.h"
#include "Delay.h"
#include "FreeRTOS.h"
#include "task.h"

struct Object_Position{
    uint8_t minX, minY, maxX, maxY;
};



int Score;
void Show_Score(void)
{
    OLED_ShowNum(98, 0, Score, 5, OLED_6X8);

}


// void Show_Ground(void)
// {

//     if(Ground_pos<128)
//     {
//       for (uint8_t i=0; i<128; i++)
//       {
//         OLED_DisplayBuf[7][i]= Ground[i+Ground_pos];
//       }  
//     }
//       if(Ground_pos>128)
//       {
//       for (uint8_t i=0; i<256-Ground_pos; i++)
//       {
//         OLED_DisplayBuf[7][i]= Ground[i+Ground_pos];
//       }  
      
//       for (uint8_t i=256-Ground_pos ; i<128; i++)
//       {
//         OLED_DisplayBuf[7][i]= Ground[i-(256-Ground_pos)];
//       }  
//      }
// }






uint16_t Ground_pos;
void Show_Ground(void)
{
    for (uint8_t i = 0; i < 128; i++)
    {
        OLED_DisplayBuf[7][i] = Ground[(i + Ground_pos) % 256];
    }
}


uint16_t Barrier_pos;
uint8_t barrier_flag;

struct Object_Position barrier;


void Show_Barrier(void)
{
    if (Barrier_pos>=143){
    barrier_flag=rand()%3;
    }
    OLED_ShowImage(127-Barrier_pos, 44, 16, 18, Barrier[barrier_flag]);
    barrier.maxX= 127-Barrier_pos+16;
    barrier.maxY= 62;
    barrier.minX= 127-Barrier_pos;
    barrier.minY= 44;


}

uint8_t Cloude_pos;
void Show_cloud(void)
{
    OLED_ShowImage(128-Cloude_pos ,9, 16, 8, Cloud );
}

uint8_t dino_jump_flag=0;   //0±¼ÅÜ£¬1ÌøÔ¾
uint8_t Key_Num;
uint16_t jump_t;
uint8_t Jump_Pos; 
extern double pi;

struct Object_Position dino;


void Show_Dino(void)
{

    Key_Num= kEY_GetNum();
    if (Key_Num==1)dino_jump_flag=1;

    Jump_Pos=28*sin((float)(pi*jump_t/1000));

    if (dino_jump_flag==0)
    {
        if(Cloude_pos%2==0)OLED_ShowImage(0, 44, 16, 18, Dino[0]);
        else OLED_ShowImage(0, 44, 16, 18, Dino[1]);
    }
    else 
    {
        OLED_ShowImage(0, 44-Jump_Pos, 16, 18, Dino[2]);
    }
    dino.maxX= 16;
    dino.maxY= 62-Jump_Pos;
    dino.minX=0;
    dino.minY= 44-Jump_Pos;

}

int isColliding(struct Object_Position *a,struct Object_Position *b)
{
    if((a->maxX > b->minX) && (a->minX < b->maxX) &&(a->maxY > b->minY) &&(a->minY < b->maxY))
    {
        OLED_Clear();
        OLED_ShowString(28, 24, "Game Over", OLED_8X16);
        OLED_Update();
        vTaskDelay(pdMS_TO_TICKS(1000));
        OLED_Clear();
        OLED_Update();
        return 1;
    }
    return 0;
}



int DinoGame_Animation(void)
{
    while(1)
    {
    int return_flag;
    OLED_Clear();
    Show_Score();
    Show_Ground();
    Show_Barrier();
    Show_cloud();
    Show_Dino();
    OLED_Update();
    return_flag= isColliding(&dino, &barrier);
    if (return_flag==1)
    {
        return 0;
    }
}
}

void Dino_Tick(void)
{
    static int Score_Count, Ground_Count, Cloud_count;
    Score_Count++;
    Ground_Count++;
    Cloud_count++;;
    if (Score_Count>=100)
    {
        Score_Count=0;
        Score++;
    }

        if (Ground_Count>=20)
    {
        Ground_Count=0;
        Ground_pos++;
        Barrier_pos++;
        if (Ground_pos>=256)Ground_pos=0;
        if (Barrier_pos>144)Barrier_pos=0;
    }

    if (Cloud_count>=50)
    {
        Cloud_count=0;
        Cloude_pos++;
        if (Cloude_pos>200)Cloude_pos=0;
    }

    if(dino_jump_flag==1)
    {
        jump_t++;
        if (jump_t>=1000)
        {
            jump_t=0;
            dino_jump_flag=0;
        }
    }

}

void Dino_Game_Pos_Init(void)
{
    Score=Ground_pos=Barrier_pos=Cloude_pos= Jump_Pos=0;
}