// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <supp/dag_android_native_app_glue.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_unicode.h>
#include <drv/hid/dag_hiCreate.h>
#include <ioSys/dag_dataBlock.h>
#include <jni.h>
#include "ime_android_attr.h"
#include <util/dag_simpleString.h>
#include <util/dag_delayedAction.h>
#include <generic/dag_tab.h>
#include <memory/dag_memBase.h>
#include <debug/dag_debug.h>
#include <supp/android/jni_native_reg.h>
#include <supp/android/jni_signatures.h>

// ============================================================================
// On-screen keyboard support, two modes:
//  - nvsoftinput: shows a Java EditText dialog (NvSoftInput.java) on top of the
//    game plus the keyboard; the final text is delivered back to native code
//    via nativeTextReport() when the dialog closes.
//  - softinput: shows only the keyboard; key events go straight to the game UI.
// ============================================================================

// guarded by g_ime_cs (declared below)
static bool canBeVisible = false, canBeHidden = false;

static WinCritSec g_ime_cs;

namespace imejni
{

static void *current_activity()
{
  android_app *state = (android_app *)win32_get_instance();
  return state ? state->activity : nullptr;
}

static JNIEnv *get_env(void *activity)
{
  JavaVM *vm = activity ? android::get_java_vm(activity) : nullptr;
  if (!vm)
    return nullptr;

  JNIEnv *env = nullptr;
  JavaVMAttachArgs args;
  args.version = JNI_VERSION_1_6;
  args.name = "NativeThread";
  args.group = nullptr;

  return android::attach_current_thread(vm, &env, &args) == JNI_OK ? env : nullptr;
}

static bool check_exception(JNIEnv *env, const char *where)
{
  if (!env || !env->ExceptionCheck())
    return false;
  env->ExceptionDescribe();
  env->ExceptionClear();
  logwarn("[ime] JNI exception cleared after %s", where ? where : "?");
  return true;
}

// RAII local-reference frame. Our threads are attached to the JVM manually and
// never return to Java, so their local reference tables are never popped by
// ART; without explicit frames every GetObjectClass/FindClass/CallObjectMethod
// leaks a slot until the 512-entry table overflows and aborts the process.
class LocalFrame
{
  JNIEnv *env;

public:
  LocalFrame(JNIEnv *e, jint capacity) : env(e)
  {
    if (env && env->PushLocalFrame(capacity) < 0)
    {
      check_exception(env, "PushLocalFrame");
      env = nullptr;
    }
  }
  ~LocalFrame()
  {
    if (env)
      env->PopLocalFrame(nullptr);
  }
  bool valid() const { return env != nullptr; }

