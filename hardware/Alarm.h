#ifndef  __ALARM_H
#define __ALARM_H


extern uint8_t Alarm_Enable ;
extern volatile uint8_t Alarm_RingFlag;


int SetAlarmTime(void);
void Alarm_Check(void);
void Alarm_RingPage(void);
void Alarm_HardwareInit(void);
void Alarm_SetHardwareAlarm(void);
void Alarm_Task(void);

#endif // ! __POWER_H
