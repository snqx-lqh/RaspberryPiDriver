#!/bin/sh

sendfile=hello_drv.ko
pi_user=pi
pi_ip=192.168.2.149
pi_dir=/home/pi/RpiDriver/01_helloworld

scp ${sendfile} ${pi_user}@${pi_ip}:${pi_dir}

# scp hello_drv.ko pi@192.168.2.149:/home/pi/RpiDriver/01_helloworld