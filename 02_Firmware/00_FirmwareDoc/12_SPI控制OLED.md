## 笔记说明

这一节，主要是设备树有点不同，其他的SPI发送和正点原子的教程差不多。

主要参考的文章

正点原子《I.MX6U 嵌入式 Linux 驱动开发指南 V1.81》

本节源码路径`02_Firmware/17_oled_drv_spi` 

## 设备树

这里为了减少篇幅，只是写了spi相关的，完整的看项目源代码。加载设备树的时候遇到了一个I2C时候没遇到的问题，就是如果在raspi-config里面打开了设备树的相关SPI配置，就会导致加载设备树失败，必须把那个关了。

```c
// Definitions for gpio-key module
/dts-v1/;
/plugin/;

/ {
	compatible = "brcm,bcm2835";
	
    fragment@1 {
		// Configure the gpio pin controller
		target = <&gpio>;
		__overlay__ {
			shbled_pins: shbled_pins@0 {
				brcm,pins = <17 27 22 23>; // gpio number
				brcm,function = <1 1 1 1>; // 0 = input, 1 = output
				brcm,pull = <2 2 2 2>; // 0 = none, 1 = pull down, 2 = pull up
			};
			shb_key_pins: shb_key_pins@0 {
				brcm,pins = <26 21 20>; // gpio number
				brcm,function = <0 0 0>; // 0 = input, 1 = output
				brcm,pull = <2 2 2>; // 0 = none, 1 = pull down, 2 = pull up
			};
			ssd1306_pins: ssd1306_pins {
				brcm,pins = <25 24>;
				brcm,function = <1 1>; /* out out */
			};
			pwm_pins: pwm_pins {
				brcm,pins = <18>;
				brcm,function = <2>; /* Alt5 */
			};
		};
	};
    
	fragment@4 {
		target = <&spi0>;
		__overlay__ {
			/* needed to avoid dtc warning */
			#address-cells = <1>;
			#size-cells = <0>;
			status = "okay";
			
			ssd1306: ssd1306@0{
				compatible = "solomon,ssd1306";
				reg = <0>;
				pinctrl-names = "default";
				pinctrl-0 = <&ssd1306_pins>;
				spi-max-frequency = <10000000>;
				reset-gpios = <&gpio 25 0>;
				dc-gpios = <&gpio 24 0>;
			};
		};
	};

	fragment@5 {
		target = <&spidev0>;
		__overlay__ {
			status = "disabled";
		};
	};

	fragment@6 {
		target = <&spidev1>;
		__overlay__ {
			status = "disabled";
		};
	};
};
```

## 代码

首先驱动出入口函数发生了变化，使用的是和spi相关的注册和注销。

```c
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
```

然后就是spi写一个值的写法

```
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
```

这个部分还遇到的一个比较麻烦的点就是之前一直用的gpiod，结果这里gpiod获取reset和dc引脚的方式我没调试出来，最后还是直接使用的gpio相关的api解决的。