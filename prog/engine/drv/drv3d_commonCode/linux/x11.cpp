// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if !USE_X11
#error x11.cpp requires -DUSE_X11
#endif
#include "x11.h"

#include "../../../workCycle/workCyclePriv.h"
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <osApiWrappers/dag_unicode.h>
#include <workCycle/dag_genGuiMgr.h>
#include <drv/3d/dag_driver.h>
#include <limits.h>
#include "../../../osApiWrappers/linux/linuxClipboardPriv.h"

X11 x11;

void cache_cursor_position()
{
  Window root, child;
  int root_x, root_y;
  int win_x, win_y;
  unsigned int mask_return;

  bool result = XQueryPointer(x11.rootDisplay, x11.mainWindow, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask_return);

  x11.lastCursorX = result ? win_x : root_x;
  x11.lastCursorY = result ? win_y : root_y;
}

void workcycle_internal::idle_loop()
{
  if (!x11.rootDisplay)
    return;

  static const int LBUTTON_BIT = 0;
  static const int RBUTTON_BIT = 1;
  static const int MBUTTON_BIT = 2;
  bool updateMousePos = false;
  static bool queryMousePos = false;

  XEvent xev;
  while (XPending(x11.rootDisplay) > 0)
  {
    // Get an event for the queue
    // Do not use XFilterEvent because it might
    // be a reason for input lags
    XNextEvent(x11.rootDisplay, &xev);

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
      case ConfigureNotify: x11.cacheWindowAttribs(); break;
      case ButtonPress:
        if (wheelDelta != 0)
          workcycle_internal::main_window_proc(&x11.mainWindow, GPCM_MouseWheel, wheelDelta * 100, 0);
        else
          workcycle_internal::main_window_proc(&x11.mainWindow, GPCM_MouseBtnPress, button, 0);
        break;

      case ButtonRelease: workcycle_internal::main_window_proc(&x11.mainWindow, GPCM_MouseBtnRelease, button, 0); break;

      case LeaveNotify:
        cache_cursor_position();
        updateMousePos = true;
        queryMousePos = true;
        break;
      case MotionNotify:
        if (xev.xmotion.window == x11.mainWindow)
        {
          x11.lastCursorX = xev.xmotion.x;
          x11.lastCursorY = xev.xmotion.y;
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
        int symb = Xutf8LookupString(x11.ic, &xev.xkey, buf, sizeof(buf), &keysym, &status);
        if (symb > 0)
        {
          Tab<wchar_t> tmpBuf(framemem_ptr());
          wchar_t *key_ptr = convert_utf8_to_u16_buf(tmpBuf, buf, symb);
          if (key_ptr)
            keychar = key_ptr[0];
        }
        workcycle_internal::main_window_proc(&x11.mainWindow, GPCM_KeyPress, xev.xkey.keycode, keychar);
        break;
      }

      case KeyRelease:
      {
        bool skipKeyRelease = false;
        if (XEventsQueued(x11.rootDisplay, QueuedAfterReading))
        {
          XEvent nev;
          XPeekEvent(x11.rootDisplay, &nev);
          if (nev.type == KeyPress && nev.xkey.time == xev.xkey.time && nev.xkey.keycode == xev.xkey.keycode)
          {
            skipKeyRelease = true;
          }
        }
        if (!skipKeyRelease)
          workcycle_internal::main_window_proc(&x11.mainWindow, GPCM_KeyRelease, xev.xkey.keycode, 0);
      }
      break;

      case KeymapNotify:
      case MappingNotify: XRefreshKeyboardMapping(&xev.xmapping); break;

      case FocusIn:
        if (xev.xfocus.mode == NotifyGrab || xev.xfocus.mode == NotifyUngrab)
          break;
        if (xev.xfocus.detail == NotifyInferior)
          break;

        XSetICFocus(x11.ic);
        workcycle_internal::main_window_proc(&x11.mainWindow, GPCM_Activate, GPCMP1_Activate, 0);
        break;

      case FocusOut:
        if (xev.xfocus.mode == NotifyGrab || xev.xfocus.mode == NotifyUngrab)
          break;
        if (xev.xfocus.detail == NotifyInferior)
          break;

        XUnsetICFocus(x11.ic);
        if (dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE)
        {
          XIconifyWindow(x11.rootDisplay, x11.mainWindow, x11.rootScreenIndex);
        }
        workcycle_internal::main_window_proc(&x11.mainWindow, GPCM_Activate, GPCMP1_Inactivate, 0);
        break;

      case ClientMessage:
        if (!x11.isDeleteMessage(xev.xclient.data.l[0]))
        {
          if (x11.isPingMessage(xev.xclient.data.l[0]))
          {
            xev.xclient.window = x11.rootWindow;
            XSendEvent(x11.rootDisplay, x11.rootWindow, False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
          }
          break;
        }
        else
        {
          if (dagor_gui_manager && !dagor_gui_manager->canCloseNow())
            break;
        }
      case DestroyNotify:
        debug("main window close signal received!");
        win32_set_main_wnd(NULL);
        d3d::window_destroyed(&x11.mainWindow);
        quit_game(0);
        break;
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

        Atom XA_TARGETS = XInternAtom(x11.rootDisplay, "TARGETS", False);
        if (req->target == XA_TARGETS)
        {
          Atom supportedFormats[] = {XA_TARGETS, XInternAtom(x11.rootDisplay, "UTF8_STRING", False)};
          XChangeProperty(x11.rootDisplay, req->requestor, req->property, XA_ATOM, 32, PropModeReplace,
            (unsigned char *)supportedFormats, sizeof(supportedFormats) / sizeof(supportedFormats[0]));
          sevent.xselection.property = req->property;
        }
        else
        {
          unsigned long overflow, nbytes;
          unsigned char *selnData;
          int selnFormat;
          int res = XGetWindowProperty(x11.rootDisplay, DefaultRootWindow(x11.rootDisplay), XA_CUT_BUFFER0, 0, INT_MAX / 4, False,
            req->target, &sevent.xselection.target, &selnFormat, &nbytes, &overflow, &selnData);
          if (res == Success)
          {
            if (sevent.xselection.target == req->target)
            {
              XChangeProperty(x11.rootDisplay, req->requestor, req->property, sevent.xselection.target, selnFormat, PropModeReplace,
                selnData, nbytes);
              sevent.xselection.property = req->property;
            }
            XFree(selnData);
          }
        }
        XSendEvent(x11.rootDisplay, req->requestor, False, 0, &sevent);
        XSync(x11.rootDisplay, False);
        break;
      }
      case SelectionNotify: clipboard_private::set_selection_ready(); break;
    }
  }

  if (queryMousePos)
  {
    cache_cursor_position();
    updateMousePos = true;
  }

  if (updateMousePos)
  {
    // when cursor is moved by XWarp, we have some pending events
    // with old cursor position, but XQueryPointer works correctly
    // so just update position with XQuery for some time
    if (x11.forceQueryCursorPosition)
    {
      cache_cursor_position();
      --x11.forceQueryCursorPosition;
    }
    if (::dgs_app_active)
      workcycle_internal::main_window_proc(&x11.mainWindow, GPCM_MouseMove, 0, 0);
  }
}

