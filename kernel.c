void mega_tusa();

void _start()
{
    mega_tusa();
    while (1)
    {
    }
}

static inline void zhmyak_out(unsigned short port, unsigned char value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline unsigned char zhmyak_in(unsigned short port)
{
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static int serialka_go()
{
    return zhmyak_in(0x3F8 + 5) & 0x20;
}

static void serialka_on()
{
    zhmyak_out(0x3F8 + 1, 0x00);
    zhmyak_out(0x3F8 + 3, 0x80);
    zhmyak_out(0x3F8 + 0, 0x03);
    zhmyak_out(0x3F8 + 1, 0x00);
    zhmyak_out(0x3F8 + 3, 0x03);
    zhmyak_out(0x3F8 + 2, 0xC7);
    zhmyak_out(0x3F8 + 4, 0x0B);
}

static void say_to_serialka(const char* s)
{
    int i = 0;
    while (s[i]) {
        while (!serialka_go()) {
        }
        zhmyak_out(0x3F8, (unsigned char)s[i]);
        i = i + 1;
    }
}

static void say_char_to_serialka(char c)
{
    while (!serialka_go())
    {
    }
    zhmyak_out(0x3F8, (unsigned char)c);
}

static void pishi_na_ekran(const char* s, unsigned char color, int row, int col)
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

static void pishi_bukvu(char c, unsigned char color, int row, int col)
{
    volatile unsigned short* vga = (volatile unsigned short*)0xB8000;
    vga[row * 80 + col] = ((unsigned short)color << 8) | (unsigned char)c;
}

static void ochisti_ekranchik(unsigned char color)
{
    volatile unsigned short* vga = (volatile unsigned short*)0xB8000;
    int i = 0;

    while (i < 80 * 25)
    {
        vga[i] = ((unsigned short)color << 8) | ' ';
        i = i + 1;
    }
}

static unsigned char klava_est()
{
    return zhmyak_in(0x64) & 1;
}

static unsigned char klava_chitai()
{
    return zhmyak_in(0x60);
}

static char skan_v_bukvu(unsigned char sc)
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

static int stroki_odinakovie(const char* a, const char* b)
{
    int i = 0;
    while (a[i] && b[i])
    {
        if (a[i] != b[i])
        {
            return 0;
        }
        i = i + 1;
    }
    return a[i] == b[i];
}

static int nachinaetsya_s(const char* s, const char* prefix)
{
    int i = 0;
    while (prefix[i])
    {
        if (s[i] != prefix[i])
        {
            return 0;
        }
        i = i + 1;
    }
    return 1;
}

static void shelly_print_line(const char* s, unsigned char color, int* row)
{
    if (*row >= 25)
    {
        *row = 2;
    }
    pishi_na_ekran(s, color, *row, 0);
    say_to_serialka(s);
    say_to_serialka("\r\n");
    *row = *row + 1;
}

static void shelly_reset_screen(unsigned char color, int* row)
{
    ochisti_ekranchik(color);
    pishi_na_ekran("Shift OS (VGA)", color, 0, 0);
    pishi_na_ekran("Mini shell: help, clear, about, echo/sayser", color, 1, 0);
    say_to_serialka("\r\n[screen cleared]\r\n");
    *row = 2;
}

void mega_tusa()
{
    const unsigned char color = 0x0F;
    int row = 2;
    int col = 0;
    char word[64];
    int len = 0;

    ochisti_ekranchik(color);
    pishi_na_ekran("Shift OS (VGA)", color, 0, 0);
    pishi_na_ekran("Mini shell: help, clear, about, echo/sayser", color, 1, 0);
    pishi_na_ekran("> ", color, row, col);
    col = 2;

    serialka_on();
    say_to_serialka("Shift OS\r\n");
    say_to_serialka("Mini shell: help, clear, about, echo <text>, sayser <text>\r\n> ");

    while (1)
    {
        unsigned char sc;
        char c;

        if (!klava_est())
        {
            continue;
        }

        sc = klava_chitai();

        if (sc & 0x80)
        {
            continue;
        }

        if (sc == 0x0E)
        {
            if (len > 0 && col > 2)
            {
                len = len - 1;
                col = col - 1;
                pishi_bukvu(' ', color, row, col);
                say_to_serialka("\b \b");
            }
            continue;
        }

        if (sc == 0x1C)
        {
            word[len] = 0;
            say_to_serialka("\r\n");

            row = row + 1;
            if (stroki_odinakovie(word, "help"))
            {
                shelly_print_line("Commands:", color, &row);
                shelly_print_line("  help", color, &row);
                shelly_print_line("  clear", color, &row);
                shelly_print_line("  about", color, &row);
                shelly_print_line("  echo <text>", color, &row);
                shelly_print_line("  sayser <text>", color, &row);
            }
            else if (stroki_odinakovie(word, "clear"))
            {
                shelly_reset_screen(color, &row);
            }
            else if (stroki_odinakovie(word, "about"))
            {
                shelly_print_line("Shift OS mini shell (32-bit protected mode)", color, &row);
            }
            else if (nachinaetsya_s(word, "echo "))
            {
                shelly_print_line(word + 5, color, &row);
            }
            else if (nachinaetsya_s(word, "sayser "))
            {
                say_to_serialka(word + 7);
                say_to_serialka("\r\n");
            }
            else if (len == 0)
            {
            }
            else
            {
                shelly_print_line("Unknown command. Type: help", color, &row);
            }

            if (row >= 25)
            {
                shelly_reset_screen(color, &row);
            }

            pishi_na_ekran("> ", color, row, 0);
            say_to_serialka("> ");
            col = 2;
            len = 0;
            continue;
        }

        c = skan_v_bukvu(sc);
        if (!c)
        {
            continue;
        }

        if (c == '\t')
        {
            continue;
        }

        if (len < 63 && col < 80)
        {
            word[len] = c;
            len = len + 1;
            pishi_bukvu(c, color, row, col);
            say_char_to_serialka(c);
            col = col + 1;
        }
    }
}