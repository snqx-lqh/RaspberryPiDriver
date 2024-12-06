## 笔记说明

如我在驱动开发总览中说的那样，一般的驱动开发模式就是有3种。

**传统设备驱动模式**：将所有的资源分配放进一个文件中。

**PlatformDevice/PlatformDriver模式**：资源放进PlatformDevice，实现驱动放进PlatformDriver文件。

**设备树/PlatformDriver模式**：资源放进设备树，实现驱动放进PlatformDriver文件。设备树一种是提前编译内核的时候就编译好，还有一种方式是添加设备树插件。

这一节主要说明**传统**的模式，就是将资源全部放进一个文件中处理。

主要参考的文章：

正点原子《I.MX6U 嵌入式 Linux 驱动开发指南 V1.81》

韦东山《01_嵌入式Linux应用开发完全手册V5.1_IMX6ULL_Pro开发板》

本节源码路径`02_Firmware/02_led_drv_cdev`

## 开发流程

1、初始化一个设备操作函数结构体 `struct file_operations`，并注册内部函数。该结构体就是这个模块文件读写的相关操作

2、编写入口函数，在这个里面注册设备，并将前面定义的`file_operations`结构体传入注册函数。自动分配节点需要创建类后创建设备节点。

3、编写出口函数，将设备注销。删除类和设备节点。

4、在入口函数完善IO引脚初始化，出口函数完善引脚相关处理。

5、在设备操作函数中完善各个操作函数的实现逻辑。

## 需要的头文件

先说明这部分代码需要包含的头文件。

```C
#include <linux/init.h>     //用于定义模块初始化和清理相关的宏以及函数原型等内容
#include <linux/module.h>   //包含了众多用于构建内核模块的基本定义、宏和函数原型
#include <asm/io.h>         //用于处理输入 / 输出操作，尤其是和硬件底层的内存映射 I/O 相关的操作
#include <linux/string.h>   //提供了一系列字符串处理相关的函数
#include <linux/fs.h>       //定义了文件系统操作的各种结构体、函数原型等内容
#include <linux/uaccess.h>  //用于处理用户空间（User Space）和内核空间（Kernel Space）之间的数据拷贝操作
#include <linux/cdev.h>     //定义了字符设备相关的结构体
```

## 设备操作函数结构体

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

## 编写入口函数

入口函数就是我们加载这个模块的时候会调用的函数，这里我们将注册一个字符型设备，并且动态的为其分配设备号，其实还有静态方法，指定设备号分配，可以看正点原子教程，这里不做说明，因为感觉没有动态好用。

```C
static dev_t  led_dev_num = 0;          // 设备编号
static struct cdev   led_cdev;          // 字符设备结构体
static struct class  *class  = NULL;    //类
static struct device *device = NULL;    //设备

static int __init led_drv_cdev_init(void)
{
    // 将该模块注册为一个字符设备，并动态分配设备号
    if (alloc_chrdev_region(&led_dev_num, 0, 1, "led_drv")) {
        printk(KERN_ERR"failed to register kernel module!\n");
        return -1;
    }
	printk(KERN_INFO"led_drv device major & minor is [%d:%d]\n", MAJOR(led_dev_num), MINOR(led_dev_num));
	
    //初始化并添加一个cdev
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

module_init(led_drv_cdev_init);
```

为什么要使用创建类和设备，因为不使用的话就需要自己加载模块后创建，使用以下指令创建设备节点文件:

```BASH
mknod /dev/led_drv c 200 0
```

“c”表示这是个字符设备，“200”是设备的主设备号，“0”是设备的次设备号。主设备号不一定是200，我是动态生成的，需要在内核加载信息中去查看，因为我在代码中动态申请设备号后写了个打印，使用如下指令：

```BASH
demsg | tail
```

但是这样没有创建类和设备方便，所以就使用创建类和设备了。

## 编写出口函数

出口函数就是，我们在注销设备的时候会执行的函数，一般我们需要注销我们前面定义的字符设备。

