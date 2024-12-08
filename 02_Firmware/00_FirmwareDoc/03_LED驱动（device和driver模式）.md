## 笔记说明

这种模式就是，资源放进device文件，驱动放进driver文件，实现一个资源和驱动分离的模式，这样就可以每次只修改device里面的东西。

然后注册的时候需要注册两个ko文件，然后自动匹配，匹配怎么实现的可以看韦东山视频。

主要参考的文章

正点原子《I.MX6U 嵌入式 Linux 驱动开发指南 V1.81》

韦东山《01_嵌入式Linux应用开发完全手册V5.1_IMX6ULL_Pro开发板》

本节源码路径`02_Firmware/03_led_drv_platform_device_driver`

## 开发流程

1、创建一个device的文件，初始化一个device函数结构体`struct platform_device`，并注册内部函数。该结构体主要就是保存资源相关的东西。

2、编写device的注册和注销相关函数。

3、完善device的资源相关内容。

4、创建一个driver文件，初始化一个设备操作函数结构体 `struct file_operations`，并注册内部函数。该结构体就是这个模块文件读写的相关操作

5、初始化一个driver函数结构体`struct platform_driver`，并注册内部函数。该结构体主要加载和卸载以及匹配驱动需要的东西。

5、编写入口函数，在这个里面注册`platform_driver`。

6、编写出口函数，在这个里面注销`platform_driver`。

7、在`platform_driver`内部函数中完善设备的加载和卸载。

8、在设备操作函数中完善各个操作函数的实现逻辑。

## device文件处理

### 头文件包含

```C
#include <linux/init.h>     //用于定义模块初始化和清理相关的宏以及函数原型等内容
#include <linux/module.h>   //包含了众多用于构建内核模块的基本定义、宏和函数原型
#include <asm/io.h>         //用于处理输入 / 输出操作，尤其是和硬件底层的内存映射 I/O 相关的操作
#include <linux/string.h>   //提供了一系列字符串处理相关的函数
#include <linux/fs.h>       //定义了文件系统操作的各种结构体、函数原型等内容
#include <linux/uaccess.h>  //用于处理用户空间（User Space）和内核空间（Kernel Space）之间的数据拷贝操作
#include <linux/cdev.h>     //定义了字符设备相关的结构体
#include <linux/device.h>   //定义了设备模型相关的重要结构体
#include <linux/platform_device.h> //定义了描述平台设备相关的变量
```

### device函数结构体

需要一个结构体来保存设备的资源描述。包含`name`，这是进行匹配需要的，只有名字相同，才能匹配上，还有一些资源大小的变量复制。

```C
/* @description     : 释放flatform设备模块的时候此函数会执行  
 * @param - dev     : 要释放的设备
 * @return          : 无
 */
static void led_release(struct device *dev)
{
    printk("led device released!\r\n");
}

/*  
 * 设备资源信息，也就是LED0所使用的所有寄存器
 */
static struct resource led_resources[] = {
    
};

static struct platform_device led_device = {
    .name = "rpi3-plus-led",
    .id = -1,
    .dev = {
        .release = &led_release,
    },
    .num_resources = ARRAY_SIZE(led_resources),
    .resource = led_resources,
};
```

### device的注册和注销函数

使用以下方法就能将这个device的资源信息，注册到内核中去。

```C
static int __init led_device_init(void)
{
    return platform_device_register(&led_device);
}

static void __exit led_device_exit(void)
{
    platform_device_unregister(&led_device);
}

module_init(led_device_init);
module_exit(led_device_exit);
```

### 完善资源相关内容

```c
/*  
 * 设备资源信息，也就是LED所使用的所有寄存器
 */
static struct resource led_resources[] = {
	[0] = {
		.start 	= BCM2837_GPIO_FSEL0,
		.end 	= (BCM2837_GPIO_FSEL0 + 3),
		.flags 	= IORESOURCE_MEM,
	},	
	[1] = {
		.start	= BCM2837_GPIO_FSEL1,
		.end	= (BCM2837_GPIO_FSEL1 + 3),
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.start	= BCM2837_GPIO_FSEL2,
		.end	= (BCM2837_GPIO_FSEL2 + 3),
		.flags	= IORESOURCE_MEM,
	},
	[3] = {
		.start	= BCM2837_GPIO_SET0,
		.end	= (BCM2837_GPIO_SET0 + 3),
		.flags	= IORESOURCE_MEM,
	},
	[4] = {
		.start	= BCM2837_GPIO_CLR0,
		.end	= (BCM2837_GPIO_CLR0 + 3),
		.flags	= IORESOURCE_MEM,
	},
    [5] = {
		.start	= BCM2837_GPIO_LEV0,
		.end	= (BCM2837_GPIO_LEV0 + 3),
		.flags	= IORESOURCE_MEM,
	},
};
```

