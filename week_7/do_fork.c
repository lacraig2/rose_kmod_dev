#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/kprobes.h>

MODULE_LICENSE("GPL");

static struct kprobe kp = {
	.symbol_name = "_do_fork",
};

static int handler_pre(struct kprobe *p, struct pt_regs *regs){
	printk(KERN_INFO "pre_handler: probe->addr=0x%p,ip=%lx,flags=0x%lx",p->addr,regs->ip,regs->flags);
	return 0;
}

static void handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags){
	printk(KERN_INFO "post_handler: p->addr: 0x%p flags=0x%lx", p->addr, regs->flags);
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
	return 0;
}

void cleanup_module(void){
	printk(KERN_INFO "Closing down...");
	unregister_kprobe(&kp);
}
