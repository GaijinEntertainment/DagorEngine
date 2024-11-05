package com.gaijinent.common;

import android.content.pm.PackageManager;
import android.content.pm.ActivityInfo;
import android.content.ComponentName;
import android.os.Bundle;
import android.util.Log;

public class DagorBaseActivity extends android.app.NativeActivity {

  static {
    //{EXTERNAL_LIBS}
  }

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    try {
      String lib = getLibraryName();
      Log.d("Dagor", "Native library: " + lib);
      System.loadLibrary(getLibraryName());
    } catch (UnsatisfiedLinkError e) {
      finish();
      System.exit(0);
    }
  }

  @SuppressWarnings("deprecation")
  public String getLibraryName() {
    try {
      return this.getPackageManager().getActivityInfo(getComponentName(), PackageManager.GET_META_DATA).metaData.getString("android.app.lib_name");
    } catch (Exception e) {
      e.printStackTrace();
    }
    return "";
  }
}
