LOCAL_PATH :=$(call my-dir)

include $(CLEAR_VARS)

SDKDIR = .
#INS_LOGIC_DIR = $(SDKDIR)

LOCAL_SRC_FILES :=zy_config.c zy_protocol.c zy_tcpclient.c zy_tcp_worker.c

#include rtkgps lib to get gps data
#include lib imu to get imu data
LOCAL_SHARED_LIBRARIES = \
						 liblog\
						 libutils\
						 libcutils
						
LOCAL_MODULE_TAGS:= optional
LLOCAL_LDLIBS := -llog
# 指定输出文件名 
LOCAL_MODULE := libzy_tcp_worker

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/

# 表明要编译
include $(BUILD_SHARED_LIBRARY)
