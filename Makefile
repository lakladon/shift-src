all: os.bin

boot.bin: boot.asm
	nasm -f bin boot.asm -o boot.bin

kernel.bin: kernel.c
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -c kernel.c -o k.o
	ld -m elf_i386 -Ttext 0x1000 --oformat binary k.o -o kernel.bin
	rm -f k.o

os.bin: boot.bin kernel.bin
	cat boot.bin kernel.bin > os.bin
	truncate -s 1474560 os.bin

run: os.bin
	qemu-system-i386 -drive format=raw,file=os.bin

run-vga: os.bin
	qemu-system-i386 -drive file=os.bin,if=floppy,format=raw -boot a

run-serial: os.bin
	qemu-system-i386 -drive file=os.bin,if=floppy,format=raw -boot a -monitor none -serial stdio -display none

clean:
	rm -f boot.bin kernel.bin os.bin k.o