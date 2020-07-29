KERN_DIR := /home/plz/shared/02/linux_JZ2440
INSDIR	 := /home/plz/nfs/fs_mini_mdev/drv


all :
	make -C $(KERN_DIR) M=`pwd`  modules V=0 
clean :
	make -C $(KERN_DIR) M=`pwd` clean
	rm -rf modules.order
install :
	cp *.ko $(INSDIR)
	cp */*.ko $(INSDIR)

obj-m += led_2440.o
#obj-m += key_2440.o
obj-m += keyirq_2440.o
obj-m += keyinput_2440.o
obj-m += led_bus_dev_drv/led_drv_2440.o
obj-m += led_bus_dev_drv/led_dev_2440.o
obj-m += lcd_2440.o
obj-m += ts_drv_2440.o
obj-m += ts_dev_2440.o


########################
#宏展开部分


showmacro : macro_expansion.o
	clear
	@grep -a "^#\{5\}.*#\{5\}$$" macro_expansion.o
	
macro_expansion.o : macro_expansion.c
	make -C $(KERN_DIR) M=`pwd`  modules V=0  obj-m:=macro_expansion.o
########################






