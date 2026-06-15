//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <jni.h>
#include <supp/android/jni_signatures.h>

struct JniNativeReg
{
  const char *className;
  const JNINativeMethod *methods;
  int methodCount;
  JniNativeReg *next;

  JniNativeReg(const char *class_name, const JNINativeMethod *m, int cnt);
};

void jni_register_all_natives();

#define JNI_NATIVE_METHOD(func, sig) {#func, sig, (void *)(func)}

#define JNI_REG_NATIVES(name, class_name, ...)                   \
  static const JNINativeMethod name##_methods[] = {__VA_ARGS__}; \
  static JniNativeReg name(class_name, name##_methods, sizeof(name##_methods) / sizeof(name##_methods[0]))

#define JNI_REG_NATIVES_PULL(name) extern size_t jni_native_reg_pull_##name = 0
