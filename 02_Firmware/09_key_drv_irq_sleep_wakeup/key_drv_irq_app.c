#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

/*
 * sudo ./key_drv_irq_app  
 */
int main(int argc, char **argv)
{
    int fd;
    int val;

    /* 1. 判断参数 */

    /* 2. 打开文件 */
    fd = open("/dev/key_drv", O_RDWR);
    if (fd == -1)
    {
        printf("can not open file /dev/key_drv\n");
        return -1;
    }

    while(1)
    {
        read(fd,&val,4);
        printf("get button :0x%x\n",val);
    }

    close(fd);

    return 0;
}