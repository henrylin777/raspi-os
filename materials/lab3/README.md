# lab3 - Exception and Interrupt



In this lab we are going to explore the interrupt mechanism such as Exception Level(EL), Exception handling, Exception vector table...etc,
and understand the process of exception (from polling an interrupt to returning from exception handler). 
The following stuffs are implemented in this lab: 

* Asynchronous read/write operation on mini UART
* One-shot CPU timer interrupt
* Concurrent interrupt management



**TODO**
* Preemptive mechanism in interrupt task queue 


## Usage

```shell
qemu-system-aarch64 -M raspi3b -kernel shell.img -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -serial null -serial stdio -display none
```


## Testing

```shell
find ./rootfs/. | cpio -o -H newc > initramfs.cpio
```

To test cpio

```shell
qemu-system-aarch64 -M raspi3b -kernel shell.img -initrd initramfs.cpio -serial null -serial stdio -display none
```

To dump dtb to dts file 
```shell
dtc -I dtb -O dts <dtbfile> > <dtsfile>
```
