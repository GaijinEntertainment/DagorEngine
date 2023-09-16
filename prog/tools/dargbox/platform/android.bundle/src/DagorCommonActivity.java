package com.gaijinent.common;

import android.os.Bundle;
import android.content.pm.PackageManager;
import android.content.pm.ActivityInfo;
import android.content.ComponentName;
import android.view.View;
import android.media.AudioManager;
import android.os.Build;

public class DagorCommonActivity extends DagorBaseActivity
{

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setVolumeControlStream(AudioManager.STREAM_MUSIC);
    hideNavigationBar();
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    setVolumeControlStream(AudioManager.STREAM_MUSIC);
    hideNavigationBar();
  }

  @Override
  public void onWindowFocusChanged(boolean hasFocus) {
    super.onWindowFocusChanged(hasFocus);
    if (hasFocus)
      hideNavigationBar();
  }

  @Override
  protected void onDestroy()
  {
  }

  @SuppressWarnings("deprecation")
  protected void hideNavigationBar()
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
      getWindow().setDecorFitsSystemWindows(false);
    } else {
      View decorView = getWindow().getDecorView();
      decorView.setSystemUiVisibility(
        View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
        | View.SYSTEM_UI_FLAG_FULLSCREEN
        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
    }
  }

  public String getLibraryName()
  {
    try
    {
      PackageManager pkgmgr = this.getPackageManager();
      ComponentName componentName = this.getComponentName();
      int metaFlag = pkgmgr.GET_META_DATA;
      ActivityInfo info = pkgmgr.getActivityInfo(componentName, metaFlag);
      Bundle bundle = info.metaData;
      return bundle.getString("android.app.lib_name");
    }
    catch (Exception e)
    {
      e.printStackTrace();
    }
    return "";
  }

}
