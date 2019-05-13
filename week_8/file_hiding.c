#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/kprobes.h>


MODULE_LICENSE("GPL");

static struct kprobe kp_read = {
	.symbol_name = "vfs_read",
};

static struct kprobe kp_open = {
	.symbol_name = "vfs_open",
};

struct file* name_overwrite = NULL; 

char file_to_hide[] = "/home/luke/workspace/week_8/do_fork.c";


char *get_file_name_from_path(char* path){
	char* position = path;
	char* last_candidate = path;
	while (true){
		if (position[0] == '/'){ last_candidate = position+1;}
		if (position[0] == 0) { break; }
		position++;
	}
	return last_candidate;
}

//int vfs_open(const struct path *, struct file*, const struct cred*)
static int handler_pre_vfs_open(struct kprobe *p, struct pt_regs *regs){
	#ifdef CONFIG_X86
	struct path* path = (struct path*) regs->di;
	struct file* file = (struct file*) regs->si;
	if (file == NULL) return 0;
	if (&file->f_path == NULL) return 0;
	struct dentry* den = file->f_path.dentry;
	if (den == NULL) return 0;
	if (&den->d_name == NULL) return 0;
	char* name = den->d_name.name;
	printk(KERN_INFO "hit post handler for vfs_open %s", name);
	printk(KERN_INFO "post_handler: p->addr: 0x%p flags=0x%lx name=%s", p->addr, regs->flags, name);
	if (memcmp(name, get_file_name_from_path(file_to_hide), 7) == 0){
		name_overwrite = filp_open("/dev/null",O_RDWR, S_IWUSR | S_IRUSR);
		regs->di = name_overwrite;	
		struct dentry* d = ((struct file*)regs->di)->f_path.dentry;
		char* n  = d->d_name.name;
		printk(KERN_INFO "Caught do_fork and replaced with %s", n);
	}
	#endif
	return 0;
	
}


//ssize_t vfs_read(struct file*, char __user*, size_t, loff_t);
static int handler_pre_vfs_read(struct kprobe *p, struct pt_regs *regs){
	#ifdef CONFIG_X86
	struct file* file = (struct file*) regs->di;
	struct dentry* den = file->f_path.dentry;
	char* name = den->d_name.name;
//	printk(KERN_INFO "post_handler: p->addr: 0x%p flags=0x%lx name=%s", p->addr, regs->flags, name);
	if (memcmp(name, get_file_name_from_path(file_to_hide), 7) == 0){
		name_overwrite = filp_open("/dev/null",O_RDWR, S_IWUSR | S_IRUSR);
		regs->di = name_overwrite;	
		struct dentry* d = ((struct file*)regs->di)->f_path.dentry;
		char* n  = d->d_name.name;
		printk(KERN_INFO "Caught do_fork and replaced with %s", n);
	}
	#endif
	return 0;
}

static void handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags){
	#ifdef CONFIG_ARM64
	//ssize_t vfs_read(struct file*, char __user*, size_t, loff_t);
	// get 
	struct file* file = (struct file*) regs->rdi;
	struct dentry* den = file->f_path.dentry;
	char* name = den->d_name.name;
	//printk(KERN_INFO "post_handler: p->addr: 0x%p flags=0x%lx name=%s", p->addr, regs->flags, name);
	#endif
	return 0;
}

static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr){
	printk(KERN_INFO "fault_handler: p->addr=0x%p,msr=0x%lx",p->addr, regs->flags);
	return 0;
}


void file_hide(char* file_input){
	struct file* filp;
	filp = filp_open(file_input,O_RDWR, S_IWUSR | S_IRUSR);
	if (filp == NULL) goto exit;
	struct file_operations* f_op = filp->f_op;
	if (f_op == NULL) goto exit;
	struct kprobe kp_file;
	kp_file.addr = (kprobe_opcode_t*)  f_op->compat_ioctl;
	if (f_op->compat_ioctl == NULL) goto exit;
	int ret = register_kprobe(&kp_file);
	if (ret < 0){
		printk(KERN_INFO "Failed to probe at %d\n", ret);
	}
		return;
	exit:
		printk(KERN_INFO "Problem");
		return;
}


int init_module(void){
	printk(KERN_INFO "Starting up...");
	int ret;
	kp_read.pre_handler = handler_pre_vfs_read;
	kp_read.post_handler = handler_post;
	kp_read.fault_handler = handler_fault;
	ret = register_kprobe(&kp_read);
	if (ret < 0){
		printk(KERN_INFO "Failed to probe at %d\n", ret);
		return ret;
	}
	printk(KERN_INFO "Planted krpobe at %p", kp_read.addr);
	kp_open.pre_handler = handler_pre_vfs_open;
	kp_open.post_handler = handler_post;
	kp_open.fault_handler = handler_fault;
	ret = register_kprobe(&kp_open);
	if (ret < 0){
		printk(KERN_INFO "Failed to probe at %d\n", ret);
		return ret;
	}
	file_hide(file_to_hide);
	return 0;
}

void cleanup_module(void){
	printk(KERN_INFO "Closing down...");
	unregister_kprobe(&kp_read);
	unregister_kprobe(&kp_open);
}
