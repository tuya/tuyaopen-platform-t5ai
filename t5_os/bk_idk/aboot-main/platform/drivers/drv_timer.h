#ifndef _TIMER_H_
#define _TIMER_H_

#define FCLK_PWM_ID           PWM0
#define FCLK_TIMER_ID           BKTIMER3
#define FCLK_DURATION_MS      2
#define FCLK_SECOND           (1000/FCLK_DURATION_MS)

int timer_init(void);

#endif //_TIMER_H_
//eof

