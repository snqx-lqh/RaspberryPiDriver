#!/bin/sh

sendfile="smartHomeBoard.dtbo oled_drv_app oled_drv_spi.ko"
pi_user=pi
pi_ip=192.168.2.149
pi_dir=/home/pi/RpiDriver/17_oled_drv_spi

scp ${sendfile} ${pi_user}@${pi_ip}:${pi_dir}

 