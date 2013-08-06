
CC = arm-none-linux-gnueabi-gcc
CPP = arm-none-linux-gnueabi-g++
STRIP = arm-none-linux-gnueabi-strip
AR = arm-none-linux-gnueabi-ar

DATA_SRC=file

SRCDIRS     := . \
			  ./render \
			  ./vdecoder/fbm \
			  ./vdecoder/vbv \
			  ./vdecoder/libvecore \
			  ./vdecoder/libcedarv \
			  ./vdecoder/adapter \
			  ./vdecoder/adapter/cdxalloc \
			  
			  
ifeq ($(DATA_SRC), file)
SRCDIRS+=./file_parser/pmp
else
SRCDIRS+=./stream_parser 
endif
			  
INCLUDES := $(foreach dir,$(SRCDIRS),-I$(dir))
INCLUDES += -I./include -I./file_parser/mov/inc
SRCCS    = $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
OBJ  	 := $(SRCCS:%.c=%.o)
#OBJ=$(addsuffix .o, $(basename $(SRCCS)))

CFLAGS = -Wall -O0 -g
ifeq ($(DATA_SRC), file)
EXT_FLAGS += -DFILE_PARSER
else
EXT_FLAGS += -DSTREAM_PARSER
endif
CFLAGS := $(CFLAGS) $(INCLUDES)
LDFLAGS += -lpthread -lcedarxalloc -lvecore -ldl -lve_base
LDFLAGS +=-L./lib

TARGET = output/ve

all: $(TARGET)

$(OBJ):%.o:%.c
	$(CC) $(CFLAGS) $(EXT_FLAGS) -o $@ -c $<

$(TARGET): $(OBJ)
	$(CPP) $(CFLAGS) ${EXT_FLAGS} $(LDFLAGS) -o $@ $^


clean:
	@rm -f $(TARGET)
	@rm -f $(OBJ)
