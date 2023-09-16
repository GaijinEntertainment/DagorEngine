// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <propPanel2/c_util.h>
#include <util/dag_simpleString.h>

#include <math/dag_math3d.h>


namespace p2util
{
static SimpleString icon_path_buffer;
static void *hwnd_main_window = 0;


const char *get_icon_path() { return icon_path_buffer.str(); }


void set_icon_path(const char *value) { icon_path_buffer = value; }


void *get_main_parent_handle() { return hwnd_main_window; }


void set_main_parent_handle(void *phandle) { hwnd_main_window = phandle; }


// Convert HSV colour specification to RGB  intensities.
// H in [0, 360], S V R G B in [0,1]
Point3 hsv2rgb(Point3 hsv)
{
  int i;
  double f, p, q, t;
  Point3 rgb = Point3(0, 0, 0);

  if (hsv.y == 0)
    rgb.x = rgb.y = rgb.z = hsv.z;
  else
  {
    if (hsv.x == 360.0)
      hsv.x = 0;
    hsv.x /= 60.0;

    i = hsv.x;
    f = hsv.x - i;
    p = hsv.z * (1.0 - hsv.y);
    q = hsv.z * (1.0 - (hsv.y * f));
    t = hsv.z * (1.0 - (hsv.y * (1.0 - f)));
    G_ASSERT(i >= 0 && i <= 5);
    switch (i)
    {
      case 0:
        rgb.x = hsv.z;
        rgb.y = t;
        rgb.z = p;
        break;

      case 1:
        rgb.x = q;
        rgb.y = hsv.z;
        rgb.z = p;
        break;

      case 2:
        rgb.x = p;
        rgb.y = hsv.z;
        rgb.z = t;
        break;

      case 3:
        rgb.x = p;
        rgb.y = q;
        rgb.z = hsv.z;
        break;

      case 4:
        rgb.x = t;
        rgb.y = p;
        rgb.z = hsv.z;
        break;

      case 5:
        rgb.x = hsv.z;
        rgb.y = p;
        rgb.z = q;
        break;
    }
  }

  return rgb;
}

// Convert RGB  intensities.to HSV colour specification
// H in [0, 360], S V R G B in [0,1]
Point3 rgb2hsv(Point3 rgb)
{
  double imax = max(rgb.x, max(rgb.y, rgb.z));
  double imin = min(rgb.x, min(rgb.y, rgb.z));
  double rc, gc, bc;
  Point3 hsv = Point3(0, 0, 0);

  hsv.z = imax;
  if (imax != 0)
    hsv.y = (imax - imin) / imax;
  else
    hsv.y = 0;

  if (hsv.y == 0)
    hsv.x = 0; //-1;
  else
  {
    rc = (imax - rgb.x) / (imax - imin);
    gc = (imax - rgb.y) / (imax - imin);
    bc = (imax - rgb.z) / (imax - imin);
    if (rgb.x == imax)
      hsv.x = bc - gc;
    else if (rgb.y == imax)
      hsv.x = 2.0 + rc - bc;
    else
      hsv.x = 4.0 + gc - rc;
    hsv.x *= 60.0;
    if (hsv.x < 0.0)
      hsv.x += 360.0;
  }

  return hsv;
}


E3DCOLOR rgb_to_e3(Point3 rgb) { return E3DCOLOR(floor(rgb.x * 255), floor(rgb.y * 255), floor(rgb.z * 255), 0); }

Point3 e3_to_rgb(E3DCOLOR e3) { return Point3(e3.r / 255.0, e3.g / 255.0, e3.b / 255.0); }
} // namespace p2util
