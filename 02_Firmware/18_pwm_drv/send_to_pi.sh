#!/bin/sh

sendfile="smartHomeBoard.dtbo pwm_drv_app pwm_drv.ko"
pi_user=pi
pi_ip=192.168.2.149
pi_dir=/home/pi/RpiDriver/18_pwm_drv

scp ${sendfile} ${pi_user}@${pi_ip}:${pi_dir}

 