obj-m = led_drv_cdev.o

KDIR = /home/linux-rpi-6.6.y/
CROSS = ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
CROSS_COMPILE = arm-linux-gnueabihf-

all:
	$(MAKE) -C $(KDIR) M=$(PWD) $(CROSS)  modules 
	$(CROSS_COMPILE)gcc -o led_drv_cdev_app led_drv_cdev_app.c

.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=`pwd` $(CROSS) clean



