# 低功耗休眠与 RTC 闹钟集成方案

## 目标

在现有 STM32 多功能手表工程中新增两个功能：

1. 低功耗休眠/唤醒：长按 KEY3 进入低功耗，按 KEY3 唤醒。
2. RTC 闹钟：在设置页面中设置闹钟时间，到点后 OLED 提醒并驱动 LED 闪烁。

设计重点是保持代码系统性，避免继续把所有逻辑都堆进 `menu.c`。

---

## 总体设计

建议新增两个独立模块：

```text
Power.c / Power.h    低功耗休眠与唤醒
Alarm.c / Alarm.h    RTC 闹钟设置与提醒
```

现有模块职责保持不变：

```text
main.c       负责初始化和主循环入口
menu.c       负责页面显示、菜单跳转、功能入口
KEY.c        负责按键扫描、短按长按判断
MyRTC.c      负责 RTC 初始化、读写当前时间
OLED.c       负责 OLED 显示底层接口
Timer.c      负责 TIM2 1ms 节拍
```

这样做的好处：

```text
1. 低功耗和闹钟都有独立接口，后期容易复用和调试。
2. menu.c 只负责“调用功能”，不承担底层配置。
3. 面试讲项目时，模块边界更清楚。
```

---

## 一、低功耗模块 Power

### 文件位置

建议新增：

```text
hardware/Power.c
hardware/Power.h
```

如果 Keil 工程没有自动加入新文件，需要手动把 `Power.c` 加进工程。

---

### 对外接口

```c
void Power_WakeupInit(void);
void Power_EnterStopMode(void);
```

---

### 1. Power_WakeupInit

作用：初始化 KEY3 外部中断唤醒。

KEY3 当前接在：

```text
PA15
```

所以需要配置：

```text
PA15 -> EXTI15
下降沿触发
EXTI15_10_IRQn 中断
```

为什么要这样做：

```text
进入 STOP 模式后，主循环和 TIM2 都不运行，不能再靠 Key_Tick 扫描按键。
所以必须用 EXTI 外部中断把 MCU 唤醒。
```

建议在 `Peripheral_Init()` 中调用：

```c
void Peripheral_Init(void)
{
    MyRTC_Init();
    KEY_Init();
    Power_WakeupInit();
    LED_Init();
    MPU6050_Init();
    AD_Init();
}
```

---

### 2. Power_EnterStopMode

作用：进入 STOP 低功耗模式，并在唤醒后恢复系统。

执行流程：

```text
显示 Sleep...
关闭 OLED 显示
关闭 TIM2
清除 EXTI 和 PWR 唤醒标志
进入 STOP 模式
等待 KEY3 中断唤醒
重新初始化系统时钟
恢复 TIM2
打开 OLED 显示
显示 Wake Up
返回首页
```

核心逻辑：

```c
PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
```

注意点：

```text
STOP 模式唤醒后，系统时钟可能回到 HSI，需要调用 SystemInit() 重新配置时钟。
如果不恢复时钟，TIM2、Delay、I2C/OLED 的速度可能异常。
```

---

### 3. EXTI15_10_IRQHandler

作用：KEY3 唤醒中断函数。

建议放在 `Power.c` 或 `stm32f10x_it.c` 中，二选一即可，不要重复定义。

```c
void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line15) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line15);
    }
}
```

中断里只清标志，不做 OLED 显示、不做复杂逻辑。

---

### 低功耗入口放哪里

建议放在首页 `First_Page_Clock()` 中。

你现在 KEY3 长按会返回：

```c
KeyNum == 4
```

所以首页中这样接入：

```c
else if (KeyNum == 4)
{
    Power_EnterStopMode();
}
```

交互含义：

```text
KEY3 短按：确认
KEY3 长按：休眠
```

---

## 二、RTC 闹钟模块 Alarm

### 文件位置

建议新增：

```text
hardware/Alarm.c
hardware/Alarm.h
```

如果暂时不想新增文件，也可以先放在 `SetTime.c` 末尾；但从系统性角度看，更推荐独立成 `Alarm.c`。

---

### 对外接口

```c
void Alarm_Init(void);
int Alarm_Page(void);
void Alarm_Check(void);
void Alarm_Ring(void);
```

内部可再拆：

```c
static void Alarm_ShowUI(void);
static int Alarm_SetHour(void);
static int Alarm_SetMin(void);
```

---

### 1. Alarm_Init

作用：初始化闹钟默认值。

例如：

```c
Alarm_Hour = 7;
Alarm_Min = 0;
Alarm_Enable = 0;
```