bool workcycle_internal::get_last_cursor_pos(int *cx, int *cy, Window w)
{
  // only one window is currently suported
  G_UNUSED(w);
  G_ASSERT(x11.mainWindow == w);

  // setted up in MotionNotify/MotionLeave process
  *cx = x11.lastCursorX;
  *cy = x11.lastCursorY;

  return true;
}

void workcycle_internal::update_cursor_position(int cx, int cy, Window w)
{
  // only one window is currently suported
  G_UNUSED(w);
  G_ASSERT(x11.mainWindow == w);

  x11.lastCursorX = cx;
  x11.lastCursorY = cy;
  x11.forceQueryCursorPosition += X11::CURSOR_WARP_RECOVERY_CYCLES;
}

const XWindowAttributes &workcycle_internal::get_window_attrib(Window w, bool translated)
{
  if (w)
  {
    G_ASSERT(w == x11.mainWindow);
    return x11.windowAttribs[translated ? X11::WND_TRANSLATED_MAIN : X11::WND_MAIN];
  }
  else
    return x11.windowAttribs[X11::WND_ROOT];
}

Display *workcycle_internal::get_root_display() { return x11.rootDisplay; }


void workcycle_internal::set_title(const char *title, bool utf8)
{
  if (utf8)
    x11.setTitleUtf8(title);
  else
    x11.setTitle(title);
}


void workcycle_internal::set_title_tooltip(const char *title, const char *tooltip, bool utf8)
{
  if (utf8)
    x11.setTitleUtf8(title, tooltip);
  else
    x11.setTitle(title, tooltip);
}
