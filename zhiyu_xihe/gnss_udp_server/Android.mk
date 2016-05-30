LOCAL_PATH :=$(call my-dir)
include $(CLEAR_VARS)

SDKDIR = .
SERVER_DIR = $(SDKDIR)

LOCAL_CFLAGS += -D_REENTRANT -DLINUX -DANDROID
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(SERVER_DIR)


LOCAL_MODULE := udp_server


# 指定源文件



LOCAL_SRC_FILES := \
					./zy_mempool.c   \
					./zy_net.c   \
					./udp_server.c   


#LOCAL_SRC_FILES += logtest.c

LOCAL_SHARED_LIBRARIES = \
						 liblog\
						 libutils\
						 libcutils\



include $(BUILD_EXECUTABLE)
