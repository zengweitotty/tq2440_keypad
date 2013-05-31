obj-m := tq2440_keypad.o
	
KDIR := /opt/EmbedSky/linux-2.6.30.4
all:
	make -C $(KDIR) M=$(PWD) 
clean:
	rm -f *.o *.mod.o *.mod.c *.symvers *.ko *.order
