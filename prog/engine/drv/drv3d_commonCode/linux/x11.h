// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

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
#include <osApiWrappers/dag_dynLib.h>
#include <supp/dag_math.h>
#include <math/dag_mathBase.h>
#include <math/integer/dag_IPoint2.h>
#include <memory/dag_framemem.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include "drv_log_defs.h"

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

  inline bool init()
  {
    xDefErrorHandler = XSetErrorHandler(&xDisplayErrorHandler);
    xDefIOErrorHandler = XSetIOErrorHandler(&xDisplayIOErrorHandler);
    XInitThreads();

    rootDisplay = XOpenDisplay(NULL);
    if (!rootDisplay)
    {
      DAG_FATAL("x11: cannot connect to X server");
      return false;
    }

    rootScreenIndex = XDefaultScreen(rootDisplay);
    rootWindow = RootWindow(rootDisplay, rootScreenIndex);
    primaryOutput = None;
    multipleOutputs = false;
    primaryOutputX = 0;
    primaryOutputY = 0;
    lastCursorX = 0;
    lastCursorY = 0;
    forceQueryCursorPosition = 0;

    // as we handle missing entry points, load any viable xrandr there is
    randrLib = os_dll_load("libXrandr.so.2");
    if (!randrLib)
      randrLib = os_dll_load("libXrandr.so.1");
    if (randrLib)
    {
      randr.init(randrLib);

      if (randr.XRRGetOutputPrimary(rootDisplay, rootWindow))
      {
        primaryOutput = randr.XRRGetOutputPrimary(rootDisplay, rootWindow);
        debug("x11: primary output %u", primaryOutput);
      }
      else
        debug("x11: can't detect primary output");

      if (randr.XRRGetScreenResources)
      {
        int activeOutputs = 0;

        XRRScreenResources *screen = randr.XRRGetScreenResources(rootDisplay, rootWindow);
        for (int output = 0; output < screen->noutput; ++output)
        {
          XRROutputInfo *outputInfo = randr.XRRGetOutputInfo(rootDisplay, screen, screen->outputs[output]);
          if (!outputInfo)
            continue;

          if (outputInfo->crtc && outputInfo->connection != RR_Disconnected)
          {
            ++activeOutputs;
            debug("x11: active output %u:%s %s (cid=%u)", output,
              outputInfo->nameLen && outputInfo->name ? outputInfo->name : "unknown",
              primaryOutput == screen->outputs[output] ? "primary" : "secondary", screen->outputs[output]);
          }

          randr.XRRFreeOutputInfo(outputInfo);
        }

        debug("x11: %u outputs %u active", screen->noutput, activeOutputs);
        multipleOutputs = activeOutputs > 1;

        randr.XRRFreeScreenResources(screen);
      }
    }
    else
    {
      debug("x11: randr is not available!");
      randr.reset();
    }

    return true;
  }

  inline void shutdown()
  {
    for (int i = 0; i < originalGammaTables.size(); ++i)
    {
      randr.XRRSetCrtcGamma(rootDisplay, originalGammaTables[i].crtc, originalGammaTables[i].gamma);
      randr.XRRFreeGamma(originalGammaTables[i].gamma);
    }
    clear_and_shrink(originalGammaTables);

    XCloseDisplay(rootDisplay);
    if (randrLib)
    {
      os_dll_close(randrLib);
      randrLib = NULL;
    }
  }

  inline bool changeGamma(float value)
  {
    if (!randr.XRRGetCrtcGamma)
      return false;

    XRRScreenResources *screen = randr.XRRGetScreenResources(rootDisplay, rootWindow);
    if (originalGammaTables.empty())
    {
      for (int i = 0; i < screen->noutput; ++i)
      {
        XRROutputInfo *outputInfo = randr.XRRGetOutputInfo(rootDisplay, screen, screen->outputs[i]);
        if (!outputInfo)
          continue;

        if (outputInfo->crtc && outputInfo->connection != RR_Disconnected)
        {
          XRRCrtcGamma *original = randr.XRRGetCrtcGamma(rootDisplay, outputInfo->crtc);
          if (original)
          {
            GammaInfo &state = originalGammaTables.push_back();
            state.crtc = outputInfo->crtc;
            state.gamma = original;
          }
        }

        randr.XRRFreeOutputInfo(outputInfo);
      }
    }

    for (int i = 0; i < screen->noutput; ++i)
    {
      XRROutputInfo *outputInfo = randr.XRRGetOutputInfo(rootDisplay, screen, screen->outputs[i]);
      if (!outputInfo)
        continue;

      if (outputInfo->crtc && outputInfo->connection != RR_Disconnected)
      {
        for (int j = 0; j < originalGammaTables.size(); ++j)
        {
          if (originalGammaTables[j].crtc == outputInfo->crtc)
          {
            int size = originalGammaTables[j].gamma->size;
            XRRCrtcGamma *gamma = randr.XRRAllocGamma(size);

            for (int j = 0; j < size; ++j)
            {
              int v = real2int(powf(j / (float)(size - 1), value) * 65535);
              gamma->red[j] = gamma->green[j] = gamma->blue[j] = v;
            }

            randr.XRRSetCrtcGamma(rootDisplay, outputInfo->crtc, gamma);
            randr.XRRFreeGamma(gamma);
            break;
          }
        }
      }

      randr.XRRFreeOutputInfo(outputInfo);
    }

    randr.XRRFreeScreenResources(screen);
    return true;
  }

  inline void getDisplaySize(int &width, int &height, bool for_primary_output = false)
  {
    width = DisplayWidth(rootDisplay, rootScreenIndex);
    height = DisplayHeight(rootDisplay, rootScreenIndex);

    debug("x11: default display size %dx%d", width, height);

    if (randr.XRRSizes)
    {
      int nsizes = 0;
      int screen = randr.XRRRootToScreen(rootDisplay, rootWindow);
      Rotation curRotation = RR_Rotate_0;
      randr.XRRRotations(rootDisplay, screen, &curRotation);
      XRRScreenSize *sizes = randr.XRRSizes(rootDisplay, screen, &nsizes);
      if (nsizes > 0)
      {
        switch (curRotation)
        {
          case RR_Rotate_0:
          case RR_Rotate_180:
            width = sizes[0].width;
            height = sizes[0].height;
            break;
          case RR_Rotate_90:
          case RR_Rotate_270:
            width = sizes[0].height;
            height = sizes[0].width;
            break;
          default:
            // still use xrandr sizes but complain as fallback
            width = sizes[0].width;
            height = sizes[0].height;
            debug("x11: unknown xrandr rotation!");
            break;
        }
        debug("x11: screen size from xrandr %dx%d rotation %u. nsizes %d", width, height, curRotation, nsizes);
      }
    }

    if (for_primary_output && multipleOutputs && (primaryOutput != None))
    {
      // multiple outputs can be true only when XRRGetScreenResources available
      XRRScreenResources *screen = randr.XRRGetScreenResources(rootDisplay, rootWindow);
      XRROutputInfo *outputInfo = randr.XRRGetOutputInfo(rootDisplay, screen, primaryOutput);

      if (!outputInfo)
      {
        randr.XRRFreeScreenResources(screen);
        D3D_ERROR("x11: can't read primary output info");
        return;
      }

      if (outputInfo->crtc && outputInfo->connection != RR_Disconnected)
      {
        XRRCrtcInfo *crtcInfo = randr.XRRGetCrtcInfo(rootDisplay, screen, outputInfo->crtc);

        if (crtcInfo)
        {
          width = crtcInfo->width;
          height = crtcInfo->height;
          primaryOutputX = crtcInfo->x;
          primaryOutputY = crtcInfo->y;
          debug("x11: screen size from primary output %dx%d (pos [%d,%d])", width, height, crtcInfo->x, crtcInfo->y);
          randr.XRRFreeCrtcInfo(crtcInfo);
        }
        else
          D3D_ERROR("x11: primary output crtc info is not available");
      }
      else
        D3D_ERROR("x11: primary output is not available");

      randr.XRRFreeOutputInfo(outputInfo);
      randr.XRRFreeScreenResources(screen);
    }
  }

  inline void getVideoModeList(Tab<String> &list)
  {
    if (randr.XRRGetScreenResources)
    {
      clear_and_shrink(list);

      XRRScreenResources *screen = randr.XRRGetScreenResources(rootDisplay, rootWindow);
      Tab<IPoint2> modesList(framemem_ptr());

      for (int output = 0; output < screen->noutput; ++output)
      {
        XRROutputInfo *outputInfo = randr.XRRGetOutputInfo(rootDisplay, screen, screen->outputs[output]);
        if (!outputInfo)
          continue;

        if (outputInfo->crtc && outputInfo->connection != RR_Disconnected)
        {
          for (int nm = 0; nm != outputInfo->nmode; ++nm)
          {
            XRRModeInfo *modeInfo = NULL;
            for (int nm2 = 0; nm2 != screen->nmode; ++nm2)
            {
              if (screen->modes[nm2].id == outputInfo->modes[nm])
              {
                modeInfo = &screen->modes[nm2];
                break;
              }
            }

            if (modeInfo && tabutils::getIndex(modesList, IPoint2(modeInfo->width, modeInfo->height)) == -1)
            {
              modesList.push_back(IPoint2(modeInfo->width, modeInfo->height));
              list.push_back(String(64, "%d x %d", modeInfo->width, modeInfo->height));
            }
          }
        }

        randr.XRRFreeOutputInfo(outputInfo);
      }

      randr.XRRFreeScreenResources(screen);
    }
    else
    {
      clear_and_shrink(list);
      static const char *default_sizes[] = {"800 x 600", "1024 x 768", "1280 x 720", "1366 x 768", "1280 x 1024", "1440 x 900",
        "1600 x 900", "1680 x 1050", "1920 x 1080", "1920 x 1200"};
      const size_t default_sizes_count = sizeof(default_sizes) / sizeof(default_sizes[0]);
      list.resize(default_sizes_count);
      for (int i = 0; i < default_sizes_count; ++i)
        list[i] = default_sizes[i];
    }
  }

  inline bool isMainWindow(void *wnd) const { return &mainWindow == wnd; }
  inline void destroyMainWindow() { XDestroyWindow(rootDisplay, mainWindow); }

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


  static inline int isMapNotify(Display *dpy, XEvent *ev, XPointer win)
  {
    G_UNREFERENCED(dpy);
    return ev->type == MapNotify && ev->xmap.window == *((Window *)win);
  }

  inline bool initWindow(XVisualInfo *vi, const char *title, int winWidth, int winHeight)
  {
    XSetWindowAttributes swa;
    swa.colormap = XCreateColormap(rootDisplay, rootWindow, vi->visual, AllocNone);
    swa.override_redirect = False;
    swa.border_pixel = 0;
    swa.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | PointerMotionMask |
                     ButtonMotionMask | KeymapStateMask | LeaveWindowMask | FocusChangeMask;
    unsigned long valuemask = CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect;
    // place window on primary output monitor
    int x = primaryOutputX;
    int y = primaryOutputY;

    debug("x11: win size %dx%d", winWidth, winHeight);

    mainWindow =
      XCreateWindow(rootDisplay, rootWindow, x, y, winWidth, winHeight, 0, vi->depth, InputOutput, vi->visual, valuemask, &swa);

    win32_set_main_wnd(&mainWindow);
    if (::dgs_get_settings()->getBool("utf8mode", false))
      setTitleUtf8(title, title);
    else
      setTitle(title, title);

    XClassHint *classhint = XAllocClassHint();
    classhint->res_name = (char *)title;
    classhint->res_class = (char *)title;

    XWMHints *wmhints = XAllocWMHints();
    wmhints->input = True;
    wmhints->flags = InputHint;

    XSizeHints *sizehints = XAllocSizeHints();
    sizehints->flags = 0;
    bool windowed = dgs_get_window_mode() != WindowMode::FULLSCREEN_EXCLUSIVE;
    if (windowed)
    {
      sizehints->min_width = sizehints->max_width = winWidth;
      sizehints->min_height = sizehints->max_height = winHeight;
      sizehints->flags |= (PMaxSize | PMinSize);
    }
    sizehints->x = x;
    sizehints->y = y;
    sizehints->flags |= USPosition;

    XSetWMProperties(rootDisplay, mainWindow, NULL, NULL, NULL, 0, sizehints, wmhints, classhint);
    XFree(wmhints);
    XFree(classhint);
    XFree(sizehints);

    setWMCCardinalProp("_NET_WM_PID", getpid());
    setWMAtomProp("_NET_WM_WINDOW_TYPE", "_NET_WM_WINDOW_TYPE_NORMAL");
    setWMCCardinalProp("_NET_WM_BYPASS_COMPOSITOR", 1);

    wmPing = XInternAtom(rootDisplay, "_NET_WM_PING", False);
    wmDelete = XInternAtom(rootDisplay, "WM_DELETE_WINDOW", False);
    Atom protocols[] = {wmPing, wmDelete};
    XSetWMProtocols(rootDisplay, mainWindow, protocols, 2);

    Atom XdndAware = XInternAtom(rootDisplay, "XdndAware", False);
    Atom xdndVersion = 5;
    XChangeProperty(rootDisplay, mainWindow, XdndAware, XA_ATOM, 32, PropModeReplace, (unsigned char *)&xdndVersion, 1);

    im = XOpenIM(rootDisplay, NULL, NULL, NULL);
    ic = XCreateIC(im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, mainWindow, XNFocusWindow, mainWindow,
      nullptr);

    if (windowed)
    {
      static const char *states[] = {"_NET_WM_STATE_FOCUSED", "_NET_WM_STATE_MAXIMIZED_VERT"
                                                              "_NET_WM_STATE_MAXIMIZED_HORZ"};
      setWMAtomProps("_NET_WM_STATE", states);
    }
    else
    {
      /* Motif flags can easily break an fullscreen mode and to be a reason of heavy lags
      so use them cautiously or don't use at all */
      static const char *states[] = {"_NET_WM_STATE_FOCUSED", "_NET_WM_STATE_FULLSCREEN"};
      setWMAtomProps("_NET_WM_STATE", states);
    }

    XFlush(rootDisplay);
    XMapRaised(rootDisplay, mainWindow);
    XMoveWindow(rootDisplay, mainWindow, x, y);
    XEvent event;
    XIfEvent(rootDisplay, &event, isMapNotify, (XPointer)&mainWindow);
    XFlush(rootDisplay);
    cacheWindowAttribs();
    return true;
  }

  inline void setWindowClassHint(const char *title)
  {
    XClassHint *classhint = XAllocClassHint();
    classhint->res_name = (char *)title;
    classhint->res_class = (char *)title;
    XSetClassHint(rootDisplay, mainWindow, classhint);
    XFree(classhint);
  }
  inline void setTitle(const char *title, const char *tooltip = NULL)
  {
    XStoreName(rootDisplay, mainWindow, title);
    setWindowClassHint(tooltip ? tooltip : title);
  }

  inline void setTitleUtf8(const char *title, const char *tooltip = NULL)
  {
    XChangeProperty(rootDisplay, mainWindow, XInternAtom(rootDisplay, "_NET_WM_NAME", False),
      XInternAtom(rootDisplay, "UTF8_STRING", False), 8, PropModeReplace, (unsigned char *)title, strlen(title));
    setWindowClassHint(tooltip ? tooltip : title);
  }

  inline bool isDeleteMessage(Atom ident) const { return ident == wmDelete; }
  inline bool isPingMessage(Atom ident) const { return ident == wmPing; }

  int getScreenRefreshRate()
  {
    if (randr.XRRGetOutputPrimary && randr.XRRGetScreenResources && randr.XRRGetOutputInfo && randr.XRRGetCrtcInfo)
    {
      if (primaryOutput == None)
        return 0;

      int ret = 0;
      XRRScreenResources *screen = randr.XRRGetScreenResources(rootDisplay, rootWindow);
      if (screen)
      {
        XRROutputInfo *output = randr.XRRGetOutputInfo(rootDisplay, screen, primaryOutput);
        if (output)
        {
          XRRCrtcInfo *crtc = randr.XRRGetCrtcInfo(rootDisplay, screen, output->crtc);
          if (crtc)
          {
            XRRModeInfo *curModeInfo = nullptr;
            for (int i = 0; i < screen->nmode; ++i)
            {
              if (crtc->mode == screen->modes[i].id)
              {
                curModeInfo = &screen->modes[i];
                break;
              }
            }
            if (curModeInfo)
            {
              if (curModeInfo->hTotal && curModeInfo->vTotal)
                ret = (int)((double)curModeInfo->dotClock / (double)(curModeInfo->hTotal * curModeInfo->vTotal));
            }
            randr.XRRFreeCrtcInfo(crtc);
          }
          randr.XRRFreeOutputInfo(output);
        }
        randr.XRRFreeScreenResources(screen);
      }
      return ret;
    }
    else
    {
      // fallback to older xrandr API, with fail for "metamodes"
      if (!randr.XRRGetScreenInfo || !randr.XRRConfigCurrentRate)
        return 0;

      XRRScreenConfiguration *sc = randr.XRRGetScreenInfo(rootDisplay, rootWindow);
      if (!sc)
        return 0;
      int ret = randr.XRRConfigCurrentRate(sc);
      randr.XRRFreeScreenConfigInfo(sc);
      return ret;
    }
  }

  inline bool setScreenResolution(int width, int height, int &viewport_wd, int &viewport_ht)
  {
    if (!randr.XRRGetScreenResources)
      return false;

    bool success = false;
    XRRScreenResources *screen = randr.XRRGetScreenResources(rootDisplay, rootWindow);

    // Hold the server grabbed while messing with
    // the screen so that apps which notice the resize
    // event and ask for xinerama information from the server
    // receive up-to-date information
    XGrabServer(rootDisplay);

    for (int output = 0; output < screen->noutput; ++output)
    {
      bool outputPrimary = randr.XRRGetOutputPrimary(rootDisplay, rootWindow) == screen->outputs[output];
      if (!outputPrimary)
        continue;

      XRROutputInfo *outputInfo = randr.XRRGetOutputInfo(rootDisplay, screen, screen->outputs[output]);
      if (!outputInfo)
        continue;

      RRCrtc crtcId = outputInfo->crtc;
      RRMode mode = -1;

      if (!crtcId || outputInfo->connection == RR_Disconnected)
      {
        randr.XRRFreeOutputInfo(outputInfo);
        continue;
      }

      for (int nm = 0; nm != outputInfo->nmode; ++nm)
      {
        XRRModeInfo *modeInfo = NULL;
        for (int nm2 = 0; nm2 != screen->nmode; ++nm2)
        {
          if (screen->modes[nm2].id == outputInfo->modes[nm])
          {
            modeInfo = &screen->modes[nm2];
            break;
          }
        }

        if (modeInfo)
        {
          if (modeInfo->width == width && modeInfo->height == height)
          {
            mode = modeInfo->id;
            break;
          }
        }
      }

      if ((int)mode == -1)
        continue;

      XRRCrtcInfo *crtc = randr.XRRGetCrtcInfo(rootDisplay, screen, crtcId);
      if (!crtc)
        continue;

      if (crtc->width == width && crtc->height == height) // already in that resolution
      {
        success = true;
      }
      else
      {
// Does not work well enough. It just changed the resolution for the entire server
// (more precisely, along with screen resolution also and the size of the root window).
// Now the user's desktop and windows got rearranged, their KDE widgets are all
// screwed up, and they have to set it all back where it's supposed to, only
// for you to sod it all up again later. But it seems SDL has some solutions for
// this issue and would be useful to look at it.
#if 0
        // Disable screen
        (*pXRRSetCrtcConfig)(x11.rootDisplay, screen, crtcId, CurrentTime, 0, 0, None, RR_Rotate_0, NULL, 0);

        // Set virtual screen size
        (*pXRRSetScreenSize)(x11.rootDisplay, x11.rootWindow, width, height, outputInfo->mm_width,
          outputInfo->mm_height);

        Status status = (*pXRRSetCrtcConfig)(x11.rootDisplay, screen, crtcId,
                             CurrentTime, crtc->x, crtc->y,
                             mode, crtc->rotation, &screen->outputs[output],
                             1);
        success = (status == Success);

        if (success)
        {
          CRTCState state;
          state.crtc = crtcId;
          state.x    = crtc->x;
          state.y    = crtc->y;
          state.rotation = crtc->rotation;
          state.mode = crtc->mode;
          state.output = screen->outputs[output];
          originalCrtcState.push_back(state);
        }

        // Restore primary
        (*pXRRSetOutputPrimary)(x11.rootDisplay, x11.rootWindow, screen->outputs[output]);
#else
        // So instead we are just applying a viewport on the whole screen
        viewport_wd = crtc->width;
        viewport_ht = crtc->height;
#endif
      }

      randr.XRRFreeOutputInfo(outputInfo);
      randr.XRRFreeCrtcInfo(crtc);
      break;
    }

    // Release display
    XUngrabServer(rootDisplay);

    randr.XRRFreeScreenResources(screen);

    return success;
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

  XWindowAttributes windowAttribs[WND_COUNT];
};

extern X11 x11;