## driver文件处理

### 头文件包含

```C
#include <linux/init.h>     //用于定义模块初始化和清理相关的宏以及函数原型等内容
#include <linux/module.h>   //包含了众多用于构建内核模块的基本定义、宏和函数原型
#include <asm/io.h>         //用于处理输入 / 输出操作，尤其是和硬件底层的内存映射 I/O 相关的操作
#include <linux/string.h>   //提供了一系列字符串处理相关的函数
#include <linux/fs.h>       //定义了文件系统操作的各种结构体、函数原型等内容
#include <linux/uaccess.h>  //用于处理用户空间（User Space）和内核空间（Kernel Space）之间的数据拷贝操作
#include <linux/cdev.h>     //定义了字符设备相关的结构体
#include <linux/device.h>   //定义了设备模型相关的重要结构体
#include <linux/platform_device.h> //定义了描述平台设备相关的变量
```

### 设备操作函数结构体

我们需要定义一个`struct file_operations`结构体，以供我们后面操作这个模块，并且需要注册这个结构体中的操作函数。

```C
// 通过文件读取，得到当前LED的状态
ssize_t led_drv_read(struct file* filp, char __user* buf, size_t len, loff_t* off)
{
    return  0;
}

// 通过向文件写入LED状态，控制LED灯
ssize_t led_drv_write(struct file* filp, const char __user* buf, size_t len, loff_t* off)
{
    return 0;
}

const struct file_operations led_fops = {
    .owner   = THIS_MODULE,
    .read    =  led_drv_read,
    .write   =  led_drv_write,
};
```

### driver结构体

这个结构体用来和设备进行匹配，如果加载模块的时候能匹配到对应的设备，就会自动执行注册好的probe函数。

```C
static int led_probe(struct platform_device *dev)
{  
    return 0;
}

static int led_remove(struct platform_device *dev)
{
    return 0;
}

/* platform驱动结构体 */
static struct platform_driver led_driver = {
    .driver     = {
        .name   = "rpi3-plus-led",          /* 驱动名字，用于和设备匹配 */
    },
    .probe      = led_probe,
    .remove     = led_remove,
};
```

### driver模块的注册和注销函数

没啥说的，就是个注册和注销，就是注意传入的参数是定义的`struct platform_driver`类型的变量。

```C
static int __init leddriver_init(void)
{
    return platform_driver_register(&led_driver);
}

static void __exit leddriver_exit(void)
{
    platform_driver_unregister(&led_driver);
}

module_init(leddriver_init);
module_exit(leddriver_exit);
```

### 完善driver的probe和remove函数

其实这一部分对于设备的处理就和传统的注册和注销很像了，不同的地方可能就是获取寄存器的方式不同了。

还是先进行模块的注册和注销。driver在匹配上device的时候，就会执行led_probe，至于怎么匹配的，可以看韦东山的视频讲解。

```C
static dev_t led_dev_num = 0;   // 设备编号
static struct cdev led_cdev;  // 字符设备结构体
static struct class  *class  = NULL;    //类
static struct device *device = NULL;    //设备

static int led_probe(struct platform_device *dev)
{  

    printk("led driver and device has matched!\r\n");

    /* 注册字符设备驱动 */
    // 将该模块注册为一个字符设备，并动态分配设备号
    if (alloc_chrdev_region(&led_dev_num, 0, 1, "led_drv")) {
        printk(KERN_ERR"failed to register kernel module!\n");
        return -1;
    }
    cdev_init(&led_cdev, &led_fops);
    cdev_add(&led_cdev, led_dev_num, 1);

    //创建类
    class = class_create("led_drv");
    if (IS_ERR(class)) {
        return PTR_ERR(class);
    }
    //创建设备
    device = device_create(class, NULL, led_dev_num, NULL, "led_drv");
    if (IS_ERR(device)) {
        return PTR_ERR(device);
    }
    return 0;
}

static int led_remove(struct platform_device *dev)
{
    cdev_del(&led_cdev);
    unregister_chrdev_region(led_dev_num, 1);

    //删除类和设备
    device_destroy(class, led_dev_num);
    class_destroy(class);
    return 0;
}
```

