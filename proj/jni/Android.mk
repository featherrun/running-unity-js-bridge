LOCAL_PATH := $(call my-dir)/..
LIB_PATH := $(LOCAL_PATH)/external/android/$(TARGET_ARCH_ABI)

include $(CLEAR_VARS)
LOCAL_MODULE := cocos_zlib_static
LOCAL_MODULE_FILENAME := zlib
LOCAL_SRC_FILES := $(LIB_PATH)/libz.a
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := cocos_mozglue_static
LOCAL_MODULE_FILENAME := mozglue
LOCAL_SRC_FILES := $(LIB_PATH)/libmozglue.a
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := spidermonkey_static
LOCAL_MODULE_FILENAME := js_static
LOCAL_SRC_FILES := $(LIB_PATH)/libjs_static.a
LOCAL_EXPORT_C_INCLUDES := $(LIB_PATH)/include/spidermonkey
LOCAL_CPPFLAGS := -D__STDC_LIMIT_MACROS=1 -Wno-invalid-offsetof
LOCAL_EXPORT_CPPFLAGS := -D__STDC_LIMIT_MACROS=1 -Wno-invalid-offsetof
LOCAL_STATIC_LIBRARIES += cocos_mozglue_static
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_C_INCLUDES := $(LIB_PATH)/include/spidermonkey/
#LOCAL_C_INCLUDES += $(LIB_PATH)/include/spidermonkey/js/
#LOCAL_C_INCLUDES += $(LIB_PATH)/include/spidermonkey/mozilla/
LOCAL_MODULE    := sm  
LOCAL_SRC_FILES := $(LOCAL_PATH)/sm.cpp
LOCAL_STATIC_LIBRARIES += spidermonkey_static
LOCAL_STATIC_LIBRARIES += cocos_zlib_static
LOCAL_LDLIBS    := -lm -llog
#APP_STL := stlport_static 
include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)
