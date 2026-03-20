#include "ekran.h"
#include "iozh.h"

void pishi_na_ekran(const char* s, unsigned char color, int row, int col)
{
    volatile unsigned short* vga = (volatile unsigned short*)0xB8000;
    int i = 0;
    int offset = row * 80 + col;

    while (s[i])
    {
        vga[offset + i] = ((unsigned short)color << 8) | (unsigned char)s[i];
        i = i + 1;
    }
}

void pishi_bukvu(char c, unsigned char color, int row, int col)
{
    volatile unsigned short* vga = (volatile unsigned short*)0xB8000;
    vga[row * 80 + col] = ((unsigned short)color << 8) | (unsigned char)c;
}

void ochisti_ekranchik(unsigned char color)
{
    volatile unsigned short* vga = (volatile unsigned short*)0xB8000;
    int i = 0;

    while (i < 80 * 25)
    {
        vga[i] = ((unsigned short)color << 8) | ' ';
        i = i + 1;
    }
}

void ubi_mig_stroku()
{
    zhmyak_out(0x3D4, 0x0A);
    zhmyak_out(0x3D5, 0x20);
}