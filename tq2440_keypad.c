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
#include <mach/irqs.h>	//use for irq number
#include <linux/sched.h>    //use for schedule
#include <linux/jiffies.h>	//using for jiffies
#include <linux/input.h>	//use for EV_KEY KEY_1...
#include <linux/platform_device.h>	//use for platform_driver

/*
#undef PDEBUG
#ifdef KEYPAD_DEBUG
	#ifdef __KERNEL__
		#define PDEBUG(fmt,args...)	printk(KERN_DEBUG fmt,##args)
	#else
		#define PDEBUF(fmt,args...) fprintf(stderr,fmt,##args)
	#endif
#else
#define PDEBUG(fmt,args...)
#endif
*/
#define PDEBUG(fmt,args...)	printk(KERN_INFO fmt,##args)

#define KEYBASE	0x56000050
#define GPFCON	0x56000050
#define GPFDAT	0x56000054
#define GPFUP	0x56000058

#define NAME_SIZE	5
#define DEBOUCE	100
#define KEY_NUM	4
#define STATE_IDLE 0
#define STATE_RUN 1
#define STATE_SUSPEND 2

struct tq2440_key{
	char name[NAME_SIZE];
	int irq;
	int index;
	char state;
	char key_value;
	unsigned long key_time;
};

struct tq2440_key *keypad;
struct input_dev *input;
struct tasklet_struct keypad_work;
char keyval[4] = {KEY_1,KEY_2,KEY_3,KEY_4};
int irqval[4] = {IRQ_EINT1,IRQ_EINT4,IRQ_EINT2,IRQ_EINT0};
volatile unsigned long* gpfcon = NULL;
volatile unsigned long* gpfdat = NULL;
volatile unsigned long* gpfup = NULL;

void keypad_do_work(unsigned long data){
	struct tq2440_key* keytemp = (struct tq2440_key*)data;
	input_report_key(input,EV_KEY,keytemp->key_value);
	input_sync(input);
	keytemp->key_time = jiffies;
	PDEBUG("[tq2440_keypad/keypad_do_work]key interrupt occur %s\n",keytemp->name);

}
irqreturn_t key_interrupt(int irq,void* dev_id){
	struct tq2440_key* keytemp = (struct tq2440_key*)dev_id;
	unsigned long j0,j1;
	/* debouce press key */
	j0 = keytemp->key_time;
	j1 = jiffies;
	//tasklet_init(&keypad_work,(void (*)(unsigned long))keypad_do_work,(unsigned long)&keytemp);
	if((j1 - j0)  > msecs_to_jiffies(DEBOUCE)){
		//j1 = jiffies + msecs_to_jiffies(DEBOUCE);
		//while(time_before(jiffies,j1)){
		//	schedule();		
		//}
		//tasklet_schedule(&keypad_work);
		//INIT_WORK(&keypad_work,(void (*)(void *))keypad_do_work,(void *)keytemp);
		//schedule_work(&keypad_work);
		keytemp->key_time = jiffies;
		input_report_key(input,EV_KEY,keytemp->key_value);
		input_sync(input);
		PDEBUG("[tq2440_keypad/key_interrupt]key interrupt occur %s\n",keytemp->name);
	}
	//input_report_key(input,EV_KEY,keytemp->key_value);
	//input_sync(input);
	//PDEBUG("[tq2440_keypad/key_interrupt]key interrupt occur %s\n",keytemp->name);
	
	return IRQ_HANDLED;
}

