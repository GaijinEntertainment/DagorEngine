package com.nvidia.Helpers;

import android.app.Activity;
import android.app.Dialog;
import android.app.NativeActivity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.text.InputFilter;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.TextView;

public class NvSoftInput extends Dialog {
    private static final String TAG = "SoftInput";

    private static native void nativeTextReport(String text, int cursorPos, boolean isCanceled);

    private Context mContext = null;
    private EditText mEditText = null;
    private Window mWindow = null;
    private static volatile NvSoftInput mCurrent = null;
    private boolean mInsideClose = false;

    private static boolean mInited = false;

    private static void Init(final Activity activity, String libName) {
        if (libName == null) {
            // First, use the NativeActivity method
            try {
                ActivityInfo ai = activity.getPackageManager().getActivityInfo(
                        activity.getIntent().getComponent(), PackageManager.GET_META_DATA);

                if (ai.metaData != null) {
                    String ln = ai.metaData.getString(NativeActivity.META_DATA_LIB_NAME);
                    if (ln != null)
                        libName = ln;
                    ln = ai.metaData.getString(NativeActivity.META_DATA_FUNC_NAME);
                    if (ln != null)
                        libName = ln;
                }
            } catch (PackageManager.NameNotFoundException e) {
            }

            // Failing that, grab the class name
            if (libName == null)
                libName = activity.getClass().getSimpleName();
        }

        try {
            // Fail gracefully if we cannot load the lib
            System.loadLibrary(libName);
            mInited = true;
        } catch (Exception e) {
            Log.d(TAG, "Native library " + libName + " loading failed!");
        }
    }

    public static void Show(final Activity activity, final String initialText, final String hint, final int editFlags,
            final int imeOptions, final int cursorPos, final int maxLength) {
        if (!mInited)
            Init(activity, null);

        activity.runOnUiThread(new Runnable() {
            public void run() {
                if (mCurrent != null) {
                    mCurrent.dismiss();
                    mCurrent = null;
                }
                mCurrent = new NvSoftInput(activity, initialText, hint, editFlags, imeOptions, cursorPos, maxLength);
                mCurrent.show();
            }
        });
    }

    public static void Hide() {
        // Read mCurrent once to avoid a race between the null check and mContext access
        NvSoftInput current = mCurrent;
        if (current == null)
            return;

        Activity activity = (Activity) current.mContext;
        activity.runOnUiThread(new Runnable() {
            public void run() {
                NvSoftInput toClose = mCurrent;
                if (toClose != null) {
                    mCurrent = null;
                    toClose.dismiss();
                }
            }
        });
    }

    public static boolean IsShown() {
        NvSoftInput current = mCurrent;
        return current != null && current.isShowing();
    }

    private NvSoftInput(
            Context context, String initialText, String hint, int editTypeFlags, int imeOptions, int cursorPos, int maxLength) {
        super(context);
        mContext = context;
        mWindow = getWindow();

        mWindow.requestFeature(Window.FEATURE_NO_TITLE);
        mWindow.setBackgroundDrawable(new ColorDrawable(0));

        setContentView(createView(initialText, hint, editTypeFlags, imeOptions, cursorPos, maxLength));

        // set cursor position should be done after adding to the view
        try {
            mEditText.setSelection(cursorPos);
        } catch (Exception e) {
        }

        setCanceledOnTouchOutside(true);
        setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                close(true);
            }
        });
    }

    private String getText() {
        if (mEditText != null)
            return mEditText.getText().toString();
        return "";
    }

    private void close(boolean isCanceled) {
        if (mInsideClose)
            return;
        mInsideClose = true;

        int cursor = mEditText != null ? mEditText.getSelectionEnd() : 0;
        // Clear mCurrent before dismiss so that any re-entrant close() via onCancel is a no-op
        if (mCurrent == this)
            mCurrent = null;
        nativeTextReport(getText(), cursor, isCanceled);
        dismiss();

        mInsideClose = false;
    }

    private View createView(String initialText, String hint, int editTypeFlags, int imeOptions, int cursorPos, int maxLength) {
        mEditText = new EditText(mContext) {
            public boolean onKeyPreIme(int keyCode, KeyEvent event) {
                if (event.getAction() != KeyEvent.ACTION_UP)
                    return super.onKeyPreIme(keyCode, event);

                if (keyCode == KeyEvent.KEYCODE_BACK) {
                    close(true);
                    return true;
                }
                if (keyCode == KeyEvent.KEYCODE_SEARCH)
                    return true;

                return super.onKeyPreIme(keyCode, event);
            }
            public void onWindowFocusChanged(boolean hasFocus) {
                super.onWindowFocusChanged(hasFocus);
                if (hasFocus) {
                    // since API 28 (android 9) and above, we can't just call showSoftInput, it does now show the softkeyboard
                    // workaround request focus for edit field and call showSoftInput with some delay
                    this.requestFocus();
                    this.postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            InputMethodManager imm = (InputMethodManager) mContext.getSystemService(Context.INPUT_METHOD_SERVICE);
                            imm.showSoftInput(mEditText, 0);
                        }
                    }, 200);
                }
            }
        };

        mEditText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_DONE || actionId == EditorInfo.IME_ACTION_NEXT) {
                    close(false);
                    return true;
                }
                return false;
            }
        });

        if (maxLength > 0) {
            InputFilter[] currentFilters = mEditText.getFilters();
            InputFilter[] newFilters = new InputFilter[currentFilters.length + 1];
            System.arraycopy(currentFilters, 0, newFilters, 0, currentFilters.length);
            newFilters[currentFilters.length] = new InputFilter.LengthFilter(maxLength);
            mEditText.setFilters(newFilters);
        }

        mEditText.setImeOptions(imeOptions);
        mEditText.setText(initialText);
        mEditText.setHint(hint);
        mEditText.setInputType(editTypeFlags);
        mEditText.setClickable(true);
        mEditText.setBackgroundColor(Color.WHITE);

        mEditText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (hasFocus)
                    mWindow.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
            }
        });

        // Use DisplayMetrics: getWindowVisibleDisplayFrame is unreliable before the view is attached
        DisplayMetrics dm = mContext.getResources().getDisplayMetrics();
        mEditText.setMinWidth(dm.widthPixels / 2);
        return mEditText;
    }
}
