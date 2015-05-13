LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := graphics.c events.c resources.c
# MStar Android Patch Begin
LOCAL_SRC_FILES += gb2313_unicode.cpp
# MStar Android Patch End

LOCAL_C_INCLUDES +=\
    external/libpng\
    external/zlib

LOCAL_MODULE := libminui

# This used to compare against values in double-quotes (which are just
# ordinary characters in this context).  Strip double-quotes from the
# value so that either will work.

ifeq ($(subst ",,$(TARGET_RECOVERY_PIXEL_FORMAT)),RGBX_8888)
  LOCAL_CFLAGS += -DRECOVERY_RGBX
endif
ifeq ($(subst ",,$(TARGET_RECOVERY_PIXEL_FORMAT)),BGRA_8888)
  LOCAL_CFLAGS += -DRECOVERY_BGRA
endif

# MStar Android Patch Begin
ifneq ($(BUILD_MSTARTV),)
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../fbdev
    LOCAL_CFLAGS += \
        -DMSOS_TYPE_LINUX \
        -DUSE_FBDEV
    LOCAL_WHOLE_STATIC_LIBRARIES := libfbdev_recovery
endif
# MStar Android Patch End

ifneq ($(TARGET_RECOVERY_OVERSCAN_PERCENT),)
  LOCAL_CFLAGS += -DOVERSCAN_PERCENT=$(TARGET_RECOVERY_OVERSCAN_PERCENT)
else
  LOCAL_CFLAGS += -DOVERSCAN_PERCENT=0
endif

include $(BUILD_STATIC_LIBRARY)
