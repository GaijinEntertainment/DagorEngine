#include <osApiWrappers/dag_progGlobals.h>
#include <supp/dag_android_native_app_glue.h>

bool isDebuggerPresent_android()
{
  android_app *app = (android_app *)win32_get_instance();
  if (!app || !app->activity)
    return false;

  JavaVM *javaVM = app->activity->vm;
  JNIEnv *jniEnv = app->activity->env;
  if (!javaVM || !jniEnv)
    return false;

  JavaVMAttachArgs attachArgs;
  attachArgs.version = JNI_VERSION_1_6;
  attachArgs.name = "NativeThread";
  attachArgs.group = NULL;

  jint result = javaVM->AttachCurrentThread(&jniEnv, &attachArgs);
  if (result == JNI_ERR)
    return false;

  jclass class_debug = jniEnv->FindClass("android/os/Debug");
  if (!class_debug)
  {
    javaVM->DetachCurrentThread();
    return false;
  }

  jmethodID method_is_debugger_connected = jniEnv->GetStaticMethodID(class_debug, "isDebuggerConnected", "()Z");
  if (!method_is_debugger_connected)
  {
    javaVM->DetachCurrentThread();
    return false;
  }

  jboolean is_debugger_connected = jniEnv->CallStaticBooleanMethod(class_debug, method_is_debugger_connected);

  javaVM->DetachCurrentThread();
  return (is_debugger_connected == JNI_TRUE);
}