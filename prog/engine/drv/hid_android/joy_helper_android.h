// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace HumanInput
{
class JoyDeviceHelper
{
public:
  enum
  {
    ROTATION_0 = 0,
    ROTATION_90 = 1,
    ROTATION_180 = 2,
    ROTATION_270 = 3
  };

  static void detach();
  static bool isDeviceChanged();
  static bool getConnectedGamepadIds(int &device_id, int &vendor_id, int &product_id);
  static bool selectGamepad(int device_id);
  static int getRotation();
  static bool isDeviceChangePending();
  static void clearDeviceChanges();
};
} // namespace HumanInput
