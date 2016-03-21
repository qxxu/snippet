PREFIX=arm-linux-gnueabi-

all:test.bin

startup.o:startup.s
	$(PREFIX)as -mcpu=arm926ej-s -g startup.s -o startup.o

test.o:test.c
	$(PREFIX)gcc -c -mcpu=arm926ej-s -g test.c -o test.o


test.elf:test.ld test.o startup.o
	$(PREFIX)ld -T test.ld test.o startup.o -o test.elf

test.bin:test.elf
	$(PREFIX)objcopy -O binary test.elf test.bin

clean:
	rm test.elf test.bin test.o startup.o

qeum:
	qemu-system-arm -M vexpress-a9 -m 512M -nographic -kernel test.bin

gdb:
	qemu-system-arm -M vexpress-a9 -m 512M -nographic -s -S -kernel test.bin
