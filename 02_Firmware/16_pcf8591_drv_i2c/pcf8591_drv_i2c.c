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

typedef struct{
    dev_t devid; /* 设备号 */
    struct cdev cdev; /* cdev */
    struct device_node *node; /* LED 设备节点 */
    struct i2c_client *client;
    void *private_data;	/* 私有数据 */
    uint8_t adc_value[3];
    uint8_t dac_value;
}pcf8591_dev_t;

static pcf8591_dev_t pcf8591_dev;

static int pcf8591_read_regs(pcf8591_dev_t *dev, u8 reg, void *val, int len)
{
	int ret;
	struct i2c_msg msg[2];
	struct i2c_client *client = (struct i2c_client *)dev->client;

	/* msg[0]为发送要读取的首地址 */
	msg[0].addr = client->addr;			/* pcf8591地址 */
	msg[0].flags = 0;					/* 标记为发送数据 */
	msg[0].buf = &reg;					/* 读取的首地址 */
	msg[0].len = 1;						/* reg长度*/

	/* msg[1]读取数据 */
	msg[1].addr = client->addr;			/* pcf8591地址 */
	msg[1].flags = I2C_M_RD;			/* 标记为读取数据*/
	msg[1].buf = val;					/* 读取数据缓冲区 */
	msg[1].len = len;					/* 要读取的数据长度*/

	ret = i2c_transfer(client->adapter, msg, 2);
	if(ret == 2) {
		ret = 0;
	} else {
		printk("i2c rd failed=%d reg=%06x len=%d\n",ret, reg, len);
		ret = -EREMOTEIO;
	}
	return ret;
}

static s32 pcf8591_write_regs(pcf8591_dev_t *dev, u8 reg, u8 *buf, u8 len)
{
	u8 b[10];
	struct i2c_msg msg;
	struct i2c_client *client = (struct i2c_client *)dev->client;
	
	b[0] = reg;					/* 寄存器首地址 */
	memcpy(&b[1],buf,len);		/* 将要写入的数据拷贝到数组b里面 */
		
	msg.addr = client->addr;	/* pcf8591地址 */
	msg.flags = 0;				/* 标记为写数据 */

	msg.buf = b;				/* 要写入的数据缓冲区 */
	msg.len = len + 1;			/* 要写入的数据长度   */

	return i2c_transfer(client->adapter, &msg, 1);
}

void pcf8591_read_data(pcf8591_dev_t *dev)
{
    uint8_t buf[3];

    pcf8591_read_regs(dev,0x40|0,&buf[0],1);
	pcf8591_read_regs(dev,0x40|1,&buf[1],1);
	pcf8591_read_regs(dev,0x40|2,&buf[2],1);
   
    dev->adc_value[0] = buf[0];
    dev->adc_value[1] = buf[1];
    dev->adc_value[2] = buf[2];
}

void pcf8591_write_data(pcf8591_dev_t *dev,uint8_t dac_value)
{
    uint8_t buf[1];

    buf[0] = dac_value;
    pcf8591_write_regs(dev,0x40,buf,1);
}

static int pcf8591_drv_open(struct inode *inode, struct file *file)
{
    file->private_data = &pcf8591_dev;
	printk("%s %s line %d\r\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

static int pcf8591_drv_release(struct inode *inode, struct file *file)
{
	printk("%s %s line %d\r\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

ssize_t pcf8591_drv_read(struct file* filp, char __user* buf, size_t len, loff_t* off)
{
    uint8_t data[4];
	long err = 0;

	pcf8591_dev_t *dev = (pcf8591_dev_t *)filp->private_data;
	
	pcf8591_read_data(dev);

	data[0] = dev->adc_value[0];
	data[1] = dev->adc_value[1];
	data[2] = dev->adc_value[2];
	err = copy_to_user(buf, data, sizeof(data));
    return 0;
}

ssize_t pcf8591_drv_write(struct file* filp, const char __user* buf, size_t len, loff_t* off)
{
    long err = 0;
    pcf8591_dev_t *dev = (pcf8591_dev_t *)filp->private_data;

    uint8_t data[4];

 
    err = copy_from_user(data, buf, len);
 
    pcf8591_write_data(dev , data[0]);
    return 0;
}


const struct file_operations pcf8591_fops = {
    .owner   = THIS_MODULE,
    .open    =  pcf8591_drv_open,
    .release =  pcf8591_drv_release,
    .read    =  pcf8591_drv_read,
    .write   =  pcf8591_drv_write,
};

/* MISC 设备结构体 */
static struct miscdevice pcf8591_miscdev = {
    .minor = 147,        //子设备号
    .name = "pcf8591_drv",   //注册名称
    .fops = &pcf8591_fops,
};

static int pcf8591_probe(struct i2c_client *client)
{	  
	int ret;   
	pcf8591_dev.client = client;

	/* 注册misc字符设备驱动 */
    ret = misc_register(&pcf8591_miscdev);
    if(ret < 0){
        printk("misc device register faikey!\r\n");
        return -EFAULT;
    }
	return 0;
}

static void pcf8591_remove(struct i2c_client *client)
{
	/* 注销misc设备 */
    misc_deregister(&pcf8591_miscdev);
}

/* 传统匹配方式ID列表 */
static const struct i2c_device_id pcf8591_id[] = {
	{"shb,pcf8591", 0},  
	{}
};

/* 设备树匹配列表 */
static const struct of_device_id pcf8591_of_match[] = {
	{ .compatible = "shb,pcf8591" },
	{ /* Sentinel */ }
};

/* i2c驱动结构体 */	
static struct i2c_driver pcf8591_driver = {
	.probe = pcf8591_probe,
	.remove = pcf8591_remove,
	.driver = {
			.owner = THIS_MODULE,
		   	.name = "pcf8591",
		   	.of_match_table = pcf8591_of_match, 
		   },
	.id_table = pcf8591_id,
};    

static int __init pcf8591_init(void)
{
	int ret = 0;
	ret = i2c_add_driver(&pcf8591_driver);
	return ret;
}

static void __exit pcf8591_exit(void)
{
	i2c_del_driver(&pcf8591_driver);
}

module_init(pcf8591_init);
module_exit(pcf8591_exit);

MODULE_LICENSE("GPL v2");   // 开源许可证
MODULE_AUTHOR("liqinghua"); // 模块作者