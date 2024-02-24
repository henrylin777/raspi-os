# raspi-os

This project contains a series of implemenetations related to essential kernel modules in operating system such as scheduler, interrupt handling, and file system...etc. 
The goal of this project is to understand the basic concept of Linux kernel by implementing kernel modules from scratch, so the source code is largely based on
Linux kernel. 

Each lab is about to build one kernel feature, it has a corresponding folder in materials directory. 

This project is still under development, for now it is functionality-limited and only supports on Raspberry Pi 3.   

<br>

## Prerequisites

Before getting started, you need to prepare the following things: 
* Raspberry Pi 3+
* USB-to-TTL (P2303HXDL)
* Micro SD card
* Card reader

You also need to build cross-platform development if you develop on non-Arm64 enviroment. 
For Ubuntu or Unix-like user, you can install cross compiler with `apt`

```shell
$ sudo apt install qemu-system
```

Maybe you need to install gdb for debugging usage.

```shell
$ sudo apt install gdb
```

<br>

## Current features

* UART communication
* Interrupt handling
* Memory allocator
* Process scheduler and context switch
* Virtual file system
