// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_clipboard.h>
#include <supp/dag_android_native_app_glue.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <startup/dag_globalSettings.h>

namespace clipboard
{
#define DETACH_VM(vm) \
  if (vm)             \
    vm->DetachCurrentThread();

bool get_clipboard_ansi_text(char *, int) { return false; }
bool set_clipboard_ansi_text(const char *) { return false; }

bool get_clipboard_utf8_text(char *dest, int buf_size)
{
  android_app *app = (android_app *)win32_get_instance();
  JavaVM *vm = app->activity->vm;
  JNIEnv *env;

  jint result = android::attach_current_thread(vm, &env, NULL);
  if (result != JNI_OK)
    return false;

  jclass context_class = env->GetObjectClass(android::get_activity_class(app->activity));
  jmethodID getSystemService_method = env->GetMethodID(context_class, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
  jstring clipboard_service = env->NewStringUTF("clipboard");
  jobject clipboard_manager =
    env->CallObjectMethod(android::get_activity_class(app->activity), getSystemService_method, clipboard_service);
  env->DeleteLocalRef(clipboard_service);
  env->DeleteLocalRef(context_class);

  if (!clipboard_manager)
  {
    DETACH_VM(vm);
    return false;
  }

  jclass clipboard_manager_class = env->GetObjectClass(clipboard_manager);
  jmethodID getPrimaryClip_method = env->GetMethodID(clipboard_manager_class, "getPrimaryClip", "()Landroid/content/ClipData;");
  jobject clip_data = env->CallObjectMethod(clipboard_manager, getPrimaryClip_method);
  env->DeleteLocalRef(clipboard_manager_class);

  if (!clip_data)
  {
    DETACH_VM(vm);
    return false;
  }

  jclass clip_data_class = env->GetObjectClass(clip_data);
  jmethodID getItemCount_method = env->GetMethodID(clip_data_class, "getItemCount", "()I");
  jint item_count = env->CallIntMethod(clip_data, getItemCount_method);
  env->DeleteLocalRef(clip_data_class);

  if (item_count <= 0)
  {
    DETACH_VM(vm);
    return false;
  }

  jmethodID getItemAt_method = env->GetMethodID(clip_data_class, "getItemAt", "(I)Landroid/content/ClipData$Item;");
  jobject clip_item = env->CallObjectMethod(clip_data, getItemAt_method, 0);
  env->DeleteLocalRef(clip_data);

  if (!clip_item)
  {
    DETACH_VM(vm);
    return false;
  }

  jclass clip_item_class = env->GetObjectClass(clip_item);
  jmethodID getText_method = env->GetMethodID(clip_item_class, "getText", "()Ljava/lang/CharSequence;");
  jobject text = env->CallObjectMethod(clip_item, getText_method);
  env->DeleteLocalRef(clip_item_class);

  if (!text)
  {
    DETACH_VM(vm);
    return false;
  }

  jstring text_string = static_cast<jstring>(text);
  const char *utf8_text = env->GetStringUTFChars(text_string, nullptr);
  if (!utf8_text)
  {
    DETACH_VM(vm);
    return false;
  }

  strncpy(dest, utf8_text, buf_size);
  env->ReleaseStringUTFChars(text_string, utf8_text);
  env->DeleteLocalRef(text);
  DETACH_VM(vm);

  return true;
}

bool set_clipboard_utf8_text(const char *text)
{
  android_app *app = (android_app *)win32_get_instance();
  JavaVM *vm = app->activity->vm;
  JNIEnv *env;

  jint result = android::attach_current_thread(vm, &env, NULL);
  if (result != JNI_OK)
    return false;

  jclass context_class = env->GetObjectClass(android::get_activity_class(app->activity));
  jmethodID getSystemService_method = env->GetMethodID(context_class, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
  jstring clipboard_service = env->NewStringUTF("clipboard");
  jobject clipboard_manager =
    env->CallObjectMethod(android::get_activity_class(app->activity), getSystemService_method, clipboard_service);
  env->DeleteLocalRef(clipboard_service);
  env->DeleteLocalRef(context_class);

  if (!clipboard_manager)
  {
    DETACH_VM(vm);
    return false;
  }

  jstring text_string = env->NewStringUTF(text);
  jclass clip_data_class = env->FindClass("android/content/ClipData");
  jmethodID newPlainText_method = env->GetStaticMethodID(clip_data_class, "newPlainText",
    "(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Landroid/content/ClipData;");
  jobject clip_data = env->CallStaticObjectMethod(clip_data_class, newPlainText_method, nullptr, text_string);
  env->DeleteLocalRef(clip_data_class);

  if (!clip_data)
  {
    DETACH_VM(vm);
    return false;
  }

  jclass clipboard_manager_class = env->GetObjectClass(clipboard_manager);
  jmethodID setPrimaryClip_method = env->GetMethodID(clipboard_manager_class, "setPrimaryClip", "(Landroid/content/ClipData;)V");
  env->CallVoidMethod(clipboard_manager, setPrimaryClip_method, clip_data);
  env->DeleteLocalRef(clipboard_manager_class);
  DETACH_VM(vm);
  return true;
}

bool set_clipboard_bmp_image(TexPixel32 * /*im*/, int /*wd*/, int /*ht*/, int /*stride*/)
{
  // no implementation yet
  return false;
}

bool set_clipboard_file(const char * /*filename*/) { return false; }

#undef DETACH_VM

} // namespace clipboard
