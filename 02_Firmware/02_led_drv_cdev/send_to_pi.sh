#!/bin/sh

sendfile="led_drv_cdev.ko led_drv_cdev_app"
pi_user=pi
pi_ip=192.168.2.149
pi_dir=/home/pi/RpiDriver/02_led_drv_cdev

scp ${sendfile} ${pi_user}@${pi_ip}:${pi_dir}

# scp hello_drv.ko pi@192.168.2.149:/home/pi/RpiDriver/01_helloworld