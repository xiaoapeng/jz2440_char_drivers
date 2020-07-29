


#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <asm/plat-s3c24xx/ts.h>

#include <asm/arch/regs-adc.h>
#include <asm/arch/regs-gpio.h>

struct jz2440ts_regs {
	unsigned long adccon;
	unsigned long adctsc;
	unsigned long adcdly;
	unsigned long adcdat0;
	unsigned long adcdat1;
	unsigned long adcupdn;
};
static char *jz2440ts_name = "jz2440 TouchScreen";


struct jz2440_ts{
	struct input_dev *dev;
	struct clk *adc_clock;
	struct timer_list ts_timer;
	int shift;
};

static struct jz2440_ts ts;
volatile struct jz2440ts_regs* adc_reg;


static inline void enter_wait_pen_down_mode(void)
{
	adc_reg->adctsc = 0xd3;
}

static inline void enter_wait_pen_up_mode(void)
{
	adc_reg->adctsc = 0x1d3;
}

static inline void enter_measure_xy_mode(void)
{
	adc_reg->adctsc = (1<<3)|(1<<2);
}

static inline void start_adc(void)
{
	adc_reg->adccon |= (1<<0);
}
//剔除偏移太大的值
static int s3c_filter_ts(int x[], int y[])
{
#define ERR_LIMIT 10

	int avr_x, avr_y;
	int det_x, det_y;

	avr_x = (x[0] + x[1])/2;
	avr_y = (y[0] + y[1])/2;

	det_x = (x[2] > avr_x) ? (x[2] - avr_x) : (avr_x - x[2]);
	det_y = (y[2] > avr_y) ? (y[2] - avr_y) : (avr_y - y[2]);

	if ((det_x > ERR_LIMIT) || (det_y > ERR_LIMIT))
		return 0;

	avr_x = (x[1] + x[2])/2;
	avr_y = (y[1] + y[2])/2;

	det_x = (x[3] > avr_x) ? (x[3] - avr_x) : (avr_x - x[3]);
	det_y = (y[3] > avr_y) ? (y[3] - avr_y) : (avr_y - y[3]);

	if ((det_x > ERR_LIMIT) || (det_y > ERR_LIMIT))
		return 0;
	
	return 1;
}

static irqreturn_t adc_irq(int irq, void *dev_id)
{
	static int cnt = 0;
	static int x[4], y[4];
	int adcdat0, adcdat1;
	
	
	//printk("转换结束\n");
	adcdat0 =adc_reg->adcdat0;
	adcdat1 = adc_reg->adcdat1;
	//检测是否已经抬起
	if (adc_reg->adcdat0 & (1<<15))
	{	
		//printk("抬起了\n");
		cnt = 0;
		//压力值为0  笔尖已经抬起
		input_report_abs(ts.dev, ABS_PRESSURE, 0);
		input_report_key(ts.dev, BTN_TOUCH, 0);
		input_sync(ts.dev);
		//进入等待按下模式
		enter_wait_pen_down_mode();
	}
	else
	{
		//多次测量取平均值
		x[cnt] = adcdat0 & 0x3ff;
		y[cnt] = adcdat1 & 0x3ff;
		++cnt;
		if (cnt == 4)
		{
			if (s3c_filter_ts(x, y))
			{	
				//上报事件
				//printk(KERN_INFO"x=%d  y=%d\n",(x[0]+x[1]+x[2]+x[3])/4,(y[0]+y[1]+y[2]+y[3])/4);
				input_report_abs(ts.dev, ABS_X, (x[0]+x[1]+x[2]+x[3])/4);
				input_report_abs(ts.dev, ABS_Y, (y[0]+y[1]+y[2]+y[3])/4);
				input_report_abs(ts.dev, ABS_PRESSURE, 1);
				input_report_key(ts.dev, BTN_TOUCH, 1);
				input_sync(ts.dev);
			}
			cnt = 0;
			enter_wait_pen_up_mode();
			//重新打开定时器
			mod_timer(&ts.ts_timer, jiffies + HZ/100);
		}
		else
		{
			//进入xy转化模式
			enter_measure_xy_mode();
			start_adc();
		}		
	}
	
	return IRQ_HANDLED;
}


static irqreturn_t pen_down_up_irq(int irq, void *dev_id)
{
	if (adc_reg->adcdat0 & (1<<15))
	{
		//printk(KERN_INFO"pen up\n");
		//上传压力值和按键事件
		input_report_abs(ts.dev, ABS_PRESSURE, 0);
		input_report_key(ts.dev, BTN_TOUCH, 0);
		input_sync(ts.dev);
		//进入等待按下模式
		enter_wait_pen_down_mode();
	}
	else
	{
		//printk(KERN_INFO"pen down\n");
		//enter_wait_pen_up_mode();
		//进入xy轴转化模式
		enter_measure_xy_mode();
		start_adc();
	}
	return IRQ_HANDLED;
}

static void s3c_ts_timer_function(unsigned long data)
{
	//如果已经抬起直接上报事件
	if (adc_reg->adcdat0 & (1<<15))
	{
		//无压力值
		input_report_abs(ts.dev, ABS_PRESSURE, 0);
		//按键为抬起
		input_report_key(ts.dev, BTN_TOUCH, 0);
		input_sync(ts.dev);
		enter_wait_pen_down_mode();
	}
	else
	{
		//设置为xy转化模式
		enter_measure_xy_mode();
		//启动adc
		start_adc();
	}
}


