// . 

#include "serialka.h"
#include "ekran.h"
#include "klava.h"
#include "stroki.h"

void mega_tusa();

void _start()
{
    mega_tusa();
    while (1)
    {
    }
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

    ubi_mig_stroku();
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