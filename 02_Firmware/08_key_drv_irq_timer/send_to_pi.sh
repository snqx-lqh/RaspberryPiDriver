#!/bin/sh

sendfile="smartHomeBoard.dtbo key_drv_irq.ko"
pi_user=pi
pi_ip=192.168.2.149
pi_dir=/home/pi/RpiDriver/08_key_drv_irq_timer

scp ${sendfile} ${pi_user}@${pi_ip}:${pi_dir}

 