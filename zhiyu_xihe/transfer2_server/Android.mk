LOCAL_PATH :=$(call my-dir)
PARENT_PATH := $(LOCAL_PATH)/
include $(CLEAR_VARS)

#SDKDIR = .
#INS_LOGIC_DIR = $(SDKDIR)

#SRC +=$(wildcard $(PARENT_PATH)/src/zlog/*.c)
SRC +=$(wildcard $(PARENT_PATH)/src/*.c)

SRC := $(SRC:$(LOCAL_PATH)/%=%)

#LOCAL_SRC_FILES := iwise_loc_hal.c 
LOCAL_SRC_FILES += $(SRC)

$(warning $(LOCAL_SRC_FILES))
#LOCAL_C_INCLUDES += $(LOCAL_PATH)/..
LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/
#LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/zlog/

#include rtkgps lib to get gps data
#include lib imu to get imu data
LOCAL_SHARED_LIBRARIES = \
						 liblog\
						 libutils\
						 libcutils
						
LOCAL_MODULE_TAGS:= optional
LLOCAL_LDLIBS := -llog
# 指定输出文件名 
LOCAL_MODULE := gnss_tran_pos_wuxi

#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/
#LOCAL_CFLAGS+= -DUBLOX  -DWRITE_LOG -DMAIN_EXE -DSMALL -DSOL_INS

# 表明要编译
#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_EXECUTABLE)
