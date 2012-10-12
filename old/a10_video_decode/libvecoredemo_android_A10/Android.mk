LOCAL_PATH := $(call my-dir)

#BXP_DMO_PATH := /media/DiskE/bsp/boxchip/android2.3.4/frameworks/base/libvecoredemo
#Build SmsSample tool

include $(CLEAR_VARS)

#include $(LOCAL_PATH)/../../Platforms/Linux/Android/Build/smsandroidmakeenv.inc

#LOCAL_SHARED_LIBRARIES := libcutils libstdc++ libc

LOCAL_CFLAGS += -Wfatal-errors
#LOCAL_CFLAGS += -fno-exceptions
#LOCAL_CFLAGS += -lstdc++

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := bxpve

#add c file below
LOCAL_SRC_FILES := \
			main.c \
			vdecoder/adapter/libve_adapter.c \
			vdecoder/adapter/os_adapter.c \
			vdecoder/fbm/fbm.c \
			vdecoder/libcedarv/vdecoder.c \
			vdecoder/vbv/vbv.c \
			pmp_file_parser/pmp.c \
			pmp_file_parser/pmp_ctrl.c



LOCAL_C_INCLUDES := $(KERNEL_HEADERS) \
			$(LOCAL_PATH)/vdecoder/adapter/cdxalloc \
			$(LOCAL_PATH)/vdecoder/adapter \
			$(LOCAL_PATH)/vdecoder/fbm \
			$(LOCAL_PATH)/vdecoder/libcedarv \
			$(LOCAL_PATH)/vdecoder/libvecore \
			$(LOCAL_PATH)/vdecoder/vbv \
			$(LOCAL_PATH)/pmp_file_parser


#LOCAL_CFLAGS		+= -Wall -g $(INCFLAGS) -DOS_LINUX
#LOCAL_CFLAGS		+= -DDEBUG_LOG
#LOCAL_CFLAGS		+= -DLINUX

#LOCAL_SHARED_LIBRARIES := libcutils libsmscontrol
LOCAL_LDFLAGS = $(LOCAL_PATH)/vdecoder/adapter/cdxalloc/libcedarxalloc.a \
		$(LOCAL_PATH)/vdecoder/libvecore/libvecore.a
#		$(LOCAL_PATH)/libsupc++.a


#LOCAL_LD_FLAGS += -L$(LOCAL_PATH)/


#LOCAL_PRELINK_MODULE := false

#zzf LOCAL_LDLIBS := -llog

include $(BUILD_EXECUTABLE)
