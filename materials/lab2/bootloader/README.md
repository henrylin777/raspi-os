

To test the bootloader, you can use `make test` or type the following command
```shell
$ qemu-system-aarch64 -M raspi3b -serial null -serial stdio -display none  -kernel bootloader.img
```
