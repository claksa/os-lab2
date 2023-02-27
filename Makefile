ifneq ($(KERNELRELEASE),)
	obj-m := my_module.o
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
all: user module
module:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
user:
	gcc -o user_app user_app.c
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	${RM} user_app
endif
