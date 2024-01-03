# Operation System Capstone(OSC)

This project is a series of implemenetations related to operate system, and  cover some common mechanisms such as booting, interrupt, file system,...etc. 
The name "OSC" and the overall architecture of this project come from the lecture with the same name at National Yang Ming Chiao Tung University, please reference this [website](https://oscapstone.github.io/index.html) for more information.

<br>

## Prerequisites

Before getting started, you need to prepare the following things: 
* Raspberry Pi 3+
* USB-to-TTL (P2303HXDL)
* Micro SD card
* Card reader

You also need to build cross-platform development if you develop on non-ARM64 enviroment. For Ubuntu or Unix-like user, you can install cross compiler with `apt`

```shell
$ sudo apt install qemu-system
```

In cross-platform development, it would be easier to test your code on an emulator before deploying on Raspberry pi 3+. For Ubuntu or Unix-like user, you can use the following command to install QEMU

```shell
$ sudo apt install qemu-system
```

Maybe you need to install gdb for debugging usage.

```shell
$ sudo apt install gdb
```

<br>

## Lab overview

### Lab1-Uart

Goal of this lab
* Practice bare metal programming 
* Learn how to access Rpi3's peripherals without OS support
* Setup communication interface between Rpi3 and host computer with mini UART

### Lab2-Booting

Goal of this lab
* Build a OS bootloader with mini UART
* Understand the concept of initial ramdisk and how to parse raw cpio archives
* Understand what is devicetree and how to parse raw dtb

### Lab3-Interrupt

Goal of this lab
* Understand the Exception mechanism in Armv8-A
* Set up exception vector table and excpetion handler
* Implement asynchronous UART read/write operation
* Implement multiplexing CPU timer 


### Lab3-Interrupt

Goal of this lab
* Understand the Exception mechanism in Armv8-A
* Set up exception vector table and excpetion handler
* Implement asynchronous UART read/write operation
* Implement multiplexing CPU timer 

### Lab4-Memory Allocator

Goal of this lab
* Implement buddy system
* Implement slub allocator 


### Lab5-Process management (in progress)

Goal of this lab
* Understand thread management mechanism
* Understand what is kerenl preemption
* Implement context switch and scheduler
* Implement POSIX signals 

### Lab6-Virtual Memory (in progress)

Goal of this lab
* Understand ARMv8-A virtual memory system architecture
* Understand how kernel manages memory for user processes
* Understand how demand paging works

<br>

## Reference
* [Raspberry Pi](https://github.com/raspberrypi)
* [Operation System Capstone, National Yang Ming Chiao Tung University](https://oscapstone.github.io/index.html) 
