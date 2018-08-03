################################################################################
# Android optee-client and optee-supplicant makefile                                                #
################################################################################
LOCAL_PATH := $(call my-dir)
################################################################################
# Include optee-client common config and flags                                 #
################################################################################
include $(LOCAL_PATH)/config.mk
include $(LOCAL_PATH)/flags.mk

################################################################################
# Build libteec.a - TEE (Trusted Execution Environment) static library         #
################################################################################
include $(CLEAR_VARS)
LOCAL_CFLAGS += -DANDROID_BUILD -D_GNU_SOURCE -DDEBUG=1
LOCAL_CFLAGS += $(CFLAGS)

ifneq ($(CFG_TEE_CLIENT_LOG_FILE),)
LOCAL_CFLAGS += -DTEEC_LOG_FILE=$(CFG_TEE_CLIENT_LOG_FILE)
endif

LOCAL_CFLAGS += -DDEBUGLEVEL_$(CFG_TEE_CLIENT_LOG_LEVEL)
LOCAL_CFLAGS += -DBINARY_PREFIX=\"TEEC\"

LOCAL_SRC_FILES += libteec/src/tee_client_api.c
LOCAL_SRC_FILES += libteec/src/teec_trace.c
LOCAL_LDLIBS := -ldl

LOCAL_C_INCLUDES := $(LOCAL_PATH)/public \
		$(LOCAL_PATH)/libteec/include \

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libteec
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)

################################################################################
# Build libteec.so - TEE (Trusted Execution Environment) shared library        #
################################################################################
include $(CLEAR_VARS)
LOCAL_CFLAGS += -DANDROID_BUILD -D_GNU_SOURCE -DDEBUG=1
LOCAL_CFLAGS += $(CFLAGS)

ifneq ($(CFG_TEE_CLIENT_LOG_FILE),)
LOCAL_CFLAGS += -DTEEC_LOG_FILE=$(CFG_TEE_CLIENT_LOG_FILE)
endif

LOCAL_CFLAGS += -DDEBUGLEVEL_$(CFG_TEE_CLIENT_LOG_LEVEL)
LOCAL_CFLAGS += -DBINARY_PREFIX=\"TEEC\"

LOCAL_SRC_FILES += libteec/src/tee_client_api.c
LOCAL_SRC_FILES += libteec/src/teec_trace.c
LOCAL_LDLIBS := -ldl

LOCAL_C_INCLUDES := $(LOCAL_PATH)/public \
                $(LOCAL_PATH)/libteec/include \

#LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libteec
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

################################################################################
# Build hyper flash driver CA                                                  #
################################################################################
include $(CLEAR_VARS)
LOCAL_CFLAGS += -DANDROID_BUILD
LOCAL_CFLAGS += $(CFLAGS)

LOCAL_SRC_FILES	+= ca/hyper_ca.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/public \
		    $(ANDROID_TOP)external/zlib
LOCAL_LDLIBS := -lz
LOCAL_STATIC_LIBRARIES := libteec libm libz libc libdl
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := hyper_ca
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_SBIN_UNSTRIPPED)
LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

################################################################################
# Build tee supplicant                                                         #
################################################################################

include $(CLEAR_VARS)
include $(LOCAL_PATH)/tee-supplicant/tee_supplicant_android.mk
