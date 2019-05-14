obj-m += file_hiding.o
all:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
restart:
		sudo rmmod file_hiding
		sudo insmod file_hiding.ko
start:
		sudo insmod file_hiding.ko
stop:
		sudo rmmod file_hiding
redo:
		$(MAKE) all
		$(MAKE) restart
run:
		$(MAKE) start
clean:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
