LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

$(shell touch $(LOCAL_PATH)/*.so)

LOCAL_MODULE := libxihelib
LOCAL_SRC_FILES := libxihelib.so
#LOCAL_SRC_FILES := libxihelib.so
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
include $(BUILD_PREBUILT)
