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



#define NAME 	"key_drvs"

MODULE_AUTHOR("yang ming peng<liziandpym@qq.com>");
MODULE_DESCRIPTION("JZ2440 KEY GPIO Pin Driver");
MODULE_LICENSE("GPL");


static int major;
module_param(major, int, 0);
MODULE_PARM_DESC(major, "Major device number");


/*
*key0	EINT0	GPF0
*key1	EINT2	GPF2
*key2	EINT11	GPG3
*key3	EINT19	GPG11
**/
struct key_2440
{
	struct class *key_class;
	int major;				//主函数状态
	int open_flag;			//open函数
	unsigned long key_val;	//key状态
	spinlock_t open_lock; 	//自旋锁变量
	rwlock_t   rwlock;		//读写锁
};
struct key_pin_mode
{
	unsigned long pin;
	unsigned long mode;
};



static struct key_2440 *key_2440p;
struct key_pin_mode key_pin_moded[]={
{S3C2410_GPF0,S3C2410_GPF4_INP},
{S3C2410_GPF2,S3C2410_GPF2_INP},
{S3C2410_GPG3,S3C2410_GPG3_INP},
{S3C2410_GPG11,S3C2410_GPG11_INP},
};
#define KEY_GPIO_SIZE (sizeof(key_pin_moded)/sizeof(key_pin_moded[0]))






static int key_drv_open(struct inode *inode, struct file *file)
{
	int i;
	printk(KERN_DEBUG"--------%s-------\n",__FUNCTION__);
	//配置GPF4 为输出
	spin_lock(&key_2440p->open_lock);
	if(key_2440p->open_flag==1)
	{
		spin_unlock(&key_2440p->open_lock);
		return 0;
	}
	key_2440p->open_flag=1;
	spin_unlock(&key_2440p->open_lock);
	//第一次初始化所有的gpio引脚
	for(i=0;i<KEY_GPIO_SIZE;i++)
	{
		s3c2410_gpio_cfgpin(key_pin_moded[i].pin,key_pin_moded[i].mode);
	}
	return 0;
}
#if 0
static ssize_t key_drv_write(struct file *file, const char __user *data,
				 size_t len, loff_t *ppos)
{
	char val;
	printk(KERN_DEBUG"--------%s-------\n",__FUNCTION__);
    if(copy_from_user(&val, data, 1)) 
    	return -EFAULT;
    //传进来的参数必须为字符'0'or'1'
    if(val!='0'&&val!='1')
    	return len;
    write_lock(key_2440p->rwlock);
	switch(MINOR(file->f_dentry->d_inode->i_rdev))
	{
		case 0:
			s3c2410_gpio_setpin(S3C2410_GPF4, !(val-'0'));
			s3c2410_gpio_setpin(S3C2410_GPF5, !(val-'0'));
			s3c2410_gpio_setpin(S3C2410_GPF6, !(val-'0'));
			if(!(val-'0')==1)
				key_2440p->key_val=7;
			else
				key_2440p->key_val=0;
		break;
		case 1:
			s3c2410_gpio_setpin(S3C2410_GPF4, !(val-'0'));
			if(!(val-'0')==1)
				key_2440p->key_val|= (1UL<<0);
			else
				key_2440p->key_val&= ~(1UL<<0);
		break;
		
		case 2:
			s3c2410_gpio_setpin(S3C2410_GPF5, !(val-'0'));
			if(!(val-'0')==1)
				key_2440p->key_val|= (1UL<<1);
			else
				key_2440p->key_val&= ~(1UL<<1);
		break;
		
		case 3:
			s3c2410_gpio_setpin(S3C2410_GPF6, !(val-'0'));
			if(!(val-'0')==1)
				key_2440p->key_val|= (1UL<<2);
			else
				key_2440p->key_val&= ~(1UL<<2);
		break;
		default:
			printk("error\n");
		break;
	}
	write_unlock(key_2440p->rwlock);
	return len;
}
#endif //0
//使用位图的方式将数据回传
static ssize_t key_drv_read(struct file *file, char __user *buf,
				size_t len, loff_t *ppos)
{
	int i;
	printk(KERN_DEBUG"--------%s-------\n",__FUNCTION__);
	
	write_lock(key_2440p->rwlock);
	for(i=0;i<KEY_GPIO_SIZE;i++)
	{
		if(s3c2410_gpio_getpin(key_pin_moded[i].pin))
			key_2440p->key_val &= ~(0x1<<i);	
		else
			key_2440p->key_val |= (0x1<<i);
	}
	write_unlock(key_2440p->rwlock);
	
	read_lock(key_2440p->rwlock);
	if(copy_to_user(buf,&key_2440p->key_val,1))
	{
		read_unlock(key_2440p->rwlock);
		return -EFAULT;
	}
	read_unlock(key_2440p->rwlock);
	return 1;
}

static const struct file_operations key_drv_fops = {
	.owner	= THIS_MODULE,
	.read	= key_drv_read,
	.open	= key_drv_open
};

/*子设备号
*	0 		:	key0	key1	ked2
*	1		:	key0
*	2		:	key1
*	3		:	key2
*/
static int __init key_drv_init(void)
{
	int err=0;
	struct class_device *clsdev;
	printk(KERN_DEBUG"--------%s call-------\n",__FUNCTION__);
	key_2440p=(struct key_2440*) kzalloc(sizeof(struct key_2440),GFP_KERNEL);
	if(key_2440p==NULL)
	{
		err = -ENOMEM;
		goto kzallocError;
	}
	memset(key_2440p,0,sizeof(struct key_2440));
	spin_lock_init(&key_2440p->open_lock);
	rwlock_init(&key_2440p->rwlock);
	major = register_chrdev(major,NAME,&key_drv_fops);
	if(major<0)
	{
		err=major;
		printk(KERN_DEBUG"error :major=%d\n",major);
		goto register_chrdevError;
	}
	key_2440p->major = major;
	key_2440p->key_class=class_create(THIS_MODULE,"keys");
	if (IS_ERR(key_2440p->key_class))
	{
		err=PTR_ERR(key_2440p->key_class);
		goto class_createError;
	}
	clsdev=class_device_create(key_2440p->key_class, NULL, MKDEV(major, 0),NULL,"keys0");
	if (IS_ERR(clsdev)) {
		printk(KERN_DEBUG "%s: faikey to create device\n",
		       __FUNCTION__);
		err=PTR_ERR(clsdev);
		goto class_device_createError;
	}

	return 0;
	class_device_createError:
	class_destroy(key_2440p->key_class);
	class_createError:
	unregister_chrdev(key_2440p->major, NAME);
	register_chrdevError:
	kfree(key_2440p);
	kzallocError:
	return err;
}


static void __exit key_drv_exit(void)
{
	printk(KERN_DEBUG"--------%s-------\n",__FUNCTION__);
	class_device_destroy(key_2440p->key_class,MKDEV(key_2440p->major, 0));
	class_destroy(key_2440p->key_class);
	unregister_chrdev(key_2440p->major,NAME);
	kfree(key_2440p);
}
//include/linux/init.h:228
module_init(key_drv_init);
module_exit(key_drv_exit);