static int jz2440ts_probe(struct platform_device *pdev)
{
	int ret=0;
	struct s3c2410_ts_mach_info *info;
	struct input_dev *input_dev;
	//获取平台数据
	info = (struct s3c2410_ts_mach_info *)pdev->dev.platform_data;
	if (!info)
	{
		printk(KERN_ERR "Hm... too bad : no platform data for ts\n");
		return -EINVAL;
	}
	//1.设置输入子系统结构体的一些参数
	memset(&ts,0,sizeof(struct jz2440_ts));
	//1.1分配input_dev结构体
	input_dev = input_allocate_device();
	if(!input_dev)
	{
		printk(KERN_ERR "Unable to allocate the input device !!\n");
		return -ENOMEM;
	}
	ts.dev=input_dev;
	//1.2设置input_dev结构体
	//1.2.1能产生哪些事件
	set_bit(EV_KEY,ts.dev->evbit);
	set_bit(EV_ABS,ts.dev->evbit);
	
	//1.2.2能产生按键事件里的哪些事件
	set_bit(BTN_TOUCH,ts.dev->keybit);

	input_set_abs_params(ts.dev,ABS_X,0,0x3ff,0,0);
	input_set_abs_params(ts.dev,ABS_Y,0,0x3ff,0,0);
	input_set_abs_params(ts.dev,ABS_PRESSURE,0,1,0,0);
	
	//ts.dev->id.bustype = BUS_RS232;
	ts.shift = info->oversampling_shift;


	//2.进行硬件的设置 
	//2.1使能ADC
	ts.adc_clock = clk_get(NULL, "adc");
	if (IS_ERR(ts.adc_clock)) {
		printk(KERN_ERR "failed to get adc clock source\n");
		ret = PTR_ERR(ts.adc_clock);
		goto clk_get_adc_error;
	}
	clk_enable(ts.adc_clock);
	//2.3寄存器映射
	adc_reg = ioremap(S3C2410_PA_ADC,sizeof(struct jz2440ts_regs));
	if (adc_reg == NULL) {
		printk(KERN_ERR "Failed to remap register block\n");
		ret=ENOMEM;
		goto ioremap_adc_regs_error;
	}
	//2.4寄存器设置

   /* 	ADCCON:
	*	bit[15] 	只读
	*	bit[14] 	使能预分频器			0x1
	*
	*	bit[13:6]	PCLK = 50MHZ
	*				A/D转换器频率=      PCLK/(bit[13:6]+1) = 1MHz 
	*				bit[13:6] = 49
	*	bit[0]		如果为1就会使能 这里先设置为 0x0
	*/
	if ((info->presc&0xff) > 0)
		adc_reg->adccon = (1<<14)|((info->presc&0xff)<<6);
	//2.5注册中断
	if(request_irq(IRQ_TC,pen_down_up_irq,IRQF_SAMPLE_RANDOM,"ts_pen",NULL))
	{
		printk(KERN_ERR "ts_drv_2440.c: Could not allocate ts IRQ_TC !\n");
		ret = -EIO;
		goto request_irq_IRQ_TC_error;

	}
	if (request_irq(IRQ_ADC,adc_irq,IRQF_SAMPLE_RANDOM,"adc",NULL))
	{
		printk(KERN_ERR "ts_drv_2440.c: Could not allocate ts IRQ_ADC !\n");
		ret = -EIO;
		goto request_irq_IRQ_ADC_error;
	}
	//2.6 设置adc转化延时值 （待电压稳定再检测）
	if ((info->delay&0xffff) > 0)
		adc_reg->adcdly= (info->delay &0xffff);
	//2.7配置定时器 使用定时器处理长按,滑动的情况
	init_timer(&ts.ts_timer);
	ts.ts_timer.function = s3c_ts_timer_function;
	add_timer(&ts.ts_timer);
	
	

	enter_wait_pen_down_mode();

	ret=input_register_device(ts.dev);
	if(ret)
	{
		printk(KERN_ERR "ts_drv_2440.c: input_register_device error !\n");
	}
	
	return ret;
	del_timer(&ts.ts_timer);
	free_irq(IRQ_ADC,NULL);
	request_irq_IRQ_ADC_error:
	free_irq(IRQ_TC,NULL);
	request_irq_IRQ_TC_error:
	iounmap(adc_reg);
	ioremap_adc_regs_error:
	clk_disable(ts.adc_clock);
	clk_put(ts.adc_clock);
	clk_get_adc_error:
	input_free_device(ts.dev);
	return ret;
}

static int jz2440ts_remove(struct platform_device *pdev)
{
	input_unregister_device(ts.dev);
	del_timer(&ts.ts_timer);
	free_irq(IRQ_ADC,NULL);
	free_irq(IRQ_TC,NULL);
	iounmap(adc_reg);
	clk_disable(ts.adc_clock);
	clk_put(ts.adc_clock);
	input_free_device(ts.dev);

	return 0;

}

static struct platform_driver jz2440ts_driver = {
       .driver         = {
	       .name   = "jz2440-ts",
	       .owner  = THIS_MODULE,
       },
       .probe          = jz2440ts_probe,
       .remove         = jz2440ts_remove,
};

static int __init jz2440_ts_init(void)
{
	
	return platform_driver_register(&jz2440ts_driver);
}

static void __exit jz2440_ts_exit(void)
{
	return platform_driver_unregister(&jz2440ts_driver);
}


module_init(jz2440_ts_init);
module_exit(jz2440_ts_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("simon <liziandpym.xyz>");
MODULE_DESCRIPTION("ts drv");



