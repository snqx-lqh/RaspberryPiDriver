## 笔记说明

主要就是修改了设备树，然后控制文件。

主要参考：

正点原子《I.MX6U 嵌入式 Linux 驱动开发指南 V1.81》

本节源码路径`02_Firmware/18_pwm_drv` 

## 设备树

这里为了减少篇幅，只是写了pwm相关的，完整的看项目源代码。

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
			pwm_pins: pwm_pins {
				brcm,pins = <18>;
				brcm,function = <2>; /* Alt5 */
			};
		};
	};
    
	fragment@7 {
		target = <&pwm>;
		__overlay__ {
			pinctrl-names = "default";
			pinctrl-0 = <&pwm_pins>;
			assigned-clock-rates = <100000000>;
			status = "okay";
		};
	};
};
```

## 命令控制

加载了设备信息后

```bash
$ ls /sys/class/pwm
pwmchip0
$ ls /sys/class/pwm/pwmchip0
device  export  npwm  power  pwm0  subsystem  uevent  unexport
```


这些伪文件就是Linux内核PWM驱动提供的操纵接口，在shell里可以通过cat读，通过echo重定向写。在任意编程语言里也可以通过读写文件的接口进行同样的操作。

首先创建一个PWM的导出，向export写几，就会创建对应的目录在pwmchipX里面：

```bash
$ echo 0 >/sys/class/pwm/pwmchip0/export
$ ls /sys/class/pwm/pwmchip0/pwm0
capture  duty_cycle  enable  period  polarity  power  uevent
```


这里面，period是以纳秒计数的PWM周期，duty_cycle是以纳秒计数的每周期高电平时间。比如我想要一个20KHz的PWM，占空比为80%，那我就应当：

```bash
$ cd /sys/class/pwm/pwmchip0/pwm0
$ echo 50000 >period # 两万Hz的时长是五万纳秒
$ echo 10000 >duty_cycle # 占空比80%，那么20%的时长就是一万纳秒
```


然后向enable写0或者1进行开关。

```bash
$ echo 1 >enable # 立即开启
```

关闭之后不会清空原有设置，再次打开会以之前设置的参数运行PWM。

如果需要释放资源，向/sys/clas/pwm/pwmchipX/unexport写对应的序号，会清空对应的PWM导出目录，并且删除配置。