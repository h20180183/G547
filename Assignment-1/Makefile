obj-m := assign_1.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

insert: assign_1.ko
	sudo insmod assign_1.ko

remove: assign_1.ko
	sudo rmmod assign_1.ko

compile: user_app.c
	gcc user_app.c -o user

clean:
	rm -rf *.o *.order *.ko *.mod *.symvers *.mod.c
