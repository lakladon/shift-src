// . 

#include "serialka.h"
#include "ekran.h"
#include "klava.h"
#include "stroki.h"
#include "timerka.h"
#include "vfs.h"

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
    pishi_na_ekran("Mini shell: help, clear, about, echo/sayser, time, uptime, ls/cat/write", color, 1, 0);
    say_to_serialka("\r\n[screen cleared]\r\n");
    *row = 2;
}

static void append_char(char* buf, int* pos, char c)
{
    if (*pos < 63)
    {
        buf[*pos] = c;
        *pos = *pos + 1;
    }
}

static void append_text(char* buf, int* pos, const char* s)
{
    int i = 0;
    while (s[i])
    {
        append_char(buf, pos, s[i]);
        i = i + 1;
    }
}

static void append_u32_dec(char* buf, int* pos, unsigned int v)
{
    char tmp[10];
    int i = 0;

    if (v == 0)
    {
        append_char(buf, pos, '0');
        return;
    }

    while (v > 0 && i < 10)
    {
        tmp[i] = (char)('0' + (v % 10));
        v = v / 10;
        i = i + 1;
    }

    while (i > 0)
    {
        i = i - 1;
        append_char(buf, pos, tmp[i]);
    }
}

static void append_2d(char* buf, int* pos, unsigned char v)
{
    append_char(buf, pos, (char)('0' + ((v / 10) % 10)));
    append_char(buf, pos, (char)('0' + (v % 10)));
}

static int first_space_pos(const char* s)
{
    int i = 0;
    while (s[i])
    {
        if (s[i] == ' ')
        {
            return i;
        }
        i = i + 1;
    }
    return -1;
}

static int str_len64(const char* s)
{
    int i = 0;
    while (i < 63 && s[i])
    {
        i = i + 1;
    }
    return i;
}

static void str_copy64(char* dst, const char* src)
{
    int i = 0;
    while (i < 63 && src[i])
    {
        dst[i] = src[i];
        i = i + 1;
    }
    dst[i] = 0;
}

static void redraw_input_line(unsigned char color, int row, char* word, int* len, int* col)
{
    int i = 2;

    while (i < 80)
    {
        pishi_bukvu(' ', color, row, i);
        i = i + 1;
    }

    i = 0;
    while (i < *len && (2 + i) < 80)
    {
        pishi_bukvu(word[i], color, row, 2 + i);
        i = i + 1;
    }

    *col = 2 + *len;
}

