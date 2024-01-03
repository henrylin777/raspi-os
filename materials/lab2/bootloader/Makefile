CFLAGS = -Wall -Wextra -Wpedantic -Werror -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles 

SRC_DIR := ./src
BUILD_DIR := ./build
SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)
OBJS := $(OBJS:src/%=build/%)
OBJS += build/start.s.o

all: bootloader.img

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.s.o: $(SRC_DIR)/%.s
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@

bootloader.img: $(OBJS)
	@echo "OBJS: " $(OBJS)
	aarch64-linux-gnu-ld $(OBJS) -T linker.ld -o bootloader.elf
	aarch64-linux-gnu-objcopy -O binary bootloader.elf bootloader.img

clean:
	rm bootloader.elf bootloader.img build/*.o >/dev/null 2>/dev/null || true

test: all
	qemu-system-aarch64 -M raspi3b -serial null -serial stdio -display none -kernel bootloader.img
