## 笔记说明

笔记主要是要记录如何使用树莓派进行linux的驱动开发，主要是学习记录ARMLinux板子的驱动开发。由于不想破坏原来的电脑环境，或者说想尽量**使得编译环境一致**，我们使用Docker虚拟环境进行编译。虚拟环境主要编译，文件处理还是使用宿主机。关于Docker可以网上找点其他教程简易入门。

当然，你也可以不使用docker，直接在宿主机编译，没有任何问题。

主要参考的文章

[peng-zhihui/Planck-Pi](https://gitee.com/peng_zhihui/Planck-Pi#1-f1cxxxs%E8%8A%AF%E7%89%87%E7%9A%84%E4%B8%8A%E7%94%B5%E5%90%AF%E5%8A%A8%E9%A1%BA%E5%BA%8F)

[Philon/rpi-drivers](https://github.com/Philon/rpi-drivers)

本节源码路径`02_Firmware/01_helloworld`

## 环境搭建

**注意**：处理文档都在宿主机下，只有编译才使用Docker环境，因为我用vscode远程连接虚拟机编辑文档的话，如果是Docker解压处理的文档，就权限不足，所以处理文档就在宿主机下。

提前安装好docker，下载镜像的时候，去https://xuanyuan.me/blog/archives/1154?from=tencent，关注加速列表。

安装docker的话根据官网一步步走下来就可以了

[Install Docker Engine on Ubuntu | Docker Docs](https://docs.docker.com/engine/install/ubuntu/#set-up-the-repository)

### 将用户添加到 Docker 组

在Ubuntu系统上，需要在运行Docker命令时添加 sudo 是因为 Docker 守护进程默认以 root 用户权限运行。Docker 执行一些需要特权权限的操作，如访问系统资源、管理容器、构建镜像、挂载文件系统等，以确保安全性和系统稳定性。如果希望不每次都使用 sudo 运行 Docker 命令，可以将当前用户加入 docker 组。

 运行以下命令将当前用户添加到docker组中：

```BASH
sudo usermod -aG docker $USER
```

注销并重新登录，或者运行以下命令使更改生效（我这里运行命令没生效，重启才可以）

```BASH
sudo service docker restart
newgrp docker
```

确保用户$USER 已经成功添加到 docker 组。(自己输入自己的用户名)，可以使用以下命令检查

```
id $USER
```

### 配置镜像环境

我这里选择的镜像源是docker.1ms.run，到时候不一定能用，注意关注我前面说的镜像表。

下载ubuntu镜像

```BASH
sudo docker pull docker.1ms.run/ubuntu:20.04
```

启动一个交互式的容器，使用名为 "rpi3b-plus-env"，并将本地主机上的 SDK 目录映射到容器内的 /home 目录，最后以 Bash shell 运行。

```BASH
#第一次运行docker环境
sudo docker run -it --name rpi3b-plus-env \
-p 8022:22 \
-v /home/lqh/rpi3b_plus_workspace:/home \
docker.1ms.run/ubuntu:20.04 /bin/bash

#第二次运行docker环境
sudo docker start -ai rpi3b-plus-env

#退出
exit
```

进入容器后需要换源，先安装一个vim。安装前需要软件库更新，不然都没有软件。

```BASH
apt-get update
apt-get install vim
```

然后执行`vim /etc/apt/sources.list`，换成你当时想要的源，可以是[清华源](https://mirrors.tuna.tsinghua.edu.cn/help/ubuntu/)，或者其他源。下面是ubuntu20的清华源

```BASH
deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ focal main restricted universe multiverse
# deb-src https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ focal main restricted universe multiverse
deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ focal-updates main restricted universe multiverse
# deb-src https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ focal-updates main restricted universe multiverse
deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ focal-backports main restricted universe multiverse
# deb-src https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ focal-backports main restricted universe multiverse

deb http://security.ubuntu.com/ubuntu/ focal-security main restricted universe multiverse
# deb-src http://security.ubuntu.com/ubuntu/ focal-security main restricted universe multiverse

# deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ focal-proposed main restricted universe multiverse
# # deb-src https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ focal-proposed main restricted universe multiverse
```

换源完成后，更新

```BASH
apt-get update
```

这个时候应该会报一个错误`Certificate verification failed: The certificate is NOT trusted.`，证书的问题这时候执行以下操作。

```BASH
# 把之前换源的https换成http
vim /etc/apt/sources.list

# 更新一下
apt-get update

# 安装证书
apt-get install --reinstall ca-certificates

# 把之前换源的http换成https
vim /etc/apt/sources.list

# 更新一下
apt-get update
```

安装需要的软件

```BASH
apt-get install xz-utils nano wget unzip build-essential git bc swig libncurses5-dev libpython3-dev libssl-dev pkg-config zlib1g-dev libusb-dev libusb-1.0-0-dev python3-pip gawk bison flex 
```

安装编译工具链

```BASH
apt install gcc-arm-linux-gnueabihf
```

安装完成后可以使用，以下指令查看是否安装成功。

```BASH
arm-linux-gnueabihf-gcc -v
```

配置完成后可以commit容器到镜像方便未来部署：

```
sudo docker commit rpi3b-plus-env rpi3b-plus-env.image
sudo docker save -o rpi3b-plus-env.image.tar rpi3b-plus-env.image
```

如果是其他计算机导入的话

```BASH
docker load -i rpi3b-plus-env.image.tar

# 或者使用

docker load < rpi3b-plus-env.image.tar
```

然后建立新环境的时候就可以

```bash
#第一次运行docker环境
sudo docker run -it --name rpi3b-plus-env \
-p 8022:22 \
-v /home/lqh/rpi3b_plus_workspace:/home \
rpi3b-plus-env.image:lasted /bin/bash
```

## 简易测试

### 下载内核源码

我们处理文件在宿主机上面进行处理，主要宿主机远程连接，才可以修改文件内容，如果直接在Docker容器里面的话，他的权限会导致必须要root才能处理，不是很方便，后面想办法解决这个问题。

```BASH
# 在宿主机中
cd /home/lqh/rpi3b_plus_workspace/
# 下载内核源码
wget https://github.com/raspberrypi/linux/archive/rpi-6.6.y.tar.gz
# 解压，进入内核目录
tar xvf rpi-6.6.y.tar.gz  

# 在Docker环境中
cd /home/linux-rpi-6.6.y
# 清理内核
make mrproper
# 加载RPi-3B+的配置
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- bcm2709_defconfig
# 编译
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j4
```

## 更换内核

我安装的树莓派固件版本是

```bash
pi@raspberrypi:~/RpiDriver/01_helloworld $ uname -a
Linux raspberrypi 6.6.51+rpt-rpi-v7 #1 SMP Raspbian 1:6.6.51-1+rpt3 (2024-10-08) armv7l GNU/Linux
```

但是我拉取github源码的话，他是6.6.62版本。所以找了资料，重新换内核。下面的步骤[参考博客](https://blog.csdn.net/Hopeless_Loop/article/details/133907371)，但是他是树莓派4B，我是3B+，所以有些不同。

你们的版本应该大概率也会不一样，如果版本不一样会报错无法加载内核。所以需要换内核

进入下载的树莓派源代码目录，设置环境变量KERNEL，并使用make指定bcm2711_defconfig配置模板生成arm架构32位的配置文件。

注意：树莓派版本不同，这里设置的环境变量KERNEL不同，所使用的配置模板也不同，其他型号的具体参数设置请参考[链接](https://www.raspberrypi.com/documentation/computers/linux_kernel.html#kernel)。

在docker环境中

```bash
cd linux-rpi-6.6.y
KERNEL=kernel7
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- bcm2709_defconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- zImage modules dtbs -j4
```

把树莓派关机，然后卡取下来，插到虚拟机上，使用`lsblk`，会发现它的两个分区，一个是boot分区小一点，一个是文件系统分区大一点，我这里，boot是sdb1，rootfs是sdb2。然后在宿主机的linux-rpi-6.6.y文件夹做以下操作。

```bash
sudo mkdir mnt
sudo mkdir mnt/fat32
sudo mkdir mnt/ext4
sudo mount /dev/sdb1 mnt/fat32
sudo mount /dev/sdb2 mnt/ext4
```

然后加载刚刚编译好的模块

```bash
KERNEL=kernel7
sudo env PATH=$PATH make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- INSTALL_MOD_PATH=mnt/ext4 modules_install
```

然后，备份原内核、复制新内核及设备树信息。mkknlimg这个工具高版本内核里面没有，去一个低版本里面下一个，然后放进来。原作者的dts下面没有分芯片，6.6版本的分了芯片，移动dtb的时候注意。

```bash
sudo cp mnt/fat32/$KERNEL.img mnt/fat32/$KERNEL-backup.img
./scripts/mkknlimg arch/arm/boot/zImage mnt/fat32/$KERNEL.img
sudo cp arch/arm/boot/dts/broadcom/*.dtb mnt/fat32/
sudo cp arch/arm/boot/dts/overlays/*.dtb* mnt/fat32/overlays/
sudo cp arch/arm/boot/dts/overlays/README mnt/fat32/overlays/
sudo umount mnt/fat32
sudo umount mnt/ext4
```

将SD卡重新插入树莓派，启动树莓派，使用下列命令即可查看内核版本。

```bash
uname -a

pi@raspberrypi:~ $ uname -a
Linux raspberrypi 6.6.62-v7 #4 SMP Sun Dec  1 17:17:33 CST 2024 armv7l GNU/Linux
```

就会发现和拉取的版本一样了。

### 简易helloworld驱动

创建工程目录：

```shell
# 在宿主机中
# 创建目录，进入
mkdir -p /home/lqh/rpi3b_plus_workspace/example/01_helloworld  
cd /home/lqh/rpi3b_plus_workspace/example/01_helloworld
# 创建驱动模块源码及Makefile
touch hello_drv.c Makefile
```

hello_drv.c

```c
#include <linux/init.h>
#include <linux/module.h>

static int __init hello_init(void)
{
    printk("init kernel\n");
    return 0;
}
module_init(hello_init);

static void __exit hello_exit(void)
{
    printk("exit kernel\n");
}
module_exit(hello_exit);

MODULE_LICENSE("GPL"); // 开源许可证
```

Makefile

```Makefile
# 模块驱动，必须以obj-m=xxx形式编写
obj-m = hello_drv.o

KDIR = ../linux-rpi-6.6.y
CROSS = ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

all:
	$(MAKE) -C $(KDIR) M=$(PWD) $(CROSS) modules

clean:
	$(MAKE) -C $(KDIR) M=`pwd` $(CROSS) clean
```

```shell
# 在Docker环境中
# 编译内核
cd /home/example/01_helloworld
make
```

## 树莓派运行

makefile编译完成后，直接make就行

```BASH
make
```

将make生成的文件，通过scp传到树莓派上，我这里写了个脚本。具体的内容参数，自己修改。

```BASH
#!/bin/sh

sendfile="hello_drv.ko"
pi_user=pi
pi_ip=192.168.2.149
pi_dir=/home/pi/RpiDriver/01_helloworld

scp ${sendfile} ${pi_user}@${pi_ip}:${pi_dir}

# scp hello_drv.ko pi@192.168.2.149:/home/pi/RpiDriver/01_helloworld
```

发送树莓派前，在树莓派的指定文件夹下面先建立这些文件夹，不然发送文件会不成功，因为没有该路径

```bash
mkdir 01_helloworld 02_led_drv_cdev 03_led_drv_platform_device_driver 04_led_drv_platform_dts_driver 05_led_drv_pinctrl_gpio 06_led_drv_misc 07_key_drv_irq 08_key_drv_irq_timer 09_key_drv_irq_sleep_wakeup 10_key_drv_irq_poll 11_key_drv_irq_fasync 12_key_drv_irq_block 13_key_drv_irq_tasklet 14_key_drv_irq_workqueue 15_key_drv_irq_threaded 16_pcf8591_drv_i2c 17_oled_drv_spi 18_pwm_drv
```

在树莓派上，我们需要加载并使用这个模块。

```BASH
sudo insmod hello_drv.ko

sudo rmmod hello_drv
```

可以使用dmesg查看运行过程中的变化

```bash
dmesg | tail -5

# 查看内核打印信息
[   19.132265] Bluetooth: RFCOMM socket layer initialized
[   19.132291] Bluetooth: RFCOMM ver 1.11
[ 6319.896017] hello_drv: loading out-of-tree module taints kernel.
[ 6319.896438] init kernel                #加载
[ 6390.206953] exit kernel                  #卸载
```

