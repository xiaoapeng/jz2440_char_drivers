#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/div64.h>

#include <asm/mach/map.h>
#include <asm/arch/regs-lcd.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/fb.h>


struct s3c_lcd_regs {
	unsigned long	lcdcon1;
	unsigned long	lcdcon2;
	unsigned long	lcdcon3;
	unsigned long	lcdcon4;
	unsigned long	lcdcon5;
    unsigned long	lcdsaddr1;
    unsigned long	lcdsaddr2;
    unsigned long	lcdsaddr3;
    unsigned long	redlut;
    unsigned long	greenlut;
    unsigned long	bluelut;
    unsigned long	reserved[9]; 	//保留 
    unsigned long	dithmode;
    unsigned long	tpal;
    unsigned long	lcdintpnd;
    unsigned long	lcdsrcpnd;
    unsigned long	lcdintmsk;
    unsigned long	lpcsel;
};




#define XRES 			(480)
#define YRES  			(272)
#define BITS_PER_PIXEL 	(16)
 
static volatile unsigned long *gpbcon;
static volatile unsigned long *gpbdat;
static volatile unsigned long *gpccon;
static volatile unsigned long *gpdcon;
static volatile unsigned long *gpgcon;
static volatile struct s3c_lcd_regs * lcd_regs;
static u32 pseudo_palette[16];


static inline unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}


static int s3c_lcdfb_setcolreg(unsigned int regno, unsigned int red,
			     unsigned int green, unsigned int blue,
			     unsigned int transp, struct fb_info *info)
{
	unsigned int val;
	//不能大于数组（调色板）长度
	if (regno > 16)
		return 1;

	/* 用red,green,blue三原色构造出val */
	val  = chan_to_field(red,	&info->var.red);
	val |= chan_to_field(green, &info->var.green);
	val |= chan_to_field(blue,	&info->var.blue);
	
	//((u32 *)(info->pseudo_palette))[regno] = val;
	pseudo_palette[regno] = val;
	return 0;
}


struct fb_info* s3c2440_lcd;
static struct fb_ops s3c2440_lcd_ops = {
	.owner		= THIS_MODULE,	
	.fb_setcolreg	= s3c_lcdfb_setcolreg,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};
