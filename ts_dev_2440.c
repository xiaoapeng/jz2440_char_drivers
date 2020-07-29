#include <linux/platform_device.h>
#include <asm/plat-s3c24xx/ts.h>
#include <linux/module.h>



static struct s3c2410_ts_mach_info s3c2410_ts_cfg = {
        .delay = 0xffff,
        .presc = 49,
        .oversampling_shift = 2,
};

static void	jz2440_ts_release(struct device * dev){}
/* NULL */



static struct platform_device jz2440ts_device = {
	.name		  = "jz2440-ts",
	.id		  = -1,
	.dev		={
		.platform_data= &s3c2410_ts_cfg,
		.release = jz2440_ts_release
	}
};

static int __init jz2440_ts_init(void)
{
	return platform_device_register(&jz2440ts_device);
}

static void __exit jz2440_ts_exit(void)
{
	platform_device_unregister(&jz2440ts_device);
}


module_init(jz2440_ts_init);
module_exit(jz2440_ts_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("simon <liziandpym.xyz>");
MODULE_DESCRIPTION("ts drv");






