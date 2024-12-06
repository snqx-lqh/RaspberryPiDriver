#!/bin/sh

sendfile="smartHomeBoard.dtbo pcf8591_drv_i2c_app pcf8591_drv_i2c.ko"
pi_user=pi
pi_ip=192.168.2.149
pi_dir=/home/pi/RpiDriver/16_pcf8591_drv_i2c

scp ${sendfile} ${pi_user}@${pi_ip}:${pi_dir}

 