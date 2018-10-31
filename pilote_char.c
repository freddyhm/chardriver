#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/types.h>
#include <linux/stat.h> 

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/cdev.h>
#include "ring_buffer.c"


MODULE_AUTHOR("Freddy Hidalgo-Monchez");
MODULE_LICENSE("Dual BSD/GPL");

// global variables
dev_t dev;
struct class *cclass;
struct cdev mycdev; 
static const int SIMPLE_BUFFER_SIZE = 6;
static const int RING_BUFFER_SIZE = 10;
static char simple_write_buffer[6];
//static char simple_read_buffer[6];

static char *write_buffer;
static cbuf_handle_t write_cbuf;

static char *read_buffer;
static cbuf_handle_t read_cbuf;


static ssize_t module_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t module_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static int module_open(struct inode *inode, struct file *filp);
static int module_release(struct inode *inode, struct file *filp);


// alloc char dev cout doit etre 2 car 2 instance pour 2 serial port
// nom du driver doit etre /dev/etsele_cdev1
// si on te donne pas le numero faut ajouter %d pour dire que c'est un chiffre
// tenir en compte quel port serial qu'on a (regarde minor)
// buffer malloc (pointeur) d'une taille fixe de 64 octets par exemple


static struct file_operations myModule_fops = {
	.owner 	 = THIS_MODULE,
	.write	 = module_write,
	.read	 = module_read,
	.open	 = module_open,
	.release = module_release,
};

static int  pilote_init (void);
static void pilote_exit (void);

// buffer is full if last index is not 0 (default value)
static bool is_simple_write_buffer_full(void){
	int last_index = (sizeof(simple_write_buffer)/sizeof(char)) - 1;
	return (simple_write_buffer[last_index] != 0);
}

static void get_simple_write_buffer(void){
	
	int i = 0;

    while(i < SIMPLE_BUFFER_SIZE)
	{
		printk("VALUE IN SIMPLE: %c\n" , simple_write_buffer[i]);
		i++;
	};
}

static void fill_simple_write_buffer(char data){

	int i = 0;
	bool is_transferred = false; 

    while(i < SIMPLE_BUFFER_SIZE)
	{
		if(!is_transferred && simple_write_buffer[i] == 0){
			simple_write_buffer[i] = data;
			is_transferred = true;
		}

		i++;
	};
}

// cbuf is a handler we create to manage operations 
static void create_ring_write_buffer(void)
{
    write_buffer  = kmalloc(RING_BUFFER_SIZE * sizeof(char), GFP_KERNEL);
	write_cbuf = circular_buf_init(write_buffer, RING_BUFFER_SIZE);
}

// cbuf is a handler we create to manage operations 
static void create_ring_read_buffer(void)
{
    read_buffer  = kmalloc(RING_BUFFER_SIZE * sizeof(char), GFP_KERNEL);
	read_cbuf = circular_buf_init(read_buffer, RING_BUFFER_SIZE);
}


// transfer simple buffer values to ring buffer and set simple buffer values to 0
static void move_simple_to_ring_write_buffer(void){
	
	int i = 0;
	while(i < SIMPLE_BUFFER_SIZE){
		circular_buf_put(write_cbuf, simple_write_buffer[i]);
		simple_write_buffer[i] = 0;
		i++;
	}	
}

// read and remove from circular buffer
static void get_ring_write_buffer(void){
	while(!circular_buf_empty(write_cbuf))
	{
		char data;
		circular_buf_get(write_cbuf, &data);
		printk("RING BUFFER: %c ", data);
	}	
}

static ssize_t module_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
   
   // write to ring read buffer 
   // move from ring read buffer to simple read buffer
   // move from simple read buffer to user
   
   printk(KERN_WARNING"Pilote READ : Hello, world\n");
   return 0;
}

static ssize_t module_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {

	char cdata = 'a';
	char *pData = &cdata;

	copy_from_user(pData, buf, count);

	fill_simple_write_buffer(*pData);

	if(is_simple_write_buffer_full())
	{
		get_simple_write_buffer();	
		printk("BUFFER FULL - TRANSFERING TO RING BUFFER");
		move_simple_to_ring_write_buffer();
		get_simple_write_buffer();
		get_ring_write_buffer();
	}

	printk(KERN_WARNING"Pilote WRITE NEWW : Hello, world\n");
   return 0;
}

static void release_buffers(void){
	kfree(write_buffer);
	kfree(read_buffer);
	circular_buf_free(write_cbuf);
	circular_buf_free(read_cbuf);
}

static int module_open(struct inode *inode, struct file *filp) {
   printk(KERN_WARNING"Pilote OPEN : Hello, world\n");
   return 0;
}

static int module_release(struct inode *inode, struct file *filp) {
   release_buffers();
   printk(KERN_WARNING"Pilote RELEASE : Hello, world\n");
   return 0;
}

static __init int pilote_init(void)	
{
	create_ring_write_buffer();
	create_ring_read_buffer();

	alloc_chrdev_region(&dev, 0, 1, "MyDriver");

	cclass = class_create(THIS_MODULE, "moduleTest");
	device_create(cclass, NULL, dev, NULL, "myModuleTest");

	cdev_init(&mycdev, &myModule_fops);
	cdev_add(&mycdev, dev, 1); // dev??

	printk(KERN_WARNING "Pilote : Hello, world (Pilote)\n");
	return 0;
}

static void __exit pilote_exit(void)
{
	cdev_del(&mycdev);

	device_destroy(cclass, dev);
	class_destroy(cclass);

	unregister_chrdev_region(dev, 1);

	printk(KERN_ALERT "Pilote: Goodbye, cruel world\n");
}

module_init(pilote_init);
module_exit(pilote_exit);
