#include <linux/init.h>
#include <linux/module.h>

static int __init hello_drv_init(void)
{
  printk("init kernel\n");
  return 0;
}

static void __exit hello_drv_exit(void)
{
  printk("exit kernel\n");
}

module_init(hello_drv_init);
module_exit(hello_drv_exit);

MODULE_LICENSE("GPL v2");   // 开源许可证