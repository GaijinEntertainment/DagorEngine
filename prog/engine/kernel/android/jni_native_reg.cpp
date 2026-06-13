// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <supp/android/jni_native_reg.h>
#include <supp/dag_android_native_app_glue.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <android/log.h>

static JniNativeReg *g_jni_native_reg_head = nullptr;

JniNativeReg::JniNativeReg(const char *class_name, const JNINativeMethod *m, int cnt) :
  className(class_name), methods(m), methodCount(cnt), next(g_jni_native_reg_head)
{
  g_jni_native_reg_head = this;
}

static bool get_jni_env(JNIEnv **out_env)
{
  android_app *state = (android_app *)win32_get_instance();
  if (!state || !state->activity)
    return false;

  JavaVM *vm = state->activity->vm;
  jint rc = vm->GetEnv(reinterpret_cast<void **>(out_env), JNI_VERSION_1_6);
  if (rc == JNI_EDETACHED)
    rc = vm->AttachCurrentThread(out_env, nullptr);
  return rc == JNI_OK;
}

static jclass find_java_class(JNIEnv *env, const char *class_name)
{
  android_app *state = (android_app *)win32_get_instance();
  jobject activityObj = state->activity->clazz;
  jclass activityClass = env->GetObjectClass(activityObj);
  jmethodID getClassLoader = env->GetMethodID(activityClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
  jobject classLoader = env->CallObjectMethod(activityObj, getClassLoader);
  env->DeleteLocalRef(activityClass);

  jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
  jmethodID loadClass = env->GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
  env->DeleteLocalRef(classLoaderClass);

  jstring className = env->NewStringUTF(class_name);
  jclass cls = (jclass)env->CallObjectMethod(classLoader, loadClass, className);
  env->DeleteLocalRef(className);
  env->DeleteLocalRef(classLoader);

  if (!cls || env->ExceptionCheck())
  {
    env->ExceptionClear();
    return nullptr;
  }
  return cls;
}

void jni_register_all_natives()
{
  JNIEnv *env = nullptr;
  if (!get_jni_env(&env))
    __android_log_assert("get_jni_env", "dagor", "jni_register_all_natives: failed to get JNIEnv");

  for (JniNativeReg *reg = g_jni_native_reg_head; reg; reg = reg->next)
  {
    jclass cls = find_java_class(env, reg->className);
    if (!cls)
      __android_log_assert("cls", "dagor", "RegisterNatives: class %s not found", reg->className);

    jint rc = env->RegisterNatives(cls, reg->methods, reg->methodCount);
    env->DeleteLocalRef(cls);
    if (rc != JNI_OK)
      __android_log_assert("rc", "dagor", "RegisterNatives: %s failed (result=%d)", reg->className, rc);

    __android_log_print(ANDROID_LOG_INFO, "dagor", "RegisterNatives: %s ok", reg->className);
  }
}
