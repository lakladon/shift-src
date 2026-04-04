ENTRY(multiboot_entry)

SECTIONS
{
    . = 0x1000;
    
    .text : {
        *(.text.multiboot)
        *(.text)
        *(.text.*)
    }
    
    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }
    
    .data : {
        *(.data)
        *(.data.*)
    }
    
    .bss : {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }
    
    /DISCARD/ : {
        *(.comment)
        *(.note.*)
    }
}
