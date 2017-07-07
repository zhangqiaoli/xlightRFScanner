#ifndef __TIMER_FOUR_H
#define __TIMER_FOUR_H

// 1ms interupt
void Time4_Init(void);

typedef void (*TM4_CallBack_t)();

extern TM4_CallBack_t TIM4_1ms_handler;
extern TM4_CallBack_t TIM4_5ms_handler;
extern TM4_CallBack_t TIM4_10ms_handler;

#endif // __TIMER_FOUR_H