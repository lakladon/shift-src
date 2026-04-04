#include "irqka.h"
#include "iozh.h"
#include "timerka.h"

struct idt_gate
{
    unsigned short off_lo;
    unsigned short sel;
    unsigned char zero;
    unsigned char flags;
    unsigned short off_hi;
} __attribute__((packed));

struct idt_ptr
{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

static struct idt_gate idt[256];
static struct idt_ptr idtr;

static void idt_set_gate(unsigned char vec, unsigned int handler, unsigned short sel, unsigned char flags)
{
    idt[vec].off_lo = (unsigned short)(handler & 0xFFFFu);
    idt[vec].sel = sel;
    idt[vec].zero = 0;
    idt[vec].flags = flags;
    idt[vec].off_hi = (unsigned short)((handler >> 16) & 0xFFFFu);
}

static void pic_remap_and_mask(void)
{
    unsigned char a1;
    unsigned char a2;

    a1 = zhmyak_in(0x21);
    a2 = zhmyak_in(0xA1);

    zhmyak_out(0x20, 0x11);
    zhmyak_out(0xA0, 0x11);
    zhmyak_out(0x21, 0x20);
    zhmyak_out(0xA1, 0x28);
    zhmyak_out(0x21, 0x04);
    zhmyak_out(0xA1, 0x02);
    zhmyak_out(0x21, 0x01);
    zhmyak_out(0xA1, 0x01);

    (void)a1;
    (void)a2;

    zhmyak_out(0x21, 0xFE);
    zhmyak_out(0xA1, 0xFF);
}

void irq0_c_handler(void)
{
    timerka_tick_irq0();
}

__attribute__((naked)) void irq0_stub(void)
{
    __asm__ volatile (
        "cli\n"
        "pusha\n"
        "call irq0_c_handler\n"
        "movb $0x20, %al\n"
        "outb %al, $0x20\n"
        "popa\n"
        "sti\n"
        "iret\n"
    );
}

__attribute__((naked)) void irq_ignore_stub(void)
{
    __asm__ volatile ("iret\n");
}

void irqka_on()
{
    int i = 0;
    unsigned int ignore = (unsigned int)(unsigned long)&irq_ignore_stub;
    unsigned int irq0 = (unsigned int)(unsigned long)&irq0_stub;

    __asm__ volatile ("cli");

    while (i < 256)
    {
        idt_set_gate((unsigned char)i, ignore, 0x08, 0x8E);
        i = i + 1;
    }
    idt_set_gate(32, irq0, 0x08, 0x8E);

    idtr.limit = (unsigned short)(sizeof(idt) - 1);
    idtr.base = (unsigned int)(unsigned long)&idt[0];
    __asm__ volatile ("lidt %0" : : "m"(idtr));

    pic_remap_and_mask();
    __asm__ volatile ("sti");
}
