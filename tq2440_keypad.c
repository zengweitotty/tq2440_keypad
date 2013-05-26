/*
    file Name:      tq2440_keypad.c
    Author:         zengweitotty
    version:        V1.0
    Data:           2013/05/25
    Email:          zengweitotty@gmail.com
    Description     a general key pad consist of 4 key
*/
#include <linux/init.h> //use for function module_init,module_exit
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>   //use for struct file and struct file_operations
#include <linux/ioport.h>   //use for request_mem_region
#include <asm/io.h>     //use for ioremap iounmap
#include <linux/interrupt.h>    //use for probe_irq_on or probe_irq_off,request_irq,free_irq
#include <asm/delay.h>  //use for udelay
#include <linux/sched.h>    //

#define KEY_NUM	4

struct tq2440_key{
	int irq;
	int index;
	int state;
}

static int __devinit key_probe(struct platform_device *pdev){
		
}
static int __devexit key_remove(struct platform_device *pdev){
		
}

static struct platform_driver key_device_driver = {
	.probe = key_probe,
	.remove = key_remove,
	.driver = {
		.name = "tq2440_keypad",
		.owner = THIS_MODULE,
	}
};

static int __init key_init(void){
	return platform_driver_register(key_device_driver);		
}
static void __exit key_exit(void){
	return platform_driver_unregister(key_device_driver);		
}

MODULE_AUTHOR("zengweitotty");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("TQ2440 KEY PAD");
