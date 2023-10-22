#include <supp/dag_android_native_app_glue.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <humanInput/dag_hiCreate.h>
#include <ioSys/dag_dataBlock.h>
#include <jni.h>
#include "ime_android_attr.h"
#include <util/dag_simpleString.h>
#include <util/dag_delayedAction.h>
#include <debug/dag_debug.h>

static bool canBeVisible = false, canBeHidden = false;

namespace softinput
{
static void *s_AttachedActivity = NULL;
static bool s_Inited = false;
static JNIEnv *s_Jni = NULL;

static jobject s_InputMethodManager = NULL;
static jmethodID s_GetVisHeightID = NULL;
static jmethodID s_HideSoftInputID = NULL;
static jmethodID s_ShowSoftInputID = NULL;

static jobject s_DecorView = NULL;
static jobject s_Token = NULL;

static jobject lNativeActivity = NULL;
static jmethodID methodShowKeyboard = NULL;
static jmethodID methodHideKeyboard = NULL;

static bool attach(void *activity);
static bool reattach(bool force = false);
static bool show(uint32_t flags);
static bool hide(uint32_t flags);
static bool isShown();
} // namespace softinput

namespace nvsoftinput
{
static void *s_AttachedActivity = NULL;
static bool s_Inited = false;
static JNIEnv *s_Jni = NULL;

static jclass s_NvSoftInputClass = NULL;
static jmethodID s_ShowID = NULL;
static jmethodID s_HideID = NULL;
static jmethodID s_IsShownID = NULL;

static bool attach(void *activity = nullptr);
static bool show(const char *initalText, const char *hint, uint32_t editFlags, uint32_t imeOptions);
static void hide();
static bool isShown();
}; // namespace nvsoftinput


void HumanInput::showScreenKeyboard(bool show)
{
  if (show)
    softinput::show(0);
  else
    softinput::hide(0);
}

static void (*ime_finish_cb)(void *userdata, const char *str, int status) = NULL;
static void *ime_finish_userdata = NULL;
static bool ime_started = false;

void android_init_soft_input(android_app *state)
{
  softinput::attach(state->activity);
  nvsoftinput::attach(state->activity);
}
void android_hide_soft_input()
{
  nvsoftinput::hide();
  softinput::hide(0);
}

bool HumanInput::isImeAvailable() { return nvsoftinput::s_Inited; }
bool HumanInput::showScreenKeyboard_IME(const DataBlock &init_params, void(on_finish_cb)(void *userdata, const char *str, int status),
  void *userdata)
{
  if (!init_params.paramCount() && !on_finish_cb && userdata)
  {
    if (ime_finish_cb && userdata == ime_finish_userdata)
    {
      ime_finish_cb = NULL;
      ime_finish_userdata = NULL;
      nvsoftinput::hide();
      return true;
    }
    return false;
  }
  if (ime_started)
  {
    logwarn("IME double start prevented");
    return true;
  }

  ime_finish_cb = on_finish_cb;
  ime_finish_userdata = userdata;
  ime_started = true;

  unsigned edit_flags = TYPE_CLASS_TEXT, ime_flags = IME_ACTION_DONE;

  if (const char *type = init_params.getStr("type", NULL))
  {
    if (strcmp(type, "lat") == 0)
      ime_flags |= IME_FLAG_FORCE_ASCII;
    else if (strcmp(type, "url") == 0)
      edit_flags |= TYPE_TEXT_VARIATION_URI;
    else if (strcmp(type, "mail") == 0)
      edit_flags |= TYPE_TEXT_VARIATION_EMAIL_ADDRESS;
    else if (strcmp(type, "num") == 0)
      edit_flags = TYPE_CLASS_NUMBER | TYPE_NUMBER_VARIATION_NORMAL;
    else
      logerr("unrecognized IME type: <%s>", type);
  }
  if (const char *type = init_params.getStr("label", NULL))
  {
    if (strcmp(type, "send") == 0)
      ime_flags |= IME_ACTION_SEND;
    else if (strcmp(type, "search") == 0)
      ime_flags |= IME_ACTION_SEARCH;
    else if (strcmp(type, "go") == 0)
      ime_flags |= IME_ACTION_GO;
    else
      logerr("unrecognized IME label: <%s>", type);
  }
  if (init_params.getBool("optMultiLine", false))
    edit_flags |= TYPE_TEXT_FLAG_IME_MULTI_LINE;
  if (init_params.getBool("optPassw", false))
    edit_flags |= TYPE_TEXT_VARIATION_PASSWORD;
  if (init_params.getBool("optNoCopy", false))
    ; // n/a
  if (init_params.getBool("optFixedPos", false))
    ; // n/a
  if (!init_params.getBool("optNoAutoCap",
        (edit_flags & (TYPE_TEXT_VARIATION_URI | TYPE_TEXT_VARIATION_EMAIL_ADDRESS | TYPE_TEXT_VARIATION_VISIBLE_PASSWORD))))
    edit_flags |= TYPE_TEXT_FLAG_CAP_WORDS;
  if (init_params.getBool("optNoLearning", false))
    edit_flags |= TYPE_TEXT_FLAG_NO_SUGGESTIONS;
  // init_params.getInt("maxChars", 128));

  // debug("show: str=<%s> hint=<%s> flg=%08X ime=%08X (%s,%s)", init_params.getStr("str"), init_params.getStr("hint", ""), edit_flags,
  // ime_flags,
  //   init_params.getStr("type", NULL), init_params.getStr("label", NULL));
  return nvsoftinput::show(init_params.getStr("str"), init_params.getStr("hint", ""), edit_flags, ime_flags);
}

