#ifndef TIMERKA_H
#define TIMERKA_H

void timerka_on(void);
void timerka_tick_irq0(void);
unsigned int uptime_sec(void);
void rtc_time_hms(unsigned char* h, unsigned char* m, unsigned char* s);
unsigned int rtc_unix_time(void);

#endif
