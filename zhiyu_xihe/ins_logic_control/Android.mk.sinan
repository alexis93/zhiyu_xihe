LOCAL_PATH :=$(call my-dir)

include $(CLEAR_VARS)

SDKDIR = .

#INS_GPS_DIR   = $(SDKIR)/gps
INS_IMU_DIR   = $(SDKDIR)/imu
#INS_SERVER_DIR = $(SDKDIR)/locationserver
#LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(INS_GPS_DIR)
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(INS_IMU_DIR)
#LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(INS_SERVER_DIR)

LOCAL_SRC_FILES := \
                    ins_main.c\
                    config.c \
                    zy_gnss.c \
                    data_parse.c
				  

LOCAL_MODULE := IMU_gnss
#include rtkgps lib to get gps data
#include lib imu to get imu data
LOCAL_SHARED_LIBRARIES = \
						 liblog\
						 libutils\
						 libcutils\
						 libzhiyulib\
						 libsensorimu\
						 libzy_tcp_worker\
						 libins


LOCAL_MODULE_TAGS:= optional 
LOCAL_CFLAGS+= -DNOVATEL  -DWRITE_LOG -DMAIN_EXE -DSMALL -DSOL_INS
#LOCAL_CFLAGS+= -DUBLOX  -DWRITE_LOG -DMAIN_EXE -DSMALL -DSOL_INS

#build a executable file
include $(BUILD_EXECUTABLE)

