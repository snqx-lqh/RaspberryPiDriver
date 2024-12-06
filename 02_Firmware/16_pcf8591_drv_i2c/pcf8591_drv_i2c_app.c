#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

/*
 * sudo ./pcf8591_drv_i2c_app -w 200
 * sudo ./pcf8591_drv_i2c_app -r 
 */
int main(int argc, char **argv)
{
    int fd;
    char writeBuff[1];
    char readBuff[5];
    int len;
    int count = 0;
    /* 1. 判断参数 */
    if (argc < 2)
    {
        printf("Usage: %s -w <string>\n", argv[0]);
        printf(" %s -r\n", argv[0]);
        return -1;
    }

    /* 2. 打开文件 */
    fd = open("/dev/pcf8591_drv", O_RDWR);
    if (fd == -1)
    {
        printf("can not open file /dev/pcf8591_drv\n");
        return -1;
    }

    /* 3. 写文件或读文件 */
    if ((0 == strcmp(argv[1], "-w")) && (argc == 3))
    {
        writeBuff[0] = (char)atoi(argv[2]);
        write(fd, writeBuff, 1);
    }
    else if((0 == strcmp(argv[1], "-r")) && (argc == 2))
    {
        count ++;
        while (count < 10)
        {
            len = read(fd, readBuff, 4);
            readBuff[4] = '\0';
            printf("APP read : %d, %d, %d, %d\n", readBuff[0], readBuff[1], readBuff[2], readBuff[3]);
            sleep(1);
        }
    }
    else{
        printf("APP Failed\n");
    }

    close(fd);

    return 0;
}