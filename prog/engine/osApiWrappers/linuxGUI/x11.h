// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_linuxGUI.h>
#include <unistd.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#ifdef Bool
#undef Bool
#endif

#include <generic/dag_tab.h>
#include <generic/dag_tabUtils.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>

#include <osApiWrappers/setProgGlobals.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_dynLib.h>
#include <supp/dag_math.h>
#include <math/dag_mathBase.h>
#include <math/integer/dag_IPoint2.h>
#include <memory/dag_framemem.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>

struct X11Randr
{
  XRROutputInfo *(*XRRGetOutputInfo)(Display *dpy, XRRScreenResources *, RROutput);
  XRRScreenConfiguration *(*XRRGetScreenInfo)(Display *dpy, Window window);
  XRRScreenResources *(*XRRGetScreenResources)(Display *dpy, Window window);
  XRRScreenSize *(*XRRSizes)(Display *dpy, int screen, int *nsizes);
  int (*XRRRootToScreen)(Display *dpy, Window root);
  void (*XRRSetScreenConfig)(Display *dpy, XRRScreenConfiguration *config, Drawable draw, int size_index, Rotation rotation,
    Time timestamp);
  SizeID (*XRRConfigCurrentConfiguration)(XRRScreenConfiguration *config, Rotation *rotation);
  void (*XRRFreeScreenConfigInfo)(XRRScreenConfiguration *config);
  void (*XRRFreeScreenResources)(XRRScreenResources *resources);
  void (*XRRFreeOutputInfo)(XRROutputInfo *outputInfo);
  void (*XRRFreeCrtcInfo)(XRRCrtcInfo *crtcInfo);
  XRRCrtcGamma *(*XRRGetCrtcGamma)(Display *dpy, RRCrtc crtc);
  void (*XRRSetCrtcGamma)(Display *dpy, RRCrtc crtc, XRRCrtcGamma *gamma);
  XRRCrtcGamma *(*XRRAllocGamma)(int size);
  void (*XRRFreeGamma)(XRRCrtcGamma *gamma);
  XRRPanning *(*XRRGetPanning)(Display *dpy, XRRScreenResources *resources, RRCrtc crtc);
  void (*XRRFreePanning)(XRRPanning *panning);
  int (*XRRSetPanning)(Display *dpy, XRRScreenResources *resources, RRCrtc crtc, XRRPanning *panning);
  void (*XRRSetOutputPrimary)(Display *dpy, Window window, RROutput output);
  RROutput (*XRRGetOutputPrimary)(Display *dpy, Window window);
  void (*XRRSetScreenSize)(Display *dpy, Window window, int width, int height, int mmWidth, int mmHeight);
  XRRCrtcInfo *(*XRRGetCrtcInfo)(Display *, XRRScreenResources *, RRCrtc);
  Status (*XRRSetCrtcConfig)(Display *, XRRScreenResources *, RRCrtc, Time, int, int, RRMode, Rotation, RROutput *, int);
  Rotation (*XRRRotations)(Display *dpy, int screen, Rotation *current_rotation);
  short (*XRRConfigCurrentRate)(XRRScreenConfiguration *config);

  void init(void *so)
  {
    *(void **)&XRRSizes = os_dll_get_symbol(so, "XRRSizes");
    *(void **)&XRRRootToScreen = os_dll_get_symbol(so, "XRRRootToScreen");
    *(void **)&XRRGetScreenInfo = os_dll_get_symbol(so, "XRRGetScreenInfo");
    *(void **)&XRRSetScreenConfig = os_dll_get_symbol(so, "XRRSetScreenConfig");
    *(void **)&XRRConfigCurrentConfiguration = os_dll_get_symbol(so, "XRRConfigCurrentConfiguration");
    *(void **)&XRRFreeScreenConfigInfo = os_dll_get_symbol(so, "XRRFreeScreenConfigInfo");
    *(void **)&XRRGetCrtcInfo = os_dll_get_symbol(so, "XRRGetCrtcInfo");
    *(void **)&XRRSetCrtcConfig = os_dll_get_symbol(so, "XRRSetCrtcConfig");
    *(void **)&XRRGetScreenResources = os_dll_get_symbol(so, "XRRGetScreenResources");
    *(void **)&XRRGetOutputInfo = os_dll_get_symbol(so, "XRRGetOutputInfo");
    *(void **)&XRRFreeScreenResources = os_dll_get_symbol(so, "XRRFreeScreenResources");
    *(void **)&XRRFreeOutputInfo = os_dll_get_symbol(so, "XRRFreeOutputInfo");
    *(void **)&XRRFreeCrtcInfo = os_dll_get_symbol(so, "XRRFreeCrtcInfo");
    *(void **)&XRRGetCrtcGamma = os_dll_get_symbol(so, "XRRGetCrtcGamma");
    *(void **)&XRRSetCrtcGamma = os_dll_get_symbol(so, "XRRSetCrtcGamma");
    *(void **)&XRRAllocGamma = os_dll_get_symbol(so, "XRRAllocGamma");
    *(void **)&XRRFreeGamma = os_dll_get_symbol(so, "XRRFreeGamma");
    *(void **)&XRRGetPanning = os_dll_get_symbol(so, "XRRGetPanning");
    *(void **)&XRRFreePanning = os_dll_get_symbol(so, "XRRFreePanning");
    *(void **)&XRRSetPanning = os_dll_get_symbol(so, "XRRSetPanning");
    *(void **)&XRRSetOutputPrimary = os_dll_get_symbol(so, "XRRSetOutputPrimary");
    *(void **)&XRRGetOutputPrimary = os_dll_get_symbol(so, "XRRGetOutputPrimary");
    *(void **)&XRRSetScreenSize = os_dll_get_symbol(so, "XRRSetScreenSize");
    *(void **)&XRRRotations = os_dll_get_symbol(so, "XRRRotations");
    *(void **)&XRRConfigCurrentRate = os_dll_get_symbol(so, "XRRConfigCurrentRate");
  }