void mega_tusa()
{
    const unsigned char color = 0x0F;
    int row = 2;
    int col = 0;
    char word[64];
    int len = 0;
    char history[10][64];
    int hist_count = 0;
    int hist_pos = -1;
    int e0_prefix = 0;

    ubi_mig_stroku();
    ochisti_ekranchik(color);
    pishi_na_ekran("Shift OS (VGA)", color, 0, 0);
    pishi_na_ekran("Mini shell: help, clear, about, echo/sayser, time, uptime, ls/cat/write", color, 1, 0);
    pishi_na_ekran("> ", color, row, col);
    col = 2;

    serialka_on();
    timerka_on();
    vfs_init();
    say_to_serialka("Shift OS\r\n");
    say_to_serialka("Mini shell: help, clear, about, echo <text>, sayser <text>, time, uptime, ls, cat <path>, write <path> <text>\r\n> ");

    while (1)
    {
        unsigned char sc;
        char c;
        char out[64];
        int out_pos;

        timerka_update();

        if (!klava_est())
        {
            continue;
        }

        sc = klava_chitai();

        if (sc == 0xE0)
        {
            e0_prefix = 1;
            continue;
        }

        if (e0_prefix)
        {
            e0_prefix = 0;

            if (sc == 0x48)
            {
                if (hist_count > 0)
                {
                    if (hist_pos == -1)
                    {
                        hist_pos = hist_count - 1;
                    }
                    else if (hist_pos > 0)
                    {
                        hist_pos = hist_pos - 1;
                    }

                    str_copy64(word, history[hist_pos]);
                    len = str_len64(word);
                    redraw_input_line(color, row, word, &len, &col);
                }
            }
            else if (sc == 0x50)
            {
                if (hist_count > 0 && hist_pos != -1)
                {
                    if (hist_pos < hist_count - 1)
                    {
                        hist_pos = hist_pos + 1;
                        str_copy64(word, history[hist_pos]);
                        len = str_len64(word);
                    }
                    else
                    {
                        hist_pos = -1;
                        len = 0;
                        word[0] = 0;
                    }

                    redraw_input_line(color, row, word, &len, &col);
                }
            }

            continue;
        }

        if (sc & 0x80)
        {
            continue;
        }

        if (sc == 0x0E)
        {
            if (len > 0 && col > 2)
            {
                len = len - 1;
                hist_pos = -1;
                col = col - 1;
                pishi_bukvu(' ', color, row, col);
                say_to_serialka("\b \b");
            }
            continue;
        }

        if (sc == 0x1C)
        {
            word[len] = 0;

            if (len > 0)
            {
                if (hist_count < 10)
                {
                    str_copy64(history[hist_count], word);
                    hist_count = hist_count + 1;
                }
                else
                {
                    int hi = 1;
                    while (hi < 10)
                    {
                        str_copy64(history[hi - 1], history[hi]);
                        hi = hi + 1;
                    }
                    str_copy64(history[9], word);
                }
            }

            hist_pos = -1;
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
                shelly_print_line("  time", color, &row);
                shelly_print_line("  uptime", color, &row);
                shelly_print_line("  ls", color, &row);
                shelly_print_line("  cat <path>", color, &row);
                shelly_print_line("  write <path> <text>", color, &row);
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
            else if (stroki_odinakovie(word, "uptime"))
            {
                out_pos = 0;
                append_text(out, &out_pos, "uptime: ");
                append_u32_dec(out, &out_pos, uptime_sec());
                append_text(out, &out_pos, " s");
                out[out_pos] = 0;
                shelly_print_line(out, color, &row);
            }
            else if (stroki_odinakovie(word, "time"))
            {
                unsigned char hh;
                unsigned char mm;
                unsigned char ss;

                rtc_time_hms(&hh, &mm, &ss);
                out_pos = 0;
                append_text(out, &out_pos, "time: ");
                append_2d(out, &out_pos, hh);
                append_char(out, &out_pos, ':');
                append_2d(out, &out_pos, mm);
                append_char(out, &out_pos, ':');
                append_2d(out, &out_pos, ss);
                out[out_pos] = 0;
                shelly_print_line(out, color, &row);
            }
            else if (stroki_odinakovie(word, "ls"))
            {
                int i = 0;
                while (i < vfs_count())
                {
                    const char* p = vfs_path_at(i);
                    if (p)
                    {
                        shelly_print_line(p, color, &row);
                    }
                    i = i + 1;
                }
            }
            else if (nachinaetsya_s(word, "cat "))
            {
                if (!vfs_read(word + 4, out, 64))
                {
                    shelly_print_line("cat: file not found", color, &row);
                }
                else
                {
                    shelly_print_line(out, color, &row);
                }
            }
            else if (nachinaetsya_s(word, "write "))
            {
                int split = first_space_pos(word + 6);

                if (split < 0)
                {
                    shelly_print_line("write: usage write <path> <text>", color, &row);
                }
                else
                {
                    char path[32];
                    int i = 0;
                    char* args = word + 6;
                    char* text;

                    while (i < split && i < 31)
                    {
                        path[i] = args[i];
                        i = i + 1;
                    }
                    path[i] = 0;

                    text = args + split + 1;
                    if (!vfs_write(path, text))
                    {
                        shelly_print_line("write: failed", color, &row);
                    }
                    else
                    {
                        shelly_print_line("write: ok", color, &row);
                    }
                }
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
            hist_pos = -1;
            word[len] = c;
            len = len + 1;
            pishi_bukvu(c, color, row, col);
            say_char_to_serialka(c);
            col = col + 1;
        }
    }
}