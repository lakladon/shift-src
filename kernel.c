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

void kmain()
{
    vga_puts("Shift OS (VGA)", 0x0F, 0, 0);

    serial_init();
    serial_write("Shift OS\r\n");

    while (1)
    {
    }
}