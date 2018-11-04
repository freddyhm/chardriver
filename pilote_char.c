#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/types.h>
#include <linux/stat.h> 
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/cred.h>
#include <linux/semaphore.h>
#include "ring_buffer.c"

MODULE_AUTHOR("Freddy Hidalgo-Monchez");
MODULE_LICENSE("Dual BSD/GPL");

// vars to keep track of open modes and user count
static int read_count;
static int write_count;
static int user_count;
static unsigned int user_owner_id;
static DEFINE_SPINLOCK(access_lock);
static int activeOpenModeList[3];

// vars to sleep
static DECLARE_WAIT_QUEUE_HEAD(waitq); 
static struct semaphore sem;
static struct semaphore write_sem;

// vars to register device 
dev_t dev;
struct class *cclass;
struct cdev mycdev; 

// vars to manage buffers
static const int SIMPLE_BUFFER_SIZE = 16;
static const int RING_BUFFER_SIZE = 16;
static char simple_write_buffer[16];
static char simple_read_buffer[16];
static char *write_buffer;
static cbuf_handle_t write_cbuf;
static char *RxBuf;
static cbuf_handle_t RxBuf_cbuf;

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

// ------------------ RING WRITE BUFFER FUNCTIONS -------------------

// cbuf is a handler we create to manage operations 
static void create_ring_write_buffer(void)
{
    write_buffer  = kmalloc(RING_BUFFER_SIZE * sizeof(char), GFP_KERNEL);
	write_cbuf = circular_buf_init(write_buffer, RING_BUFFER_SIZE);
}

// ------------------ RING READ BUFFER FUNCTIONS -------------------

static void create_ring_read_buffer(void)
{
	// cbuf is a handler we create to manage operations 
    RxBuf  = kmalloc(RING_BUFFER_SIZE * sizeof(char), GFP_KERNEL);
	RxBuf_cbuf = circular_buf_init(RxBuf, RING_BUFFER_SIZE);
}

static void set_up_ring_read_buffer(void){

	// write to ring read buffer 
	int i = 0;
	while(i < RING_BUFFER_SIZE){
		circular_buf_put(RxBuf_cbuf, 'a');
		i++;
	}
}

// ------------------ RING TO SIMPLE BUFFER READ FUNCTIONS -------------------

static void fill_simple_read_buffer(char data){

	int i = 0;
	bool is_transferred = false; 

    while(i < SIMPLE_BUFFER_SIZE)
	{
		if(!is_transferred && simple_read_buffer[i] == 0){
			simple_read_buffer[i] = data;
			is_transferred = true;
		}

		i++;
	};

}

// sends from ring read to simple only once 
static bool move_ring_read_to_simple(void){

	if(!circular_buf_empty(RxBuf_cbuf)){
		char data = 'c';
		circular_buf_get(RxBuf_cbuf, &data);
		printk("data retrieved: %c", data);
		fill_simple_read_buffer(data);
		return true;
	}else{
		return false;
	}
}


// ------------------ SIMPLE READ BUFFER FUNCTIONS -------------------

static void replace_with_zeroes_read_buffer(int dataSentCount){

	int i;
	i = 0;

	while(i < dataSentCount){
		simple_read_buffer[i] = 0;
		i++;
	}	

}

static void remove_simple_read_buffer(int dataSentCount){

	int i;
	int oldIndex;
	oldIndex = 0;
	i = 0;

	// set data in buffer to 0s 
	replace_with_zeroes_read_buffer(dataSentCount);

	// shift data that remains to the left
	while((dataSentCount + i) < SIMPLE_BUFFER_SIZE){
		oldIndex = dataSentCount + i;
		simple_read_buffer[i] = simple_read_buffer[oldIndex];
		simple_read_buffer[oldIndex] = 0;
		i++;
	}
}

static int get_simple_read_buffer_count(void){

	int i;
	int dataCount;
	i = 0;
	dataCount = 0;

	while(i < SIMPLE_BUFFER_SIZE)
	{
		if(simple_read_buffer[i] != 0){
			dataCount++;
		}

		i++;
	};

	return dataCount;
}

