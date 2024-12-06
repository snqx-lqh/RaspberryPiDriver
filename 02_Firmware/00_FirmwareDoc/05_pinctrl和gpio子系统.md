
## 笔记说明

主要参考的文章

正点原子《I.MX6U 嵌入式 Linux 驱动开发指南 V1.81》

韦东山《01_嵌入式Linux应用开发完全手册V5.1_IMX6ULL_Pro开发板》

[Philon/rpi-drivers](https://github.com/Philon/rpi-drivers)

上一节使用的设备树，但是还是把寄存器信息等，进行ioremap。

内核的pinctrl子系统和gpio子系统已经可以自己映射和初始化。所以我们可以直接配置设备树初始化，并且使用内核控制GPIO的方式。

pinctrl 子系统主要工作内容如下：

1、获取设备树中 pin 信息。

2、根据获取到的 pin 信息来设置 pin 的复用功能

3、根据获取到的 pin 信息来设置 pin 的电气特性，比如上/下拉、速度、驱动能力等。对于我们使用者来讲，只需要在设备树里面设置好某个 pin 的相关属性即可，其他的初始化工作均由 pinctrl 子系统来完成，pinctrl 子系统源码目录为 drivers/pinctrl。

pinctrl 配置好以后就是设置 gpio 了

如在节点信息中，配置某个属性

```DTS
pinctrl-0 = <&shbled_pins>;
led1-gpios = <&gpio 17 0>;
```

这里就是配置gpio17为低电平有效。

本节源码路径`02_Firmware/05_led_drv_pinctrl_gpio`

## 设备树编写

这一部分的设备树，编写方式和上一节不太一样，先贴代码。

```C
// Definitions for gpio-key module
/dts-v1/;
/plugin/;

/ {
    compatible = "brcm,bcm2835";
    
    fragment@0{
        target-path = "/";
        __overlay__ {
            shbled: shbled@0 {
                compatible = "shb-led";
                pinctrl-names = "default";
                pinctrl-0 = <&shbled_pins>;
                status = "okay";

                led1-gpios = <&gpio 17 0>;  //17引脚低电平有效
                led2-gpios = <&gpio 27 0>;  
                led3-gpios = <&gpio 22 0>;  
                led4-gpios = <&gpio 23 0>;  
            };
        };
    };

    fragment@1 {
        // Configure the gpio pin controller
        target = <&gpio>;
        __overlay__ {
            shbled_pins: shbled_pins@0 {
                brcm,pins = <17 27 22 23>; // gpio number
                brcm,function = <1 1 1 1>; // 0 = input, 1 = output
                brcm,pull = <2 2 2 2>; // 0 = none, 1 = pull down, 2 = pull up
            };
        };
    };
};
```

观察上述设备树，我们先在根节点目录下定义了一个shbled的设备节点，然后在里面，定义了几个led的引脚和低电平有效，pinctrl的初始化方式是shbled_pins，这个会在gpio节点下再次定义，主要是配置初始化的方式，输入还是输出，是否上拉等。

## Driver文件编写

driver文件编写就和之前差的有点多，但是也不是太多，主要就是gpio的控制方式不需要我们来控制寄存器处理了。

还有就是为了方便管理变量，我将全部的变量保存到了一个结构体中，后面不做说明

```C
typedef struct{
    dev_t devid; /* 设备号 */
    struct cdev cdev; /* cdev */
    struct device_node *node; /* LED 设备节点 */
    struct class  *class;    //类
    struct device *device;    //设备
    int    led_gpio[4];
}led_dev_t;

static led_dev_t led_dev;
```

编写的话，分为老的一套gpio的API，以及新的gpiod的API，但是有时候感觉老的好用。这里都会讲解。

如果使用gpiod，结构体定义就是使用的`struct gpio_desc`

```C
typedef struct{
    dev_t devid; /* 设备号 */
    struct cdev cdev; /* cdev */
    struct device_node *node; /* LED 设备节点 */
    struct class  *class;    //类
    struct device *device;    //设备
    struct gpio_desc * led_gpiod[4];
}led_dev_t;

static led_dev_t led_dev;
```
## gpio方式
### probe中的初始化

基本的方式就是，读节点信息，通过节点中对gpio的命名来获取gpio，然后就可以用一些api来控制gpio的输出和输入了。

```C
static int led_probe(struct platform_device *dev)
{  
    int ret;

    //读节点信息
    led_dev.node = dev->dev.of_node;            

    // 获得gpio信息
    led_dev.led_gpio[0] = of_get_named_gpio(led_dev.node, "led1-gpios", 0);
    led_dev.led_gpio[1] = of_get_named_gpio(led_dev.node, "led2-gpios", 0);
    led_dev.led_gpio[2] = of_get_named_gpio(led_dev.node, "led3-gpios", 0);
    led_dev.led_gpio[3] = of_get_named_gpio(led_dev.node, "led4-gpios", 0);

    //设置gpio为输出模式，同时初始化为低电平
    ret = gpio_direction_output(led_dev.led_gpio[0], 0);
    ret = gpio_direction_output(led_dev.led_gpio[1], 0);
    ret = gpio_direction_output(led_dev.led_gpio[2], 0);
    ret = gpio_direction_output(led_dev.led_gpio[3], 0);

    // 注册字符设备、创建类和设备
	// ************** //

    return 0;
}
```

### remove中的释放

gpio的释放也和之前的不太一样，有专门的api

```C
static int led_remove(struct platform_device *dev)
{
    //设置引脚电平为高电平
    gpio_set_value(led_dev.led_gpio[0], 1);
    gpio_set_value(led_dev.led_gpio[1], 1);
    gpio_set_value(led_dev.led_gpio[2], 1);
    gpio_set_value(led_dev.led_gpio[3], 1);

    //释放引脚
    gpio_free(led_dev.led_gpio[0]);
    gpio_free(led_dev.led_gpio[1]);
    gpio_free(led_dev.led_gpio[2]);
    gpio_free(led_dev.led_gpio[3]);

	// 释放字符设备、删除类和设备
	// ************** //
    return 0;
}
```

### 设备操作函数

这一部分的修改就是没有使用我以前控制寄存器的方式了。而是直接使用gpio的api。主要的API看代码即可。

```C
#define MIN(a, b) (a < b ? a : b)

// 通过文件读取，得到当前LED的状态
ssize_t led_drv_read(struct file* filp, char __user* buf, size_t len, loff_t* off)
{
    int ret = 0;
    int char_len = 0;
    char led_state[4];

    printk("%s %s line %d\r\n", __FILE__, __FUNCTION__, __LINE__);

    led_state[0] = gpio_get_value(led_dev.led_gpio[0]);
    led_state[1] = gpio_get_value(led_dev.led_gpio[1]);
    led_state[2] = gpio_get_value(led_dev.led_gpio[2]);
    led_state[3] = gpio_get_value(led_dev.led_gpio[3]);

    char_len = sizeof(led_state);
    int real_len = MIN(len,char_len);
    ret = copy_to_user(buf, led_state, real_len);
    return ret < 0 ? ret : real_len;
}

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
    gpio_set_value(led_dev.led_gpio[(int)led_state[0]], led_state[1]);
    return 0;
}
```

## gpiod方式

他的api感觉就是在每个操作函数后面接了个d。
### probe中的初始化

基本的方式就是，读节点信息，通过节点中对gpio的命名来获取gpio，然后就可以用一些api来控制gpio的输出和输入了。但是gpiod的话，可以使用`gpiod_get`，这样就不用输入完整的gpio名称。

```C
static int led_probe(struct platform_device *dev)
{  
    int ret;

    // 获取GPIO描述符
    led_dev.led_gpiod[0] = gpiod_get(&dev->dev,"led1",0);
    led_dev.led_gpiod[1] = gpiod_get(&dev->dev,"led2",0);
    led_dev.led_gpiod[2] = gpiod_get(&dev->dev,"led3",0);
    led_dev.led_gpiod[3] = gpiod_get(&dev->dev,"led4",0);

    //设置gpio为输出模式，同时初始化为低电平
    ret = gpiod_direction_output(led_dev.led_gpiod[0], 0);
    ret = gpiod_direction_output(led_dev.led_gpiod[1], 0);
    ret = gpiod_direction_output(led_dev.led_gpiod[2], 0);
    ret = gpiod_direction_output(led_dev.led_gpiod[3], 0);

    // 注册字符设备、创建类和设备
	// ************** //

    return 0;
}
```

### remove中的释放

gpio的释放也和之前的不太一样，有专门的api

```C
static int led_remove(struct platform_device *dev)
{
    //设置引脚电平为高电平
    gpiod_set_value(led_dev.led_gpiod[0], 1);
    gpiod_set_value(led_dev.led_gpiod[1], 1);
    gpiod_set_value(led_dev.led_gpiod[2], 1);
    gpiod_set_value(led_dev.led_gpiod[3], 1);

    //释放引脚
    gpiod_put(led_dev.led_gpiod[0]);
    gpiod_put(led_dev.led_gpiod[1]);
    gpiod_put(led_dev.led_gpiod[2]);
    gpiod_put(led_dev.led_gpiod[3]);

	// 释放字符设备、删除类和设备
	// ************** //
    return 0;
}
```

### 设备操作函数

这一部分的修改就是没有使用我以前控制寄存器的方式了。而是直接使用gpio的api。主要的API看代码即可。

```C
#define MIN(a, b) (a < b ? a : b)

// 通过文件读取，得到当前LED的状态
ssize_t led_drv_read(struct file* filp, char __user* buf, size_t len, loff_t* off)
{
    int ret = 0;
    int char_len = 0;
    char led_state[4];

    printk("%s %s line %d\r\n", __FILE__, __FUNCTION__, __LINE__);

    led_state[0] = gpiod_get_value(led_dev.led_gpiod[0]);
    led_state[1] = gpiod_get_value(led_dev.led_gpiod[1]);
    led_state[2] = gpiod_get_value(led_dev.led_gpiod[2]);
    led_state[3] = gpiod_get_value(led_dev.led_gpiod[3]);

    char_len = sizeof(led_state);
    int real_len = MIN(len,char_len);
    ret = copy_to_user(buf, led_state, real_len);
    return ret < 0 ? ret : real_len;
}

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
    gpiod_set_value(led_dev.led_gpiod[(int)led_state[0]], led_state[1]);
    return 0;
}
```

## 编译加载

参考前面的博文，有一个问题就是加载不同的设备树，需要重启后加载