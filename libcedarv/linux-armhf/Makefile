#
# Assume standlone toolchain
#
CC  = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
AR  = $(CROSS_COMPILE)ar

CEDARDIR=$(shell pwd)

include $(CEDARDIR)/cedardefs.mk

CFLAGS += $(CEDARINCLUDES)

SRCS = \
	libcedarv/vdecoder.c \
	libcedarv/awprintf.c \
	fbm/fbm.c \
	vbv/vbv.c \
	adapter/libve_adapter.c \
	adapter/os_adapter.c \
	adapter/avheap/avheap.c

OBJS = $(SRCS:%.c=%.o)

LIB = libcedarv.a

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) cq $(LIB) $(OBJS)

clean:
	rm -f $(LIB) $(OBJS)