第一版可以只用 RAM 保存，断电后恢复默认值。

后续升级方向：

```text
用 BKP 备份寄存器或内部 Flash 保存闹钟时间和开关状态。
```

---

### 2. Alarm_Page

作用：闹钟设置页面。

建议菜单项：

```text
返回
设置小时
设置分钟
闹钟开关
```

按键逻辑保持和现有页面一致：

```text
KEY1：上一项 / 数值 +1
KEY2：下一项 / 数值 -1
KEY3：确认
```

示例显示：

```text
Alarm:07:30
State:ON
```

选中位置用 `OLED_ReverseArea()` 反色。

---

### 3. Alarm_Check

作用：检查当前 RTC 时间是否到达闹钟时间。

调用位置：

```c
First_Page_Clock()
```

建议首页每轮调用一次：

```c
Alarm_Check();
```

判断逻辑：

```text
如果闹钟关闭：直接返回
读取 RTC 当前时间
比较当前时、分是否等于闹钟时、分
如果到点：调用 Alarm_Ring()
```

需要锁存变量：

```c
static uint8_t Alarm_Latched;
```

原因：

```text
如果只比较“时”和“分”，那么 07:30:00 到 07:30:59 都满足条件。
如果没有锁存，闹钟会在这一分钟内反复响。
```

锁存逻辑：

```text
第一次进入该分钟：响一次，Alarm_Latched = 1
仍在该分钟：不重复响
离开该分钟：Alarm_Latched = 0
```

---

### 4. Alarm_Ring

作用：闹钟提醒界面。

建议实现：

```text
OLED 显示 ALARM
显示闹钟时间
LED 闪烁
按任意键关闭
```

如果以后加蜂鸣器，只需要在这个函数里增加蜂鸣器翻转即可。

---

## 三、设置页面怎么集成闹钟

当前设置页面是：

```text
返回
日期时间设置
```

建议改成：

```text
返回
日期时间设置
Alarm
```

需要修改三个地方。

### 1. Show_SettingPage_UI

增加一行显示：

```c
OLED_ShowString(0, 32, "Alarm", OLED_8X16);
```

---

### 2. setflag 范围

原来范围是 1~2：

```c
if (setflag <= 0){setflag = 2;}
if (setflag >= 3){setflag = 1;}
```

改成 1~3：

```c
if (setflag <= 0){setflag = 3;}
if (setflag >= 4){setflag = 1;}
```

---

### 3. 确认逻辑

原来：

```c
if (setflag_temp == 1){return 0;}
else if (setflag_temp == 2){SetTime();}
```

改成：

```c
if (setflag_temp == 1){return 0;}
else if (setflag_temp == 2){SetTime();}
else if (setflag_temp == 3){Alarm_Page();}
```

---

## 四、main.c 怎么接入

`main.c` 尽量不放业务逻辑，保持现在结构即可：

```c
OLED_Init();
OLED_Clear();
Peripheral_Init();
Init_Timer();

while (1)
{
    clkflag1 = First_Page_Clock();
    if (clkflag1 == 1){Meun1();}
    else if (clkflag1 == 2){SettingPage();}
}
```

新增模块的初始化放进 `Peripheral_Init()`，不要散落在 `main.c`。

---

## 五、推荐调用关系

```text
main.c
 └── Peripheral_Init()
      ├── MyRTC_Init()
      ├── KEY_Init()
      ├── Power_WakeupInit()
      ├── Alarm_Init()
      ├── LED_Init()
      ├── MPU6050_Init()
      └── AD_Init()

main.c
 └── First_Page_Clock()
      ├── kEY_GetNum()
      ├── Alarm_Check()
      ├── Power_EnterStopMode()    KEY3 长按
      └── Show_Clock_UI()

SettingPage()
 └── Alarm_Page()
      ├── Alarm_SetHour()
      ├── Alarm_SetMin()
      └── Alarm_Enable 开关

Alarm_Check()
 └── Alarm_Ring()
```

---

## 六、第一版与第二版规划

### 第一版：软件检查闹钟

特点：

```text
首页运行时检查 RTC 时间
到点后响闹钟
实现简单，容易调试
```

缺点：

```text
如果 MCU 正在 STOP 休眠，软件检查不运行，闹钟不会主动唤醒。
```

适合作为第一阶段。

---

### 第二版：RTC Alarm 中断唤醒

特点：

```text
设置 RTC 硬件 Alarm
MCU 进入 STOP 后，RTC 到点自动唤醒
唤醒后进入 Alarm_Ring()
```

优点：

```text
更像真实智能手表
低功耗和闹钟功能真正打通
```

