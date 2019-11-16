//使用中断的方式读取按键

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/arch/regs-gpio.h>
#include <asm/irq.h>
#include <linux/irq.h>
#include <linux/memory_hotplug.h>
#include <asm/string.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <asm/poll.h>


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
	int key_val;			//key状态
	spinlock_t open_lock; 	//自旋锁变量
	rwlock_t   rwlock;		//读写锁
	wait_queue_head_t key_queue; 
	int wait_queue_flag;	
};
struct key_irq_mode
{
	unsigned long irq;		//中断号
	unsigned long flag; 	//触发方式
	irq_handler_t handler; 	//中断处理函数 
	char* drv_name;			//中断名 
	
};
struct key_pin_mode
{
	unsigned long pin;
	unsigned long mode;
};



static struct key_2440 *key_2440p;
extern struct key_irq_mode key_irq_moded[];
extern struct key_pin_mode key_pin_moded[];

static irqreturn_t key_interrupt(int irq, void *dev_id)
{
	int  subscript=(int) dev_id;
	//printk(KERN_DEBUG"irq=%ud \n devname=%s\n",irq,key_irq_moded[subscript].drv_name);
	if(s3c2410_gpio_getpin(key_pin_moded[subscript].pin))
		key_2440p->key_val =(subscript+1);	
	else
		key_2440p->key_val =0-(subscript+1);
	key_2440p->wait_queue_flag=1;
	wake_up(&key_2440p->key_queue);
	return 0;
}






struct key_irq_mode key_irq_moded[]={
{IRQ_EINT0,IRQT_BOTHEDGE,key_interrupt,"S2"},
{IRQ_EINT2,IRQT_BOTHEDGE,key_interrupt,"S3"},
{IRQ_EINT11,IRQT_BOTHEDGE,key_interrupt,"S4"},
{IRQ_EINT19,IRQT_BOTHEDGE,key_interrupt,"S5"},
};
#define KEY_GPIO_SIZE (sizeof(key_irq_moded)/sizeof(key_irq_moded[0]))

struct key_pin_mode key_pin_moded[KEY_GPIO_SIZE]={
{S3C2410_GPF0,S3C2410_GPF4_INP},
{S3C2410_GPF2,S3C2410_GPF2_INP},
{S3C2410_GPG3,S3C2410_GPG3_INP},
{S3C2410_GPG11,S3C2410_GPG11_INP}
};







static int key_drv_open(struct inode *inode, struct file *file)
{
	int i,irqval;
	printk(KERN_DEBUG"--------%s-------\n",__FUNCTION__);
	//配置GPF4 为输出
	spin_lock(&key_2440p->open_lock);
	if(key_2440p->open_flag>0)
	{
		key_2440p->open_flag++;
		spin_unlock(&key_2440p->open_lock);
		return 0;
	}
	key_2440p->open_flag=1;
	spin_unlock(&key_2440p->open_lock);
	//进行中断的注册
	for(i=0;i<KEY_GPIO_SIZE;i++)
	{
		irqval = request_irq(key_irq_moded[i].irq,key_irq_moded[i].handler,key_irq_moded[i].flag,key_irq_moded[i].drv_name,(void *)i);
		if (irqval) {
			printk(KERN_DEBUG "3c507: unable to get IRQ %d (irqval=%d).\n", key_irq_moded[i].irq, irqval);
			return -EAGAIN;
		}
	}
	return 0;
}
static int key_release (struct inode *inodep, struct file *filp)
{
	int i;
	spin_lock(&key_2440p->open_lock);
	key_2440p->open_flag--;
	if(key_2440p->open_flag>0)
	{
		spin_unlock(&key_2440p->open_lock);
		return 0;
	}
	spin_unlock(&key_2440p->open_lock);

	for(i=0;i<KEY_GPIO_SIZE;i++)
	{
		free_irq(key_irq_moded[i].irq,i);
	}
	return 0;
}


//使用位图的方式将数据回传
static ssize_t key_drv_read(struct file *file, char __user *buf,
				size_t len, loff_t *ppos)
{

	
	unsigned long flags;
	printk(KERN_DEBUG"--------%s-------\n",__FUNCTION__);
	wait_event(key_2440p->key_queue,key_2440p->wait_queue_flag);
	
	//下面应该有一个关中断的过程
  	local_irq_save(flags);
	if(copy_to_user(buf,&key_2440p->key_val,4))
	{
		return -EFAULT;
	}
	key_2440p->wait_queue_flag=0;
	local_irq_restore(flags);
	return 4;
}
static unsigned int key_poll(struct file *filp, struct poll_table_struct *wait)
{
	unsigned int mask =0;
	poll_wait(filp,&(key_2440p->key_queue),wait);

	if(key_2440p->wait_queue_flag)
	{
		mask |=	POLLIN|POLLRDNORM;
	}
	return mask;
}

static const struct file_operations key_drv_fops = {
	.owner	= THIS_MODULE,
	.read	= key_drv_read,
	.open	= key_drv_open,
	.release= key_release,
	.poll 	= key_poll
};

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
	init_waitqueue_head(&key_2440p->key_queue); 
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



