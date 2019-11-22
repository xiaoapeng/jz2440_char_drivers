/*
 * Driver for keys on GPIO lines capable of generating interrupts.
 *
 * Copyright 2005 Phil Blundell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/version.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/gpio_keys.h>

#include <asm/gpio.h>

struct key_irq_mode
{
	unsigned int irq;				//中断号
	unsigned long flag; 			//触发方式
	irq_handler_t handler; 			//中断处理函数 
	char* drv_name;					//中断名 
	unsigned int key_val;			//按键值
	unsigned long pin;				//gpio值
	struct timer_list key_timer;	//用于消抖的定时器
	
};
static struct key_irq_mode key_irq_moded[];
static int timer_flag=1;		//让新加的定时器不立即执行
static struct input_dev *keyinput_dev;
static irqreturn_t key_interrupt(int irq, void *dev_id);



static struct key_irq_mode key_irq_moded[]={
{IRQ_EINT0,IRQT_BOTHEDGE,key_interrupt,"S2",KEY_L,S3C2410_GPF0},
{IRQ_EINT2,IRQT_BOTHEDGE,key_interrupt,"S3",KEY_S,S3C2410_GPF2},
{IRQ_EINT11,IRQT_BOTHEDGE,key_interrupt,"S4",KEY_ENTER,S3C2410_GPG3},
{IRQ_EINT19,IRQT_BOTHEDGE,key_interrupt,"S5",KEY_LEFTSHIFT,S3C2410_GPG11},
};
#define KEY_GPIO_SIZE (sizeof(key_irq_moded)/sizeof(key_irq_moded[0]))

//static struct keyinput_2440 *keyinput_2440p;

static irqreturn_t key_interrupt(int irq, void *dev_id)
{
	int  subscript=(int) dev_id;
	//printk(KERN_DEBUG"irq=%ud \n devname=%s\n",irq,key_irq_moded[subscript].drv_name);
	timer_flag=0;
	mod_timer(&key_irq_moded[subscript].key_timer, jiffies+HZ/100);
	return IRQ_RETVAL(IRQ_HANDLED);
}
void key_timerfun(unsigned long data)
{
	
	unsigned long subscript=data;
	int state;
	if(timer_flag)
	{
		return ;
	}
	state = !s3c2410_gpio_getpin(key_irq_moded[subscript].pin);	
	input_event(keyinput_dev, EV_KEY, key_irq_moded[subscript].key_val,state);
	input_sync(keyinput_dev);

}


static irqreturn_t gpio_keys_isr(int irq, void *dev_id)
{
	int i;
	struct platform_device *pdev = dev_id;
	struct gpio_keys_platform_data *pdata = pdev->dev.platform_data;
	struct input_dev *input = platform_get_drvdata(pdev);

	for (i = 0; i < pdata->nbuttons; i++) {
		struct gpio_keys_button *button = &pdata->buttons[i];
		int gpio = button->gpio;

		if (irq == gpio_to_irq(gpio)) {
			unsigned int type = button->type ?: EV_KEY;
			int state = (gpio_get_value(gpio) ? 1 : 0) ^ button->active_low;

			input_event(input, type, button->code, !!state);
			input_sync(input);
		}
	}
	return IRQ_HANDLED;
}
static int __init gpio_keys_init(void)
{
	int error,i,irqval;
	#if 0
	keyinput_2440p = (struct keyinput_2440*)kzalloc(sizeof(struct keyinput_2440),GFP_KERNEL);
	if(key_2440p==NULL)
	{
		error = -ENOMEM;
		return error;
	}
	#endif
	//##主线##向核心层申请一个input_dev结构体
	//printk(KERN_ERR"!!!!!!!!\n");
	keyinput_dev = input_allocate_device();
	if (!keyinput_dev)
	{
		error = -ENOMEM;
		goto input_allocate_deviceError;
	}

	//##主线##对input_dev进行一系列初始化 
	set_bit(EV_KEY,keyinput_dev->evbit);
	set_bit(KEY_L,keyinput_dev->keybit);
	set_bit(KEY_S,keyinput_dev->keybit);
	set_bit(KEY_ENTER,keyinput_dev->keybit);
	set_bit(KEY_LEFTSHIFT,keyinput_dev->keybit);
	
	keyinput_dev->name = "keyinput_2440";
	keyinput_dev->phys = "gpio-keys/input0";
	//设置进行比对的id
	keyinput_dev->id.bustype = BUS_HOST;
	//keyinput_dev->id.vendor = 0x0001;
	//keyinput_dev->id.product = 0x0001;
	//keyinput_dev->id.version = 0x0100;

	
	//##主线##提交该结构体
	error = input_register_device(keyinput_dev);
	if (error) {
		printk(KERN_ERR "Unable to register gpio-keys input device\n");
		goto input_register_deviceError;
	}
	//##主线##初始化硬件操作
	//中断配置
	for(i=0;i<KEY_GPIO_SIZE;i++)
	{
		irqval = request_irq(key_irq_moded[i].irq,key_irq_moded[i].handler,key_irq_moded[i].flag,key_irq_moded[i].drv_name,(void *)i);
		if (irqval) {
			printk(KERN_DEBUG "3c507: unable to get IRQ %u (irqval=%d).\n", key_irq_moded[i].irq, irqval);
			error = -EAGAIN;
			goto request_irqError;
		}
	}
	for(i=0;i<KEY_GPIO_SIZE;i++)
	{
		init_timer(&key_irq_moded[i].key_timer);
		key_irq_moded[i].key_timer.data=i;
		key_irq_moded[i].key_timer.function=key_timerfun;
		add_timer(&key_irq_moded[i].key_timer);
	}
	return 0;
	request_irqError:
	for(--i;i>=0;i--)
	{
		free_irq(key_irq_moded[i].irq,(void *)i);
	}
	input_unregister_device(keyinput_dev);
	input_register_deviceError:
	input_free_device(keyinput_dev);
	input_allocate_deviceError:
	//kfree(keyinput_2440p);
	return error;
}

static void __exit gpio_keys_exit(void)
{
	int i;
	for(i=0;i<KEY_GPIO_SIZE;i++)
	{
		del_timer(&key_irq_moded[i].key_timer);
		free_irq(key_irq_moded[i].irq,(void *)i);
	}
	input_unregister_device(keyinput_dev);
	input_free_device(keyinput_dev);
	//kfree(keyinput_2440p);
}

module_init(gpio_keys_init);
module_exit(gpio_keys_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phil Blundell <pb@handhelds.org>");
MODULE_DESCRIPTION("Keyboard driver for CPU GPIOs");