然后就是虚拟地址的映射和解除映射，这里的区别就是这个寄存器变量值，是从device文件中获取的

```C
// 寄存器对应的虚拟ioremap后的地址，会在后面初始化
static void __iomem *BCM2837_GPIO_FSEL0 = NULL;
static void __iomem *BCM2837_GPIO_FSEL1 = NULL;
static void __iomem *BCM2837_GPIO_FSEL2 = NULL;
static void __iomem *BCM2837_GPIO_SET0  = NULL;
static void __iomem *BCM2837_GPIO_CLR0  = NULL;
static void __iomem *BCM2837_GPIO_LEV0  = NULL;

static int led_probe(struct platform_device *dev)
{  
	int i = 0;
    int ressize[6];
    struct resource *ledsource[6];
    printk("led driver and device has matched!\r\n");

    /* 1、获取资源 */
    for (i = 0; i < 6; i++) {
        ledsource[i] = platform_get_resource(dev, IORESOURCE_MEM, i); /* 依次MEM类型资源 */
        if (!ledsource[i]) {
            dev_err(&dev->dev, "No MEM resource for always on\n");
            return -ENXIO;
        }
        ressize[i] = resource_size(ledsource[i]);  
    }  
    
    /* 寄存器地址映射 */
    BCM2837_GPIO_FSEL0 = ioremap(ledsource[0]->start, ressize[0]);
    BCM2837_GPIO_FSEL1 = ioremap(ledsource[1]->start, ressize[1]);
    BCM2837_GPIO_FSEL2 = ioremap(ledsource[2]->start, ressize[2]);
    BCM2837_GPIO_SET0  = ioremap(ledsource[3]->start, ressize[3]);
    BCM2837_GPIO_CLR0  = ioremap(ledsource[4]->start, ressize[4]);
    BCM2837_GPIO_LEV0  = ioremap(ledsource[5]->start, ressize[5]);
	
    printk("led driver and device has matched!\r\n");

    // 注册字符设备、创建类和设备
	// ************** //
    return 0;
}

static int led_remove(struct platform_device *dev)
{
	// 取消gpio物理内存映射
    iounmap(BCM2837_GPIO_FSEL0);
    iounmap(BCM2837_GPIO_FSEL1);
    iounmap(BCM2837_GPIO_FSEL2);
    iounmap(BCM2837_GPIO_SET0);
    iounmap(BCM2837_GPIO_CLR0);
    iounmap(BCM2837_GPIO_LEV0);
    // 释放字符设备、删除类和设备
	// ************** //
    return 0;
}
```

我们可以使用映射后的寄存器写一些对寄存器的读写操作来控制引脚电平的读写。

