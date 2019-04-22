# Algorithm Agnostic Program Checksum

This program exposes a procfs file at `/proc/hash_pid` which allows a user to send it a PID and the user will see in `dmesg` the hashes of each `vm_area_struct` held by the program.

e.g. `echo "1" > /proc/hash_pid` will cause the hashes`vm_area_structs` in process 1 to be printed to dmesg.

## Creating a procfs file

The program creates and instantiates a procfs file through the `proc_create` mechanism. It overwrites the write function and properly scans for a PID from the user. It will fail if not provided a properly formatted PID. 

## Getting the `task_struct` from the PID

In the kernel the function `find_task_by_vpid` exists and it works as it sounds. However, this function is not exported to modules. For this reason we use a less efficient version below:
```
	struct task_struct *task_list;
     for_each_process(task_list){
         if (task_list->pid == PID){
             task_lock(task_list);
             list_pages(task_list->mm);
             task_unlock(task_list);
         }
     }
```

## Iterating the `vm_area_struct`s

The structure of `struct vm_area_struct` allows for a `next` member which can be followed until it is null.

## Printing md5sum

In `Documentation/printk-formats.txt` there is a listing which explains printing a raw buffer as an hex string. It provides several methods. We use the one listed for "%15phN". It is important to null terminate your hex string.

## Algorithm Agnostic Hashing Function

Our algorithm agnostic hashing function takes an algorithm name, `vm_area_struct` and a pointer for the results. It uses the interal shash structures from the crypto library. It allocates an algorihm and pairs it with a descriptor. It allocates a buffer and then fills it 1024 bytes at a time. It then runs the `crypto_shash_update` on this buffer. It then takes output through `crypto_shash_final` and cleans up after itself and returns.

