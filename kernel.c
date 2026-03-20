void kmain();

void _start()
{
    kmain();
    while (1)
    {
    }
}

static inline void outb(unsigned short port, unsigned char value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline unsigned char inb(unsigned short port)
{
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static int serial_ready()
{
    return inb(0x3F8 + 5) & 0x20;
}

static void serial_init()
{
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x03);
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);
}

static void serial_write(const char* s)
{
    int i = 0;
    while (s[i]) {
        while (!serial_ready()) {
        }
        outb(0x3F8, (unsigned char)s[i]);
        i = i + 1;
    }
}

static void serial_write_char(char c)
{
    while (!serial_ready())
    {
    }
    outb(0x3F8, (unsigned char)c);
}

static void vga_puts(const char* s, unsigned char color, int row, int col)
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

static void vga_putc_at(char c, unsigned char color, int row, int col)
{
    volatile unsigned short* vga = (volatile unsigned short*)0xB8000;
    vga[row * 80 + col] = ((unsigned short)color << 8) | (unsigned char)c;
}

static void vga_clear(unsigned char color)
{
    volatile unsigned short* vga = (volatile unsigned short*)0xB8000;
    int i = 0;

    while (i < 80 * 25)
    {
        vga[i] = ((unsigned short)color << 8) | ' ';
        i = i + 1;
    }
}

static unsigned char kbd_has_data()
{
    return inb(0x64) & 1;
}

static unsigned char kbd_read_scancode()
{
    return inb(0x60);
}

static char scancode_to_ascii(unsigned char sc)
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

void kmain()
{
    const unsigned char color = 0x0F;
    int row = 2;
    int col = 0;
    char word[64];
    int len = 0;

    vga_clear(0x0F);
    vga_puts("Shift OS (VGA)", color, 0, 0);
    vga_puts("Type words and press Enter:", color, 1, 0);
    vga_puts("> ", color, row, col);
    col = 2;

    serial_init();
    serial_write("Shift OS\r\n");
    serial_write("Type words and press Enter:\r\n> ");

    while (1)
    {
        unsigned char sc;
        char c;

        if (!kbd_has_data())
        {
            continue;
        }

        sc = kbd_read_scancode();

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
                vga_putc_at(' ', color, row, col);
                serial_write("\b \b");
            }
            continue;
        }

        if (sc == 0x1C)
        {
            word[len] = 0;
            serial_write("\r\n");

            row = row + 1;
            if (row >= 25)
            {
                vga_clear(color);
                vga_puts("Shift OS (VGA)", color, 0, 0);
                vga_puts("Type words and press Enter:", color, 1, 0);
                row = 2;
            }

            vga_puts("You typed: ", color, row, 0);
            vga_puts(word, color, row, 11);

            row = row + 1;
            if (row >= 25)
            {
                vga_clear(color);
                vga_puts("Shift OS (VGA)", color, 0, 0);
                vga_puts("Type words and press Enter:", color, 1, 0);
                row = 2;
            }

            vga_puts("> ", color, row, 0);
            serial_write("> ");
            col = 2;
            len = 0;
            continue;
        }

        c = scancode_to_ascii(sc);
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
            vga_putc_at(c, color, row, col);
            serial_write_char(c);
            col = col + 1;
        }
    }
}