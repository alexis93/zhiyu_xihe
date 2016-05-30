# Copyright (C) 2011 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# 指定源文件
RTKLIB :=$(wildcard $(LOCAL_PATH)/*.c)
RTKLIB +=$(wildcard $(LOCAL_PATH)/rcv/*.c)
RTKLIB +=$(wildcard $(LOCAL_PATH)/xihe/*.c)
RTKLIB := $(RTKLIB:$(LOCAL_PATH)/%=%)


LOCAL_MODULE := librtklib
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/
LOCAL_SRC_FILES := $(RTKLIB)
LOCAL_CFLAGS := -Wmissing-field-initializers
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS+= -Wimplicit-function-declaration -Wmissing-field-initializers
include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)
