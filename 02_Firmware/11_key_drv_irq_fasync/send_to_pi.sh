#!/bin/sh

sendfile="smartHomeBoard.dtbo key_drv_irq_app key_drv_irq.ko"
pi_user=pi
pi_ip=192.168.2.149
pi_dir=/home/pi/RpiDriver/11_key_drv_irq_fasync

scp ${sendfile} ${pi_user}@${pi_ip}:${pi_dir}

 