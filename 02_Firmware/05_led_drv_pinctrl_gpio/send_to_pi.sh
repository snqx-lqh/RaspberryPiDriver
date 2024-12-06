#!/bin/sh

sendfile="smartHomeBoard.dtbo led_drv_driver_gpio.ko led_drv_driver_gpiod.ko led_drv_app"
pi_user=pi
pi_ip=192.168.2.149
pi_dir=/home/pi/RpiDriver/05_led_drv_pinctrl_gpio

scp ${sendfile} ${pi_user}@${pi_ip}:${pi_dir}

# scp hello_drv.ko pi@192.168.2.149:/home/pi/RpiDriver/01_helloworld