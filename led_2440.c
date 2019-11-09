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
#include <linux/memory_hotplug.h>
#include <asm/string.h>
#include <asm/uaccess.h>


#define LED_GPIO_SIZE 4
#define NAME 	"led_drvs"

MODULE_AUTHOR("yang ming peng<liziandpym@qq.com>");
MODULE_DESCRIPTION("JZ2440 KEY GPIO Pin Driver");
MODULE_LICENSE("GPL");


static int major;
module_param(major, int, 0);
MODULE_PARM_DESC(major, "Major device number");



//led0 gpf4
//led1 gpf5
//led2 gpf6
struct led_2440
{
	struct class *led_class;
	int major;				//主函数状态
	int open_flag;			//open函数
	unsigned long led_val;	//led状态
	spinlock_t open_lock; 	//自旋锁变量
	rwlock_t   rwlock;		//读写锁
};

static struct led_2440 *led_2440p;





static int led_drv_open(struct inode *inode, struct file *file)
{
	printk(KERN_DEBUG"--------%s-------\n",__FUNCTION__);
	//配置GPF4 为输出
	spin_lock(&led_2440p->open_lock);
	if(led_2440p->open_flag==1)
	{
		spin_unlock(&led_2440p->open_lock);
		return 0;
	}
	led_2440p->open_flag=1;
	spin_unlock(&led_2440p->open_lock);
	//第一次初始化所有的gpio引脚
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
	char val;
	printk(KERN_DEBUG"--------%s-------\n",__FUNCTION__);
    if(copy_from_user(&val, data, 1)) 
    	return -EFAULT;
    //传进来的参数必须为字符'0'or'1'
    if(val!='0'&&val!='1')
    	return len;
    write_lock(led_2440p->rwlock);
	switch(MINOR(file->f_dentry->d_inode->i_rdev))
	{
		case 0:
			s3c2410_gpio_setpin(S3C2410_GPF4, !(val-'0'));
			s3c2410_gpio_setpin(S3C2410_GPF5, !(val-'0'));
			s3c2410_gpio_setpin(S3C2410_GPF6, !(val-'0'));
			if(!(val-'0')==1)
				led_2440p->led_val=7;
			else
				led_2440p->led_val=0;
		break;
		case 1:
			s3c2410_gpio_setpin(S3C2410_GPF4, !(val-'0'));
			if(!(val-'0')==1)
				led_2440p->led_val|= (1UL<<0);
			else
				led_2440p->led_val&= ~(1UL<<0);
		break;
		
		case 2:
			s3c2410_gpio_setpin(S3C2410_GPF5, !(val-'0'));
			if(!(val-'0')==1)
				led_2440p->led_val|= (1UL<<1);
			else
				led_2440p->led_val&= ~(1UL<<1);
		break;
		
		case 3:
			s3c2410_gpio_setpin(S3C2410_GPF6, !(val-'0'));
			if(!(val-'0')==1)
				led_2440p->led_val|= (1UL<<2);
			else
				led_2440p->led_val&= ~(1UL<<2);
		break;
		default:
			printk("error\n");
		break;
	}
	write_unlock(led_2440p->rwlock);
	return len;
}
static ssize_t led_drv_read(struct file *file, char __user *buf,
				size_t len, loff_t *ppos)
{
	int val;
	printk(KERN_DEBUG"--------%s-------\n",__FUNCTION__);
	read_lock(led_2440p->rwlock);
	switch(MINOR(file->f_dentry->d_inode->i_rdev))
	{
		case 0:
			val=led_2440p->led_val;
		break;
		case 1:
			val= (led_2440p->led_val>>0)& ~(0x1);
		break;
		
		case 2:
			val= (led_2440p->led_val>>1)& ~(0x1);
		break;
		
		case 3:
			val= (led_2440p->led_val>>2)& ~(0x1);
		break;
		default:
			printk("error\n");
		break;
	}
	read_unlock(led_2440p->rwlock);
	if(copy_to_user(buf,&val,1))
		return -EFAULT;
	return 1;
}

static const struct file_operations led_drv_fops = {
	.owner	= THIS_MODULE,
	.write	= led_drv_write,
	.read	= led_drv_read,
	.open	= led_drv_open
};

/*子设备号
*	0 		:	led0	led1	ked2
*	1		:	led0
*	2		:	led1
*	3		:	led2
*/
static int __init led_drv_init(void)
{
	int err=0,i;
	struct class_device *clsdev;
	printk(KERN_DEBUG"--------%s call-------\n",__FUNCTION__);
	led_2440p=(struct led_2440*) kzalloc(sizeof(struct led_2440),GFP_KERNEL);
	if(led_2440p==NULL)
	{
		err = -ENOMEM;
		goto kzallocError;
	}
	memset(led_2440p,0,sizeof(struct led_2440));
	spin_lock_init(&led_2440p->open_lock);
	rwlock_init(&led_2440p->rwlock);
	major = register_chrdev(major,NAME,&led_drv_fops);
	if(major<0)
	{
		err=major;
		printk(KERN_DEBUG"error :major=%d\n",major);
		goto register_chrdevError;
	}
	led_2440p->major = major;
	led_2440p->led_class=class_create(THIS_MODULE,"leds");
	if (IS_ERR(led_2440p->led_class))
	{
		err=PTR_ERR(led_2440p->led_class);
		goto class_createError;
	}
	clsdev=class_device_create(led_2440p->led_class, NULL, MKDEV(major, 0),NULL,"leds0");
	if (IS_ERR(clsdev)) {
		printk(KERN_DEBUG "%s: failed to create device\n",
		       __FUNCTION__);
		err=PTR_ERR(clsdev);
		goto class_device_createError;
	}
	for(i=1;i<LED_GPIO_SIZE;i++)
	{
		clsdev=class_device_create(led_2440p->led_class, NULL, MKDEV(major, i),NULL,"leds%d",i);
		if (IS_ERR(clsdev)) {
			printk(KERN_DEBUG "%s: failed to create device\n",
			       __FUNCTION__);
			err=PTR_ERR(clsdev);
			goto class_device_createError2;
		}
	}
	return 0;
	class_device_createError2:
	i--;
	while(i)
	{
		class_device_destroy(led_2440p->led_class,MKDEV(led_2440p->major, i));
		i--;
	}
	class_device_destroy(led_2440p->led_class,MKDEV(led_2440p->major, 0));
	class_device_createError:
	class_destroy(led_2440p->led_class);
	class_createError:
	unregister_chrdev(led_2440p->major, NAME);
	register_chrdevError:
	kfree(led_2440p);
	kzallocError:
	return err;
}


static void __exit led_drv_exit(void)
{
	int i;
	printk(KERN_DEBUG"--------%s-------\n",__FUNCTION__);
	for(i=1;i<LED_GPIO_SIZE;i++)
		class_device_destroy(led_2440p->led_class,MKDEV(led_2440p->major, i));
	class_device_destroy(led_2440p->led_class,MKDEV(led_2440p->major, 0));
	class_destroy(led_2440p->led_class);
	unregister_chrdev(led_2440p->major,NAME);
	kfree(led_2440p);
}
//include/linux/init.h:228
module_init(led_drv_init);
module_exit(led_drv_exit);



