# Week 7 - Experimentation with kprobes

## kprobes features
- pre_handler
- post_handler
- handler_fault

## Example:
### Replacing a file on `vfs_read` call.
- Check file name via path and dentry
- get file pointer
-  replace from di argument
### Do fork
- add symbol to do_fork
- add methods to kprobe

