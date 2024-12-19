/*
 * 这个驱动文件测试，树莓派使用misc的方式进行驱动设备注册以及使用
 * misc的方式不需要自己获得设备号，他会自己使用设备号为0的设备
 * 和上一个示例的区别就是，这里注册设备使用了misc，没有使用cdev_init和cdev_add添加设备信息
 * 使用树莓派gpio子系统的gpiod开头的api控制对应的引脚电平  
 */
#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/miscdevice.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>

typedef struct{
    dev_t devid; /* 设备号 */
    struct cdev cdev; /* cdev */
    struct device_node *node; /* LED 设备节点 */
    void *private_data;	/* 私有数据 */
    struct gpio_desc * reset_gpiod;
	struct gpio_desc * dc_gpiod;
	int reset_gpio;
	int dc_gpio;
}oled_dev_t;

static oled_dev_t oled_dev;

static void spi_write_len_data(oled_dev_t *dev, u8 *buf, u8 len)
{
	int ret = -1;
	unsigned char *txdata;
	struct spi_message  m;
	struct spi_transfer *t;
	struct spi_device   *spi = (struct spi_device *)dev->private_data;

	t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);	/* 申请内存 */
 
	txdata = kzalloc(sizeof(char)+len, GFP_KERNEL);
	if(!txdata) {
		goto out1;
	}
	
    memcpy(txdata, buf, len);	/* 把len个寄存器拷贝到txdata里，等待发送 */
	t->tx_buf = txdata;			/* 要发送的数据 */
	t->len = len;				/* t->len=发送的长度+读取的长度 */
	spi_message_init(&m);		/* 初始化spi_message */
	spi_message_add_tail(t, &m);/* 将spi_transfer添加到spi_message队列 */
	ret = spi_sync(spi, &m);	/* 同步发送 */
    if(ret) {
        goto out2;
    }
	
out2:
	kfree(txdata);				/* 释放内存 */
out1:
	kfree(t);					/* 释放内存 */
 
}

// 打开文件
static int oled_drv_open(struct inode *inode, struct file *file)
{
    file->private_data = &oled_dev;
	printk("%s %s line %d\r\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

// 关闭文件
static int oled_drv_release(struct inode *inode, struct file *file)
{
	printk("%s %s line %d\r\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

 
ssize_t oled_drv_read(struct file* filp, char __user* buf, size_t len, loff_t* off)
{

    return 0;
}

 
ssize_t oled_drv_write(struct file* filp, const char __user* buf, size_t len, loff_t* off)
{
    long err = 0;
    oled_dev_t *dev = (oled_dev_t *)filp->private_data;

	uint8_t data[2];
	err = copy_from_user(data, buf, len);

    if (data[0] == 0x40) { // 写数据
		gpio_set_value(dev->dc_gpio,1);
    }
    else { // 写命令
		gpio_set_value(dev->dc_gpio,0);
    }
    spi_write_len_data(dev, &data[1], 1);
 
    return 0;
}

long oled_drv_ioctl(struct file *filp, unsigned int cmd, unsigned long reg)
{
	if(reg == 0)
	{
		gpio_set_value(oled_dev.reset_gpio,0);
	}	
	else if(reg == 1)
	{
		gpio_set_value(oled_dev.reset_gpio,1);
	}
	return 0;
}

const struct file_operations oled_fops = {
    .owner   = THIS_MODULE,
    .open    =  oled_drv_open,
    .release =  oled_drv_release,
    .read    =  oled_drv_read,
    .write   =  oled_drv_write,
	.unlocked_ioctl = oled_drv_ioctl,
};

/* MISC 设备结构体 */
static struct miscdevice oled_miscdev = {
    .minor = 148,        //子设备号
    .name = "oled_drv",   //注册名称
    .fops = &oled_fops,
};

static int ssd1306_probe(struct spi_device *spi)
{	 
	int ret;    
	/*初始化spi_device */
	spi->mode = SPI_MODE_0;	/*MODE0，CPOL=0，CPHA=0*/
	spi_setup(spi);
	oled_dev.private_data = spi; /* 设置私有数据 */

	//读节点信息 
    oled_dev.node = spi->dev.of_node;            

    // 获得gpio信息
	oled_dev.reset_gpio  = of_get_named_gpio(oled_dev.node, "reset-gpios", 0);
    oled_dev.dc_gpio     = of_get_named_gpio(oled_dev.node, "dc-gpios", 0);
    
	//设置gpio为输出模式，同时初始化为低电平
    ret = gpio_direction_output(oled_dev.reset_gpio, 0);
    ret = gpio_direction_output(oled_dev.dc_gpio   , 0);
  
	/* 注册misc字符设备驱动 */
    ret = misc_register(&oled_miscdev);
    if(ret < 0){
        printk("misc device register faikey!\r\n");
        return -EFAULT;
    }
	return 0;
}

static void ssd1306_remove(struct spi_device *spi)
{
	/* 注销misc设备 */
    misc_deregister(&oled_miscdev);
}

/* 传统匹配方式ID列表 */
static const struct spi_device_id ssd1306_id[] = {
	{"solomon,ssd1306", 0},  
	{}
};

/* 设备树匹配列表 */
static const struct of_device_id ssd1306_of_match[] = {
	{ .compatible = "solomon,ssd1306" },
	{ /* Sentinel */ }
};

/* i2c驱动结构体 */	
static struct spi_driver ssd1306_driver = {
	.probe = ssd1306_probe,
	.remove = ssd1306_remove,
	.driver = {
			.owner = THIS_MODULE,
		   	.name = "ssd1306",
		   	.of_match_table = ssd1306_of_match, 
		   },
	.id_table = ssd1306_id,
};    

static int __init ssd1306_init(void)
{
	int ret = 0;
	ret = spi_register_driver(&ssd1306_driver);
	return ret;
}

static void __exit ssd1306_exit(void)
{
	spi_unregister_driver(&ssd1306_driver);
}

module_init(ssd1306_init);
module_exit(ssd1306_exit);

MODULE_LICENSE("GPL v2");   // 开源许可证
MODULE_AUTHOR("liqinghua"); // 模块作者