int HumanInput::getScreenKeyboardStatus_android()
{
  if (!canBeVisible)
    return 0;

  if (softinput::isShown())
  {
    canBeHidden = true;
    return 2;
  }
  else if (nvsoftinput::isShown())
  {
    canBeHidden = true;
    return 1;
  }

  if (canBeHidden)
  {
    canBeVisible = false;
    ime_started = false;
  }
  return 0;
}


static bool softinput::attach(void *activity)
{
  if (!activity && !s_Inited)
    return false;

  JavaVMAttachArgs jvmAttachArgs;
  jvmAttachArgs.version = JNI_VERSION_1_6;
  jvmAttachArgs.name = "NativeThread";
  jvmAttachArgs.group = NULL;

  // always attach to the caller's thread
  jint result = android::get_java_vm(activity)->AttachCurrentThread(&s_Jni, &jvmAttachArgs);
  if (result == JNI_ERR)
    return false;

  // check if already inited
  if (s_AttachedActivity == activity && s_Inited)
    return true;

  s_AttachedActivity = activity;

  lNativeActivity = android::get_activity_class(activity);
  jclass ClassNativeActivity = s_Jni->GetObjectClass(lNativeActivity);

  jclass ClassContext = s_Jni->FindClass("android/content/Context");
  jfieldID FieldINPUT_METHOD_SERVICE = s_Jni->GetStaticFieldID(ClassContext, "INPUT_METHOD_SERVICE", "Ljava/lang/String;");
  jobject INPUT_METHOD_SERVICE = s_Jni->GetStaticObjectField(ClassContext, FieldINPUT_METHOD_SERVICE);

  jclass ClassInputMethodManager = s_Jni->FindClass("android/view/inputmethod/InputMethodManager");
  jmethodID MethodGetSystemService =
    s_Jni->GetMethodID(ClassNativeActivity, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
  s_InputMethodManager = s_Jni->NewGlobalRef(s_Jni->CallObjectMethod(lNativeActivity, MethodGetSystemService, INPUT_METHOD_SERVICE));

  s_HideSoftInputID = s_Jni->GetMethodID(ClassInputMethodManager, "hideSoftInputFromWindow", "(Landroid/os/IBinder;I)Z");
  s_ShowSoftInputID = s_Jni->GetMethodID(ClassInputMethodManager, "showSoftInput", "(Landroid/view/View;I)Z");
  // Android-L and up method; an exception can occur
  s_GetVisHeightID = s_Jni->GetMethodID(ClassInputMethodManager, "getInputMethodWindowVisibleHeight", "()I");
  s_Jni->ExceptionClear();

  jmethodID MethodGetWindow = s_Jni->GetMethodID(ClassNativeActivity, "getWindow", "()Landroid/view/Window;");
  jobject Window = s_Jni->CallObjectMethod(lNativeActivity, MethodGetWindow);
  jclass ClassWindow = s_Jni->FindClass("android/view/Window");
  jmethodID MethodGetDecorView = s_Jni->GetMethodID(ClassWindow, "getDecorView", "()Landroid/view/View;");
  s_DecorView = s_Jni->NewGlobalRef(s_Jni->CallObjectMethod(Window, MethodGetDecorView));

  jclass ClassView = s_Jni->FindClass("android/view/View");
  jmethodID MethodGetWindowToken = s_Jni->GetMethodID(ClassView, "getWindowToken", "()Landroid/os/IBinder;");
  s_Token = s_Jni->NewGlobalRef(s_Jni->CallObjectMethod(s_DecorView, MethodGetWindowToken));

  methodShowKeyboard = s_Jni->GetMethodID(ClassNativeActivity, "showKeyboard", "()V");
  s_Jni->ExceptionClear();

  methodHideKeyboard = s_Jni->GetMethodID(ClassNativeActivity, "hideKeyboard", "()V");
  s_Jni->ExceptionClear();

  // a minimal working set without s_GetVisHeightID
  s_Inited = s_InputMethodManager && s_DecorView && s_Token && s_ShowSoftInputID && s_HideSoftInputID;

  debug("softinput inited:   %d", s_Inited);
  return s_Inited;
}

static bool softinput::reattach(bool force)
{
  android_app *state = (android_app *)win32_get_instance();
  s_Inited = force ? false : s_Inited;
  return attach(state->activity);
}

static bool softinput::show(uint32_t flags)
{
  if (!reattach())
    return false;

  bool result;
  if (methodShowKeyboard)
  {
    debug("CommonActivity::showKeyboard");
    s_Jni->CallVoidMethod(lNativeActivity, methodShowKeyboard);
    result = true;
  }
  else
  {
    result = s_Jni->CallBooleanMethod(s_InputMethodManager, s_ShowSoftInputID, s_DecorView, flags) == JNI_TRUE;
    // window probably changed, try forced reinit
    if (!result)
    {
      if (!reattach(true))
        return false;

      result = s_Jni->CallBooleanMethod(s_InputMethodManager, s_ShowSoftInputID, s_DecorView, flags) == JNI_TRUE;
    }
  }

  canBeVisible = true;
  canBeHidden = false;
  return result;
}

static bool softinput::hide(uint32_t flags)
{
  if (!reattach())
    return false;

  bool result;
  if (methodHideKeyboard)
  {
    debug("CommonActivity::hideKeyboard");
    s_Jni->CallVoidMethod(lNativeActivity, methodHideKeyboard);
    result = true;
  }
  else
  {
    result = s_Jni->CallBooleanMethod(s_InputMethodManager, s_HideSoftInputID, s_Token, flags) == JNI_TRUE;
    // window probably changed, try forced reinit
    if (!result)
    {
      if (!reattach(true))
        return false;

      result = s_Jni->CallBooleanMethod(s_InputMethodManager, s_HideSoftInputID, s_Token, flags) == JNI_TRUE;
    }
  }

  canBeVisible = false;
  return result;
}

static bool softinput::isShown()
{
  if (!reattach())
    return false;

  return s_GetVisHeightID ? s_Jni->CallIntMethod(s_InputMethodManager, s_GetVisHeightID) > 0 : false;
}


bool nvsoftinput::attach(void *activity)
{
  if (!activity && !s_Inited)
    return false;

  // always attach to the caller's thread
  jint result = android::get_java_vm(activity ? activity : s_AttachedActivity)->AttachCurrentThread(&s_Jni, NULL);
  if (result == JNI_ERR)
    return false;

  // check if already inited
  if (s_AttachedActivity == activity && s_Inited)
    return true;

  if (!activity && s_Inited)
    return true;

  s_AttachedActivity = activity;

  jclass myClass = s_Jni->GetObjectClass(android::get_activity_class(activity));
  jmethodID getClassLoaderID = s_Jni->GetMethodID(myClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
  jobject classLoader = s_Jni->CallObjectMethod(android::get_activity_class(activity), getClassLoaderID);
  if (!classLoader)
    return false;
  jmethodID findClass = s_Jni->GetMethodID(s_Jni->GetObjectClass(classLoader), "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
  jstring strClassName = s_Jni->NewStringUTF("com/nvidia/Helpers/NvSoftInput");
  jobject cls = s_Jni->CallObjectMethod(classLoader, findClass, strClassName);
  s_Jni->ExceptionClear();
  s_NvSoftInputClass = cls ? (jclass)s_Jni->NewGlobalRef(cls) : NULL;
  s_Jni->DeleteLocalRef(strClassName);
  if (!s_NvSoftInputClass)
    return false;

  s_ShowID = s_Jni->GetStaticMethodID(s_NvSoftInputClass, "Show", "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;II)V");
  s_HideID = s_Jni->GetStaticMethodID(s_NvSoftInputClass, "Hide", "()V");
  s_IsShownID = s_Jni->GetStaticMethodID(s_NvSoftInputClass, "IsShown", "()Z");

  s_Inited = s_ShowID && s_HideID && s_IsShownID && s_NvSoftInputClass;

  debug("nvsoftinput inited: %d", s_Inited);
  return s_Inited;
}

bool nvsoftinput::show(const char *initalText, const char *hint, uint32_t editFlags, uint32_t imeOptions)
{
  if (!attach())
    return false;

  canBeVisible = true;
  canBeHidden = false;
  jstring strText = s_Jni->NewStringUTF(initalText);
  jstring strHint = s_Jni->NewStringUTF(hint);

  s_Jni->CallStaticVoidMethod(s_NvSoftInputClass, s_ShowID, android::get_activity_class(s_AttachedActivity), strText, strHint,
    editFlags, imeOptions);

  return true;
}

void nvsoftinput::hide()
{
  if (attach())
  {
    s_Jni->CallStaticVoidMethod(s_NvSoftInputClass, s_HideID);
    if (ime_finish_cb)
    {
      ime_finish_cb(ime_finish_userdata, "", -1);
      ime_finish_cb = NULL;
      ime_finish_userdata = NULL;
    }
    ime_started = false;
  }
}

bool nvsoftinput::isShown()
{
  if (!attach())
    return false;

  return s_Jni->CallStaticBooleanMethod(s_NvSoftInputClass, s_IsShownID);
}

extern "C" // private static native void nativeTextReport(String text, boolean isCancelled);
  JNIEXPORT void JNICALL
  Java_com_nvidia_Helpers_NvSoftInput_nativeTextReport(JNIEnv *jni, jclass cls, jstring text, jboolean isCancelled)
{
  if (ime_finish_cb)
  {
    struct FinishAction : public DelayedAction
    {
      void (*finish_cb)(void *userdata, const char *str, int status);
      void *finish_userdata;
      SimpleString utf8;
      bool confirm;

      FinishAction(const char *text_utf8, bool _confirm)
      {
        utf8 = text_utf8;
        finish_cb = ime_finish_cb;
        finish_userdata = ime_finish_userdata;
        confirm = _confirm;
      }

      virtual void performAction()
      {
        // debug("text=%s confirm=%d", utf8, confirm);
        finish_cb(finish_userdata, utf8, confirm);
      }
    };

    const char *text_utf8 = jni->GetStringUTFChars(text, NULL);
    FinishAction *a = new FinishAction(text_utf8, isCancelled ? 0 : 1);
    jni->ReleaseStringUTFChars(text, text_utf8);

    ime_started = false;
    ime_finish_cb = NULL;
    ime_finish_userdata = NULL;
    add_delayed_action(a);
  }
}
