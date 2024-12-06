#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include "oled.h"
/*
 * ./led_drv_cdev_app -w 0 1
 * ./led_drv_cdev_app -i 0 1
 * ./led_drv_cdev_app -r 
 */
int main(int argc, char **argv)
{
    OLED_Init();
    OLED_Clear();
    OLED_ShowString(0,0,(char*)"HELLO 44444",16,1);
    OLED_Refresh();//更新显示
    OLED_CLOSE();

    return 0;
}