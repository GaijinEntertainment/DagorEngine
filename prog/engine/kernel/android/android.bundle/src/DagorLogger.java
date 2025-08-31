package com.gaijinent.common;

import android.util.Log;

public class DagorLogger {

  private static final String TAG = "DagorLogger";

  private static native void nativeDebug(String msg);
  private static native void nativeWarning(String msg);
  private static native void nativeError(String msg);

  public static void logError(String msg) {
    try {
      nativeError(msg);
    } catch (Throwable e) {
      Log.e(TAG, msg);
    }
  }

  public static void logDebug(String msg) {
    try {
      nativeDebug(msg);
    } catch (Throwable e) {
      Log.d(TAG, msg);
    }
  }

  public static void logWarning(String msg) {
    try {
      nativeWarning(msg);
    } catch (Throwable e) {
      Log.w(TAG, msg);
    }
  }

}