LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := imusensortest
LOCAL_SRC_FILES := test.c
LOCAL_SHARED_LIBRARIES := libsensorimu
include $(BUILD_EXECUTABLE)