  inline void reset() { memset(this, 0, sizeof(*this)); }
};

struct X11
{
#include "muxInterface.inc.h"

  struct GammaInfo
  {
    RRCrtc crtc;
    XRRCrtcGamma *gamma;
  };

  struct CRTCState
  {
    RRCrtc crtc;
    RRMode mode;
    int x, y;
    Rotation rotation;
    RROutput output;
  };

  static int xDisplayErrorHandler(Display *display, XErrorEvent *err)
  {
    char text[512];
    XGetErrorText(display, err->error_code, text, sizeof(text));
    logwarn("x11: error [ %s ]", text);
    return 0;
  }

  static int xDisplayIOErrorHandler(Display *display)
  {
    G_UNREFERENCED(display);
    DAG_FATAL("x11: IO fatal error occurred! Application will be terminated");
    return 0;
  }

  inline void setWMCCardinalProp(const char *prop_name, long value)
  {
    XChangeProperty(rootDisplay, mainWindow, XInternAtom(rootDisplay, prop_name, False), XA_CARDINAL, 32, PropModeReplace,
      (unsigned char *)&value, 1);
  }

  template <size_t N>
  inline void setWMAtomProps(const char *prop_name, const char *(&value_atoms)[N])
  {
    Atom atoms[N];
    for (int i = 0; i < N; ++i)
      atoms[i] = XInternAtom(rootDisplay, value_atoms[i], False);

    XChangeProperty(rootDisplay, mainWindow, XInternAtom(rootDisplay, prop_name, False), XA_ATOM, 32, PropModeReplace,
      (const unsigned char *)atoms, N);
  }

  inline void setWMAtomProp(const char *prop_name, const char *value_atom)
  {
    const char *valueArr[1] = {value_atom};
    setWMAtomProps(prop_name, valueArr);
  }

  inline bool isDeleteMessage(Atom ident) const { return ident == wmDelete; }
  inline bool isPingMessage(Atom ident) const { return ident == wmPing; }

  void setWindowClassHint(const char *title)
  {
    XClassHint *classhint = XAllocClassHint();
    classhint->res_name = (char *)title;
    classhint->res_class = (char *)title;
    XSetClassHint(rootDisplay, mainWindow, classhint);
    XFree(classhint);
  }

  static inline int isMapNotify(Display *dpy, XEvent *ev, XPointer win)
  {
    G_UNREFERENCED(dpy);
    return ev->type == MapNotify && ev->xmap.window == *((Window *)win);
  }

  inline void restoreCrtcState()
  {
    if (!randr.XRRGetScreenResources)
      return;

    XRRScreenResources *screen = randr.XRRGetScreenResources(rootDisplay, rootWindow);
    for (int i = 0; i < originalCrtcState.size(); ++i)
    {
      CRTCState &state = originalCrtcState[i];
      randr.XRRSetCrtcConfig(rootDisplay, screen, state.crtc, CurrentTime, state.x, state.y, state.mode, state.rotation, &state.output,
        1);
    }

    randr.XRRFreeScreenResources(screen);
  }

  void cacheWindowAttribs()
  {
    Window dummy;
    XGetWindowAttributes(rootDisplay, mainWindow, &windowAttribs[WND_MAIN]);
    XGetWindowAttributes(rootDisplay, rootWindow, &windowAttribs[WND_ROOT]);

    XWindowAttributes &trAttrib = windowAttribs[WND_TRANSLATED_MAIN];
    trAttrib = windowAttribs[WND_MAIN];
    XTranslateCoordinates(rootDisplay, mainWindow, rootWindow, 0, 0, &trAttrib.x, &trAttrib.y, &dummy);
  }

  const XWindowAttributes &getWindowAttrib(Window w, bool translated);

  enum
  {
    WND_MAIN = 0,
    WND_ROOT = 1,
    WND_TRANSLATED_MAIN = 2,
    WND_COUNT = 3,

    CURSOR_WARP_RECOVERY_CYCLES = 30
  };

  XErrorHandler xDefErrorHandler;
  XIOErrorHandler xDefIOErrorHandler;

  Display *rootDisplay;
  int rootScreenIndex;
  Window rootWindow;

  void *randrLib;
  X11Randr randr;

  Tab<GammaInfo> originalGammaTables;
  Tab<CRTCState> originalCrtcState;

  Window mainWindow;
  Atom wmDelete;
  Atom wmPing;

  XIM im;
  XIC ic;

  RROutput primaryOutput;
  bool multipleOutputs;
  int primaryOutputX;
  int primaryOutputY;

  int lastCursorX;
  int lastCursorY;
  int forceQueryCursorPosition;
  void cacheCursorPosition();

  XWindowAttributes windowAttribs[WND_COUNT];
  bool selectionReady = false;
};
