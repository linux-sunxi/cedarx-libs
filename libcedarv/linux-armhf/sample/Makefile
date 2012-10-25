CEDARDIR=..

include $(CEDARDIR)/cedardefs.mk

CROSS_COMPILE = arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)g++
STRIP = $(CROSS_COMPILE)strip
AR = $(CROSS_COMPILE)ar

DATA_SRC=file

SRCDIRS := . ./render 

ifeq ($(DATA_SRC), file)
SRCDIRS+=./file_parser
#EXT_FLAGS = -DFILE_PARSER
else
SRCDIRS+=./stream_parser
#EXT_FLAGS = -DSTREAM_PARSER
endif

INCLUDES := $(foreach dir,$(SRCDIRS),-I$(dir)) $(CEDARINCLUDES)
SRCCS    = $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
OBJ  	 := $(SRCCS:%.c=%.o)

CFLAGS = -Wall -O0 -g
ifeq ($(DATA_SRC), file)
CFLAGS += -DFILE_PARSER
else
CFLAGS += -DSTREAM_PARSER
endif
#CFLAGS := $(CFLAGS) $(INCLUDES) -march=armv7-a -mfloat-abi=softfp -mfpu=neon
CFLAGS := $(CFLAGS) $(INCLUDES) -march=armv7-a -mfpu=neon
LIBS += -lpthread $(CEDARLIBS)

TARGET = ve

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(EXT_FLAGS) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJ)
