CROSS_COMPILE = arm-none-linux-gnueabi-
CC = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)g++
STRIP = $(CROSS_COMPILE)strip
AR = $(CROSS_COMPILE)ar

CFLAGS += -Wall -DOS_LINUX -I./cedarv
LIBS += -pthread ./lib/libcedarv.a ./lib/libh264enc.a ./lib/libcedarv_osal.a ./lib/libcedarxalloc.a

SRC := $(wildcard *.c)
OBJ := $(SRC:%.c=%.o)

TARGET := EncoderDemo
.PHONY : clean all

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CPP) $(CFLAGS) -o $@ $^ $(LIBS)
	$(STRIP) $@

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJ)

