//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// only for error messages
#include <util/dag_string.h>
#include "IHDRDecoder.h"

struct TexPixel32;
class BaseTexture;
typedef BaseTexture Texture;
struct Driver3dPerspective;

class ScreenShotSystem
{
public:
  typedef bool (*write_image_cb)(const char *fn, TexPixel32 *im, int wd, int ht, int stride);

  class ScreenShot
  {
    int w, h, stride;
    void *picture;
    bool allocated, fastLocked, hasWaterMark;
    friend class ScreenShotSystem;

  public:
    ScreenShot();
    ScreenShot(const ScreenShot &) = delete;
    ScreenShot &operator=(const ScreenShot &) = delete;
    ~ScreenShot() { clear(); }
    void clear();
    void reset()
    {
      clear();
      w = h = stride = 0;
      fastLocked = hasWaterMark = false;
    }
  };
  static bool makeScreenShot(ScreenShot &info, IHDRDecoder *decoder = nullptr);
  static bool makeTexScreenShot(ScreenShot &info, Texture *tex);
  static bool makeTriplanarTexScreenShot(ScreenShot &info, Texture *tex_r, Texture *tex_g, Texture *tex_b);
  static bool makeDepthTexScreenShot(ScreenShot &info, Texture *tex);
  static bool makeHugeScreenShot(float fov, float aspect, float znear, float zfar, int quadrants, ScreenShot &info);
  static bool makeHugeScreenShot(const Driver3dPerspective &persp, int multiply, ScreenShot &info);
  static bool saveScreenShot(ScreenShot &info);
  static bool saveScreenShotTo(ScreenShot &info, char *fileName);
  static bool saveScreenShotToClipboard(ScreenShot &info);
  static const char *lastMessage();
  static void setWriteImageCB(write_image_cb cb, const char *file_ext);
  static void setPrefix(const char *name) { prefix = name; }

protected:
  static char *lockScreen(int &w, int &h, int &stride, int &format, bool &fast);
  static void unlockScreen(bool fast);
  static bool setFrustrum(float left, float right, float bottom, float top, float znear, float zfar);
  static void makeWaterMark(ScreenShot &info);
  static void message(const String &err);

  static String last_error;
  static String file_ext;
  static write_image_cb writeImageCb;
  static String prefix;
};
