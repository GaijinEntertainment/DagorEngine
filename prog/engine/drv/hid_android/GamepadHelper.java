package com.gaijinent.helpers;

import android.app.Activity;
import android.content.Context;
import android.hardware.input.InputManager;
import android.view.Display;
import android.view.InputDevice;
import android.view.Surface;
import android.util.Log;
import android.os.Looper;
import android.os.Handler;

import com.gaijinent.common.DagorLogger;

class GamepadListener implements InputManager.InputDeviceListener {
  private boolean mDeviceChanged = true;
  public static final int UNKNOWN_ID = -1;
  private int mCurrentDeviceId = UNKNOWN_ID;

  public GamepadListener() {
    int[] deviceIds = InputDevice.getDeviceIds();
    for (int i = 0; i < deviceIds.length; ++i) {
      int id = deviceIds[i];
      if (isGamepad(id)) {
        mCurrentDeviceId = id;
        DagorLogger.logDebug("gamepad: started with gamepad=" + mCurrentDeviceId);
        break;
      }
    }
  }

  private boolean isGamepad(int deviceId) {
    if (deviceId == 0)
      return false;

    InputDevice dev = InputDevice.getDevice(deviceId);
    if (dev != null) {
      int sources = dev.getSources();
      return (((sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD)
        && ((sources & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK));
    }
    else
      return false;
  }

  private static native void nativeOnChangeCurrentInputDevice();

  private void changeCurrentDevice(int deviceId, String src) {
    DagorLogger.logDebug("gamepad: changing gamepad: new=" + deviceId + " old=" + mCurrentDeviceId + " context:" + src);
    mDeviceChanged = true;
    mCurrentDeviceId = deviceId;

    nativeOnChangeCurrentInputDevice();
  }

  public void onInputDeviceAdded(int deviceId) {
    if (isGamepad(deviceId)) {
      DagorLogger.logDebug("gamepad: os added gamepad, id=" + deviceId);
      if (mCurrentDeviceId != deviceId)
        changeCurrentDevice(deviceId, "[device added]");
    }
  }

  public void onInputDeviceChanged(int deviceId) {
    if (isGamepad(deviceId)) {
      DagorLogger.logDebug("gamepad: os changed gamepad, id=" + deviceId);
      if (mCurrentDeviceId != deviceId)
        changeCurrentDevice(deviceId, "[device changed]");
    }
  }

  public void onInputDeviceRemoved(int deviceId) {
    if (mCurrentDeviceId == deviceId)
      changeCurrentDevice(UNKNOWN_ID, "[device removed]");
  }

  public boolean acquireDeviceChanges() {
    boolean res = mDeviceChanged;
    mDeviceChanged = false;
    return res;
  }

  public boolean isGamepadConnected() {
    return mCurrentDeviceId != UNKNOWN_ID;
  }

  public int getConnectedGamepadVendorId() {
    return isGamepadConnected() ?
            InputDevice.getDevice(mCurrentDeviceId).getVendorId() :
            UNKNOWN_ID;
  }
  public int getConnectedGamepadProductId() {
    return isGamepadConnected() ?
            InputDevice.getDevice(mCurrentDeviceId).getProductId() :
            UNKNOWN_ID;
  }
}


public class GamepadHelper {
    private static GamepadListener gamepadListener = null;
    private static InputManager imm = null;

    public static void listenGamepads(final Activity activity) {
      if (gamepadListener == null) {
        Context context = activity;
        imm = (InputManager)context.getSystemService(Context.INPUT_SERVICE);
        gamepadListener = new GamepadListener();
        Looper looper = Looper.getMainLooper();
        Handler handler = new Handler(looper);
        imm.registerInputDeviceListener(gamepadListener, handler);
        imm.getInputDevice(0);
      }
    }

    public static void stopListeningGamepads() {
      if (gamepadListener != null) {
        imm.unregisterInputDeviceListener(gamepadListener);
        gamepadListener = null;
      }
    }

    public static boolean isDeviceChanged() {
      return gamepadListener != null ?
              gamepadListener.acquireDeviceChanges() :
              false;
    }

    public static boolean isGamepadConnected() {
      return gamepadListener != null ?
              gamepadListener.isGamepadConnected() :
              false;
    }

    public static int getConnectedGamepadVendorId() {
      return gamepadListener != null ?
              gamepadListener.getConnectedGamepadVendorId() :
              GamepadListener.UNKNOWN_ID;
    }
    public static int getConnectedGamepadProductId() {
      return gamepadListener != null ?
              gamepadListener.getConnectedGamepadProductId() :
              GamepadListener.UNKNOWN_ID;
    }

    public static int getDisplayRotation(final Activity activity) {
      Context context = activity;
      Display display = context.getDisplay();
      int rotation = display.getRotation();

      switch (rotation) {
        case Surface.ROTATION_90: return 1;
        case Surface.ROTATION_180: return 2;
        case Surface.ROTATION_270: return 3;
        default: return 0;
      }
    }
}