static int __init lcd_2440fb_init(void)
{
	printk(KERN_INFO"参数构造\n");
	/* 1.分配一个fb_info */
	s3c2440_lcd = framebuffer_alloc(0,NULL);
	/* 2.  设置 */
	/* 2.1 设置固定参数*/
	
	strcpy(s3c2440_lcd->fix.id,"mylcd");
	s3c2440_lcd->fix.smem_len=XRES*YRES*BITS_PER_PIXEL/8;
	s3c2440_lcd->fix.type=FB_TYPE_PACKED_PIXELS;
	s3c2440_lcd->fix.visual=FB_VISUAL_TRUECOLOR;
	s3c2440_lcd->fix.line_length=XRES*2;
	/* 2.2 设置可变参数*/	
	s3c2440_lcd->var.xres =XRES;
	s3c2440_lcd->var.yres =YRES;
	s3c2440_lcd->var.xres_virtual=XRES;
	s3c2440_lcd->var.yres_virtual=YRES;
	s3c2440_lcd->var.bits_per_pixel =BITS_PER_PIXEL;
	/////////////// 
	/* 565 */
	s3c2440_lcd->var.red.offset=11;
	s3c2440_lcd->var.red.length=5;
	
	s3c2440_lcd->var.green.offset=5;
	s3c2440_lcd->var.green.length=6;
	
	s3c2440_lcd->var.blue.offset=0;
	s3c2440_lcd->var.blue.length=5;
	s3c2440_lcd->var.activate= FB_ACTIVATE_NOW;
	
	/* 2.1 设置操作函数*/
	s3c2440_lcd->fbops=&s3c2440_lcd_ops;
	/* 2.1 其他设置*/
	s3c2440_lcd->pseudo_palette=  pseudo_palette	;/*调色板设置*/
	//s3c2440_lcd->screen_base = 	;/* 显存的虚拟地址*/
	s3c2440_lcd->screen_size = XRES*YRES*BITS_PER_PIXEL/8 ;/* 显存的大小*/
	
	/* 3.硬件相关初始化*/
	/*	3.1 设置引脚为lcd*/
	printk(KERN_INFO"初始化引脚\n");

	gpbcon = ioremap(0x56000010,8);
	gpbdat = gpbcon + 1;
	gpccon = ioremap(0x56000020,4);
	gpdcon = ioremap(0x56000030,4);
	gpgcon = ioremap(0x56000060,4);


	*gpccon = 0xaaaaaaaa;
	*gpdcon = 0xaaaaaaaa;
	
	*gpbcon &= ~(3<<0);  /*配置背光控制引脚*/
	*gpbcon |= 0x1;
	*gpbdat &= ~(0x1);	 /* 暂时输出低电平*/

	*gpgcon |= (3<<8);   /* 电源使能 */
		
	/*	3.2 设置lcd控制器*/
	printk(KERN_INFO"初始化LCD控制器\n");
	lcd_regs = ioremap(0X4D000000,sizeof(lcd_regs[0]));
	/* bit[17:8]: VCLK = HCLK / [(CLKVAL+1) x 2], LCD手册P14
	 *            10MHz(100ns) = 100MHz / [(CLKVAL+1) x 2]
	 *            CLKVAL = 4
	 * bit[6:5]: 0b11, TFT LCD
	 * bit[4:1]: 0b1100, 16 bpp for TFT
	 * bit[0]  : 0 = Disable the video output and the LCD control signal.
	 */
	lcd_regs->lcdcon1  = (4<<8) | (3<<5) | (0x0c<<1);
	/* 垂直方向的时间参数
	 * bit[31:24]: VBPD, VSYNC之后再过多长时间才能发出第1行数据
	 *             LCD手册 T0-T2-T1=4
	 *             VBPD=3
	 * bit[23:14]: 多少行, 272, 所以LINEVAL=320-1=319
	 * bit[13:6] : VFPD, 发出最后一行数据之后，再过多长时间才发出VSYNC
	 *             LCD手册T2-T5=322-320=2, 所以VFPD=2-1=1
	 * bit[5:0]  : VSPW, VSYNC信号的脉冲宽度, LCD手册T1=1, 所以VSPW=1-1=0
	 */
	lcd_regs->lcdcon2  = (1<<24) | (271<<14) | (1<<6) | (9);
	/* 水平方向的时间参数
	 * bit[25:19]: HBPD, VSYNC之后再过多长时间才能发出第1行数据
	 *             LCD手册 T6-T7-T8=17
	 *             HBPD=16
	 * bit[18:8]: 多少列, 480, 所以HOZVAL=240-1=239
	 * bit[7:0] : HFPD, 发出最后一行里最后一个象素数据之后，再过多长时间才发出HSYNC
	 *             LCD手册T8-T11=251-240=11, 所以HFPD=11-1=10
	 */
	lcd_regs->lcdcon3 = (1<<19) | (479<<8) | (1);

	/* 水平方向的同步信号
	 * bit[7:0]	: HSPW, HSYNC信号的脉冲宽度, LCD手册T7=5, 所以HSPW=5-1=4
	 */	
	lcd_regs->lcdcon4 = 40;
	/* 信号的极性 
	 * bit[11]: 1 = 565 format
	 * bit[10]: 0 = The video data is fetched at VCLK falling edge
	 * bit[9] : 1 = HSYNC信号要反转,即低电平有效 
	 * bit[8] : 1 = VSYNC信号要反转,即低电平有效 
	 * bit[6] : 0 = VDEN不用反转
	 * bit[3] : 0 = PWREN输出0
	 * bit[1] : 0 = BSWP
	 * bit[0] : 1 = HWSWP 2440手册P413
	 */
	lcd_regs->lcdcon5 = (1<<11) | (0<<10) | (1<<9) | (1<<8) | (1<<0);
	
	/*	3.3 分配显存*/
	s3c2440_lcd->screen_base = dma_alloc_writecombine(NULL, s3c2440_lcd->fix.smem_len, &s3c2440_lcd->fix.smem_start, GFP_KERNEL);
	
	lcd_regs->lcdsaddr1  = (s3c2440_lcd->fix.smem_start >> 1) & ~(3<<30);
	lcd_regs->lcdsaddr2  = ((s3c2440_lcd->fix.smem_start + s3c2440_lcd->fix.smem_len) >> 1) & 0x1fffff;
	lcd_regs->lcdsaddr3  = (480*16/16);  /* 一行的长度(单位: 2字节) */	
	
	//s3c_lcd->fix.smem_start = xxx;  /* 显存的物理地址 */
	/* 启动LCD */
	printk(KERN_INFO"启动LCD\n");
	lcd_regs->lcdcon1 |= (1<<0); /* 使能LCD控制器 */
	lcd_regs->lcdcon5 |= (1<<3); /* 使能LCD本身 */
	*gpbdat |= 1;     /* 输出高电平, 使能背光 */		
	/* 4.注册 */
	printk(KERN_INFO"注册\n");
	register_framebuffer(s3c2440_lcd);
	printk(KERN_INFO"结束\n");


	return 0;
}
static void __exit lcd_2440fb_exit(void)
{
	unregister_framebuffer(s3c2440_lcd);
	lcd_regs->lcdcon1 &= ~(1<<0); /* 关闭LCD本身 */
	*gpbdat &= ~1;     /* 关闭背光 */
	dma_free_writecombine(NULL, s3c2440_lcd->fix.smem_len, s3c2440_lcd->screen_base, s3c2440_lcd->fix.smem_start);
	iounmap(lcd_regs);
	iounmap(gpbcon);
	iounmap(gpccon);
	iounmap(gpdcon);
	iounmap(gpgcon);
	framebuffer_release(s3c2440_lcd);


}

module_init(lcd_2440fb_init);
module_exit(lcd_2440fb_exit);
MODULE_AUTHOR("plz<liziandpym.xyz>");
MODULE_DESCRIPTION("Framebuffer driver for the s3c2440");
MODULE_LICENSE("GPL");