// ------------------ SIMPLE WRITE BUFFER FUNCTIONS -------------------

// buffer is full if last index is not 0 (default value)
// we add data continguously 
static bool is_simple_write_buffer_full(void){
	int last_index = (sizeof(simple_write_buffer)/sizeof(char)) - 1;
	return (simple_write_buffer[last_index] != 0);
}

static int get_simple_write_buffer_count(void){
	
	int i;
	int dataCount;
	i = 0;
	dataCount = 0;

	while(i < SIMPLE_BUFFER_SIZE)
	{
		if(simple_write_buffer[i] != 0){
			dataCount++;
		}

		i++;
	}

	return dataCount;
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
		char data = 'c';
		circular_buf_get(write_cbuf, &data);
		printk("RING BUFFER: %c ", data);
	}	
}

// ------------------ MAIN DRIVER FUNCTIONS -------------------

static ssize_t module_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	
	int dataSentCount;
	int error_num;

	// grab the semaphore
	if (down_interruptible(&sem)){
		 printk("Unable to acquire Semaphore\n");
		return -ERESTARTSYS;
	}

	// non blocking and our data count is 0
	if(filp->f_flags & O_NONBLOCK && (get_simple_read_buffer_count() == 0)){
			printk("NO DATA SENT TO USER!");
			return -EAGAIN;
	}

	while(count > get_simple_read_buffer_count() && !(filp->f_flags & O_NONBLOCK)){
		
		up(&sem); // release the semaphore so that a writer can wake us up!

		// sleep here until we're woken up and test our condition for true
		if (wait_event_interruptible(waitq, count <= get_simple_read_buffer_count())){
			return -ERESTARTSYS;
		}

		// grab semaphore before trying again
		if(down_interruptible(&sem)){
			return -ERESTARTSYS;
		}
	} 

	// send as much as we can 
	// if we're here from a blocking operation then we know that we have enough to fill the request so send count
	// if we're here from a non-blocking operation, we send what we have 
	if(filp->f_flags & O_NONBLOCK){
		dataSentCount = get_simple_read_buffer_count();
	}else{
		dataSentCount = count;
	}

	error_num = copy_to_user(buf, simple_read_buffer, dataSentCount); 
	if(error_num == 0){
		remove_simple_read_buffer(dataSentCount);
	}else{
		// release semaphore if our copy fails
		up(&sem);
		printk(KERN_WARNING"Failed to read characters %d\n", error_num);
		return -EFAULT;
	}

	// release semaphore
	up(&sem);

	printk(KERN_WARNING"Pilote READ : Hello, world\n");
	return 0;
}

static ssize_t module_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {

	int dataSentCount;
	int error_num;

	// grab the semaphore
	if (down_interruptible(&write_sem)){
		 printk("Unable to acquire Semaphore\n");
		return -ERESTARTSYS;
	}



	// non blocking, send data right away to simple write buffer, and if full fail sending data
	if(filp->f_flags & O_NONBLOCK)){
		printk("NO DATA SENT TO USER!");
		return -EAGAIN;
	}

	printk("%s", simple_write_buffer);

	/*
	while(count > get_simple_read_buffer_count() && !(filp->f_flags & O_NONBLOCK)){
		
		up(&sem); // release the semaphore so that a writer can wake us up!

		// sleep here until we're woken up and test our condition for true
		if (wait_event_interruptible(waitq, count <= get_simple_read_buffer_count())){
			return -ERESTARTSYS;
		}

		// grab semaphore before trying again
		if(down_interruptible(&sem)){
			return -ERESTARTSYS;
		}
	} 

	// send as much as we can 
	// if we're here from a blocking operation then we know that we have enough to fill the request so send count
	// if we're here from a non-blocking operation, we send what we have 
	if(filp->f_flags & O_NONBLOCK){
		dataSentCount = get_simple_read_buffer_count();
	}else{
		dataSentCount = count;
	}

	error_num = copy_to_user(buf, simple_read_buffer, dataSentCount); 
	if(error_num == 0){
		remove_simple_read_buffer(dataSentCount);
	}else{
		// release semaphore if our copy fails
		up(&sem);
		printk(KERN_WARNING"Failed to read characters %d\n", error_num);
		return -EFAULT;
	}

	

	*/

	// release semaphore
	up(&write_sem);

	wake_up_interruptible(&waitq);

	printk(KERN_WARNING"Pilote WRITE : Hello, world\n");
   return 0;
}

