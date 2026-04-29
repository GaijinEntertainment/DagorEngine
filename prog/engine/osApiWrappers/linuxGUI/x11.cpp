// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "x11.h"

#include <X11/Xcursor/Xcursor.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <osApiWrappers/dag_unicode.h>
#include <workCycle/dag_genGuiMgr.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_resetDevice.h>
#include <limits.h>

namespace workcycle_internal
{
extern intptr_t main_window_proc(void *, unsigned, uintptr_t, intptr_t);
}

bool X11::init()
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
          debug("x11: active output %u:%s %s (cid=%u)", output, outputInfo->nameLen && outputInfo->name ? outputInfo->name : "unknown",
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

void X11::shutdown()
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

bool X11::changeGamma(float value)
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

void X11::getDisplaySize(int &width, int &height, bool for_primary_output)
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
      logerr("x11: can't read primary output info");
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
        logerr("x11: primary output crtc info is not available");
    }
    else
      logerr("x11: primary output is not available");

    randr.XRRFreeOutputInfo(outputInfo);
    randr.XRRFreeScreenResources(screen);
  }
}

void X11::getVideoModeList(Tab<String> &list)
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

static_assert(sizeof(void *) >= sizeof(Window), "x11 window type should fit into pointer type!");
void *X11::getMainWindowPtrHandle() const { return (void *)mainWindow; }
bool X11::isMainWindow(void *wnd) const { return wnd == getMainWindowPtrHandle(); }
void X11::destroyMainWindow()
{
  XDestroyWindow(rootDisplay, mainWindow);
  mainWindow = None;
  win32_set_main_wnd(nullptr);
}

