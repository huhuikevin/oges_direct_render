# Copyright (C) 2010 The Android Open Source Project
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
#
LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)
ANDROID_PRIVATE_LIBDIR := $(LOCAL_PATH)/android-libs
ANDROID_SYS_HEADERS    := $(LOCAL_PATH)/android-headers
TARGET_TUPLE=arm-linux-androideabi
ANDROID_API := 18
SYSROOT=$(ANDROID_NDK)/platforms/android-$(ANDROID_API)/arch-arm

################
# PRIVATE LIBS #
################

ANDROID_PRIVATE_LIBS=$(ANDROID_PRIVATE_LIBDIR)/$(TARGET_TUPLE)/libstagefright.so \
					 $(ANDROID_PRIVATE_LIBDIR)/$(TARGET_TUPLE)/libmedia.so \
					 $(ANDROID_PRIVATE_LIBDIR)/$(TARGET_TUPLE)/libutils.so \
					 $(ANDROID_PRIVATE_LIBDIR)/$(TARGET_TUPLE)/libcutils.so \
					 $(ANDROID_PRIVATE_LIBDIR)/$(TARGET_TUPLE)/libbinder.so \
					 $(ANDROID_PRIVATE_LIBDIR)/$(TARGET_TUPLE)/libui.so \
					 $(ANDROID_PRIVATE_LIBDIR)/$(TARGET_TUPLE)/libhardware.so

$(ANDROID_PRIVATE_LIBDIR)/$(TARGET_TUPLE)/%.so: $(ANDROID_PRIVATE_LIBDIR)/%.c
	mkdir -p $(ANDROID_PRIVATE_LIBDIR)/$(TARGET_TUPLE)
	$(GEN)$(TARGET_TUPLE)-gcc $< -shared -o $@ --sysroot=$(SYSROOT)

$(ANDROID_PRIVATE_LIBDIR)/%.c: $(ANDROID_PRIVATE_LIBDIR)/%.symbols
	$(VERBOSE)rm -f $@ && touch $@
	$(GEN)for s in `cat $<`; do echo "void $$s() {}" >> $@; done

###########
# libiOMX #
###########


# no hwbuffer for gingerbread
LIBIOMX_INCLUDES_10 := $(LIBIOMX_INCLUDES_COMMON) \
	$(ANDROID_SYS_HEADERS)/10/frameworks/base/include \
	$(ANDROID_SYS_HEADERS)/10/system/core/include \
	$(ANDROID_SYS_HEADERS)/10/hardware/libhardware/include

LIBIOMX_INCLUDES_13 := $(LIBIOMX_INCLUDES_COMMON) \
	$(ANDROID_SYS_HEADERS)/13/frameworks/base/include \
	$(ANDROID_SYS_HEADERS)/13/frameworks/base/native/include \
	$(ANDROID_SYS_HEADERS)/13/system/core/include \
	$(ANDROID_SYS_HEADERS)/13/hardware/libhardware/include

LIBIOMX_INCLUDES_14 := $(LIBIOMX_INCLUDES_COMMON) \
	$(ANDROID_SYS_HEADERS)/14/frameworks/base/include \
	$(ANDROID_SYS_HEADERS)/14/frameworks/base/native/include \
	$(ANDROID_SYS_HEADERS)/14/system/core/include \
	$(ANDROID_SYS_HEADERS)/14/hardware/libhardware/include

LIBIOMX_INCLUDES_18 := $(LIBIOMX_INCLUDES_COMMON) \
	$(ANDROID_SYS_HEADERS)/18/frameworks/native/include \
	$(ANDROID_SYS_HEADERS)/18/frameworks/av/include \
	$(ANDROID_SYS_HEADERS)/18/system/core/include \
	$(ANDROID_SYS_HEADERS)/18/hardware/libhardware/include

LIBIOMX_INCLUDES_19 := $(LIBIOMX_INCLUDES_COMMON) \
	$(ANDROID_SYS_HEADERS)/19/frameworks/native/include \
	$(ANDROID_SYS_HEADERS)/19/frameworks/av/include \
	$(ANDROID_SYS_HEADERS)/19/system/core/include \
	$(ANDROID_SYS_HEADERS)/19/hardware/libhardware/include

LIBIOMX_INCLUDES_21 := $(LIBIOMX_INCLUDES_COMMON) \
	$(ANDROID_SYS_HEADERS)/21/frameworks/native/include \
	$(ANDROID_SYS_HEADERS)/21/frameworks/av/include \
	$(ANDROID_SYS_HEADERS)/21/system/core/include \
	$(ANDROID_SYS_HEADERS)/21/hardware/libhardware/include


#######
# ANW #
#######
LIBANW_SRC_FILES_COMMON := nativewindow.c


#LOCAL_MODULE := nativewin
#LOCAL_SRC_FILES  := $(LIBANW_SRC_FILES_COMMON)
#LOCAL_C_INCLUDES := $(LIBIOMX_INCLUDES_$(ANDROID_API))
#LOCAL_LDLIBS     := -L$(ANDROID_PRIVATE_LIBDIR)/$(TARGET_TUPLE) -llog -lhardware
#LOCAL_CFLAGS     := $(LIBIOMX_CFLAGS_COMMON) -DANDROID_API=$(ANDROID_API)
#$(TARGET_OUT)/$(LOCAL_MODULE).so: $(ANDROID_PRIVATE_LIBS)
#include $(BUILD_SHARED_LIBRARY)



include $(CLEAR_VARS)

LOCAL_MODULE    := native-activity
LOCAL_SRC_FILES := main.c $(LIBANW_SRC_FILES_COMMON)
LOCAL_C_INCLUDES := $(LIBIOMX_INCLUDES_$(ANDROID_API))
LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv1_CM -L$(ANDROID_PRIVATE_LIBDIR)/$(TARGET_TUPLE) -llog -lhardware
LOCAL_STATIC_LIBRARIES := android_native_app_glue
#LOCAL_SHARED_LIBRARIES := nativewin
LOCAL_CFLAGS     := $(LIBIOMX_CFLAGS_COMMON) -DANDROID_API=$(ANDROID_API)
$(TARGET_OUT)/$(LOCAL_MODULE).so: $(ANDROID_PRIVATE_LIBS)
include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)

