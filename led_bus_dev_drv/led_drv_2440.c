#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>
#include <asm/mach/flash.h>

#include <asm/arch/map.h>
#include <asm/arch/bast-map.h>
#include <asm/arch/bast-cpld.h>


static int led_remove(struct platform_device *pdev)
{
	printk(KERN_INFO"-----%s call!-----\n",__FUNCTION__);
	return 0;
}

static int led_probe(struct platform_device *pdev)
{
	printk(KERN_INFO"-----%s call!-----\n",__FUNCTION__);
	return 0;
}

static struct platform_driver led_drviver = {
	.probe		= led_probe,
	.remove		= led_remove,
	.driver		= {
		.name	= "led_2440",
		.owner	= THIS_MODULE,
	},
};

static int __init led_drv_init(void)
{
	return platform_driver_register(&led_drviver);
}

static void __exit led_drv_exit(void)
{
	platform_driver_unregister(&led_drviver);
}

module_init(led_drv_init);
module_exit(led_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ben Dooks <ben@simtec.co.uk>");
MODULE_DESCRIPTION("BAST MTD Map driver");
