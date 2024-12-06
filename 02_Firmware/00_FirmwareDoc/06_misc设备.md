## 笔记说明

这种驱动的特点是不需要自己申请设备号，他的设备号都是10。只是每个设备的子设备号不一样，主要代码修改的位置就是注册设备那一节代码有变化。

主要参考的文章

正点原子《I.MX6U 嵌入式 Linux 驱动开发指南 V1.81》

本节源码路径`02_Firmware/06_led_drv_misc`

## 编程流程

创建一个misc设备结构体，里面包含设备操作函数信息，注册的名称，子设备号。

```C
/* MISC 设备结构体 */
static struct miscdevice led_miscdev = {
    .minor = 144,        //子设备号
    .name = "led_drv",   //注册名称
    .fops = &led_fops,
};
```

然后就是注册设备驱动，这个注册，就不需要cdev，class，device啥的。

```C
/* 注册misc字符设备驱动 */
ret = misc_register(&led_miscdev);
if(ret < 0){
	printk("misc device register failed!\r\n");
	return -EFAULT;
}
```

然后就是注销设备驱动。

```C
/* 注销misc设备 */
misc_deregister(&led_miscdev);
```

## 编译加载

参考之前的，有一个问题就是加载不同的设备树，需要重启后加载
