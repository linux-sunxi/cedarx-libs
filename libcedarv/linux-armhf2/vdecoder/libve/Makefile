
CC          = arm-linux-gnueabihf-gcc
CPP         = arm-linux-gnueabihf-g++
AR         	= arm-linux-gnueabihf-ar
ARFLAGS    	= -crv
CEDARX_CHIP_VERSION = A20

ROOT       	= .
TARGET_DIR  = $(ROOT)
TARGET      = $(TARGET_DIR)/libvecore.so
SRCDIRS     = $(shell find $(ROOT) -maxdepth 4 -type d)

INCLUDES    = $(foreach dir,$(SRCDIRS),-I$(dir))
INCLUDES    +=-I./../ -I./../adapter -I./../adapter/adapter_cedarx_linux_$(CEDARX_CHIP_VERSION) -I../../include

CFLAGS     :=-Wall -O2 -c  $(INCLUDES) -fPIC -ldl

SRCCS      	= $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
OBJS       	= $(ROOT)/*.o

all:objs
	$(CC) -shared -o $(TARGET) $(OBJS)
	
objs:
	@echo $(SRCCS)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRCCS)

	
clean:
	-rm $(OBJS)

