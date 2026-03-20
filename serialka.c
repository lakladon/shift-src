#include "serialka.h"
#include "iozh.h"

static int serialka_go()
{
    return zhmyak_in(0x3F8 + 5) & 0x20;
}

void serialka_on()
{
    zhmyak_out(0x3F8 + 1, 0x00);
    zhmyak_out(0x3F8 + 3, 0x80);
    zhmyak_out(0x3F8 + 0, 0x03);
    zhmyak_out(0x3F8 + 1, 0x00);
    zhmyak_out(0x3F8 + 3, 0x03);
    zhmyak_out(0x3F8 + 2, 0xC7);
    zhmyak_out(0x3F8 + 4, 0x0B);
}

void say_to_serialka(const char* s)
{
    int i = 0;
    while (s[i]) {
        while (!serialka_go()) {
        }
        zhmyak_out(0x3F8, (unsigned char)s[i]);
        i = i + 1;
    }
}

void say_char_to_serialka(char c)
{
    while (!serialka_go())
    {
    }
    zhmyak_out(0x3F8, (unsigned char)c);
}