  LocalFrame(const LocalFrame &) = delete;
  LocalFrame &operator=(const LocalFrame &) = delete;
};

template <typename T>
static void release_global_ref(JNIEnv *env, T &ref)
{
  if (env && ref)
    env->DeleteGlobalRef(ref);
  ref = nullptr;
}

// Chat text is raw UTF-8 and may contain emoji (4-byte sequences / surrogate
// pairs) that are illegal in the "modified UTF-8" NewStringUTF expects -> UB.
// Convert to real UTF-16 and use NewString. wchar_t is 4 bytes on Android, so
// go through the explicit uint16_t path.
static jstring make_jstring_utf16(JNIEnv *env, const char *utf8)
{
  if (!utf8)
    utf8 = "";
  const int srcLen = (int)strlen(utf8);
  Tab<uint16_t> u16(tmpmem);
  u16.resize(srcLen + 1); // UTF-16 unit count is always <= UTF-8 byte count
  const int n = utf8_to_utf16(utf8, srcLen, u16.data(), u16.size());
  jstring s = env->NewString((const jchar *)u16.data(), n);
  if (check_exception(env, "NewString"))
    return nullptr;
  return s;
}

// The reverse conversion for text coming back from Java. GetStringUTFChars
// returns "modified UTF-8" (CESU-8): every emoji comes out as a 6-byte
// surrogate encoding that the rest of the engine treats as garbage. Convert
// real UTF-16 (with surrogate pairs) to real UTF-8 by hand. Always writes a
// terminating NUL.
static void jchars_to_utf8(const jchar *src, int len, Tab<char> &out)
{
  out.clear();
  out.reserve(len * 3 + 1);
  for (int i = 0; i < len; i++)
  {
    uint32_t cp = src[i];
    if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < len && src[i + 1] >= 0xDC00 && src[i + 1] <= 0xDFFF)
    {
      cp = 0x10000 + ((cp - 0xD800) << 10) + (src[i + 1] - 0xDC00);
      i++;
    }
    else if (cp >= 0xD800 && cp <= 0xDFFF)
      cp = 0xFFFD; // lone surrogate -> replacement char

    if (cp < 0x80)
      out.push_back((char)cp);
    else if (cp < 0x800)
    {
      out.push_back((char)(0xC0 | (cp >> 6)));
      out.push_back((char)(0x80 | (cp & 0x3F)));
    }
    else if (cp < 0x10000)
    {
      out.push_back((char)(0xE0 | (cp >> 12)));
      out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
      out.push_back((char)(0x80 | (cp & 0x3F)));
    }
    else
    {
      out.push_back((char)(0xF0 | (cp >> 18)));
      out.push_back((char)(0x80 | ((cp >> 12) & 0x3F)));
      out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
      out.push_back((char)(0x80 | (cp & 0x3F)));
    }
  }
  out.push_back(0);
}

} // namespace imejni


namespace softinput
{
static void *s_AttachedActivity = nullptr;
static bool s_Inited = false;

static jobject s_InputMethodManager = nullptr;
static jmethodID s_GetVisHeightID = nullptr;
static jmethodID s_HideSoftInputID = nullptr;
static jmethodID s_ShowSoftInputID = nullptr;

static jobject s_DecorView = nullptr;
static jobject s_Token = nullptr;

// optional methods of a custom (Common)Activity, if present
static jmethodID s_ShowKeyboardID = nullptr;
static jmethodID s_HideKeyboardID = nullptr;

static void reset(JNIEnv *env);
static bool attach(void *activity = nullptr, bool force = false);
static bool show(uint32_t flags);
static bool hide(uint32_t flags);
static bool isShown();
static void invalidate(JNIEnv *env);
} // namespace softinput

namespace nvsoftinput
{
static void *s_AttachedActivity = nullptr;
static bool s_Inited = false;

static jclass s_NvSoftInputClass = nullptr;
static jmethodID s_ShowID = nullptr;
static jmethodID s_HideID = nullptr;
static jmethodID s_IsShownID = nullptr;

// isImeAvailable() re-attaches on demand and is polled every frame; if the
// NvSoftInput class is genuinely absent from the apk, remember the activity we
// failed on so we don't redo the class-loader lookup (and spam logwarn) each
// frame. Cleared on invalidate/reset, so every new window/activity retries once.
static void *s_FailedActivity = nullptr;

static void reset(JNIEnv *env);
static bool attach(void *activity = nullptr);
static bool show(const char *initalText, const char *hint, uint32_t editFlags, uint32_t imeOptions, int cursorPos = 0,
  int maxText = 0);
static void hide();
static bool isShown();
static void invalidate(JNIEnv *env);
} // namespace nvsoftinput


void HumanInput::showScreenKeyboard(bool show)
{
  if (show)
    softinput::show(0);
  else
    softinput::hide(0);
}

// guarded by g_ime_cs: written by the game thread and by the Java UI thread
// (via nativeTextReport)
static void (*ime_finish_cb)(void *userdata, const char *str, int cursor, int status) = nullptr;
static void *ime_finish_userdata = nullptr;
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

