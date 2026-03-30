#include "timerka.h"
#include "iozh.h"

static unsigned int tiks = 0;

static unsigned char cmos_read(unsigned char reg)
{
    zhmyak_out(0x70, reg);
    return zhmyak_in(0x71);
}

static unsigned char bcd_to_bin(unsigned char v)
{
    return (unsigned char)((v & 0x0F) + ((v >> 4) * 10));
}

static int is_leap(unsigned int y)
{
    if ((y % 400) == 0)
    {
        return 1;
    }
    if ((y % 100) == 0)
    {
        return 0;
    }
    return (y % 4) == 0;
}

void timerka_on()
{
    unsigned short divisor = 11932;

    zhmyak_out(0x43, 0x34);
    zhmyak_out(0x40, (unsigned char)(divisor & 0xFF));
    zhmyak_out(0x40, (unsigned char)(divisor >> 8));

    tiks = 0;
}

void timerka_tick_irq0()
{
    tiks = tiks + 1;
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

unsigned int rtc_unix_time()
{
    unsigned char sec;
    unsigned char min;
    unsigned char hour;
    unsigned char day;
    unsigned char mon;
    unsigned char year;
    unsigned char reg_b;
    unsigned int y;
    unsigned int days = 0;
    unsigned int m;
    static const unsigned char mdays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

    while (cmos_read(0x0A) & 0x80)
    {
    }

    sec = cmos_read(0x00);
    min = cmos_read(0x02);
    hour = cmos_read(0x04);
    day = cmos_read(0x07);
    mon = cmos_read(0x08);
    year = cmos_read(0x09);
    reg_b = cmos_read(0x0B);

    if ((reg_b & 0x04) == 0)
    {
        sec = bcd_to_bin(sec);
        min = bcd_to_bin(min);
        hour = bcd_to_bin((unsigned char)(hour & 0x7F));
        day = bcd_to_bin(day);
        mon = bcd_to_bin(mon);
        year = bcd_to_bin(year);
    }
    else
    {
        hour = (unsigned char)(hour & 0x7F);
    }

    if (mon < 1 || mon > 12 || day < 1 || day > 31)
    {
        return 0;
    }

    y = 1970;
    while (y < (2000u + (unsigned int)year))
    {
        days = days + (unsigned int)(is_leap(y) ? 366 : 365);
        y = y + 1;
    }

    m = 1;
    while (m < (unsigned int)mon)
    {
        days = days + (unsigned int)mdays[m - 1];
        if (m == 2 && is_leap(2000u + (unsigned int)year))
        {
            days = days + 1;
        }
        m = m + 1;
    }

    days = days + (unsigned int)(day - 1);
    return (days * 86400u) + ((unsigned int)hour * 3600u) + ((unsigned int)min * 60u) + (unsigned int)sec;
}