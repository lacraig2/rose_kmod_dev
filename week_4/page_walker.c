#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <crypto/hash.h>
//#include <linux/task.h>

MODULE_LICENSE("GPL");
#define BUFFSIZE 128
#define SHA1_LENGTH 20

ssize_t write_pid_get_hash(struct file*, const char __user*, size_t, loff_t*);

struct proc_dir_entry *hash_pid;
struct file_operations hash_pid_fops = {.owner = THIS_MODULE, .write=write_pid_get_hash};

bool alg_sum_page(char* alg_name, struct vm_area_struct *vma, uint8_t* result){
	unsigned long pos = vma->vm_start;
	unsigned long end = vma->vm_end;

	struct shash_desc *sdesc;
	struct crypto_shash *alg;
	alg = crypto_alloc_shash(alg_name, 0, CRYPTO_ALG_ASYNC);
	sdesc =  kzalloc(sizeof(*sdesc) + crypto_shash_descsize(alg), GFP_KERNEL);
	if (sdesc == NULL){
		printk(KERN_INFO "did not allocate shash_desc");
		return false;
	}
	if (alg == NULL){
		printk(KERN_INFO "algorithm not allocated");
		return false;
	}

	sdesc->tfm = alg; 
	sdesc->flags = CRYPTO_TFM_REQ_MAY_SLEEP;
	if (sdesc->tfm == NULL)
		return false;
	crypto_shash_init(sdesc);
	char *buffer = kmalloc(1024, GFP_KERNEL);
	while (pos < end){
		copy_from_user(buffer, (void*)pos, 1024);
		crypto_shash_update(sdesc, buffer, 1024);
//		printk(KERN_INFO "Copying %p to %p", pos, buffer);
		pos += (pos > end) ? -1024 : 1024;
	}
	kfree(buffer);
//	printk(KERN_INFO "Location %p", result);
	crypto_shash_final(sdesc,result);
	crypto_free_shash(sdesc->tfm);
	return true;
}

void list_pages(struct mm_struct *mm){
	if (mm != NULL){
		struct vm_area_struct *vma = mm->mmap;
		int n;
		while (vma != NULL){
			uint8_t *md5 = kzalloc((15+1)*sizeof(char), GFP_KERNEL);
			bool proper = alg_sum_page("md5", vma, md5);
			md5[15] = 0;
			if (proper){
				printk(KERN_INFO "vm_start: %lx vm_end: %lx hash: %15phN", vma->vm_start, vma->vm_end,md5);
			}else{
				printk(KERN_INFO "highly improper");
			}
			kzfree(md5);
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
