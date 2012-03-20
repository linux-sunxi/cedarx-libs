
CC = arm-none-linux-gnueabi-gcc
CPP = arm-none-linux-gnueabi-g++
STRIP = arm-none-linux-gnueabi-strip
AR = arm-none-linux-gnueabi-ar

SRCDIRS     = . \
			  ./pmp_file_parser \
			  ./vdecoder/fbm \
			  ./vdecoder/vbv \
			  ./vdecoder/libvecore \
			  ./vdecoder/libcedarv \
			  ./vdecoder/adapter \
			  ./vdecoder/adapter/cdxalloc

INCLUDES := $(foreach dir,$(SRCDIRS),-I$(dir))
SRCCS    = $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
OBJ  	 := $(SRCCS:%.c=%.o)

CFLAGS = -Wall -O0 -g
CFLAGS := $(CFLAGS) $(INCLUDES)
LIBS += -lpthread ./vdecoder/adapter/cdxalloc/libcedarxalloc.a ./vdecoder/libvecore/libvecore.a

TARGET = ve

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CPP) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJ)
