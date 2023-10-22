package com.gaijinent.MessageBox;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.util.Log;
import java.util.concurrent.Semaphore;

public final class MessageBoxWrapper
{
  private static final String TAG = "Dagor";
  private static boolean messageBoxVisible;
  private static int result = MessageBoxWrapper.GUI_MB_FAIL;
  private static final Semaphore semaphore = new Semaphore(0, true);

  // Return values for message_box. must be in sync with C++
  private static final int GUI_MB_FAIL     = -1;  // fail when creating window
  private static final int GUI_MB_CLOSE    =  0;  // window just closed
  private static final int GUI_MB_BUTTON_1 =  1;  // first  button pressed
  private static final int GUI_MB_BUTTON_2 =  2;  // second button pressed
  private static final int GUI_MB_BUTTON_3 =  3;   // third  button pressed

  private static final class ClickHandler implements DialogInterface.OnClickListener
  {
    private int buttonId;

    public ClickHandler(int buttonId)
    {
      this.buttonId = buttonId;
    }

    public void onClick(DialogInterface dialog, int id)
    {
      MessageBoxWrapper.result = buttonId;
      MessageBoxWrapper.semaphore.release();
    }
  }

  // This function will be called from C++ by name and signature
  public static int showAlert(final Activity activity, final String title, final String message,
    final String bt1Text, final String bt2Text, final String bt3Text)
  {
    Log.e(TAG,"ShowAlert");
    if (MessageBoxWrapper.messageBoxVisible)
    {
      Log.e(TAG, String.format("Another message box visible while showAlert(%s) was called", title));
      return MessageBoxWrapper.GUI_MB_FAIL;
    }

    try
    {
      MessageBoxWrapper.messageBoxVisible = true;
      MessageBoxWrapper.result = MessageBoxWrapper.GUI_MB_CLOSE;

      activity.runOnUiThread(new Runnable()
      {
        public void run()
        {
          try
          {
            AlertDialog.Builder builder = new AlertDialog.Builder(activity);

            builder.setTitle(title);
            builder.setMessage(message);
            builder.setPositiveButton(bt1Text, new ClickHandler(MessageBoxWrapper.GUI_MB_BUTTON_1));

            builder.setOnCancelListener(new OnCancelListener() {
              public void onCancel(DialogInterface dialog) {
                MessageBoxWrapper.result = MessageBoxWrapper.GUI_MB_CLOSE;
                MessageBoxWrapper.semaphore.release();
              }
            });

            if (bt2Text != "" && bt3Text != "")
            {
              builder.setNeutralButton(bt2Text, new ClickHandler(MessageBoxWrapper.GUI_MB_BUTTON_2));
              builder.setNegativeButton(bt3Text, new ClickHandler(MessageBoxWrapper.GUI_MB_BUTTON_3));
            }
            else if (bt2Text != "")
            {
              builder.setNegativeButton(bt2Text, new ClickHandler(MessageBoxWrapper.GUI_MB_BUTTON_2));
            }

            AlertDialog dialog = builder.create();
            dialog.show();
          }
          catch (Exception e)
          {
            Log.e(TAG, e.toString());
            MessageBoxWrapper.semaphore.release();
            MessageBoxWrapper.result = MessageBoxWrapper.GUI_MB_FAIL;
          }
        }
      });

      try
      {
          semaphore.acquire();
      }
      catch (InterruptedException e) { }
    }
    catch (Exception e)
    {
      Log.e(TAG, e.toString());
      MessageBoxWrapper.semaphore.release();
      MessageBoxWrapper.result = MessageBoxWrapper.GUI_MB_FAIL;
    }
    finally
    {
      MessageBoxWrapper.messageBoxVisible = false;
    }

    return result;
  }
}