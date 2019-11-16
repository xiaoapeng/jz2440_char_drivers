KERN_DIR := /home/plz/shared/02/linux_JZ2440
INSDIR	 := /home/plz/nfs/fs_mini_mdev/drv


all :
	make -C $(KERN_DIR) M=`pwd` modules 
clean :
	make -C $(KERN_DIR) M=`pwd` clean
	rm -rf modules.order
	
install :
	cp *.ko $(INSDIR)

obj-m += led_2440.o
#obj-m += key_2440.o
obj-m += keyirq_2440.o




