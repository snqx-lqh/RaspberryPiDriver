/*
 * 这个驱动文件测试，树莓派使用platform_device和platform_driver的方式进行驱动设备注册以及使用
 * 使用树莓派寄存器控制对应的引脚电平
 * 这个文件是device文件，主要存储寄存器相关资源信息  
 */
#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>


#define BCM2837_GPIO_FSEL0     0x3F200000   // GPIO功能选择寄存器0  
#define BCM2837_GPIO_FSEL1     0x3F200004   // GPIO功能选择寄存器1  
#define BCM2837_GPIO_FSEL2     0x3F200008   // GPIO功能选择寄存器2 
#define BCM2837_GPIO_SET0      0x3F20001C   // GPIO置位寄存器0      
#define BCM2837_GPIO_CLR0      0x3F200028   // GPIO清零寄存器0      
#define BCM2837_GPIO_LEV0      0x3F200034   // GPIO清零寄存器0     


/* @description		: 释放flatform设备模块的时候此函数会执行	
 * @param - dev 	: 要释放的设备 
 * @return 			: 无
 */
static void	led_release(struct device *dev)
{
	printk("led device released!\r\n");	
}

/*  
 * 设备资源信息，也就是LED0所使用的所有寄存器
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

static struct platform_device led_device = {
    .name = "rpi3-plus-led",
	.id = -1,
	.dev = {
		.release = &led_release,
	},
	.num_resources = ARRAY_SIZE(led_resources),
	.resource = led_resources,
};
 		
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

MODULE_LICENSE("GPL v2");   // 开源许可证
MODULE_AUTHOR("liqinghua"); // 模块作者