#!/bin/sh

sendfile="smartHomeBoard.dtbo led_drv_driver_misc.ko led_drv_app"
pi_user=pi
pi_ip=192.168.2.149
pi_dir=/home/pi/RpiDriver/06_led_drv_misc

scp ${sendfile} ${pi_user}@${pi_ip}:${pi_dir}

 