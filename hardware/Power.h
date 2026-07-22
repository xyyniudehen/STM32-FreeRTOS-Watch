#ifndef  __POWER_H
#define __POWER_H

void Power_WakeupInit(void);
void Power_EnterStopMode(void);
void Power_PrepareSleep(void);
void Power_AfterWakeup(void);

#define POWER_AUTO_STOP_MS    30000U

void Power_Tick1ms(void);
void Power_ReportActivity(void);
void Power_Task(void);

#define POWER_LOCK_STOPWATCH    0x01U
#define POWER_LOCK_GAME         0x02U
#define POWER_LOCK_ALARM        0x04U

void Power_Lock(uint8_t reason);
void Power_Unlock(uint8_t reason);

void Power_UIActivityTask(uint8_t key_num);

#endif // ! __POWER_H
