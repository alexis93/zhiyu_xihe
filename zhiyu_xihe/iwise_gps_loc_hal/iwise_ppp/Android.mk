LOCAL_PATH := $(call my-dir)
PARENT_PATH := $(LOCAL_PATH)/

include $(CLEAR_VARS)

# 指定源文件

SRC +=$(wildcard $(PARENT_PATH)/*.c)

SRC := $(SRC:$(LOCAL_PATH)/%=%)

LOCAL_SRC_FILES += $(SRC)
$(warning $(LOCAL_SRC_FILES))



LOCAL_C_INCLUDES += $(LOCAL_PATH)/..

LOCAL_SHARED_LIBRARIES := \
	liblog	\
	libutils	\
	libcutils  \

LOCAL_MODULE_TAGS:= optional 
#LOCAL_CFLAGS+= -DUBLOX  -DMAIN_EXE -DWRITE_LOG -DSMALL


# 指定输出文件名
LOCAL_MODULE := libxihelib
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/

# 表明要编译成一个动态库
include $(BUILD_SHARED_LIBRARY)