建议等第一版稳定后再做。

---

## 七、为什么不建议全部写进 menu.c

`menu.c` 现在已经包含：

```text
首页
设置页
滑动菜单
秒表
手电筒
MPU6050
游戏
表情
水平仪
ADC 电池显示
```

如果再把低功耗、闹钟、EXTI 中断、闹钟设置都塞进去，后面会很难维护。

更好的方式是：

```text
menu.c 负责页面入口
Power.c 负责低功耗
Alarm.c 负责闹钟
```

这样面试时也能讲成：

```text
我把项目拆成 UI 层、输入层、时间服务层、电源管理层和应用功能层。
```

---

## 八、建议最终文件结构

```text
hardware/
 ├── menu.c/.h        页面和菜单
 ├── KEY.c/.h         按键扫描
 ├── MyRTC.c/.h       RTC 时间
 ├── SetTime.c/.h     日期时间设置
 ├── Alarm.c/.h       闹钟功能
 ├── Power.c/.h       低功耗管理
 ├── OLED.c/.h        OLED 显示
 ├── LED.c/.h         LED 控制
 └── AD.c/.h          电池 ADC 检测
```

---

## 九、面试可讲点

完成后简历可以写：

```text
实现低功耗休眠/唤醒功能，长按按键关闭 OLED 并进入 STOP 模式，通过 EXTI 外部中断唤醒后恢复系统时钟和显示。
```

```text
实现 RTC 闹钟功能，支持闹钟时间设置、开关控制、到点提醒和按键关闭；通过锁存机制避免同一分钟内重复触发。
```

如果后续升级 RTC Alarm 中断，可以写：

```text
基于 RTC Alarm 中断实现低功耗状态下的定时唤醒，提高系统待机实用性。
```

---

## 十、功能测试步骤

新增功能不能只看能不能编译，还要按步骤验证。建议分成四轮测试：基础功能测试、低功耗测试、闹钟测试、集成回归测试。

---

### 1. 基础编译与启动测试

目标：确认新增模块没有破坏原工程。

测试步骤：

```text
1. 将 Power.c / Power.h、Alarm.c / Alarm.h 加入 Keil 工程。
2. 编译工程，确认没有 error。
3. 下载程序到开发板。
4. 上电后确认 OLED 能正常显示首页时钟。
5. KEY1、KEY2 能切换首页“菜单/设置”选项。
6. KEY3 短按能正常进入菜单或设置页面。
```

预期结果：

```text
首页显示正常，原有按键逻辑不受影响。
```

如果失败，优先检查：

```text
1. 新增 .c 文件是否加入工程。
2. .h 文件是否 include 正确。
3. 是否出现函数重复定义，例如 EXTI15_10_IRQHandler 定义了两次。
```

---

### 2. 低功耗休眠/唤醒测试

目标：验证长按 KEY3 可以进入 STOP 模式，再通过 KEY3 唤醒。

测试步骤：

```text
1. 保持在首页时钟界面。
2. 长按 KEY3，直到触发 KeyNum == 4。
3. 观察 OLED 是否显示 Sleep...。
4. 观察 OLED 是否熄屏。
5. 等待 3~5 秒。
6. 再次按下 KEY3。
7. 观察 OLED 是否重新点亮并显示 Wake Up。
8. 确认系统回到首页时钟界面。
```

预期结果：

```text
长按 KEY3 后 OLED 熄屏，按 KEY3 后 OLED 重新点亮，时钟界面恢复正常。
```

重点检查：

```text
1. 唤醒后按键是否还能用。
2. 唤醒后 OLED 是否还能刷新。
3. 唤醒后秒表、菜单动画等依赖 TIM2 的功能是否还正常。
```

常见问题：

```text
1. 唤醒后 OLED 不显示：可能没有重新打开 OLED，或 I2C 时序受系统时钟影响。
2. 唤醒后定时器不走：可能 STOP 后没有重新使能 TIM2。
3. 唤醒后 Delay 速度异常：可能没有调用 SystemInit() 恢复系统时钟。
4. 按 KEY3 不能唤醒：检查 PA15 是否配置成 EXTI15，触发边沿是否正确。
```

---

### 3. RTC 闹钟设置页面测试

目标：验证设置页面中可以进入 Alarm 页面，并能设置小时、分钟和开关。

测试步骤：

