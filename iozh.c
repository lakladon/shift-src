#include "iozh.h"

void zhmyak_out(unsigned short port, unsigned char value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

unsigned char zhmyak_in(unsigned short port)
{
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void zhmyak_out16(unsigned short port, unsigned short value)
{
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

unsigned short zhmyak_in16(unsigned short port)
{
    unsigned short ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}