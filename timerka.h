#ifndef TIMERKA_H
#define TIMERKA_H

void timerka_on();
void timerka_update();
unsigned int uptime_sec();
void rtc_time_hms(unsigned char* h, unsigned char* m, unsigned char* s);

#endif