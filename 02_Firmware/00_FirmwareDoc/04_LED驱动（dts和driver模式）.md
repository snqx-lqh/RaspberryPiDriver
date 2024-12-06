## 笔记说明

现在很多的驱动，编写方式都是使用设备树来描述设备信息，然后使用driver文件做驱动。因为如果许多设备都需要写device文件的话，那这个内核编译出来就比较大，所以我们使用设备树的方式。

正点原子和韦东山教的都是直接重编译全部的设备树，然后执行，但是有个动态设备树的概念，我们可以编写动态设备树，然后动态加载这个设备树。就不用修改原设备树了。

主要参考的文章

正点原子《I.MX6U 嵌入式 Linux 驱动开发指南 V1.81》

韦东山《01_嵌入式Linux应用开发完全手册V5.1_IMX6ULL_Pro开发板》

[Philon/rpi-drivers](https://github.com/Philon/rpi-drivers)

本节源码路径`02_Firmware/04_led_drv_platform_dts_driver`

## 开发流程

1、编写设备树文件，编译成动态加载的格式。

2、创建一个driver文件，初始化一个设备操作函数结构体 `struct file_operations`，并注册内部函数。该结构体就是这个模块文件读写的相关操作

3、初始化一个driver函数结构体`struct platform_driver`，并注册内部函数。该结构体主要加载和卸载以及匹配驱动需要的东西。

4、编写入口函数，在这个里面注册`platform_driver`。

5、编写出口函数，在这个里面注销`platform_driver`。

6、在`platform_driver`内部函数中完善设备的加载和卸载。

7、在设备操作函数中完善各个操作函数的实现逻辑。

## 设备树编写

设备树，其实动态设备树语法和普通的设备树语法一样，不过加了点东西。如下，会发现多了个`fragment@0`和`__overlay__`还有`target-path = "/";`，fragment是有一个块就要加一个@，target-path = "/";就是追加到哪个设备树节点下，`__overlay__`来修饰追加的内容。

具体的设备树语法可以看正点和韦东山的教程。

```DTS
/dts-v1/;
/plugin/;

/ {
  fragment@0 {
    target-path = "/";
    __overlay__ {
      shbled:shbled@0x3f200000 {
        #address-cells = <1>;
        #size-cells = <1>;
        compatible = "shb-led";
        status = "okay";

        reg = <
            0x3F200000 0X04    /* BCM2837_GPIO_FSEL0 */
            0x3F200004 0X04    /* BCM2837_GPIO_FSEL1 */
            0x3F200008 0X04    /* BCM2837_GPIO_FSEL2 */
            0x3F20001C 0X04    /* BCM2837_GPIO_SET0  */
            0x3F200028 0X04    /* BCM2837_GPIO_CLR0  */
            0x3F200034 0X04 >; /* BCM2837_GPIO_LEV0  */
      };
    };
  };
};
```

然后编译设备树，就使用内核中自带的工具

```BASH
/home/linux-rpi-6.6.y/scripts/dtc/dtc -I dts -o smartHomeBoard.dtbo smartHomeBoard.dts
```

## driver编写

大部分和上节内容一样，说一些不同的点。

### driver匹配

上一节是匹配的device中的内容，这一节需要匹配设备树中的内容。就多了一个匹配列表，匹配表中选择和设备树节点上`compatible`名字一样。还有就是驱动结构体中的`name`需要和设备树中的节点名称一致。

```C
/* 匹配列表 */
static const struct of_device_id led_of_match[] = {
    { .compatible = "shb-led" },
    { /* Sentinel */ }
};

/* platform驱动结构体 */
static struct platform_driver led_driver = {
    .driver     = {
        .name   = "shbled",         /* 驱动名字，用于和设备匹配 */
        .of_match_table = led_of_match
    },
    .probe      = led_probe,
    .remove     = led_remove,
};
```

### 寄存器地址获取

还有一个不同的点，就是寄存器获取的方式变了，通过获得设备树节点，然后读取节点的资源信息，来读取寄存器值。

```C
static int led_probe(struct platform_device *dev)
{  
    uint32_t regdata[14];
    int ret;

    //读节点信息
    led_dev.node = dev->dev.of_node;                  

    //第一种 读REG然后寄存器地址映射
    ret = of_property_read_u32_array(led_dev.node, "reg", regdata, 12);
    if(ret < 0) {
        printk("reg property read failed!\r\n");
    }

    BCM2837_GPIO_FSEL0   = ioremap(regdata[0],  regdata[1]);
    BCM2837_GPIO_FSEL1   = ioremap(regdata[2],  regdata[3]);
    BCM2837_GPIO_FSEL2   = ioremap(regdata[4],  regdata[5]);
    BCM2837_GPIO_SET0    = ioremap(regdata[6],  regdata[7]);
    BCM2837_GPIO_CLR0    = ioremap(regdata[8],  regdata[9]);
    BCM2837_GPIO_LEV0    = ioremap(regdata[10], regdata[11]);

    // 第二种 使用节点信息，寄存器地址映射
    //BCM2837_GPIO_FSEL0 = of_iomap(led_dev.node,  0);
    //BCM2837_GPIO_FSEL1 = of_iomap(led_dev.node,  1);
    //BCM2837_GPIO_FSEL2 = of_iomap(led_dev.node,  2);
    //BCM2837_GPIO_SET0  = of_iomap(led_dev.node,  3);
    //BCM2837_GPIO_CLR0  = of_iomap(led_dev.node,  4);
    //BCM2837_GPIO_LEV0  = of_iomap(led_dev.node,  5);

	// 初始化LED引脚
	// ************** //

	// 注册字符设备、创建类和设备
	// ************** //
	
    return 0;
}
```

其他就和之前的一样了

## 编译加载

makefile主要参考正点原子和韦东山的写法。这个Makefile会把驱动模块和应用程序一起编译。

```Makefile
# 模块驱动，必须以obj-m=xxx形式编写
obj-m = led_drv_driver.o

KDIR = /home/linux-rpi-6.6.y/
CROSS = ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
CROSS_COMPILE = arm-linux-gnueabihf-

all:
    $(MAKE) -C $(KDIR) M=$(PWD) $(CROSS)  modules
    $(CROSS_COMPILE)gcc -o led_drv_cdev_app led_drv_cdev_app.c

.PHONY: clean
clean:
    $(MAKE) -C $(KDIR) M=`pwd` $(CROSS) clean

dts:
    /home/linux-rpi-6.6.y/scripts/dtc/dtc -I dts -o smartHomeBoard.dtbo smartHomeBoard.dts
```

makefile编译完成后，直接make就行，还可以使用make dts编译设备树。

```BASH
make
make dts
```

将make生成的文件，如我上文写了脚本，通过scp传到树莓派上。具体的内容参数，自己修改。

```BASH
#!/bin/sh

sendfile="smartHomeBoard.dtbo led_drv_driver.ko led_drv_app"
pi_user=pi
pi_ip=192.168.2.149
pi_dir=/home/pi/RpiDriver/04_led_drv_platform_dts_driver

scp ${sendfile} ${pi_user}@${pi_ip}:${pi_dir}
```

在树莓派上，我们需要加载并使用这个模块，还需要加载设备树。

```BASH
sudo dtoverlay smartHomeBoard.dtbo
sudo insmod led_drv_driver.ko

sudo ./led_drv_app -w 0 1
sudo ./led_drv_app -r

sudo rmmod led_drv_driver
```

可以使用dmesg查看运行过程中的变化

```BASH
dmesg | tail
```