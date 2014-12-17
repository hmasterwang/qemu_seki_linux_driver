ccflags-y := -std=gnu99 -Wall -Wunused

DEBUG = y

ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g -DSEKI_DEBUG -DDEBUG
else
  DEBFLAGS = -O2
endif

EXTRA_CFLAGS := $(DEBFLAGS)
EXTRA_CFLAGS += -I$(LDDINC)

ifneq ($(KERNELRELEASE),)
seki_emu-objs := seki_pcie_device.o seki_procfs.o
obj-m	:= seki_emu.o
else
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) LDDINC=$(PWD)/../include modules

endif


clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions \
	    Module.markers  modules.order  Module.symvers

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend


ifeq (.depend,$(wildcard .depend))
include .depend
endif
