# Copyright (C) 2007 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    recovery.cpp \
    bootloader.cpp \
    install.cpp \
    roots.cpp \
    ui.cpp \
    screen_ui.cpp \
    verifier.cpp \
    adb_install.cpp
# MStar Android Patch Begin
LOCAL_SRC_FILES += backup_restore.cpp
# MStar Android Patch End

LOCAL_MODULE := recovery

LOCAL_FORCE_STATIC_EXECUTABLE := true

RECOVERY_API_VERSION := 3
RECOVERY_FSTAB_VERSION := 2
LOCAL_CFLAGS += -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)

LOCAL_STATIC_LIBRARIES := \
    libext4_utils_static \
    libsparse_static \
    libminzip \
    libz \
    libmtdutils \
    libmincrypt \
    libminadbd \
    libminui \
    libpixelflinger_static \
    libpng \
    libfs_mgr \
    libcutils \
    liblog \
    libselinux \
    libstdc++ \
    libm \
    libc

ifeq ($(TARGET_USERIMAGES_USE_EXT4), true)
    LOCAL_CFLAGS += -DUSE_EXT4
    LOCAL_C_INCLUDES += system/extras/ext4_utils
    LOCAL_STATIC_LIBRARIES += libext4_utils_static libz
endif

# MStar Android Patch Begin
LOCAL_C_INCLUDES += external/e2fsprogs/lib
LOCAL_CFLAGS += -O2 -g -W -Wall \
    -DHAVE_UNISTD_H \
    -DHAVE_ERRNO_H \
    -DHAVE_NETINET_IN_H \
    -DHAVE_SYS_IOCTL_H \
    -DHAVE_SYS_MMAN_H \
    -DHAVE_SYS_MOUNT_H \
    -DHAVE_SYS_RESOURCE_H \
    -DHAVE_SYS_SELECT_H \
    -DHAVE_SYS_STAT_H \
    -DHAVE_SYS_TYPES_H \
    -DHAVE_STDLIB_H \
    -DHAVE_STRDUP \
    -DHAVE_MMAP \
    -DHAVE_UTIME_H \
    -DHAVE_GETPAGESIZE \
    -DHAVE_EXT2_IOCTLS \
    -DHAVE_TYPE_SSIZE_T \
    -DHAVE_SYS_TIME_H \
    -DHAVE_SYS_PARAM_H \
    -DHAVE_SYSCONF \
    -DHAVE_LINUX_FD_H \
    -DHAVE_SYS_PRCTL_H \
    -DHAVE_LSEEK64 \
    -DHAVE_LSEEK64_PROTOTYPE
LOCAL_STATIC_LIBRARIES += libext2_blkid libext2_uuid
# MStar Android Patch End

# This binary is in the recovery ramdisk, which is otherwise a copy of root.
# It gets copied there in config/Makefile.  LOCAL_MODULE_TAGS suppresses
# a (redundant) copy of the binary in /system/bin for user builds.
# TODO: Build the ramdisk image in a more principled way.
LOCAL_MODULE_TAGS := eng

ifeq ($(TARGET_RECOVERY_UI_LIB),)
  LOCAL_SRC_FILES += default_device.cpp
else
  LOCAL_STATIC_LIBRARIES += $(TARGET_RECOVERY_UI_LIB)
endif

# MStar Android Patch Begin
ifneq ($(BUILD_MSTARTV),)
    LOCAL_WHOLE_STATIC_LIBRARIES := libutopia
# Update ursa
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/ursautils
    LOCAL_CFLAGS += -DUSE_URSA_UTILS
    LOCAL_STATIC_LIBRARIES += libursautils

# Update mboot
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/mbootutils
    LOCAL_CFLAGS += -DUSE_MBOOT_UTILS
    LOCAL_STATIC_LIBRARIES += libmbootutils

# Secure boot
ifeq ($(BUILD_WITH_SECURE_BOOT),true)
    LOCAL_CFLAGS += -DBUILD_WITH_SECURE_BOOT
    LOCAL_STATIC_LIBRARIES += libsecureboot
endif
LOCAL_STATIC_LIBRARIES += libmcrypto

# led light
ifeq ($(BUILD_WITH_RECOVERY_LED),true)
    LOCAL_C_INCLUDES += $(TARGET_UTOPIA_LIBS_DIR)/include
    LOCAL_CFLAGS += -DBUILD_WITH_RECOVERY_LED
endif

# MI service
ifeq ($(BUILD_WITH_RECOVERY_MI),true)
    LOCAL_CFLAGS += -DBUILD_WITH_RECOVERY_MI
endif

ifeq ($(BUILD_FOR_STB),true)
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/envutils
    LOCAL_CFLAGS += -DUSE_ENV_UTILS
    LOCAL_STATIC_LIBRARIES += libenvutils
endif
endif
# MStar Android Patch End

LOCAL_C_INCLUDES += system/extras/ext4_utils

include $(BUILD_EXECUTABLE)



include $(CLEAR_VARS)
LOCAL_MODULE := verifier_test
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_TAGS := tests
LOCAL_SRC_FILES := \
    verifier_test.cpp \
    verifier.cpp \
    ui.cpp
LOCAL_STATIC_LIBRARIES := \
    libmincrypt \
    libminui \
    libcutils \
    libstdc++ \
    libc
include $(BUILD_EXECUTABLE)

# MStar Android Patch Begin
include $(LOCAL_PATH)/minui/Android.mk \
    $(LOCAL_PATH)/minelf/Android.mk \
    $(LOCAL_PATH)/minzip/Android.mk \
    $(LOCAL_PATH)/minadbd/Android.mk \
    $(LOCAL_PATH)/mtdutils/Android.mk \
    $(LOCAL_PATH)/tools/Android.mk \
    $(LOCAL_PATH)/edify/Android.mk \
    $(LOCAL_PATH)/updater/Android.mk \
    $(LOCAL_PATH)/applypatch/Android.mk \
    $(LOCAL_PATH)/mbootutils/Android.mk \
    $(LOCAL_PATH)/secureboot/Android.mk \
    $(LOCAL_PATH)/envutils/Android.mk \
    $(LOCAL_PATH)/fbdev/Android.mk \
    $(LOCAL_PATH)/ursautils/Android.mk
# MStar Android Patch End