bool X11::initWindow(const char *title, int winWidth, int winHeight)
{
  XVisualInfo vInfoTemplate = {};
  vInfoTemplate.screen = rootScreenIndex;
  long visualMask = VisualScreenMask | VisualDepthMask;

  // this bugs out vulkan render on some compositors/WMs (namely xfce, cinammon)
  // vInfoTemplate.c_class = DirectColor;
  // visualMask |= VisualClassMask;

  int numberOfVisuals;
  XVisualInfo *vi = nullptr;
  for (int i = 0; i < 2 && !vi; ++i)
  {
    vInfoTemplate.depth = i ? 32 : 24;
    vi = XGetVisualInfo(rootDisplay, visualMask, &vInfoTemplate, &numberOfVisuals);
  }
  if (!vi)
  {
    DAG_FATAL("x11: can't get usefull VisualInfo from xlib");
    return false;
  }

  debug("x11: zero visual info id: %u class: %i depth: %u", vi->visualid, vi->c_class, vi->depth);

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

  win32_set_main_wnd(getMainWindowPtrHandle());
  if (::dgs_get_settings()->getBool("utf8mode", false))
    setTitleUTF8(title, title);
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
  WindowMode windowMode = dgs_get_window_mode();
  bool windowed = windowMode != WindowMode::FULLSCREEN_EXCLUSIVE && windowMode != WindowMode::WINDOWED_NO_BORDER &&
                  windowMode != WindowMode::WINDOWED_FULLSCREEN;
  bool resizable = windowMode == WindowMode::WINDOWED_RESIZABLE;
  if (windowed)
  {
    if (resizable)
    {
      sizehints->min_width = 320;
      sizehints->min_height = 240;
      sizehints->flags |= PMinSize;
    }
    else
    {
      sizehints->min_width = sizehints->max_width = winWidth;
      sizehints->min_height = sizehints->max_height = winHeight;
      sizehints->flags |= (PMaxSize | PMinSize);
    }
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
  ic =
    XCreateIC(im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, mainWindow, XNFocusWindow, mainWindow, nullptr);

  if (windowed)
  {
    static const char *states[] = {"_NET_WM_STATE_FOCUSED", "_NET_WM_STATE_MAXIMIZED_VERT", "_NET_WM_STATE_MAXIMIZED_HORZ"};
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
  XEvent event;
  XIfEvent(rootDisplay, &event, isMapNotify, (XPointer)&mainWindow);
  XFlush(rootDisplay);
  cacheWindowAttribs();
  XFree(vi);
  return true;
}

void X11::getWindowPosition(void *w, int &cx, int &cy)
{
  const XWindowAttributes &attributes = getWindowAttrib((intptr_t)w, true);
  cx = attributes.x;
  cy = attributes.y;
}

void X11::setTitle(const char *title, const char *tooltip)
{
  XStoreName(rootDisplay, mainWindow, title);
  setWindowClassHint(tooltip ? tooltip : title);
}

void X11::setTitleUTF8(const char *title, const char *tooltip)
{
  XChangeProperty(rootDisplay, mainWindow, XInternAtom(rootDisplay, "_NET_WM_NAME", False),
    XInternAtom(rootDisplay, "UTF8_STRING", False), 8, PropModeReplace, (unsigned char *)title, strlen(title));
  setWindowClassHint(tooltip ? tooltip : title);
}

int X11::getScreenRefreshRate()
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

void X11::setFullscreenMode(bool)
{
  // TODO: implement if needed
}

const XWindowAttributes &X11::getWindowAttrib(Window w, bool translated)
{
  if (w)
  {
    G_ASSERT(w == mainWindow);
    return windowAttribs[translated ? X11::WND_TRANSLATED_MAIN : X11::WND_MAIN];
  }
  else
    return windowAttribs[X11::WND_ROOT];
}

bool X11::getWindowScreenRect(void *w, linux_GUI::RECT *rect, linux_GUI::RECT *rect_unclipped)
{
  if (!w || !rect)
    return false;

  const XWindowAttributes &attrib = getWindowAttrib((intptr_t)w, true);
  const XWindowAttributes &rootAttrib = getWindowAttrib(0, false);

  linux_GUI::RECT winRect;
  linux_GUI::RECT rootRect;

  winRect.left = attrib.x;
  winRect.right = attrib.x + attrib.width;
  winRect.top = attrib.y;
  winRect.bottom = attrib.y + attrib.height;

  rootRect.left = rootAttrib.x;
  rootRect.right = rootAttrib.x + rootAttrib.width;
  rootRect.top = rootAttrib.y;
  rootRect.bottom = rootAttrib.y + rootAttrib.height;

  if (rect_unclipped)
    *rect_unclipped = winRect;

  // clip visible area
  if (winRect.left < rootRect.left)
    winRect.left = rootRect.left;

  if (winRect.right > rootRect.right)
    winRect.right = rootRect.right;

  if (winRect.top < rootRect.top)
    winRect.top = rootRect.top;

  if (winRect.bottom > rootRect.bottom)
    winRect.bottom = rootRect.bottom;

  *rect = winRect;
  rect->left += attrib.border_width;
  rect->right -= attrib.border_width;
  rect->top += attrib.border_width;
  rect->bottom -= attrib.border_width;

  return true;
}

void X11::cacheCursorPosition()
{
  Window root, child;
  int root_x, root_y;
  int win_x, win_y;
  unsigned int mask_return;

  bool result = XQueryPointer(rootDisplay, mainWindow, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask_return);

  lastCursorX = result ? win_x : root_x;
  lastCursorY = result ? win_y : root_y;
}

void X11::processMessages()
{
  if (!rootDisplay)
    return;

  static const int LBUTTON_BIT = 0;
  static const int RBUTTON_BIT = 1;
  static const int MBUTTON_BIT = 2;
  bool updateMousePos = false;
  static bool queryMousePos = false;

  XEvent xev;
  while (XPending(rootDisplay) > 0)
  {
    // Get an event for the queue
    // Do not use XFilterEvent because it might
    // be a reason for input lags
    XNextEvent(rootDisplay, &xev);

    int button = 0;
    int wheelDelta = 0;

    if (xev.xbutton.button == Button1)
      button = LBUTTON_BIT;
    else if (xev.xbutton.button == Button3)
      button = RBUTTON_BIT;
    else if (xev.xbutton.button == Button2)
      button = MBUTTON_BIT;
    else if (xev.xbutton.button == Button4)
      wheelDelta = 1;
    else if (xev.xbutton.button == Button5)
      wheelDelta = -1;
    else if (xev.xbutton.button >= 8)
      button = MBUTTON_BIT + 1 + xev.xbutton.button - 8;

    switch (xev.type)
    {
      case ConfigureNotify:
        cacheWindowAttribs();
        notify_window_resized(xev.xconfigure.width, xev.xconfigure.height);
        break;
      case ButtonPress:
        if (wheelDelta != 0)
          workcycle_internal::main_window_proc(&mainWindow, GPCM_MouseWheel, wheelDelta * 100, 0);
        else
          workcycle_internal::main_window_proc(&mainWindow, GPCM_MouseBtnPress, button, 0);
        break;

      case ButtonRelease: workcycle_internal::main_window_proc(&mainWindow, GPCM_MouseBtnRelease, button, 0); break;

      case LeaveNotify:
        cacheCursorPosition();
        updateMousePos = true;
        queryMousePos = true;
        break;
      case MotionNotify:
        if (xev.xmotion.window == mainWindow)
        {
          lastCursorX = xev.xmotion.x;
          lastCursorY = xev.xmotion.y;
        }
        queryMousePos = false;
        updateMousePos = true;
        break;

      case KeyPress:
      {
        KeySym keysym = 0;
        wchar_t keychar = 0;
        Status status = 0;
        char buf[32];
        int symb = Xutf8LookupString(ic, &xev.xkey, buf, sizeof(buf), &keysym, &status);
        if (symb > 0)
        {
          Tab<wchar_t> tmpBuf(framemem_ptr());
          wchar_t *key_ptr = convert_utf8_to_u16_buf(tmpBuf, buf, symb);
          if (key_ptr)
            keychar = key_ptr[0];
        }
        workcycle_internal::main_window_proc(&mainWindow, GPCM_KeyPress, xev.xkey.keycode, keychar);
        break;
      }

      case KeyRelease:
      {
        bool skipKeyRelease = false;
        if (XEventsQueued(rootDisplay, QueuedAfterReading))
        {
          XEvent nev;
          XPeekEvent(rootDisplay, &nev);
          if (nev.type == KeyPress && nev.xkey.time == xev.xkey.time && nev.xkey.keycode == xev.xkey.keycode)
          {
            skipKeyRelease = true;
          }
        }
        if (!skipKeyRelease)
          workcycle_internal::main_window_proc(&mainWindow, GPCM_KeyRelease, xev.xkey.keycode, 0);
      }
      break;

      case KeymapNotify:
      case MappingNotify: XRefreshKeyboardMapping(&xev.xmapping); break;

      case FocusIn:
        if (xev.xfocus.mode == NotifyGrab || xev.xfocus.mode == NotifyUngrab)
          break;
        if (xev.xfocus.detail == NotifyInferior)
          break;
        if ((void *)xev.xfocus.window != win32_get_main_wnd())
          continue;

        XSetICFocus(ic);
        workcycle_internal::main_window_proc(&mainWindow, GPCM_Activate, GPCMP1_Activate, 0);
        break;

      case FocusOut:
        if (xev.xfocus.mode == NotifyGrab || xev.xfocus.mode == NotifyUngrab)
          break;
        if (xev.xfocus.detail == NotifyInferior)
          break;
        if ((void *)xev.xfocus.window != win32_get_main_wnd())
          continue;

        XUnsetICFocus(ic);
        if (dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE)
        {
          XIconifyWindow(rootDisplay, mainWindow, rootScreenIndex);
        }
        workcycle_internal::main_window_proc(&mainWindow, GPCM_Activate, GPCMP1_Inactivate, 0);
        break;

      case ClientMessage:
        if (!isDeleteMessage(xev.xclient.data.l[0]))
        {
          if (isPingMessage(xev.xclient.data.l[0]))
          {
            xev.xclient.window = rootWindow;
            XSendEvent(rootDisplay, rootWindow, False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
          }
          break;
        }
        else
        {
          if (dagor_gui_manager && !dagor_gui_manager->canCloseNow())
            break;
        }
      case DestroyNotify:
      {
        void *ptrWindowHandle = (void *)xev.xdestroywindow.window;
        void *ptrParentHandle = (void *)xev.xdestroywindow.event;
        if ((win32_get_main_wnd() == ptrWindowHandle) || (win32_get_main_wnd() == ptrParentHandle))
        {
          debug("main window close signal received!");
          d3d::window_destroyed(ptrWindowHandle);
          win32_set_main_wnd(NULL);
          quit_game(0);
        }
        break;
      }
      case SelectionRequest:
      {
        XSelectionRequestEvent *req = &xev.xselectionrequest;
        XEvent sevent;
        memset(&sevent, 0, sizeof(sevent));
        sevent.xany.type = SelectionNotify;
        sevent.xselection.type = SelectionNotify;
        sevent.xselection.selection = req->selection;
        sevent.xselection.target = req->target;
        sevent.xselection.property = None;
        sevent.xselection.requestor = req->requestor;
        sevent.xselection.time = req->time;

        Atom XA_TARGETS = XInternAtom(rootDisplay, "TARGETS", False);
        if (req->target == XA_TARGETS)
        {
          Atom supportedFormats[] = {XA_TARGETS, XInternAtom(rootDisplay, "UTF8_STRING", False)};
          XChangeProperty(rootDisplay, req->requestor, req->property, XA_ATOM, 32, PropModeReplace, (unsigned char *)supportedFormats,
            sizeof(supportedFormats) / sizeof(supportedFormats[0]));
          sevent.xselection.property = req->property;
        }
        else
        {
          unsigned long overflow, nbytes;
          unsigned char *selnData;
          int selnFormat;
          int res = XGetWindowProperty(rootDisplay, DefaultRootWindow(rootDisplay), XA_CUT_BUFFER0, 0, INT_MAX / 4, False, req->target,
            &sevent.xselection.target, &selnFormat, &nbytes, &overflow, &selnData);
          if (res == Success)
          {
            if (sevent.xselection.target == req->target)
            {
              XChangeProperty(rootDisplay, req->requestor, req->property, sevent.xselection.target, selnFormat, PropModeReplace,
                selnData, nbytes);
              sevent.xselection.property = req->property;
            }
            XFree(selnData);
          }
        }
        XSendEvent(rootDisplay, req->requestor, False, 0, &sevent);
        XSync(rootDisplay, False);
        break;
      }
      case SelectionNotify: selectionReady = true; break;
    }
  }

  if (queryMousePos)
  {
    cacheCursorPosition();
    updateMousePos = true;
  }

  if (updateMousePos)
  {
    // when cursor is moved by XWarp, we have some pending events
    // with old cursor position, but XQueryPointer works correctly
    // so just update position with XQuery for some time
    if (forceQueryCursorPosition)
    {
      cacheCursorPosition();
      --forceQueryCursorPosition;
    }
    if (::dgs_app_active)
      workcycle_internal::main_window_proc(&mainWindow, GPCM_MouseMove, 0, 0);
  }
}

bool X11::getLastCursorPos(int *cx, int *cy, void *w)
{
  // only one window is currently supported
  G_UNUSED(w);
  G_ASSERT(mainWindow == (intptr_t)w);

  // setted up in MotionNotify/MotionLeave process
  *cx = lastCursorX;
  *cy = lastCursorY;

  return true;
}

void X11::setCursor(void *w, const char *cursor_name)
{
  G_UNUSED(w);
  G_ASSERT(mainWindow == (intptr_t)w);

  Cursor cursor = XcursorLibraryLoadCursor(rootDisplay, cursor_name);
  if (cursor == None)
    cursor = XcursorLibraryLoadCursor(rootDisplay, "default");
  XDefineCursor(rootDisplay, mainWindow, cursor);
}

void X11::setCursorPosition(int cx, int cy, void *w)
{
  XWarpPointer(rootDisplay, None, (intptr_t)w, 0, 0, 0, 0, cx, cy);
  // update cached cursor position directly, avoiding extra api call
  // only one window is currently supported
  G_UNUSED(w);
  G_ASSERT(mainWindow == (intptr_t)w);

  lastCursorX = cx;
  lastCursorY = cy;
  forceQueryCursorPosition += X11::CURSOR_WARP_RECOVERY_CYCLES;
}

void X11::getAbsoluteCursorPosition(int &cx, int &cy)
{
  Window unusedWindow;
  int unusedInt;
  unsigned unusedUnsigned;
  XQueryPointer(rootDisplay, mainWindow, &unusedWindow, &unusedWindow, &cx, &cy, &unusedInt, &unusedInt, &unusedUnsigned);
}

bool X11::getCursorDelta(int &, int &, void *) { return false; }

void X11::clipCursor()
{
  if (mainWindow)
  {
    XSetInputFocus(rootDisplay, mainWindow, RevertToParent, CurrentTime);
    XGrabPointer(rootDisplay, mainWindow, 0, ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ButtonMotionMask, GrabModeAsync,
      GrabModeAsync, mainWindow, None, CurrentTime);
  }
}

void X11::unclipCursor() { XUngrabPointer(rootDisplay, CurrentTime); }

void X11::hideCursor(bool hide)
{
  if (hide)
  {
    Pixmap bm_no;
    Colormap cmap;
    Cursor no_ptr;
    XColor black, dummy;
    static char bm_no_data[] = {0, 0, 0, 0, 0, 0, 0, 0};

    cmap = DefaultColormap(rootDisplay, DefaultScreen(rootDisplay));
    XAllocNamedColor(rootDisplay, cmap, "black", &black, &dummy);
    bm_no = XCreateBitmapFromData(rootDisplay, mainWindow, bm_no_data, 8, 8);
    no_ptr = XCreatePixmapCursor(rootDisplay, bm_no, bm_no, &black, &black, 0, 0);

    XDefineCursor(rootDisplay, mainWindow, no_ptr);
    XFreeCursor(rootDisplay, no_ptr);
    if (bm_no != None)
      XFreePixmap(rootDisplay, bm_no);
    XFreeColors(rootDisplay, cmap, &black.pixel, 1, 0);
  }
  else
  {
    XUndefineCursor(rootDisplay, mainWindow);
  }
}

void *X11::getNativeDisplay() { return (void *)rootDisplay; }
void *X11::getNativeWindow(void *w) { return w; }

bool X11::getClipboardUTF8Text(char *dest, int buf_size)
{
  Atom XA_CLIPBOARD = XInternAtom(rootDisplay, "CLIPBOARD", 0);
  if (XA_CLIPBOARD == None)
    return false;

  Atom format = XInternAtom(rootDisplay, "UTF8_STRING", False);
  Window owner = XGetSelectionOwner(rootDisplay, XA_CLIPBOARD);
  Atom selection;

  if ((owner == None) || (owner == mainWindow))
  {
    owner = DefaultRootWindow(rootDisplay);
    selection = XA_CUT_BUFFER0;
  }
  else
  {
    selection = XInternAtom(rootDisplay, "DAG_SEL_PROP", False);
    XConvertSelection(rootDisplay, XA_CLIPBOARD, format, selection, mainWindow, CurrentTime);
    owner = mainWindow;

    int waitStart = get_time_msec();
    selectionReady = false;
    while (!selectionReady)
    {
      processMessages();
      if ((get_time_msec() - waitStart) > 1000 && !selectionReady) // timeout
      {
        selectionReady = false;
        setClipboardUTF8Text("");
        return false;
      }
    }
  }

  Atom selnType;
  int selnFormat;
  unsigned long nbytes;
  unsigned long overflow;
  unsigned char *src;
  if (XGetWindowProperty(rootDisplay, owner, selection, 0, INT_MAX / 4, False, format, &selnType, &selnFormat, &nbytes, &overflow,
        &src) == Success)
  {
    bool result = false;
    if (selnType == format)
    {
      unsigned long ncopy = buf_size - 1;
      if (nbytes < ncopy)
        ncopy = nbytes;
      memcpy(dest, src, ncopy);
      dest[ncopy] = 0;
      result = true;
    }
    XFree(src);
    return result;
  }

  return false;
}

bool X11::setClipboardUTF8Text(const char *text)
{
  Atom XA_CLIPBOARD = XInternAtom(rootDisplay, "CLIPBOARD", 0);
  Atom format = XInternAtom(rootDisplay, "UTF8_STRING", False);
  XChangeProperty(rootDisplay, DefaultRootWindow(rootDisplay), XA_CUT_BUFFER0, format, 8, PropModeReplace, (const unsigned char *)text,
    strlen(text));
  if (XGetSelectionOwner(rootDisplay, XA_CLIPBOARD) != mainWindow)
    XSetSelectionOwner(rootDisplay, XA_CLIPBOARD, mainWindow, CurrentTime);
  if (XGetSelectionOwner(rootDisplay, XA_PRIMARY) != mainWindow)
    XSetSelectionOwner(rootDisplay, XA_PRIMARY, mainWindow, CurrentTime);
  return true;
}