void android_invalidate_soft_input()
{
  WinAutoLock lock(g_ime_cs);
  void *activity = imejni::current_activity();
  JNIEnv *env = activity ? imejni::get_env(activity) : nullptr;
  softinput::invalidate(env);
  nvsoftinput::invalidate(env);
  ime_started = false;
}

// Must not just report a cached flag: a window recreation (keyboard
// fixed/floating mode switch, split-screen, ...) invalidates all cached JNI
// state, and the game polls this to choose between nvsoftinput and softinput
// UI. Re-attach on demand so a transient invalidation doesn't permanently
// switch the IME to softinput mode.
bool HumanInput::isImeAvailable() { return nvsoftinput::attach(); }

bool HumanInput::showScreenKeyboard_IME(const DataBlock &init_params,
  void(on_finish_cb)(void *userdata, const char *str, int cursor, int status), void *userdata)
{
  WinAutoLock lock(g_ime_cs);

  if (!init_params.paramCount() && !on_finish_cb && userdata)
  {
    if (ime_finish_cb && userdata == ime_finish_userdata)
    {
      ime_finish_cb = nullptr;
      ime_finish_userdata = nullptr;
      nvsoftinput::hide();
      return true;
    }
    return false;
  }
  if (ime_started)
  {
    logwarn("[ime] double start prevented");
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
  int max_chars = init_params.getInt("maxChars", 128);
  int cursor_pos = init_params.getInt("optCursorPos", 0);

  const bool ok =
    nvsoftinput::show(init_params.getStr("str"), init_params.getStr("hint", ""), edit_flags, ime_flags, cursor_pos, max_chars);
  if (!ok)
  {
    // roll back so a failed show doesn't wedge us in the "double start prevented" state forever
    ime_started = false;
    ime_finish_cb = nullptr;
    ime_finish_userdata = nullptr;
  }
  return ok;
}

int HumanInput::getScreenKeyboardStatus_android()
{
  // lock the whole function: canBeVisible/canBeHidden are written under g_ime_cs by the game
  // thread (show/hide) while this is polled every frame from the input thread
  WinAutoLock lock(g_ime_cs);

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


// ============================================================================
// softinput
// ============================================================================

// Drops every cached ref/id. Must be called with a valid env whenever
// possible: without one the global refs leak (that is still preferable to
// keeping stale ones - a stale global ref to a dead decor view makes
// showSoftInput silently fail forever).
static void softinput::reset(JNIEnv *env)
{
  imejni::release_global_ref(env, s_InputMethodManager);
  imejni::release_global_ref(env, s_DecorView);
  imejni::release_global_ref(env, s_Token);
  s_GetVisHeightID = s_HideSoftInputID = s_ShowSoftInputID = nullptr;
  s_ShowKeyboardID = s_HideKeyboardID = nullptr;
  s_AttachedActivity = nullptr;
  s_Inited = false;
}

static bool softinput::attach(void *activity, bool force)
{
  WinAutoLock lock(g_ime_cs);

  if (!activity)
    activity = imejni::current_activity();
  if (!activity)
    return false;

  if (s_Inited && s_AttachedActivity == activity && !force)
    return true;

  JNIEnv *env = imejni::get_env(activity);
  if (!env)
    return false;

  imejni::LocalFrame frame(env, 32);
  if (!frame.valid())
    return false;

  reset(env);

  jobject activityObj = android::get_activity_class(activity);
  if (!activityObj)
    return false;

  jclass ClassNativeActivity = env->GetObjectClass(activityObj);

  jclass ClassContext = env->FindClass("android/content/Context");
  if (imejni::check_exception(env, "FindClass Context") || !ClassContext)
    return false;
  jfieldID FieldINPUT_METHOD_SERVICE = env->GetStaticFieldID(ClassContext, "INPUT_METHOD_SERVICE", JNI_STRING);
  jobject INPUT_METHOD_SERVICE = env->GetStaticObjectField(ClassContext, FieldINPUT_METHOD_SERVICE);
  if (imejni::check_exception(env, "INPUT_METHOD_SERVICE") || !INPUT_METHOD_SERVICE)
    return false;

  jclass ClassInputMethodManager = env->FindClass("android/view/inputmethod/InputMethodManager");
  if (imejni::check_exception(env, "FindClass InputMethodManager") || !ClassInputMethodManager)
    return false;

  jmethodID MethodGetSystemService =
    env->GetMethodID(ClassNativeActivity, "getSystemService", JNI_SIGNATURE(JNI_OBJECT("java/lang/Object"), JNI_STRING));
  if (imejni::check_exception(env, "getSystemService id") || !MethodGetSystemService)
    return false;
  jobject imm = env->CallObjectMethod(activityObj, MethodGetSystemService, INPUT_METHOD_SERVICE);
  if (imejni::check_exception(env, "getSystemService") || !imm)
    return false;
  s_InputMethodManager = env->NewGlobalRef(imm);

  s_HideSoftInputID = env->GetMethodID(ClassInputMethodManager, "hideSoftInputFromWindow",
    JNI_SIGNATURE(JNI_BOOL, JNI_OBJECT("android/os/IBinder") JNI_INT));
  s_ShowSoftInputID =
    env->GetMethodID(ClassInputMethodManager, "showSoftInput", JNI_SIGNATURE(JNI_BOOL, JNI_OBJECT("android/view/View") JNI_INT));
  imejni::check_exception(env, "show/hideSoftInput ids");
  // Android-L and up hidden method; an exception can occur, keyboard state
  // polling just degrades without it
  s_GetVisHeightID = env->GetMethodID(ClassInputMethodManager, "getInputMethodWindowVisibleHeight", JNI_SIGNATURE(JNI_INT, ));
  env->ExceptionClear();

  jmethodID MethodGetWindow = env->GetMethodID(ClassNativeActivity, "getWindow", JNI_SIGNATURE(JNI_OBJECT("android/view/Window"), ));
  jobject Window = MethodGetWindow ? env->CallObjectMethod(activityObj, MethodGetWindow) : nullptr;
  if (imejni::check_exception(env, "getWindow") || !Window)
    return false;
  jclass ClassWindow = env->FindClass("android/view/Window");
  jmethodID MethodGetDecorView =
    ClassWindow ? env->GetMethodID(ClassWindow, "getDecorView", JNI_SIGNATURE(JNI_OBJECT("android/view/View"), )) : nullptr;
  jobject decorView = MethodGetDecorView ? env->CallObjectMethod(Window, MethodGetDecorView) : nullptr;
  if (imejni::check_exception(env, "getDecorView") || !decorView)
    return false;
  s_DecorView = env->NewGlobalRef(decorView);

  jclass ClassView = env->FindClass("android/view/View");
  jmethodID MethodGetWindowToken =
    ClassView ? env->GetMethodID(ClassView, "getWindowToken", JNI_SIGNATURE(JNI_OBJECT("android/os/IBinder"), )) : nullptr;
  jobject token = MethodGetWindowToken ? env->CallObjectMethod(decorView, MethodGetWindowToken) : nullptr;
  if (imejni::check_exception(env, "getWindowToken") || !token)
    return false;
  s_Token = env->NewGlobalRef(token);

  // optional helpers implemented by our custom activity, absence is fine
  s_ShowKeyboardID = env->GetMethodID(ClassNativeActivity, "showKeyboard", JNI_SIGNATURE(JNI_VOID, ));
  env->ExceptionClear();
  s_HideKeyboardID = env->GetMethodID(ClassNativeActivity, "hideKeyboard", JNI_SIGNATURE(JNI_VOID, ));
  env->ExceptionClear();

  // a minimal working set without s_GetVisHeightID
  s_Inited = s_InputMethodManager && s_DecorView && s_Token && s_ShowSoftInputID && s_HideSoftInputID;
  if (s_Inited)
    s_AttachedActivity = activity;
  else
    reset(env);

  return s_Inited;
}

static bool softinput::show(uint32_t flags)
{
  WinAutoLock lock(g_ime_cs);

  if (!attach())
    return false;

  JNIEnv *env = imejni::get_env(s_AttachedActivity);
  if (!env)
    return false;

  imejni::LocalFrame frame(env, 8);
  if (!frame.valid())
    return false;

  bool result;
  if (s_ShowKeyboardID)
  {
    env->CallVoidMethod(android::get_activity_class(s_AttachedActivity), s_ShowKeyboardID);
    result = !imejni::check_exception(env, "showKeyboard");
  }
  else
  {
    result = env->CallBooleanMethod(s_InputMethodManager, s_ShowSoftInputID, s_DecorView, flags) == JNI_TRUE;
    imejni::check_exception(env, "showSoftInput");
    // the window (decor view / token) probably changed, force full reinit and retry
    if (!result)
    {
      if (!attach(nullptr, /*force*/ true))
        return false;

      result = env->CallBooleanMethod(s_InputMethodManager, s_ShowSoftInputID, s_DecorView, flags) == JNI_TRUE;
      imejni::check_exception(env, "showSoftInput (retry)");
    }
  }

  // only on success: a stale canBeVisible=true makes getScreenKeyboardStatus_android() (polled
  // every frame from the input thread) do two JNI isShown() round-trips per frame until the
  // next window teardown clears the flag
  if (result)
  {
    canBeVisible = true;
    canBeHidden = false;
  }
  return result;
}

static bool softinput::hide(uint32_t flags)
{
  WinAutoLock lock(g_ime_cs);

  if (!attach())
    return false;

  JNIEnv *env = imejni::get_env(s_AttachedActivity);
  if (!env)
    return false;

  imejni::LocalFrame frame(env, 8);
  if (!frame.valid())
    return false;

  bool result;
  if (s_HideKeyboardID)
  {
    env->CallVoidMethod(android::get_activity_class(s_AttachedActivity), s_HideKeyboardID);
    result = !imejni::check_exception(env, "hideKeyboard");
  }
  else
  {
    result = env->CallBooleanMethod(s_InputMethodManager, s_HideSoftInputID, s_Token, flags) == JNI_TRUE;
    imejni::check_exception(env, "hideSoftInputFromWindow");
    // the window (decor view / token) probably changed, force full reinit and retry
    if (!result)
    {
      if (!attach(nullptr, /*force*/ true))
        return false;

      result = env->CallBooleanMethod(s_InputMethodManager, s_HideSoftInputID, s_Token, flags) == JNI_TRUE;
      imejni::check_exception(env, "hideSoftInputFromWindow (retry)");
    }
  }

  canBeVisible = false;
  return result;
}

static bool softinput::isShown()
{
  WinAutoLock lock(g_ime_cs);

  if (!attach())
    return false;

  JNIEnv *env = imejni::get_env(s_AttachedActivity);
  if (!env || !s_GetVisHeightID)
    return false;

  const bool shown = env->CallIntMethod(s_InputMethodManager, s_GetVisHeightID) > 0;
  if (imejni::check_exception(env, "getInputMethodWindowVisibleHeight"))
    return false;
  return shown;
}

static void softinput::invalidate(JNIEnv *env) { reset(env); }


// ============================================================================
// nvsoftinput
// ============================================================================

static void nvsoftinput::reset(JNIEnv *env)
{
  imejni::release_global_ref(env, s_NvSoftInputClass);
  s_ShowID = s_HideID = s_IsShownID = nullptr;
  s_AttachedActivity = nullptr;
  s_FailedActivity = nullptr;
  s_Inited = false;
}

static bool nvsoftinput::attach(void *activity)
{
  WinAutoLock lock(g_ime_cs);

  if (!activity)
    activity = imejni::current_activity(); // self-heal after invalidate / activity recreation
  if (!activity)
    return false; // no live activity (headless test / shutdown) - nothing to attach to

  if (s_Inited && s_AttachedActivity == activity)
    return true;
  if (activity == s_FailedActivity)
    return false; // already failed for this activity, don't retry every frame

  JNIEnv *env = imejni::get_env(activity);
  if (!env)
    return false;

  imejni::LocalFrame frame(env, 16);
  if (!frame.valid())
    return false;

  reset(env); // never leak the previous class ref on re-attach

  jobject activityObj = android::get_activity_class(activity);
  if (!activityObj)
    return false;

  // NvSoftInput ships in the app dex; FindClass on a manually attached native
  // thread only sees the system class loader, so go through the activity's one.
  jclass activityClass = env->GetObjectClass(activityObj);
  jmethodID getClassLoaderID = env->GetMethodID(activityClass, "getClassLoader", JNI_SIGNATURE(JNI_OBJECT("java/lang/ClassLoader"), ));
  if (imejni::check_exception(env, "getClassLoader id") || !getClassLoaderID)
    return false;
  jobject classLoader = env->CallObjectMethod(activityObj, getClassLoaderID);
  if (imejni::check_exception(env, "getClassLoader") || !classLoader)
    return false;
  jmethodID loadClassID =
    env->GetMethodID(env->GetObjectClass(classLoader), "loadClass", JNI_SIGNATURE(JNI_OBJECT("java/lang/Class"), JNI_STRING));
  if (imejni::check_exception(env, "loadClass id") || !loadClassID)
    return false;

  jstring className = env->NewStringUTF("com/nvidia/Helpers/NvSoftInput");
  jobject cls = className ? env->CallObjectMethod(classLoader, loadClassID, className) : nullptr;
  if (imejni::check_exception(env, "loadClass NvSoftInput"))
    cls = nullptr;
  if (!cls)
  {
    logwarn("[ime] NvSoftInput class not found; nvsoftinput IME unavailable");
    s_FailedActivity = activity;
    return false;
  }
  s_NvSoftInputClass = (jclass)env->NewGlobalRef(cls);
  if (!s_NvSoftInputClass)
    return false;

  s_ShowID = env->GetStaticMethodID(s_NvSoftInputClass, "Show",
    JNI_SIGNATURE(JNI_VOID, JNI_OBJECT("android/app/Activity") JNI_STRING JNI_STRING JNI_INT JNI_INT JNI_INT JNI_INT));
  s_HideID = env->GetStaticMethodID(s_NvSoftInputClass, "Hide", JNI_SIGNATURE(JNI_VOID, ));
  s_IsShownID = env->GetStaticMethodID(s_NvSoftInputClass, "IsShown", JNI_SIGNATURE(JNI_BOOL, ));
  imejni::check_exception(env, "NvSoftInput method ids");

  s_Inited = s_ShowID && s_HideID && s_IsShownID;
  if (s_Inited)
    s_AttachedActivity = activity;
  else
  {
    reset(env);
    s_FailedActivity = activity;
  }

  return s_Inited;
}

static bool nvsoftinput::show(const char *initalText, const char *hint, uint32_t editFlags, uint32_t imeOptions, int cursorPos,
  int maxText)
{
  WinAutoLock lock(g_ime_cs);

  if (!attach())
    return false;

  JNIEnv *env = imejni::get_env(s_AttachedActivity);
  if (!env)
    return false;

  imejni::LocalFrame frame(env, 16);
  if (!frame.valid())
    return false;

  bool ok = false;
  jstring strText = imejni::make_jstring_utf16(env, initalText);
  jstring strHint = imejni::make_jstring_utf16(env, hint);
  if (strText && strHint)
  {
    env->CallStaticVoidMethod(s_NvSoftInputClass, s_ShowID, android::get_activity_class(s_AttachedActivity), strText, strHint,
      editFlags, imeOptions, cursorPos, maxText);
    ok = !imejni::check_exception(env, "NvSoftInput.Show");
  }

  if (ok)
  {
    canBeVisible = true;
    canBeHidden = false;
  }
  return ok;
}

static void nvsoftinput::hide()
{
  void (*finish_cb)(void *userdata, const char *str, int cursor, int status) = nullptr;
  void *finish_userdata = nullptr;

  {
    WinAutoLock lock(g_ime_cs);

    if (attach())
    {
      JNIEnv *env = imejni::get_env(s_AttachedActivity);
      if (env)
      {
        env->CallStaticVoidMethod(s_NvSoftInputClass, s_HideID);
        imejni::check_exception(env, "NvSoftInput.Hide");
      }
    }

    // Java-side Hide() dismisses the dialog without reporting text back, so the pending
    // callback must be finalized from here; detach it under the lock
    finish_cb = ime_finish_cb;
    finish_userdata = ime_finish_userdata;
    ime_finish_cb = nullptr;
    ime_finish_userdata = nullptr;
    ime_started = false;
  }

  // Invoke the callback outside g_ime_cs: it is opaque game code and may block, while the
  // input thread polls getScreenKeyboardStatus_android() under the same lock every frame (the
  // completion path in nativeTextReport defers its callback for the same reason). When hide()
  // is reached from showScreenKeyboard_IME, the callback was already detached by the caller,
  // so nothing runs under its (recursive) lock.
  if (finish_cb)
    finish_cb(finish_userdata, "", 0, HumanInput::IME_STATUS_ERROR);
}

static bool nvsoftinput::isShown()
{
  WinAutoLock lock(g_ime_cs);

  if (!attach())
    return false;

  JNIEnv *env = imejni::get_env(s_AttachedActivity);
  if (!env)
    return false;

  const jboolean shown = env->CallStaticBooleanMethod(s_NvSoftInputClass, s_IsShownID);
  if (imejni::check_exception(env, "NvSoftInput.IsShown"))
    return false;
  return shown == JNI_TRUE;
}

static void nvsoftinput::invalidate(JNIEnv *env) { reset(env); }

static void nativeTextReport(JNIEnv *jni, jclass /*cls*/, jstring text, jint cursorPos, jboolean isCancelled)
{
  Tab<char> utf8(tmpmem);
  if (text)
  {
    const jchar *chars = jni->GetStringChars(text, nullptr);
    if (chars)
    {
      imejni::jchars_to_utf8(chars, jni->GetStringLength(text), utf8);
      jni->ReleaseStringChars(text, chars);
    }
  }
  const char *text_utf8 = utf8.size() ? utf8.data() : "";

  WinAutoLock lock(g_ime_cs);
  if (!ime_finish_cb)
    return;

  // The callback expects the game thread; defer it instead of calling straight
  // from the Java UI thread.
  struct FinishAction final : public DelayedAction
  {
    void (*finish_cb)(void *userdata, const char *str, int cursor, int status);
    void *finish_userdata;
    SimpleString utf8;
    int status;
    int cursor;

    FinishAction(const char *text_utf8, int cursor_pos, int _status) :
      utf8(text_utf8), finish_cb(ime_finish_cb), finish_userdata(ime_finish_userdata), status(_status), cursor(cursor_pos)
    {}

    void performAction() override { finish_cb(finish_userdata, utf8, cursor, status); }
  };

  FinishAction *a =
    new FinishAction(text_utf8, cursorPos, isCancelled ? HumanInput::IME_STATUS_CANCELLED : HumanInput::IME_STATUS_CLOSED);

  ime_started = false;
  ime_finish_cb = nullptr;
  ime_finish_userdata = nullptr;
  add_delayed_action(a);
}

JNI_REG_NATIVES(NvSoftInputNatives, "com.nvidia.Helpers.NvSoftInput",
  JNI_NATIVE_METHOD(nativeTextReport, JNI_SIGNATURE(JNI_VOID, JNI_STRING JNI_INT JNI_BOOL)));
