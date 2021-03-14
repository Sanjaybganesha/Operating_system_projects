#include<linux/init.h>
#include<linux/module.h>
#include<linux/miscdevice.h>
#include<linux/fs.h>
#include<linux/uaccess.h>
#include<linux/sched.h>
#include<linux/slab.h>
#include<linux/kernel.h>
#include<linux/moduleparam.h>

#define BUFFER_SIZE 100
#define DEVICE_NAME "linepipe"

MODULE_LICENSE("GPL");

/* Declaring semaphores and structs*/
int pipe_size=5;//pipe size

struct semaphore empty;
struct semaphore full;
struct semaphore mutex;

typedef struct fifo_queue{
	char ** buff;
	int first,last;	
}Queue;


Queue pipe;//Global PIPE varaibale
//declaration of functions
static ssize_t linepipe_read(struct file *file, char __user * out, size_t size, loff_t * off);//function to read
static ssize_t linepipe_write(struct file *file, const char * out, size_t size, loff_t * off);//function to write
static int linepipe_open(struct inode*,struct file *);
static int linepipe_close(struct inode*, struct file *);


static struct file_operations linepipe_fops = {//file operation definition
	.owner = THIS_MODULE,	
	.open = linepipe_open,
	.read = linepipe_read,
	.write = linepipe_write,
	.release = linepipe_close
};
static struct miscdevice linepipe_device  = {//declaring a device struct
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &linepipe_fops
};

static int linepipe_open(struct inode * inode,struct file * file) {//called when line to be openned
	printk(KERN_INFO "LinePipe: Device Opened\n");	
	return 0;
}

static int linepipe_close(struct inode * inode,struct file *file) {//called when line to be closed
	printk(KERN_INFO "LinePipe: Device Closed!\n");
	return 0;
}
static ssize_t linepipe_write(struct file *filp, const char *buff, size_t len, loff_t * off)//called to write in pipe(producer)
{
	int status,ret;
	printk(KERN_INFO "LinePipe: Writing to the pipe...\n");
	
	if(down_interruptible(&empty) == 0 ) {
		if(down_interruptible(&mutex)==0){
			if(pipe.last >= pipe_size)
                        	pipe.last=0;
			
			memset(pipe.buff[pipe.last],0,sizeof(char)*BUFFER_SIZE);	
			status = copy_from_user(pipe.buff[pipe.last],buff,len);
			printk(KERN_INFO "Writing %s", pipe.buff[pipe.last]);
			if(status!=0) {
				printk(KERN_ALERT "LinePipe: Error in writing!");
				return status;
			}
			ret=strlen(pipe.buff[pipe.last]);
			pipe.last++;
			up(&mutex);
		} 
		up(&full);
	}
	else {
		//if the device got interrupted the lock wont be applied 
		printk(KERN_ALERT "LinePipe: Cannot in acquire lock. Device might have been interrupted!\n");
		return -EFAULT;
        }
	return ret;
}

static ssize_t linepipe_read(struct file *file, char *out, size_t size, loff_t * off)//called to read from pipe(consumer)
{
	int status,len;
	printk(KERN_INFO "LinePipe: Reading from the pipe...\n");
	if(down_interruptible(&full)==0) {
		if(down_interruptible(&mutex)==0){
			if(pipe.first >=pipe_size)
				pipe.first=0;
			printk(KERN_INFO "Sending: %s",pipe.buff[pipe.first]);
			len=strlen(pipe.buff[pipe.first]);
			status = copy_to_user(out, pipe.buff[pipe.first], len+1);
			if(status !=0)
			{	
				printk(KERN_ALERT "LinePipe: Error in copying data to user!");	
				return -EFAULT;
			}	
			pipe.first++;
			up(&mutex);
		} else {
			
			printk(KERN_ALERT "LinePipe: Cannot acquire lock. Device might have been interrupted!\n");
			return -EFAULT;	
		}
		up(&empty);
	} else {
		//if the device got interrupted the lock wont be applied 
		printk(KERN_ALERT "LinePipe: Cannot acquire lock. Device might have been interrupted!\n");
		return -EFAULT;
	}
	return len;
}
int __init init_module(void){
	int reg_status,i;
	printk(KERN_INFO "LinePipe: Initializing the LinePipe KO with pipe_size=%d \n",pipe_size);
	reg_status = misc_register(&linepipe_device);
	
	pipe.buff = (char**)kmalloc(sizeof(char*) * pipe_size,GFP_KERNEL);//allocate memmory
	if(pipe.buff == NULL){
		
		printk(KERN_ALERT "LinePipe ERROR: Kernel cannot allocate Memory!");
		return -EFAULT;
	}
	pipe.first=0;  
	pipe.last=0;	

	for(i=0;i<pipe_size;i++){
                pipe.buff[i] = (char*)kmalloc(sizeof(char)*BUFFER_SIZE,GFP_KERNEL);
		if(pipe.buff[i]==NULL){
		printk(KERN_ALERT "LinePipe ERROR: Kernel cannot allocate Memory!");
                	return -EFAULT;
		}
        }
//initialising all the 
	sema_init(&full,0);
	sema_init(&empty,pipe_size);
	sema_init(&mutex,1);

	return 0;
}

void __exit cleanup_module(void) {
	int i;
	printk(KERN_INFO "LinePipe: Exiting from the LinePipe\n");

	for(i=0;i<pipe_size;i++){//deallocate memory
                kfree(pipe.buff[i]);
        }
	kfree(pipe.buff);
	misc_deregister(&linepipe_device);
}


//register
module_param(pipe_size,int,S_IRUGO);