```text
1. 首页选择“设置”，短按 KEY3 进入设置页面。
2. 使用 KEY1/KEY2 切换选项。
3. 确认设置页面中有：返回、日期时间设置、Alarm。
4. 选中 Alarm，短按 KEY3 进入闹钟页面。
5. 在闹钟页面切换“返回 / 小时 / 分钟 / 开关”。
6. 进入小时设置，KEY1 加 1，KEY2 减 1，KEY3 确认返回。
7. 进入分钟设置，KEY1 加 1，KEY2 减 1，KEY3 确认返回。
8. 选中开关项，KEY3 切换 ON/OFF。
9. 选中返回，KEY3 返回设置页面。
```

预期结果：

```text
闹钟小时范围 0~23 循环变化。
闹钟分钟范围 0~59 循环变化。
闹钟开关能在 ON/OFF 之间切换。
返回逻辑正常。
```

常见问题：

```text
1. 页面选项乱跳：检查 Alarm_Flag 的范围是否正确。
2. 小时出现 24 或负数：检查上下限回绕逻辑。
3. 分钟出现 60 或负数：检查上下限回绕逻辑。
4. 进入 Alarm 后无法返回：检查返回项是否对应 return 0。
```

---

### 4. RTC 闹钟触发测试

目标：验证当前 RTC 时间到达闹钟时间后能触发提醒。

测试步骤：

```text
1. 进入日期时间设置，把当前时间设置成接近测试时间。
   例如设置为 12:29:50。
2. 进入 Alarm 页面，把闹钟设置为 12:30。
3. 将闹钟开关设置为 ON。
4. 返回首页时钟界面。
5. 等待时间走到 12:30。
6. 观察 OLED 是否显示 ALARM。
7. 观察 LED 是否闪烁。
8. 按任意键关闭闹钟提醒。
9. 保持在 12:30 这一分钟内，观察闹钟是否不会反复弹出。
10. 等到 12:31 后，如果第二天或下一轮再到 12:30，闹钟应能再次触发。
```

预期结果：

```text
到达闹钟时、分后触发一次提醒。
按键可以关闭提醒。
同一分钟内不会重复触发。
```

重点检查：

```text
1. Alarm_Enable 是否为 ON。
2. Alarm_Check() 是否在首页循环里被调用。
3. MyRTC_ReadTime() 是否正常更新当前时间。
4. Alarm_Latched 是否避免了同一分钟重复触发。
```

常见问题：

```text
1. 到点不响：可能没有调用 Alarm_Check()，或闹钟开关没打开。
2. 一分钟内反复响：可能没有 Alarm_Latched 锁存。
3. 按键无法关闭：Alarm_Ring() 里没有读取 kEY_GetNum()，或 TIM2 没有运行。
```

---

### 5. 低功耗与闹钟组合测试

第一版采用软件检查闹钟，需要注意：

```text
MCU 进入 STOP 后，主循环不运行，Alarm_Check() 也不会运行。
所以第一版闹钟不能在 STOP 模式下主动唤醒 MCU。
```

第一版组合测试步骤：

```text
1. 设置闹钟并打开闹钟开关。
2. 回到首页，等待闹钟时间到达。
3. 确认闹钟能正常响。
4. 关闭闹钟后，长按 KEY3 进入低功耗。
5. 按 KEY3 唤醒。
6. 唤醒后确认首页、菜单、闹钟设置页面仍正常。
```

预期结果：

```text
闹钟在正常运行状态下能响。
低功耗唤醒后系统功能仍正常。
```

第二版升级目标：

```text
使用 RTC Alarm 硬件中断，让 MCU 在 STOP 模式下也能被闹钟唤醒。
```

这属于下一阶段功能，不建议第一版一开始就做。

---

### 6. 原有功能回归测试

新增功能完成后，还要确认原来的手表功能没有被破坏。

测试项目：

```text
1. 首页时钟显示是否正常。
2. 菜单滑动动画是否正常。
3. 秒表开始、停止、清零是否正常。
4. 手电筒开关是否正常。
5. MPU6050 姿态显示是否正常。
6. 小恐龙游戏是否能进入、跳跃、碰撞、退出。
7. 动态表情包是否正常刷新。
8. 水平仪是否正常显示。
9. ADC 电池电量显示是否正常。
10. 长按 KEY3 是否只在首页触发休眠，不影响普通确认键使用。
```

预期结果：

```text
新增低功耗和闹钟后，原有页面和功能仍能正常进入、退出和显示。
```

---

### 7. 建议记录的测试结果

每次测试建议简单记录：

```text
测试日期：
测试版本：
测试功能：低功耗 / 闹钟 / 回归测试
现象：
是否通过：
发现问题：
解决方法：
```

这样后面可以继续整理进“项目问题错题本”，也方便面试时讲真实调试过程。
