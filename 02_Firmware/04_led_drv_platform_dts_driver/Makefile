# 模块驱动，必须以obj-m=xxx形式编写
obj-m = led_drv_driver.o

KDIR = /home/linux-rpi-6.6.y/
CROSS = ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
CROSS_COMPILE = arm-linux-gnueabihf-

all:
	$(MAKE) -C $(KDIR) M=$(PWD) $(CROSS) modules
	$(CROSS_COMPILE)gcc -o led_drv_app led_drv_app.c
	
clean:
	$(MAKE) -C $(KDIR) M=`pwd` $(CROSS) clean

dts:
	/home/linux-rpi-6.6.y/scripts/dtc/dtc -I dts -o smartHomeBoard.dtbo smartHomeBoard.dts