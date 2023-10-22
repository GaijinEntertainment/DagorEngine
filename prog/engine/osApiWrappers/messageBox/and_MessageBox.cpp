#include <osApiWrappers/dag_messageBox.h>
#include <osApiWrappers/dag_unicode.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <debug/dag_debug.h>
#include <string>
#include <vector>

#include <jni.h>
#include <android/native_activity.h>

#include <supp/dag_android_native_app_glue.h>

static int show_alert(const char *utf8_text, const char *utf8_caption, int flags)
{

  android_app *app = (android_app *)win32_get_instance();
  if (app == nullptr)
  {
    debug("%s no android_app instance", __FUNCTION__);
    return GUI_MB_FAIL;
  }

  if (app->activity == nullptr)
  {
    debug("%s no android activity found", __FUNCTION__);
    return GUI_MB_FAIL;
  }

  JNIEnv *jni = NULL;

  if (app->activity->vm->AttachCurrentThread(&jni, NULL) == JNI_ERR)
  {
    debug("%s Can't connect to JVM", __FUNCTION__);
    return GUI_MB_FAIL;
  }

  jobject jActivity = android::get_activity_class(app->activity);
  jclass myClass = jni->GetObjectClass(jActivity);
  jmethodID getClassLoaderID = jni->GetMethodID(myClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
  jobject classLoader = jni->CallObjectMethod(jActivity, getClassLoaderID);
  if (!classLoader)
    return GUI_MB_FAIL;
  jmethodID findClass = jni->GetMethodID(jni->GetObjectClass(classLoader), "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
  jstring strClassName = jni->NewStringUTF("com/gaijinent/MessageBox/MessageBoxWrapper");
  jobject cls = jni->CallObjectMethod(classLoader, findClass, strClassName);
  jni->ExceptionClear();
  jclass clazz_MessageBoxWrapper = cls ? (jclass)jni->NewGlobalRef(cls) : NULL;
  jni->DeleteLocalRef(strClassName);
  if (!clazz_MessageBoxWrapper)
    return GUI_MB_FAIL;

  // public static int showAlert(final Activity activity, final String title, final String message,
  //   final String bt1Text, final String bt2Text, final String bt3Text)
  jmethodID id_showAlert = jni->GetStaticMethodID(clazz_MessageBoxWrapper, "showAlert",
    "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");

  if (id_showAlert == nullptr)
  {
    debug("%s JAVA method not found: com.gaijinent.MessageBox.MessageBoxWrapper.showAlert", __FUNCTION__);
    return GUI_MB_FAIL;
  }

  jstring jtitle = jni->NewStringUTF(utf8_caption);
  jstring jmessage = jni->NewStringUTF(utf8_text);

  const char *buttons[3] = {"OK", "", ""};

  switch (flags & 0xF)
  {
    case GUI_MB_OK_CANCEL:
    {
      buttons[0] = "OK";
      buttons[1] = "Cancel";
    }
    break;

    case GUI_MB_YES_NO:
    {
      buttons[0] = "Yes";
      buttons[1] = "No";
    }
    break;

    case GUI_MB_RETRY_CANCEL:
    {
      buttons[0] = "Retry";
      buttons[1] = "Cancel";
    }
    break;

    case GUI_MB_ABORT_RETRY_IGNORE:
    {
      buttons[0] = "Abort";
      buttons[1] = "Retry";
      buttons[2] = "Ignore";
    }
    break;

    case GUI_MB_YES_NO_CANCEL:
    {
      buttons[0] = "Yes";
      buttons[1] = "No";
      buttons[2] = "Cancel";
    }
    break;

    case GUI_MB_CANCEL_TRY_CONTINUE:
    {
      buttons[0] = "Cancel";
      buttons[1] = "Try";
      buttons[2] = "Continue";
    }
    break;
  }


  jstring jBt1 = jni->NewStringUTF(buttons[0]);
  jstring jBt2 = jni->NewStringUTF(buttons[1]);
  jstring jBt3 = jni->NewStringUTF(buttons[2]);

  int result = jni->CallStaticIntMethod(clazz_MessageBoxWrapper, id_showAlert, jActivity, jtitle, jmessage, jBt1, jBt2, jBt3);

  jni->DeleteLocalRef(jtitle);
  jni->DeleteLocalRef(jmessage);
  jni->DeleteLocalRef(jBt1);
  jni->DeleteLocalRef(jBt2);
  jni->DeleteLocalRef(jBt3);

  jni->DeleteGlobalRef(clazz_MessageBoxWrapper);

  app->activity->vm->DetachCurrentThread();

  return result;
}

int os_message_box(const char *utf8_text, const char *utf8_caption, int flags)
{
  debug("%s(%s, %s, %d)", __FUNCTION__, utf8_text, utf8_caption, flags);
  if (utf8_text == nullptr || utf8_caption == nullptr)
  {
    debug("%s no text or caption", __FUNCTION__);
    return GUI_MB_FAIL;
  }

  int result = show_alert(utf8_text, utf8_caption, flags);

  return result;
}
