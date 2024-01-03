all: func.img

func.o: func.s
	aarch64-linux-gnu-gcc -c func.s -o func.o

func.img: func.o
	aarch64-linux-gnu-ld func.o -T linker.ld -o func.elf
	aarch64-linux-gnu-objcopy -O binary func.elf func.img

clean:
	rm func.o func.elf > /dev/null 2>/dev/null || true