```C
static void __exit led_drv_cdev_exit(void)
{
    // 释放字符设备
    cdev_del(&led_cdev);
    unregister_chrdev_region(led_dev_num, 1);

    //删除类和设备
    device_destroy(class, led_dev_num);
    class_destroy(class);
}

module_exit(led_drv_cdev_exit);
```

## 出口&入口函数完善引脚初始化

我们把框架搭好后，就可以开始初始化引脚相关的寄存器了，其实操作寄存器的方式和单片机感觉很像，但是内核不能直接操作寄存器物理地址，我们需要先把寄存器相关地址，映射到虚拟地址，然后进行操作。

首先就是虚拟地址的映射，具体操作相关如下，映射后我们才能使用相关的寄存器。

```C
// 寄存器地址
#define BCM2837_GPIO_FSEL0_BASE     0x3F200000   // GPIO功能选择寄存器0  
#define BCM2837_GPIO_FSEL1_BASE     0x3F200004   // GPIO功能选择寄存器1  
#define BCM2837_GPIO_FSEL2_BASE     0x3F200008   // GPIO功能选择寄存器2
#define BCM2837_GPIO_SET0_BASE      0x3F20001C   // GPIO置位寄存器0      
#define BCM2837_GPIO_CLR0_BASE      0x3F200028   // GPIO清零寄存器0      
#define BCM2837_GPIO_LEV0_BASE      0x3F200034   // GPIO清零寄存器0  

// 寄存器对应的虚拟ioremap后的地址，会在后面初始化
static void __iomem *BCM2837_GPIO_FSEL0 = NULL;
static void __iomem *BCM2837_GPIO_FSEL1 = NULL;
static void __iomem *BCM2837_GPIO_FSEL2 = NULL;
static void __iomem *BCM2837_GPIO_SET0  = NULL;
static void __iomem *BCM2837_GPIO_CLR0  = NULL;
static void __iomem *BCM2837_GPIO_LEV0  = NULL;

static int __init led_drv_cdev_init(void)
{
    // 将树莓派引脚控制相关的寄存器进行虚拟地址映射，使得可以控制对应寄存器
    BCM2837_GPIO_FSEL0 = ioremap(BCM2837_GPIO_FSEL0_BASE, 0x04);
    BCM2837_GPIO_FSEL1 = ioremap(BCM2837_GPIO_FSEL1_BASE, 0x04);
    BCM2837_GPIO_FSEL2 = ioremap(BCM2837_GPIO_FSEL2_BASE, 0x04);
    BCM2837_GPIO_SET0  = ioremap(BCM2837_GPIO_SET0_BASE , 0x04);
    BCM2837_GPIO_CLR0  = ioremap(BCM2837_GPIO_CLR0_BASE , 0x04);
    BCM2837_GPIO_LEV0  = ioremap(BCM2837_GPIO_LEV0_BASE , 0x04);

	// 注册字符设备、创建类和设备
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

然后在初始化的时候我们就可以初始化我们的引脚

```C
static int __init led_drv_cdev_init(void)
{
    // 寄存器进行虚拟地址映射
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
```

在出口函数完善相关引脚处理，主要是释放内存映射。

```C
static void __exit led_drv_cdev_exit(void)
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
}
```

## 设备操作函数完善逻辑

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
obj-m = led_drv_cdev.o

KDIR = /home/linux-rpi-6.6.y/
CROSS = ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
CROSS_COMPILE = arm-linux-gnueabihf-

all:
    $(MAKE) -C $(KDIR) M=$(PWD) $(CROSS)  modules
    $(CROSS_COMPILE)gcc -o led_drv_cdev_app led_drv_cdev_app.c

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

sendfile="led_drv_cdev.ko led_drv_cdev_app"
pi_user=pi
pi_ip=192.168.2.149
pi_dir=/home/pi/RpiDriver/02_led_drv_cdev

scp ${sendfile} ${pi_user}@${pi_ip}:${pi_dir}
```

在树莓派上，我们需要加载并使用这个模块。

```BASH
sudo insmod led_drv_cdev.ko

sudo ./led_drv_cdev_app -w 0 1
sudo ./led_drv_cdev_app -r

sudo rmmod led_drv_cdev
```

可以使用dmesg查看运行过程中的变化

```BASH
dmesg | tail
```