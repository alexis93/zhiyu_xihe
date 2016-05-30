LOCAL_PATH := $(call my-dir)
PARENT_PATH := $(LOCAL_PATH)/

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

LOCAL_SRC_FILES := iwise_loc_hal.c iwise_net_log.c
LOCAL_SRC_FILES += $(SRC)
$(warning $(LOCAL_SRC_FILES))



LOCAL_C_INCLUDES += $(LOCAL_PATH)/..

LOCAL_SHARED_LIBRARIES := \
	liblog	\
	libutils	\
	libcutils  \
	libxihelib \
#	libzy_tcp_worker\
#	libAndroidPos 
LOCAL_MODULE_TAGS:= optional 
#LOCAL_CFLAGS+= -DUBLOX  -DMAIN_EXE -DWRITE_LOG -DSMALL
#LOCAL_CFLAGS+= -DHEXIN  -DWRITE_LOG  -DMAIN_EXE -DSMALL
#LOCAL_CFLAGS+= -DUBLOX  -DWRITE_LOG -DMAIN_EXE -DSMALL  -DSOL_INS -DGPS_DRIVER_LOG
LOCAL_CFLAGS+= -DNOVATEL  -DWRITE_LOG -DMAIN_EXE -DSMALL  -DSOL_INS -DSINAN -DGPS_DRIVER_LOG

# 指定输出文件名
LOCAL_MODULE := libzhiyulib
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/

# 表明要编译成一个动态库
include $(BUILD_SHARED_LIBRARY)