int hardware_init(void){
	int ret = 0;
	if(!request_mem_region(KEYBASE,3,"TQ2440 KEYPAD PIN")){
		PDEBUG("[tq2440_keypad/hardware_init]Can not request key pin memory region\n");		
		ret = -ENOMEM;
		return ret;
	}
	gpfcon = (volatile unsigned long*)ioremap_nocache(GPFCON,1);
	gpfdat = (volatile unsigned long*)ioremap_nocache(GPFDAT,1);
	gpfup = (volatile unsigned long*)ioremap_nocache(GPFUP,1);
	*gpfcon &= ~(0x03 << 0 |
				0x03 << 2 |
				0x03 << 4 |
				0x03 << 8);
	*gpfcon |= (0x02 << 0 |
			   0x02 << 2 |
			   0x02 << 4 |
			   0x02 << 8);
	*gpfup &= ~(0x01 << 0 |
			   0x01 << 1 |
			   0x01 << 2 |
			   0x01 << 4);
	PDEBUG("[tq2440_keypad/hardware_init]Hardware Initialize Ok\n");		
	return ret;
}
void hardware_release(void){
	iounmap((void*)gpfcon);
	iounmap((void*)gpfdat);
	iounmap((void*)gpfup);
	release_mem_region(KEYBASE,3);
}
void key_setup(struct tq2440_key *keytemp,int irqtemp,int index,char keyvaltemp){
	snprintf(keytemp->name,NAME_SIZE,"KEY%d",index);
	keytemp->irq = irqtemp;
	keytemp->index = index;
	keytemp->state = STATE_IDLE;
	keytemp->key_value = keyvaltemp;
	keytemp->key_time = 0;
	PDEBUG("[tq2440_keypad/key_setup]setup key num %i\n",index);
}
static int __devinit key_probe(struct platform_device *pdev){
	int ret = 0;
	int index = 0;
	keypad = kmalloc(KEY_NUM * sizeof(struct tq2440_key),GFP_KERNEL);
	if(keypad == NULL){
		PDEBUG("[tq2440_keypad/key_probe]Can not malloc keypad\n");
		return -ENOMEM;
	}
	memset(keypad,0,KEY_NUM * sizeof(struct tq2440_key));
	for(index = 0;index < KEY_NUM;index++){
		key_setup(&keypad[index],irqval[index],index,keyval[index]);
	}
	ret = hardware_init();
	if(ret < 0){
		PDEBUG("[tq2440_keypad/key_probe]hardware initialize may occur something wrong\n");
		goto failed1;		
	}
	for(index = 0;index < KEY_NUM;index++){
		ret = request_irq(keypad[index].irq,key_interrupt,IRQF_DISABLED | IRQF_TRIGGER_FALLING,keypad[index].name,(void*)&keypad[index]);
		if(ret != 0){
			goto failed2;				
		}
		PDEBUG("[tq2440_keypad/key_probe]Success to request irq num %d\n",index);
	}
	input = input_allocate_device();
	if(!input){
		PDEBUG("[tq2440_keypad/key_probe]Can not allocate input device\n");
		ret = -ENOMEM;
		goto failed2;
	}
	input->name = pdev->name;
	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;
	for(index = 0;index < KEY_NUM;index++){
		input_set_capability(input,EV_KEY,keypad[index].key_value);		
	}
	ret = input_register_device(input);
	if(ret){
		PDEBUG("[tq2440_keypad/key_probe]Can not allocate input device\n");
		input_free_device(input);
		goto failed2;
	}
	return ret;
failed2:
	for(;index >= 0;index--){
		PDEBUG("[tq2440_keypad/key_setup]Success to free irq num %d\n",index);
		free_irq(keypad[index].irq,key_interrupt);		
	}
failed1:
	kfree(keypad);
	return ret;
}
static int __devexit key_remove(struct platform_device *pdev){
	int index = 0;
	hardware_release();
	for(index = 0;index < KEY_NUM;index++){
        PDEBUG("[tq2440_keypad/key_setup]Success to free irq num %d\n",index);
		free_irq(keypad[index].irq,(void*)&keypad[index]);
	}
	kfree(keypad);
	input_unregister_device(input);
	input_free_device(input);
	return 0;
}

struct platform_driver key_device_driver = {
	.probe = key_probe,
	.remove = key_remove,
	.driver = {
		.name = "tq2440_keypad",
		.owner = THIS_MODULE,
	}
};

static int __init keypad_init(void){
	return platform_driver_register(&key_device_driver);		
}
static void __exit keypad_exit(void){
	platform_driver_unregister(&key_device_driver);		
}

module_init(keypad_init);
module_exit(keypad_exit);

MODULE_AUTHOR("zengweitotty");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("TQ2440 KEY PAD");
