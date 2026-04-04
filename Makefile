all: os.bin

vfs.img:
	@if [ ! -f vfs.img ]; then dd if=/dev/zero of=vfs.img bs=1M count=4 status=none; fi

boot.bin: boot.asm
	nasm -f bin boot.asm -o boot.bin

kernel.elf: kernel.c iozh.c serialka.c ekran.c klava.c stroki.c timerka.c irqka.c vfs.c multiboot.asm
	nasm -f elf32 multiboot.asm -o multiboot.o
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -c kernel.c -o kernel.o
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -c iozh.c -o iozh.o
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -c serialka.c -o serialka.o
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -c ekran.c -o ekran.o
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -c klava.c -o klava.o
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -c stroki.c -o stroki.o
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -c timerka.c -o timerka.o
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -c irqka.c -o irqka.o
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -c vfs.c -o vfs.o
	ld -m elf_i386 -T linker.ld multiboot.o kernel.o iozh.o serialka.o ekran.o klava.o stroki.o timerka.o irqka.o vfs.o -o kernel.elf
	rm -f multiboot.o kernel.o iozh.o serialka.o ekran.o klava.o stroki.o timerka.o irqka.o vfs.o

kernel.bin: kernel.elf
	objcopy -O binary kernel.elf kernel.bin

os.bin: boot.bin kernel.bin
	cat boot.bin kernel.bin > os.bin
	truncate -s 1474560 os.bin

run: os.bin vfs.img
	qemu-system-i386 -serial stdio -monitor none -no-reboot -no-shutdown -drive file=os.bin,if=floppy,format=raw -boot a -drive file=vfs.img,if=ide,format=raw

iso-grub: kernel.elf
	mkdir -p iso-grub/boot/grub
	cp kernel.elf iso-grub/boot/kernel.elf
	printf 'set timeout=0\nset default=0\n\nmenuentry "SHIFT OS" {\n    multiboot /boot/kernel.elf\n    boot\n}\n' > iso-grub/boot/grub/grub.cfg
	grub-mkrescue -o shift-os.iso iso-grub/
	rm -rf iso-grub/

clean:
	rm -f boot.bin kernel.bin os.bin kernel.elf k.o kernel.o iozh.o serialka.o ekran.o klava.o stroki.o timerka.o irqka.o vfs.o
	rm -rf iso/ iso-grub/
