package com.nvidia.Helpers;

import android.app.*;
import android.content.*;
import android.graphics.drawable.ColorDrawable;
import android.text.*;
import android.text.method.TextKeyListener;
import android.view.*;
import android.view.inputmethod.*;
import android.view.View.*;
import android.widget.*;
import android.util.Log;
import java.io.*;
import android.graphics.Rect;
import android.content.pm.*;
import android.os.Build;

public class NvSoftInput extends Dialog
{
  private static final String TAG = "SoftInput";

  private static native void nativeTextReport(String text, boolean isCanceled);

  private Context mContext = null;
  private EditText mEditText = null;
  private Window mWindow = null;
  private static NvSoftInput mCurrent = null;
  private boolean mInsideClose = false;

  private static int mWidth = 720;
  private static int mHeight = 100;
  private static boolean mInited = false;

  private static void Init(final Activity activity, String libName)
  {
    if (libName == null)
    {
      // First, use the NativeActivity method
      try
      {
        ActivityInfo ai = activity.getPackageManager().getActivityInfo(
          activity.getIntent().getComponent(), PackageManager.GET_META_DATA);

        if (ai.metaData != null)
        {
          String ln = ai.metaData.getString(NativeActivity.META_DATA_LIB_NAME);
          if (ln != null)
            libName = ln;
          ln = ai.metaData.getString(NativeActivity.META_DATA_FUNC_NAME);
          if (ln != null)
            libName = ln;
        }
      }
      catch (PackageManager.NameNotFoundException e)
      {
      }

      // Failing that, grab the class name
      if (libName == null)
        libName = activity.getClass().getSimpleName();
    }

    try
    {
      // Fail gracefully if we cannot load the lib
      System.loadLibrary(libName);
      mInited = true;
    }
    catch (Exception e)
    {
      Log.d(TAG,"Native library " + libName + " loading failed!");
    }
  }

  public static void Show(final Activity activity, final String initialText, final String hint, final int editFlags, final int imeOptions)
  {
    if (!mInited)
      Init(activity, null);

    activity.runOnUiThread(new Runnable() {
      public void run()
      {
        if (mCurrent != null)
        {
          mCurrent.dismiss();
          mCurrent = null;
        }

        mCurrent = new NvSoftInput(activity, initialText, hint, editFlags, imeOptions);
        mCurrent.show();
      }
    });
  }

  public static void Hide()
  {
    if (mCurrent == null)
      return;

    Activity activity = (Activity)mCurrent.mContext;
    activity.runOnUiThread(new Runnable() {
      public void run()
      {
        if (mCurrent != null)
        {
          mCurrent.dismiss();
          mCurrent = null;
        }
      }
    });
  }

  public static boolean IsShown()
  {
    if (mCurrent != null)
      return mCurrent.isShowing();
    return false;
  }

  private NvSoftInput(Context context, String initialText, String hint, int editTypeFlags, int imeOptions)
  {
    super (context);
    mContext = context;
    mWindow = getWindow();

    mWindow.requestFeature(Window.FEATURE_NO_TITLE);
    mWindow.setBackgroundDrawable(new ColorDrawable(0));

    setContentView(createView(initialText, hint, editTypeFlags, imeOptions));

    setCanceledOnTouchOutside(true);
    setOnCancelListener(new DialogInterface.OnCancelListener() {
      @Override public void onCancel(DialogInterface dialog) { close(true); }
    });
  }

  private String getText()
  {
    if (mEditText != null)
      return mEditText.getText().toString().trim();
    return null;
  }

  private void close(boolean isCanceled)
  {
    if (mInsideClose)
      return;

    mInsideClose = true;

    nativeTextReport(getText(), isCanceled);

    //Shield Portable Android 4.x BSP WAR
    //selection decorators can appear on the screen after IME was closed
    //resetting selection doesn't work so kill the text.
    if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.KITKAT)
    {
      mEditText.setText("");
    }

    NvSoftInput.Hide();
    mInsideClose = false;
  }

  private View createView(String initialText, String hint, int editTypeFlags, int imeOptions)
  {
    mEditText = new EditText(mContext) {
      public boolean onKeyPreIme(int keyCode, KeyEvent event)
      {
        if (event.getAction() != KeyEvent.ACTION_UP)
          return super.onKeyPreIme(keyCode, event);

        if (keyCode == KeyEvent.KEYCODE_BACK)
        {
          close(true);
          return true;
        }
        if (keyCode == KeyEvent.KEYCODE_SEARCH)
          return true;

        return super.onKeyPreIme(keyCode, event);
      }
      public void onWindowFocusChanged(boolean hasFocus)
      {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus)
        {
          //since API 28 (android 9) and above, we can't just call showSoftInput, it does now show the softkeyboard
          //workaround request focus for edit field and call showSoftInput with some delay
          this.requestFocus();
          this.postDelayed(new Runnable() {
            @Override
            public void run() {
              InputMethodManager imm = (InputMethodManager)mContext.getSystemService(Context.INPUT_METHOD_SERVICE);
              imm.showSoftInput(mEditText, 0);
            }
          },200);
        }
      }
    };

    mEditText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
      public boolean onEditorAction(TextView v, int actionId, KeyEvent event)
      {
        if (actionId == EditorInfo.IME_ACTION_DONE || actionId == EditorInfo.IME_ACTION_NEXT)
          close(false);
        return false;
      }
    });

    mEditText.setImeOptions(imeOptions);
    mEditText.setText(initialText);
    mEditText.setHint(hint);
    mEditText.setInputType(editTypeFlags);
    mEditText.setClickable(true);

    if ((editTypeFlags & EditorInfo.TYPE_TEXT_FLAG_IME_MULTI_LINE) == 0)
    {
      //bug? can't use selectAll() for Leanback keyboard
      //it vanishes the text that was previously set via setText()
      //need to run on UI thread syncronously?
      //just move cursor to the end of line for now
      mEditText.setSelection(initialText.length());
    }

    mEditText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
      @Override public void onFocusChange(View v, boolean hasFocus)
      {
        if (hasFocus)
          mWindow.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
      }
    });

    Rect dim = new Rect();
    mEditText.getWindowVisibleDisplayFrame(dim);
    mWidth = dim.width()/2;

    mEditText.measure(0, 0);
    mHeight = mEditText.getMeasuredHeight();

    mEditText.setMinWidth(mWidth);

    return mEditText;
  }

  public void onBackPressed() { close(true); }
}
