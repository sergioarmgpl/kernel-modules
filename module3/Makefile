# Docker Container Lister Kernel Module Makefile

obj-m += docker_lister.o
docker_lister-objs := process_lister.o

# Directorio de construcción del kernel
KERNEL_DIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Target por defecto
all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

# Target de limpieza
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f modules.order Module.symvers
