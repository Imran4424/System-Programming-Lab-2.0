# Build simple.c as a loadable module named simple.ko
obj-m += simple.o

# Path to the build tree for the running kernel (override with: make KDIR=/path/to/build)
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

.PHONY: all clean

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean