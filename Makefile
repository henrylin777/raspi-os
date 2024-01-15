CFLAGS := -g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-function -Iinclude -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles 
CFLAGS += -I /usr/lib/gcc-cross/aarch64-linux-gnu/11/include
# CFLAGS += -D CONFIG_MM_DEBUG

BUILD_DIR := ./build
SRC_DIR := ./src

SRC = $(wildcard src/*.c)
OBJS = $(SRC:.c=.o)
OBJS := $(OBJS:src/%=build/%)
OBJS += build/start.s.o build/schedule.s.o build/utils.s.o


all: kernel8.img

# build step for c
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@
	
# build step for assembly
$(BUILD_DIR)/%.s.o: $(SRC_DIR)/assembly/%.s
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@


kernel8.img: $(OBJS)
	aarch64-linux-gnu-ld $(OBJS) -T linker/linker.ld -o kernel8.elf
	aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img


.PHONY: clean
clean:
	$(RM) *.elf kernel8.img $(BUILD_DIR)/*.o *.o > /dev/null 2> /dev/null || true

test:
	qemu-system-aarch64 -M raspi3b \
		-kernel kernel8.img \
		-initrd initramfs.cpio \
		-dtb bcm2710-rpi-3-b-plus.dtb \
		-drive if=sd,file=myfs.img,format=raw \
		-serial null -serial stdio -display none
