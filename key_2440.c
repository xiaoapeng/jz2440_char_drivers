#include <linux/fs.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/arch/regs-gpio.h>
#include <asm/irq.h>



#define LED_GPIO_SIZE 3
#define NAME 	"led_drvs"

MODULE_AUTHOR("yang ming peng<liziandpym@qq.com>");
MODULE_DESCRIPTION("JZ2440 LEDS GPIO Pin Driver");
MODULE_LICENSE("GPL");


static int major;
module_param(major, int, 0);
MODULE_PARM_DESC(major, "Major device number");

//led0 gpf4
//led1 gpf5
//led2 gpf6



static int led_drv_open(struct inode *inode, struct file *file)
{
	printk(KERN_DEBUG"--------%s-------\n",__FUNCTION__);
	//配置GPF4 为输出
	s3c2410_gpio_cfgpin(S3C2410_GPF4,S3C2410_GPF4_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPF5,S3C2410_GPF5_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPF6,S3C2410_GPF6_OUTP);
	s3c2410_gpio_setpin(S3C2410_GPF4, 1);
	s3c2410_gpio_setpin(S3C2410_GPF5, 1);
	s3c2410_gpio_setpin(S3C2410_GPF6, 1);
	return 0;
}
static ssize_t led_drv_write(struct file *file, const char __user *data,
				 size_t len, loff_t *ppos)
{
	printk(KERN_DEBUG"--------%s-------\n",__FUNCTION__);
	s3c2410_gpio_setpin(S3C2410_GPF4, 1);
	s3c2410_gpio_setpin(S3C2410_GPF5, 1);
	s3c2410_gpio_setpin(S3C2410_GPF6, 1);
	return len;
}
static ssize_t led_drv_read(struct file *file, char __user *buf,
				size_t len, loff_t *ppos)
{
	printk(KERN_DEBUG"--------%s-------\n",__FUNCTION__);
	s3c2410_gpio_setpin(S3C2410_GPF4, 0);
	s3c2410_gpio_setpin(S3C2410_GPF5, 0);
	s3c2410_gpio_setpin(S3C2410_GPF6, 0);
	return 0;
}

static const struct file_operations led_drv_fops = {
	.owner	= THIS_MODULE,
	.write	= led_drv_write,
	.read	= led_drv_read,
	.open	= led_drv_open
};
static struct class *led_class;
struct class_device *clsdev;



static int __init led_drv_init(void)
{
	int err=0;
	
	printk(KERN_DEBUG"--------%s call-------\n",__FUNCTION__);
	major = register_chrdev(major,NAME,&led_drv_fops);
	if(major<0)
	{
		err=major;
		printk(KERN_DEBUG"error :major=%d\n",major);
		goto register_chrdevError;
	}
	led_class=class_create(THIS_MODULE,"leds");
	if (IS_ERR(led_class))
	{
		err=PTR_ERR(led_class);
		goto class_createError;
	}
	clsdev=class_device_create(led_class, NULL, MKDEV(major, 0),NULL,"leds0");
	if (IS_ERR(clsdev)) {
		printk(KERN_DEBUG "%s: failed to create device\n",
		       __FUNCTION__);
		err=PTR_ERR(clsdev);
		goto class_device_createError;
	}
	return 0;
	class_device_createError:
	class_destroy(led_class);
	class_createError:
	unregister_chrdev(major, NAME);
	register_chrdevError:
	return err;
}
static void __exit led_drv_exit(void)
{
	printk(KERN_DEBUG"--------%s-------\n",__FUNCTION__);
	class_device_destroy(led_class,MKDEV(major, 0));
	class_destroy(led_class);
	unregister_chrdev(major,NAME);
}
//include/linux/init.h:228
module_init(led_drv_init);
module_exit(led_drv_exit);



