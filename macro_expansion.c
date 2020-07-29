
//这里必须要加入该宏所依赖的头文件
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <asm/arch/fb.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>

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

#include <asm/arch/regs-serial.h>
#include <asm/arch/udc.h>

#include <asm/plat-s3c24xx/devs.h>
#include <asm/plat-s3c24xx/cpu.h>
#include <asm/plat-s3c24xx/ts.h>

#include <asm/arch/regs-spi.h>





#define   PRINT_MACRO(x)   "\n######"#x"######\n" 
#define   PRINT_DEF(x)	printk(KERN_INFO"%s\n",PRINT_MACRO(x))

MODULE_LICENSE("GPL");
static int __init macro_expansion_init(void)
{
	//在这里填上你想展开的宏：注意如果有参数，一定要加参数
	PRINT_DEF((__iomem));
}


static void __exit macro_expansion_exit(void){	}
module_init(macro_expansion_init);
module_exit(macro_expansion_exit);



