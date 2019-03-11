#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#define BUFFSIZE 128

ssize_t stopwatch_set_timeout(struct file*, const char __user *, size_t, loff_t*);
ssize_t start_new_timer(struct file* , const char __user *, size_t , loff_t* );
ssize_t stop_new_timer(struct file* file, const char __user *buf, size_t count, loff_t* data);
int open_general(struct inode*, struct file*);


int timeout = 40;


struct timer_container {
	char *name;
	int time_start;
	struct timer_container *next;
	struct timer_container *prev;
};

void intrpt_routine(struct work_struct *work);

ssize_t pet_timer(struct file*, char __user *, size_t, loff_t*);
struct timer_container *first_timer_container = NULL;

struct proc_dir_entry *proc_parent;
struct proc_dir_entry *start;
struct proc_dir_entry *stop;
struct proc_dir_entry *set;
struct proc_dir_entry *pet;

static struct workqueue_struct *my_workqueue;
static struct delayed_work Task;
static DECLARE_DELAYED_WORK(Task, intrpt_routine);

struct file_operations set_fops = {.owner = THIS_MODULE,
										.write = stopwatch_set_timeout,};


ssize_t stopwatch_set_timeout(struct file* file, const char __user *buf, size_t count, loff_t* data){
	int value,tval;
	char buffer[BUFFSIZE];
	count = (count<BUFFSIZE) ? count : BUFFSIZE;
	if(copy_from_user(buffer, buf, count))
		return -EFAULT;
	tval = sscanf(buffer, "%d", &value);
	if (tval!=1)
		return -EFAULT;
	timeout = value;
	printk(KERN_INFO "Timer value has been set to: %d\n", value);
	return count;
}


struct file_operations start_fops = {.owner = THIS_MODULE,
										.write = start_new_timer,};

ssize_t start_new_timer(struct file* file, const char __user *buf, size_t count, loff_t* data){
	printk(KERN_INFO "I get called");
	int value,tval;
	char buffer[BUFFSIZE];
	count = (count<BUFFSIZE) ? count : BUFFSIZE;
	if(copy_from_user(buffer, buf, count))
		return -EFAULT;
	char name_buffer[BUFFSIZE];
	tval = sscanf(buffer, "%s", name_buffer);
	printk(KERN_INFO "%s", name_buffer);
	if (tval!=1)
		return -EFAULT;
	
	if (first_timer_container == NULL){
		first_timer_container = (struct timer_container*)kmalloc((sizeof(struct timer_container)), GFP_KERNEL);
		first_timer_container->next = NULL;
		first_timer_container->prev = NULL;
		first_timer_container->time_start = jiffies;
		first_timer_container->name = (char*) kmalloc((sizeof(char)*count), GFP_KERNEL);
		memcpy(first_timer_container->name, name_buffer, count);
	}else{
		struct timer_container* current_tc = first_timer_container;
		while (current_tc->next != NULL){
			current_tc = current_tc->next;
		}

		current_tc->next = (struct timer_container*)kmalloc((sizeof(struct timer_container)), GFP_KERNEL);
		struct timer_container* old_current = current_tc;
		current_tc = old_current->next;
		current_tc->next = NULL;
		current_tc->prev = old_current;
		current_tc->time_start = jiffies;
		current_tc->name = (char*) kmalloc((sizeof(char)*count), GFP_KERNEL);
		memcpy(current_tc->name, name_buffer, count);
	}
	struct timer_container* tc = first_timer_container;
	int num = 0;
	do {	
		printk(KERN_INFO "#%d: found timer [%s] at %d", ++num, tc->name, tc->time_start);
		tc = tc->next;
	}while (tc!= NULL);
	
	return count;
}

struct file_operations stop_fops = {.owner = THIS_MODULE,
										.write = stop_new_timer,};

ssize_t stop_new_timer(struct file* file, const char __user *buf, size_t count, loff_t* data){
	printk(KERN_INFO "I get called");
	int value,tval;
	char buffer[BUFFSIZE];
	count = (count<BUFFSIZE) ? count : BUFFSIZE;
	if(copy_from_user(buffer, buf, count))
		return -EFAULT;
	char name_buffer[BUFFSIZE];
	tval = sscanf(buffer, "%s", name_buffer);
	printk(KERN_INFO "%s", name_buffer);
	if (tval!=1)
		return -EFAULT;
	
	struct timer_container* tc = first_timer_container;
	int num = 0;
	if (tc==NULL)
			return count;
	do {	
		if (strncmp(name_buffer, tc->name, count)){
			printk(KERN_INFO "#%d: found timer [%s] at %d", ++num, tc->name, tc->time_start);
		}else{
				if (tc == first_timer_container){
					first_timer_container = first_timer_container->next;
				}else{
					struct timer_container* prev = tc->prev;
					prev->next = tc->next;
					struct timer_container* next = tc->next;
					if (tc->next != NULL){
						next->prev = prev;
					}
					kfree(tc->name);
					kfree(tc);
				}
		}
		tc = tc->next;
	}while (tc!= NULL);
	
	return count;
}


void intrpt_routine(struct work_struct *work){
	printk(KERN_INFO "Watchdog timer hit.");
}

struct file_operations pet_fops = {.owner = THIS_MODULE,
										.read = pet_timer,};
ssize_t pet_timer(struct file* f, char __user *userbuf, size_t count, loff_t* data){	
	printk(KERN_INFO "hit pet timer");
	cancel_delayed_work(&Task);
	queue_delayed_work(my_workqueue, &Task, 100*timeout);
	return 0;
}

#define MY_WORK_QUEUE_NAME "FIRST"

int init_module(void){
	printk(KERN_INFO "first init...");
	proc_parent = proc_mkdir("mystopwatch", NULL);
	start = proc_create("start",0666, proc_parent, &start_fops);
	stop = proc_create("stop", 0666, proc_parent, &stop_fops);
	set = proc_create("set", 0666, proc_parent, &set_fops);
	pet = proc_create("pet", 0666, proc_parent, &pet_fops);
	my_workqueue = create_workqueue(MY_WORK_QUEUE_NAME);
	return 0;
}

void cleanup_module(void){
	proc_remove(set);
	proc_remove(start);
	proc_remove(set);
	proc_remove(pet);
	proc_remove(proc_parent);
	cancel_delayed_work(&Task);
	flush_workqueue(my_workqueue);
	destroy_workqueue(my_workqueue);

	struct timer_container* tc = first_timer_container;
	while (tc != NULL){
		kfree(tc->name);
		kfree(tc);
		tc = tc->next;
	}
	printk(KERN_INFO "first exiting normally... Bye.\n");
}
MODULE_LICENSE("GPL");
