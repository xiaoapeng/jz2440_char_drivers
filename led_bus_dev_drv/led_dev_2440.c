#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/init.h>

//平台资源
static struct resource led_res[]={
	[0] = {
		.start		= 0x56000050,
		.end		= 0x56000050 + 8 - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1]	= {
		.start		= 4,
		.end		= 4,
		.flags		= IORESOURCE_IRQ,
	},


};
static void	led_release(struct device * dev)
{
	

}

static struct platform_device led_device = {
	.name			= "led_2440",
	.id				= -1,
	.num_resources	= ARRAY_SIZE(led_res),
	.resource		= led_res,
	.dev			={
		.release = led_release,
	}
};

static int __init led_dev_init(void)
{
	return platform_device_register(&led_device);
}

module_init(led_dev_init);

MODULE_LICENSE("GPL");







