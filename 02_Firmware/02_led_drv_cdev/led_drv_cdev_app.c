#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

/*
 * sudo ./led_drv_cdev_app -w 0 0
 * sudo ./led_drv_cdev_app -r 
 */
int main(int argc, char **argv)
{
    int fd;
    char writeBuff[3];
    char readBuff[5];
    int len;

    if (argc < 2)
    {
        printf("Usage: %s -w <string>\n", argv[0]);
        printf(" %s -r\n", argv[0]);
        return -1;
    }

    fd = open("/dev/led_drv", O_RDWR);
    if (fd == -1)
    {
        printf("can not open file /dev/led_drv\n");
        return -1;
    }

    if ((0 == strcmp(argv[1], "-w")) && (argc == 4))
    {
        writeBuff[0] = (char)atoi(argv[2]);
        writeBuff[1] = (char)atoi(argv[3]);
        writeBuff[2] = '\0';
        printf("write : %d, %d\n", writeBuff[0], writeBuff[1]);
        write(fd, writeBuff, 3);
    }else if((0 == strcmp(argv[1], "-r")) && (argc == 2))
    {
        len = read(fd, readBuff, 5);
        readBuff[4] = '\0';
        printf("pin read : 0x%x, 0x%x, 0x%x, 0x%x\n", readBuff[0], readBuff[1], readBuff[2], readBuff[3]);
    }
    else{
        printf("APP Failed\n");
    }

    close(fd);

    return 0;
}