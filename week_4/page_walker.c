#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
//#include <linux/task.h>

MODULE_LICENSE("GPL");
#define BUFFSIZE 128
#define SHA1_LENGTH 20

ssize_t write_pid_get_hash(struct file*, const char __user*, size_t, loff_t*);

struct proc_dir_entry *hash_pid;
struct file_operations hash_pid_fops = {.owner = THIS_MODULE, .write=write_pid_get_hash};


unsigned char temp_buf[128];

long hash_page(struct vm_area_struct *vma){
	unsigned long pos = vma->vm_start;
	unsigned long end = vma->vm_end;
	long checksum = 0;
	int i = 0;
	for (; pos < end; pos +=128){
		int ret = copy_from_user(temp_buf, (void*)pos, sizeof(temp_buf));
		if (ret >= 0){
			for (i=0; i<128; i++){
				if (temp_buf[i] != 0){
					printk(KERN_INFO "not zero");
				}
				checksum += temp_buf[i];
			}
		}
	}
	return checksum;	
} 

void list_pages(struct mm_struct *mm){
	if (mm != NULL){
		struct vm_area_struct *vma = mm->mmap;
		while (vma != NULL){
			printk(KERN_INFO "vm_start: %lx vm_end: %lx checksum: %lx", vma->vm_start, vma->vm_end, hash_page(vma));
			vma = vma->vm_next;
		}
	}
}

void procs_info_print(int PID){
	struct task_struct *task_list;
	for_each_process(task_list){
		if (task_list->pid == PID){
			task_lock(task_list);
			list_pages(task_list->mm);
			task_unlock(task_list);
		}
	}
}


ssize_t write_pid_get_hash(struct file* file, const char __user *buf, size_t count, loff_t* data){
	int pid,tval;
	char buffer[BUFFSIZE];
	count = (count<BUFFSIZE) ? count : BUFFSIZE;
	if(copy_from_user(buffer, buf, count))
		return -EFAULT;
	tval = sscanf(buffer, "%d", &pid);
	if (tval!=1){
		printk(KERN_INFO "PID invalid");
		return -EFAULT;
	}
	printk(KERN_INFO "PID passed: %d\n", pid);
	procs_info_print(pid);
	return count;
}


int init_module(void){
	printk(KERN_INFO "Starting up...");
	hash_pid = proc_create("hash_pid", 0666, NULL,&hash_pid_fops); 
	//procs_info_print();	
	return 0;
}

void cleanup_module(void){
	printk(KERN_INFO "Closing down...");
	proc_remove(hash_pid);
}