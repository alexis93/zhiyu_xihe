LOCAL_PATH :=$(call my-dir)
include $(CLEAR_VARS)

SDKDIR = .
SERVER_DIR = $(SDKDIR)

LOCAL_CFLAGS += -D_REENTRANT -DLINUX -DANDROID
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(SERVER_DIR)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/
#LOCAL_C_INCLUDES += $(LOCAL_PATH)/zlog

LOCAL_MODULE := tcp_channel_server
# 指定源文件
#SRC +=$(wildcard $(PARENT_PATH)/*.c)
#SRC +=$(wildcard $(PARENT_PATH)/zlog/*.c)
#SRC := $(SRC:$(LOCAL_PATH)/%=%)

#LOCAL_SRC_FILES += $(SRC)
LOCAL_SRC_FILES +=tcp_channel_server.c zy_config.c zy_log.c zy_epoll.c zy_mempool.c zy_protocol.c zy_tcpclient.c zy_tcpserver.c zy_tcp_worker.c 

LOCAL_SHARED_LIBRARIES = \
						 liblog\
						 libutils\
						 libcutils\

include $(BUILD_EXECUTABLE)
