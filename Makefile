all: os.bin

boot.bin: boot.asm
	nasm -f bin boot.asm -o boot.bin

kernel.bin: kernel.c iozh.c serialka.c ekran.c klava.c stroki.c timerka.c vfs.c
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -c kernel.c -o kernel.o
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -c iozh.c -o iozh.o
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -c serialka.c -o serialka.o
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -c ekran.c -o ekran.o
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -c klava.c -o klava.o
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -c stroki.c -o stroki.o
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -c timerka.c -o timerka.o
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -c vfs.c -o vfs.o
	ld -m elf_i386 -Ttext 0x1000 --oformat binary kernel.o iozh.o serialka.o ekran.o klava.o stroki.o timerka.o vfs.o -o kernel.bin
	rm -f kernel.o iozh.o serialka.o ekran.o klava.o stroki.o timerka.o vfs.o

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
	rm -f boot.bin kernel.bin os.bin k.o kernel.o iozh.o serialka.o ekran.o klava.o stroki.o timerka.o vfs.o