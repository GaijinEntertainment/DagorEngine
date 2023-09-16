package com.gaijinent.common;

import android.util.Log;

public class DagorLogger {

  private static boolean mInited = false;

  private static final String TAG = "Dagor";

  private static native void nativeDebug(String msg);
  private static native void nativeWarning(String msg);
  private static native void nativeError(String msg);

  public static void init(String libName) {
    if (mInited)
      return;

    try {
      System.loadLibrary(libName);
      mInited = true;
    } catch (Exception e) {
      logError("Failed to load " + libName);
    }
  }

  public static void logError(String msg) {
    if (mInited) {
      try {
        nativeError(msg);
        return;
      } catch (Throwable e) {
        mInited = false;
      }
    }
    Log.e(TAG, msg);
  }

  public static void logDebug(String msg) {
    if (mInited) {
      try {
        nativeDebug(msg);
        return;
      } catch (Throwable e) {
        mInited = false;
      }
    }
    Log.d(TAG, msg);
  }

  public static void logWarning(String msg) {
    if (mInited) {
      try {
        nativeWarning(msg);
        return;
      } catch (Throwable e) {
        mInited = false;
      }
    }
    Log.w(TAG, msg);
  }
}