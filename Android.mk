LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	systemmonitor.cpp 

LOCAL_SHARED_LIBRARIES := \
	libsurfaceflinger \
	libbinder \
	libutils \
	libcutils
	
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../services/surfaceflinger

LOCAL_MODULE := systemmonitor

include $(BUILD_EXECUTABLE)
