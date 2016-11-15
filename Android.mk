################################################################################
# Android optee-client and optee-supplicant makefile                                                #
################################################################################
LOCAL_PATH := $(call my-dir)
#export CROSS_COMPILE=$(ANDROID_BUILD_TOP)/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-gnu-5.1/bin/aarch64-linux-gnu-
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

ifeq ($(CFG_TEE_CLIENT_LOG_FILE), true)
LOCAL_CFLAGS += -DTEEC_LOG_FILE=$(CFG_TEE_CLIENT_LOG_FILE)
endif
#CROSS_COMPILE := $(ANDROID_BUILD_TOP)/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-gnu-5.1/bin/aarch64-linux-gnu-

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

ifeq ($(CFG_TEE_CLIENT_LOG_FILE), true)
LOCAL_CFLAGS += -DTEEC_LOG_FILE=$(CFG_TEE_CLIENT_LOG_FILE)
endif
#CROSS_COMPILE := $(ANDROID_BUILD_TOP)/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-gnu-5.1/bin/aarch64-linux-gnu-

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
include $(BUILD_SHARED_LIBRARY)


################################################################################
# Build tee supplicant                                                         #
################################################################################
include $(CLEAR_VARS)
LOCAL_CFLAGS += -DANDROID_BUILD -DDEBUG=1
LOCAL_CFLAGS += $(CFLAGS)

LOCAL_CFLAGS += -DDEBUGLEVEL_$(CFG_TEE_SUPP_LOG_LEVEL)
LOCAL_CFLAGS += -DBINARY_PREFIX=\"TEES\"
LOCAL_CFLAGS += -DTEEC_LOAD_PATH=\"$(CFG_TEE_CLIENT_LOAD_PATH)\"
LOCAL_SRC_FILES := $(call all-c-files-under, ./tee-supplicant/src/)
#LOCAL_SRC_FILES += tee-supplicant/src/handle.c
#LOCAL_SRC_FILES += tee-supplicant/src/tee_supp_fs.c
#LOCAL_SRC_FILES	+= tee-supplicant/src/tee_supplicant.c
#LOCAL_SRC_FILES	+= tee-supplicant/src/teec_ta_load.c
#LOCAL_SRC_FILES	+= tee-supplicant/src/rpmb.c
#LOCAL_SRC_FILES	+= tee-supplicant/src/sha2.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/public \
		$(LOCAL_PATH)/libteec/include \
		$(LOCAL_PATH)/tee-supplicant/src

#LOCAL_LDLIBS := -ldl
LOCAL_STATIC_LIBRARIES := libteec libm libz libc libdl
LOCAL_MODULE := tee-supp
LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := true
#LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
#LOCAL_POST_PROCESS_COMMAND := $(hide) cp $(ANDROID_PRODUCT_OUT)/system/bin/tee-supp $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_POST_INSTALL_CMD :=  cp $(ANDROID_PRODUCT_OUT)/system/bin/tee-supp $(TARGET_RECOVERY_ROOT_OUT)/sbin

include $(BUILD_EXECUTABLE)

################################################################################
# Build hyper flash driver CA                                                         #
################################################################################
include $(CLEAR_VARS)
LOCAL_CFLAGS += -DANDROID_BUILD
LOCAL_CFLAGS += $(CFLAGS)


LOCAL_SRC_FILES	+= ca/hyper_ca.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/public \
		    $(ANDROID_TOP)external/zlib
LOCAL_LDLIBS := -lz
LOCAL_STATIC_LIBRARIES := libteec libm libz libc libdl
LOCAL_MODULE := hyper_ca
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_FORCE_STATIC_EXECUTABLE := true
include $(BUILD_EXECUTABLE)

################################################################################
# Build hyper flash driver CA                                                         #
################################################################################
include $(CLEAR_VARS)
LOCAL_CFLAGS += -DANDROID_BUILD
LOCAL_CFLAGS += $(CFLAGS)


LOCAL_SRC_FILES	+= loader/tee_loader.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/public

LOCAL_STATIC_LIBRARIES := libteec libm libz libc libdl
LOCAL_MODULE := tee_loader
LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin

include $(BUILD_EXECUTABLE)