```C
// 想要控制的LED灯，BCM引脚定义，这是设计的扩展板上的定义
#define LED2 17
#define LED3 27
#define LED4 22
#define LED5 23

#define LED_OUTPUT 1
#define LED_INPUT  0

/**
 * @brief 设置对应引脚的高低电平
 * @param pin    需要设置的引脚
 * @param level  1是高电平 0是低电平
 */
void gpio_set_level(int pin, int level)
{
    // 通过想要的电平判断现在想要控制的寄存器
    void* reg = (level ? BCM2837_GPIO_SET0 : BCM2837_GPIO_CLR0);
    iowrite32(1 << pin, reg);
}

/**
 * @brief 获得引脚的电平
 * @param pin 需要获得的引脚
 * @return 1是高电平 0是低电平
 */
static int gpio_get_level(int pin)
{
    int ret = 0;
    int pin_level = 0;
    // 读取引脚电平寄存器
    pin_level = ioread32( BCM2837_GPIO_LEV0 );
    // 将想要读取的引脚的状态，提取出来
    ret = (1 << pin );
    ret = ret & pin_level;
    if(ret){
        return 1;
    }else{
        return 0;
    }
}

/**
 * @brief 设置GPIO的寄存器，配置GPIO是输入还是输出
 * @param pin   需要设置的引脚
 * @param mode  1是输出模式 0是输入模式
 */
static void gpio_set_mode(int pin, int mode)
{
    void *reg = NULL;
    int   val = 0;
    int   pin_ctl = 0;
    // 通过pin号来确定要控制的寄存器
    if(pin < 10){
        reg = BCM2837_GPIO_FSEL0;
    }else if(pin < 20){
        reg = BCM2837_GPIO_FSEL1;
    }else if(pin < 30){
        reg = BCM2837_GPIO_FSEL2;
    }
    // 比如我要控制11号脚，就是要控制BCM2837_GPIO_FSEL1_BASE的1号位置的3个位。
    pin_ctl = pin % 10;
    // 将对应的gpio的功能选择位全部写0
    val =  ~(7   << (pin_ctl * 3));
    val &= ioread32(reg);
    // 控制设置对应的gpio的功能选择位写 000 还是 001
    val |= (mode << (pin_ctl * 3));
    iowrite32(val, reg);
}
```

然后在初始化的时候我们就可以初始化我们的引脚，在释放的时候完善相关引脚处理，主要是释放内存映射。

```C
static int led_probe(struct platform_device *dev)
{
    // 将树莓派引脚控制相关的寄存器进行虚拟地址映射，使得可以控制对应寄存器
	// ************** //

	// 控制引脚输入输出状态
    gpio_set_mode(LED2, LED_OUTPUT);
    gpio_set_mode(LED3, LED_OUTPUT);
    gpio_set_mode(LED4, LED_OUTPUT);
    gpio_set_mode(LED5, LED_OUTPUT);

    // 设置引脚电平
    gpio_set_level(LED2, 0);
    gpio_set_level(LED3, 0);
    gpio_set_level(LED4, 0);
    gpio_set_level(LED5, 0);

	// 注册字符设备、创建类和设备
	// ************** //
    return 0;
}

static int led_remove(struct platform_device *dev)
{
    // 设置电平为高
    gpio_set_level(LED2, 1);
    gpio_set_level(LED3, 1);
    gpio_set_level(LED4, 1);
    gpio_set_level(LED5, 1);

    // 取消gpio物理内存映射
    iounmap(BCM2837_GPIO_FSEL0);
    iounmap(BCM2837_GPIO_FSEL1);
    iounmap(BCM2837_GPIO_FSEL2);
    iounmap(BCM2837_GPIO_SET0);
    iounmap(BCM2837_GPIO_CLR0);
    iounmap(BCM2837_GPIO_LEV0);

    // 释放字符设备、删除类和设备
	// ************** //
	return 0;
}
```

### 完善设备操作函数

这一部分就和上一节一模一样了，就是怎么使用寄存器控制gpio口的读写，具体直接看源码，或者看上一节。

我们之前不是写了write和read的操作函数吗，现在就需要完善这些操作函数，使得用户文件在调用这个文件的时候可以操作gpio进行控制。

先解释读函数，这里比较重要的操作就是`copy_to_user`，他会把第二个参数里面的值复制到第一个参数中去，这里的`buf`就是我们用户层想读的值，`len`是用户写的长度。

```C
#define MIN(a, b) (a < b ? a : b)

// 通过文件读取，得到当前LED的状态
ssize_t led_drv_read(struct file* filp, char __user* buf, size_t len, loff_t* off)
{
    int ret = 0;
    int char_len = 0;
    char led_state[4];

    printk("%s %s line %d\r\n", __FILE__, __FUNCTION__, __LINE__);

    led_state[0] = gpio_get_level(LED2);
    led_state[1] = gpio_get_level(LED3);
    led_state[2] = gpio_get_level(LED4);
    led_state[3] = gpio_get_level(LED5);

    char_len = sizeof(led_state);
    int real_len = MIN(len,char_len);
    ret = copy_to_user(buf, led_state, real_len);
    return ret < 0 ? ret : real_len;
}
```

