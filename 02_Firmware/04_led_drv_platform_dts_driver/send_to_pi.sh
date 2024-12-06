#!/bin/sh

sendfile="smartHomeBoard.dtbo led_drv_driver.ko led_drv_app"
pi_user=pi
pi_ip=192.168.2.149
pi_dir=/home/pi/RpiDriver/04_led_drv_platform_dts_driver

scp ${sendfile} ${pi_user}@${pi_ip}:${pi_dir}

# scp hello_drv.ko pi@192.168.2.149:/home/pi/RpiDriver/01_helloworld