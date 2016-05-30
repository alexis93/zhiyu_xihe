LOCAL_PATH := $(call my-dir)
PARENT_PATH := $(LOCAL_PATH)/..

include $(CLEAR_VARS)

# 指定源文件

SRC +=$(wildcard $(PARENT_PATH)/iwise_loc_base/Parserlib/*.c)
SRC +=$(wildcard $(PARENT_PATH)/iwise_loc_base/Parserlib/rcv/*.c)
SRC +=$(wildcard $(PARENT_PATH)/iwise_loc_base/*.c)
SRC +=$(wildcard $(PARENT_PATH)/iwise_loc_hardware_layer3/*.c)
SRC +=$(wildcard $(PARENT_PATH)/iwise_loc_hardware_layer3/rcv/*.c)
SRC +=$(wildcard $(PARENT_PATH)/iwise_loc_handle_layer2/*.c)
SRC +=$(wildcard $(PARENT_PATH)/iwise_loc_interface_layer1/*.c)

SRC := $(SRC:$(LOCAL_PATH)/%=%)

LOCAL_SRC_FILES := ../iwise_loc_hal.c realtime-test.c
LOCAL_SRC_FILES += $(SRC)
$(warning $(LOCAL_SRC_FILES))



LOCAL_C_INCLUDES += $(LOCAL_PATH)/..

LOCAL_SHARED_LIBRARIES := \
	liblog	\
	libutils	\
	libcutils  \
	libxihelib

LOCAL_MODULE_TAGS:= optional 

#LOCAL_CFLAGS+= -DUBLOX -DWRITE_LOG -DMAIN_EXE -DSMALL
#LOCAL_CFLAGS+= -DUBLOX -DMAIN_EXE -DSMALL
#LOCAL_CFLAGS+= -DHEXIN  -DWRITE_LOG -DMAIN_EXE -DSMALL
LOCAL_CFLAGS+= -DUBLOX  -DWRITE_LOG -DMAIN_EXE -DSMALL -DSOL_INS

# 指定输出文件名
LOCAL_MODULE := gps_realtime_local

# 表明要编译成一个可执行文件
include $(BUILD_EXECUTABLE)
