// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_clipboard.h>
#include <osApiWrappers/dag_unicode.h>
#include <supp/dag_android_native_app_glue.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <startup/dag_globalSettings.h>
#include <string.h>

namespace clipboard
{
#define DETACH_VM(vm) \
  if (vm)             \
    vm->DetachCurrentThread();

bool get_clipboard_ansi_text(char *, int) { return false; }
bool set_clipboard_ansi_text(const char *) { return false; }

bool get_clipboard_utf8_text(char *dest, int buf_size)
{
  if (!dest || buf_size < 1)
    return false;
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
  // GetStringUTFChars gives JNI modified UTF-8 (non-BMP as CESU-8 pairs), not standard UTF-8;
  // getBytes does. Convert only the dest-sized prefix, not a whole huge clipboard.
  jint str_len = env->GetStringLength(text_string);
  jint want = buf_size - 1 < str_len ? buf_size - 1 : str_len; // >= 1 UTF-8 byte per UTF-16 unit
  if (want > 0 && want < str_len)
  {
    jchar last = 0;
    env->GetStringRegion(text_string, want - 1, 1, &last);
    if (last >= 0xD800 && last <= 0xDBFF) // don't split a surrogate pair; getBytes would emit '?'
      --want;
  }

  if (want <= 0) // nothing to copy
  {
    dest[0] = '\0';
    env->DeleteLocalRef(text);
    DETACH_VM(vm);
    return true;
  }

  jclass string_class = env->GetObjectClass(text_string);
  jmethodID substring_method = env->GetMethodID(string_class, "substring", "(II)Ljava/lang/String;");
  jmethodID getBytes_method = env->GetMethodID(string_class, "getBytes", "(Ljava/lang/String;)[B");
  env->DeleteLocalRef(string_class);
  jstring prefix = (jstring)env->CallObjectMethod(text_string, substring_method, 0, want);
  jstring charset = env->NewStringUTF("UTF-8");
  jbyteArray byte_array = (jbyteArray)env->CallObjectMethod(prefix, getBytes_method, charset);
  env->DeleteLocalRef(charset);
  env->DeleteLocalRef(prefix);
  if (!byte_array)
  {
    env->DeleteLocalRef(text);
    DETACH_VM(vm);
    return false;
  }

  jsize len = env->GetArrayLength(byte_array);
  jbyte *bytes = env->GetByteArrayElements(byte_array, nullptr);
  if (!bytes)
  {
    env->DeleteLocalRef(byte_array);
    env->DeleteLocalRef(text);
    DETACH_VM(vm);
    return false;
  }
  int n = utf8_truncate_len((const char *)bytes, len < buf_size - 1 ? (int)len : buf_size - 1);
  memcpy(dest, bytes, n);
  dest[n] = '\0';
  env->ReleaseByteArrayElements(byte_array, bytes, JNI_ABORT);
  env->DeleteLocalRef(byte_array);
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