然后是写函数，其实就和读差不多了，主要是`copy_from_user`，能够把用户传进的值进行复制处理。

```C
// 通过向文件写入LED状态，控制LED灯
ssize_t led_drv_write(struct file* filp, const char __user* buf, size_t len, loff_t* off)
{
    int ret = 0;
    char led_state[4];
    int char_len = 0;

    printk("%s %s line %d\r\n", __FILE__, __FUNCTION__, __LINE__);

    char_len = sizeof(led_state);
    int real_len = MIN(len,char_len);
    ret = copy_from_user(led_state, buf, real_len);

    switch (led_state[0])
    {
        case 0:
            gpio_set_level(LED2, led_state[1]);
            break;  
        case 1:
            gpio_set_level(LED3, led_state[1]);
            break;
        case 2:
            gpio_set_level(LED4, led_state[1]);
            break;
        case 3:
            gpio_set_level(LED5, led_state[1]);
            break;
        default:
            break;
    }
    return 0;
}
```

到这里，这个驱动文件就编写完成了，后面就是编译加载了。
## 编写应用层函数

驱动编写完成后，我们需要有一个应用层函数来调用这个驱动，写出以下示例。主要功能就是打开驱动，写和读驱动的内容。

```C
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

/*
 * sudo ./led_drv_cdev_app -w 0 0
 * sudo ./led_drv_cdev_app -r
 */

int main(int argc, char **argv)
{
    int fd;
    char writeBuff[3];
    char readBuff[5];
    int len;

    if (argc < 2)
    {
        printf("Usage: %s -w <string>\n", argv[0]);
        printf(" %s -r\n", argv[0]);
        return -1;
    }

    fd = open("/dev/led_drv", O_RDWR);
    if (fd == -1)
    {
        printf("can not open file /dev/led_drv\n");
        return -1;
    }

    if ((0 == strcmp(argv[1], "-w")) && (argc == 4))
    {
        writeBuff[0] = (char)atoi(argv[2]);
        writeBuff[1] = (char)atoi(argv[3]);
        writeBuff[2] = '\0';
        printf("write : %d, %d\n", writeBuff[0], writeBuff[1]);
        write(fd, writeBuff, 3);
    }else if((0 == strcmp(argv[1], "-r")) && (argc == 2))
    {
        len = read(fd, readBuff, 5);
        readBuff[4] = '\0';
        printf("pin read : 0x%x, 0x%x, 0x%x, 0x%x\n", readBuff[0], readBuff[1], readBuff[2], readBuff[3]);
    }
    else{
        printf("APP Failed\n");
    }
    
    close(fd);
    return 0;
}
```

## 编译加载

makefile主要参考正点原子和韦东山的写法。这个Makefile会把驱动模块和应用程序一起编译。

```Makefile
# 模块驱动，必须以obj-m=xxx形式编写
obj-m = led_drv_device.o led_drv_driver.o

KDIR = /home/linux-rpi-6.6.y/
CROSS = ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
CROSS_COMPILE = arm-linux-gnueabihf-

all:
    $(MAKE) -C $(KDIR) M=$(PWD) $(CROSS)  modules
    $(CROSS_COMPILE)gcc -o led_drv_app led_drv_app.c

.PHONY: clean
clean:
    $(MAKE) -C $(KDIR) M=`pwd` $(CROSS) clean
```

makefile编译完成后，直接make就行

```BASH
make
```

将make生成的文件，如我上文写了脚本，通过scp传到树莓派上。具体的内容参数，自己修改。

```BASH
#!/bin/sh

sendfile="led_drv_device.ko led_drv_driver.ko led_drv_app"
pi_user=pi
pi_ip=192.168.2.149
pi_dir=/home/pi/RpiDriver/03_led_drv_platform_device_driver

scp ${sendfile} ${pi_user}@${pi_ip}:${pi_dir}
```

在树莓派上，我们需要加载并使用这个模块。

```BASH
sudo insmod led_drv_device.ko 
sudo insmod led_drv_driver.ko

sudo ./led_drv_app -w 0 1
sudo ./led_drv_app -r

sudo rmmod led_drv_device
sudo rmmod led_drv_driver
```

可以使用dmesg查看运行过程中的变化

```BASH
dmesg | tail
```