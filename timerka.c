#include "timerka.h"
#include "iozh.h"

static unsigned int tiks = 0;
static unsigned short last_pit = 0;
static int last_set = 0;

static unsigned short pit_read_counter0()
{
    unsigned char lo;
    unsigned char hi;

    zhmyak_out(0x43, 0x00);
    lo = zhmyak_in(0x40);
    hi = zhmyak_in(0x40);
    return (unsigned short)(lo | ((unsigned short)hi << 8));
}

static unsigned char cmos_read(unsigned char reg)
{
    zhmyak_out(0x70, reg);
    return zhmyak_in(0x71);
}

static unsigned char bcd_to_bin(unsigned char v)
{
    return (unsigned char)((v & 0x0F) + ((v >> 4) * 10));
}

void timerka_on()
{
    unsigned short divisor = 11932;

    zhmyak_out(0x43, 0x34);
    zhmyak_out(0x40, (unsigned char)(divisor & 0xFF));
    zhmyak_out(0x40, (unsigned char)(divisor >> 8));

    last_pit = pit_read_counter0();
    last_set = 1;
    tiks = 0;
}

void timerka_update()
{
    unsigned short now = pit_read_counter0();

    if (!last_set)
    {
        last_pit = now;
        last_set = 1;
        return;
    }

    if (now > last_pit)
    {
        tiks = tiks + 1;
    }

    last_pit = now;
}

unsigned int uptime_sec()
{
    return tiks / 100;
}

void rtc_time_hms(unsigned char* h, unsigned char* m, unsigned char* s)
{
    unsigned char sec;
    unsigned char min;
    unsigned char hour;
    unsigned char reg_b;

    while (cmos_read(0x0A) & 0x80)
    {
    }

    sec = cmos_read(0x00);
    min = cmos_read(0x02);
    hour = cmos_read(0x04);
    reg_b = cmos_read(0x0B);

    if ((reg_b & 0x04) == 0)
    {
        sec = bcd_to_bin(sec);
        min = bcd_to_bin(min);
        hour = bcd_to_bin((unsigned char)(hour & 0x7F));
    }
    else
    {
        hour = (unsigned char)(hour & 0x7F);
    }

    *h = hour;
    *m = min;
    *s = sec;
}