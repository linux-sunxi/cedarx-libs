LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES += \
    libcedarv/vdecoder.c \
    libcedarv/awprintf.c \
    fbm/fbm.c \
    vbv/vbv.c \
    adapter/libve_adapter.c \
    adapter/os_adapter.c

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/adapter \
    $(LOCAL_PATH)/adapter/cdxalloc \
    $(LOCAL_PATH)/fbm \
    $(LOCAL_PATH)/libcedarv \
    $(LOCAL_PATH)/libvecore \
    $(LOCAL_PATH)/vbv

LOCAL_MODULE := libcedarv

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := \
    adapter/cdxalloc/libcedarxalloc.a \
    libvecore/libvecore.a

LOCAL_MODULE_TAGS := optional

include $(BUILD_MULTI_PREBUILT)
