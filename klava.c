#include "klava.h"
#include "iozh.h"

unsigned char klava_est()
{
    return zhmyak_in(0x64) & 1;
}

unsigned char klava_chitai()
{
    return zhmyak_in(0x60);
}

char skan_v_bukvu(unsigned char sc)
{
    static const char map[128] = {
        0,
        27,
        '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
        '\b',
        '\t',
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
        '\n',
        0,
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0,
        '\\',
        'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
        0,
        '*',
        0,
        ' ',
    };

    if (sc < 128)
    {
        return map[sc];
    }
    return 0;
}