static void release_buffers(void){
//	kfree(write_buffer);
	kfree(RxBuf);
//	circular_buf_free(write_cbuf);
	circular_buf_free(RxBuf_cbuf);
//	release_region(MY_BASEPORT, MY_NR_PORTS);
}

static int module_open(struct inode *inode, struct file *filp) {

	spin_lock(&access_lock);

	// do not let in more than one user at a time unless it's the same user
	if(user_count == 1 && user_owner_id != current_uid().val){
		spin_unlock(&access_lock);
		printk("NOT THE SAME USER %u", user_owner_id);
		return -ENOTTY;
	}

	// set user id if it's the first time for user
	if(user_count == 0){
		user_owner_id = current_uid().val;
	}

	user_count++;
	spin_unlock(&access_lock);

	// check what mode we are in and increment counter
	if((filp->f_flags & O_ACCMODE) == O_RDONLY && read_count == 0){
		activeOpenModeList[0] = ++read_count; 
	}else if((filp->f_flags & O_ACCMODE) == O_WRONLY && write_count == 0){
		activeOpenModeList[1] = ++write_count; 
	}else if((filp->f_flags & O_ACCMODE) == O_RDWR && ((write_count == 0) && (read_count == 0))){
		activeOpenModeList[0] = ++read_count; 
		activeOpenModeList[1] = ++write_count; 
	}else{
		// kick out user that tries to open a mode more than once
		printk("ONLY ONE READ, WRITE, OR READWRITE read:%d write:%d", read_count, write_count);
		return -ENOTTY;
	}

	printk(KERN_WARNING"Pilote OPEN : Hello, world\n");
	return 0;
}

// gets called when user closes file or program ends 
static int module_release(struct inode *inode, struct file *filp) {

	// remove active mode from list
	if((filp->f_flags & O_ACCMODE) == O_RDONLY){
		read_count = 0;
		activeOpenModeList[0] = read_count;
	}else if((filp->f_flags & O_ACCMODE) == O_WRONLY){
		write_count = 0;
		activeOpenModeList[1] = write_count;
	}else if((filp->f_flags & O_ACCMODE) == O_RDWR){
		read_count = 0;
		write_count = 0;
		activeOpenModeList[0] = read_count;
		activeOpenModeList[1] = write_count;	
	}

	// decrement user count if no more open modes
	if(activeOpenModeList[0] == 0 && activeOpenModeList[1] == 0){
		spin_lock(&access_lock);
		user_count--;
		spin_unlock(&access_lock);
	}

   printk(KERN_WARNING"Pilote RELEASE : Hello, world\n");
   return 0;
}

static __init int pilote_init(void)	
{
	//create_ring_write_buffer();
	create_ring_read_buffer();
	set_up_ring_read_buffer();

	alloc_chrdev_region(&dev, 0, 1, "MyDriver");

	cclass = class_create(THIS_MODULE, "moduleTest");
	device_create(cclass, NULL, dev, NULL, "myModuleTest");

	cdev_init(&mycdev, &myModule_fops);
	cdev_add(&mycdev, dev, 1); 

	sema_init(&sem, 1);
	sema_init(&write_sem, 1);

	printk(KERN_WARNING "Pilote : Hello, world (Pilote)\n");
	return 0;
}

// gets called when rmmod is called
static void __exit pilote_exit(void)
{
	release_buffers();

	cdev_del(&mycdev);

	device_destroy(cclass, dev);
	class_destroy(cclass);

	unregister_chrdev_region(dev, 1);

	printk(KERN_ALERT "Pilote: Goodbye, cruel world\n");
}

module_init(pilote_init);
module_exit(pilote_exit);
