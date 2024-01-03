
To test cpio

```shell
qemu-system-aarch64 -M raspi3b -kernel shell.img -initrd initramfs.cpio -serial null -serial stdio -display none
```

To test dtb

```shell
qemu-system-aarch64 -M raspi3b -kernel shell.img -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -serial null -serial stdio -display none
```

To dump dtb to dts file 
```shell
dtc -I dtb -O dts <dtbfile> > <dtsfile>
```
