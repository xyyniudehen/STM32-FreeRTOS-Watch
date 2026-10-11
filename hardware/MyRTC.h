#ifndef __MYRTC_H
#define __MYRTC_H

extern int MyRTC_Time[];
void MyRTC_Init(void);
void MyRTC_SetTime(void);
void MyRTC_ReadTime(void);

void MyRTC_RequestBootUpdate(void);


#endif // !__MYRTC_H