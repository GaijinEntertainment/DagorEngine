package com.gaijinent.helpers;

import android.app.Activity;
import android.content.Context;
import android.hardware.input.InputManager;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Display;
import android.view.InputDevice;
import android.view.Surface;
import com.gaijinent.common.DagorLogger;
import java.util.HashSet;

class GamepadListener implements InputManager.InputDeviceListener {
    private volatile boolean mDeviceChanged = true;
    public static final int UNKNOWN_ID = -1;
    private volatile int mCurrentDeviceId = UNKNOWN_ID;
    private final HashSet<Integer> mKnownGamepads = new HashSet<>();

    public GamepadListener() {
        for (int id : InputDevice.getDeviceIds()) {
            if (isGamepad(id)) {
                mKnownGamepads.add(id);
                if (mCurrentDeviceId == UNKNOWN_ID)
                    mCurrentDeviceId = id;
            }
        }
        if (mCurrentDeviceId != UNKNOWN_ID)
            DagorLogger.logDebug("gamepad: started with gamepad=" + mCurrentDeviceId);
    }

    private int findConnectedGamepad(int excludedDeviceId) {
        for (int id : InputDevice.getDeviceIds()) {
            if (id != excludedDeviceId && isGamepad(id))
                return id;
        }
        return UNKNOWN_ID;
    }

    private boolean isGamepad(int deviceId) {
        if (deviceId == 0)
            return false;

        InputDevice dev = InputDevice.getDevice(deviceId);
        if (dev != null) {
            int sources = dev.getSources();
            return (((sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD)
                    && ((sources & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK));
        } else
            return false;
    }

    private static native void nativeOnChangeCurrentInputDevice();

    private void changeCurrentDevice(int deviceId, String src) {
        DagorLogger.logDebug("gamepad: changing gamepad: new=" + deviceId + " old=" + mCurrentDeviceId + " context:" + src);
        mDeviceChanged = true;
        mCurrentDeviceId = deviceId;

        nativeOnChangeCurrentInputDevice();
    }

    public synchronized void onInputDeviceAdded(int deviceId) {
        if (isGamepad(deviceId)) {
            mKnownGamepads.add(deviceId);
            DagorLogger.logDebug("gamepad: os added gamepad, id=" + deviceId);
            if (mCurrentDeviceId != deviceId)
                changeCurrentDevice(deviceId, "[device added]");
        }
    }

    public synchronized void onInputDeviceChanged(int deviceId) {
        if (!isGamepad(deviceId)) {
            mKnownGamepads.remove(deviceId);
            if (mCurrentDeviceId == deviceId)
                changeCurrentDevice(findConnectedGamepad(deviceId), "[device changed]");
            return;
        }
        DagorLogger.logDebug("gamepad: os changed gamepad, id=" + deviceId);
        if (mKnownGamepads.add(deviceId) && mCurrentDeviceId != deviceId)
            changeCurrentDevice(deviceId, "[device changed]");
    }

    public synchronized void onInputDeviceRemoved(int deviceId) {
        mKnownGamepads.remove(deviceId);
        if (mCurrentDeviceId == deviceId)
            changeCurrentDevice(findConnectedGamepad(deviceId), "[device removed]");
    }

    // a button of this device was pressed: a gamepad becomes current; returns false when the device is not a gamepad
    public synchronized boolean selectGamepad(int deviceId) {
        if (!isGamepad(deviceId)) {
            DagorLogger.logDebug("gamepad: pressed device id=" + deviceId + " is not a gamepad");
            return false;
        }
        if (mCurrentDeviceId != deviceId) {
            mKnownGamepads.add(deviceId);
            changeCurrentDevice(deviceId, "[button pressed]");
        }
        return true;
    }

    public synchronized boolean acquireDeviceChanges() {
        boolean res = mDeviceChanged;
        mDeviceChanged = false;
        return res;
    }

    private InputDevice getConnectedGamepad() {
        final int deviceId = mCurrentDeviceId;
        return deviceId != UNKNOWN_ID ? InputDevice.getDevice(deviceId) : null;
    }

    public boolean isGamepadConnected() {
        return getConnectedGamepad() != null;
    }

    public int getConnectedGamepadVendorId() {
        InputDevice dev = getConnectedGamepad();
        return dev != null ? dev.getVendorId() : UNKNOWN_ID;
    }

    public int getConnectedGamepadProductId() {
        InputDevice dev = getConnectedGamepad();
        return dev != null ? dev.getProductId() : UNKNOWN_ID;
    }

    // {device id, vendor id, product id} of one InputDevice, so a device event can't mix two gamepads; null when none
    public int[] getConnectedGamepadIds() {
        InputDevice dev = getConnectedGamepad();
        return dev != null ? new int[] {dev.getId(), dev.getVendorId(), dev.getProductId()} : null;
    }
}

// Only the native part calls the public static methods, and one build can pair this class with the native parts of
// several releases: keep the methods and their signatures, add a new method instead of changing one, and don't call
// new native methods from here.
public class GamepadHelper {
    private static GamepadListener gamepadListener = null;
    private static InputManager imm = null;

    public static void listenGamepads(final Activity activity) {
        if (gamepadListener == null) {
            Context context = activity;
            imm = (InputManager) context.getSystemService(Context.INPUT_SERVICE);
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
        return gamepadListener != null ? gamepadListener.acquireDeviceChanges() : false;
    }

    // the native part of older releases
    public static boolean isGamepadConnected() {
        return gamepadListener != null ? gamepadListener.isGamepadConnected() : false;
    }

    // the native part of older releases
    public static int getConnectedGamepadVendorId() {
        return gamepadListener != null ? gamepadListener.getConnectedGamepadVendorId() : GamepadListener.UNKNOWN_ID;
    }

    // the native part of older releases
    public static int getConnectedGamepadProductId() {
        return gamepadListener != null ? gamepadListener.getConnectedGamepadProductId() : GamepadListener.UNKNOWN_ID;
    }

    public static int[] getConnectedGamepadIds() {
        return gamepadListener != null ? gamepadListener.getConnectedGamepadIds() : null;
    }

    // the native part calls it from its thread after a button of another device was pressed; returns false when the
    // device is not a gamepad
    public static boolean selectGamepad(int deviceId) {
        final GamepadListener listener = gamepadListener;
        return listener != null && listener.selectGamepad(deviceId);
    }

    @SuppressWarnings("deprecation")
    public static int getDisplayRotation(final Activity activity) {
        Display display;
        try {
            display = Build.VERSION.SDK_INT >= Build.VERSION_CODES.R ? activity.getDisplay()
                                                                     : activity.getWindowManager().getDefaultDisplay();
        } catch (RuntimeException e) {
            return -1;
        }
        if (display == null)
            return -1;

        int rotation = display.getRotation();

        switch (rotation) {
            case Surface.ROTATION_90:
                return 1;
            case Surface.ROTATION_180:
                return 2;
            case Surface.ROTATION_270:
                return 3;
            default:
                return 0;
        }
    }
}
