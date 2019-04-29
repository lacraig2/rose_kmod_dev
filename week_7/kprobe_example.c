#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/kprobes.h>


MODULE_LICENSE("GPL");

static struct kprobe kp = {
	.symbol_name = "vfs_read",
};

struct file* name_overwrite = NULL; 


//ssize_t vfs_read(struct file*, char __user*, size_t, loff_t);
static int handler_pre(struct kprobe *p, struct pt_regs *regs){
	#ifdef CONFIG_X86
	struct file* file = (struct file*) regs->di;
	struct dentry* den = file->f_path.dentry;
	char* name = den->d_name.name;
	printk(KERN_INFO "post_handler: p->addr: 0x%p flags=0x%lx name=%s", p->addr, regs->flags, name);
	if (memcmp(name, "do_fork", 7) == 0){
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
	printk(KERN_INFO "post_handler: p->addr: 0x%p flags=0x%lx name=%s", p->addr, regs->flags, name);
	#endif
	return 0;
}

static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr){
	printk(KERN_INFO "fault_handler: p->addr=0x%p,msr=0x%lx",p->addr, regs->flags);
	return 0;
}


int init_module(void){
	printk(KERN_INFO "Starting up...");
	int ret;
	kp.pre_handler = handler_pre;
	kp.post_handler = handler_post;
	kp.fault_handler = handler_fault;
	ret = register_kprobe(&kp);
	if (ret < 0){
		printk(KERN_INFO "Failed to probe at %d\n", ret);
		return ret;
	}
	printk(KERN_INFO "Planted krpobe at %p", kp.addr);
	#ifdef CONFIG_X86
		printk(KERN_INFO "config x86");
	#endif
	#ifdef CONFIG_PPC
		printk(KERN_INFO "config PPC");
	#endif
	#ifdef CONFIG_MIPS
		printk(KERN_INFO "config MIPS");
	#endif
	#ifdef CONFIG_ARM64
		printk(KERN_INFO "config ARM64");
	#endif
	#ifdef CONFIG_S390
		printk(KERN_INFO "config S390");
	#endif




	return 0;
}

void cleanup_module(void){
	printk(KERN_INFO "Closing down...");
	unregister_kprobe(&kp);
}
