// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_clipboard.h>
#include <debug/dag_debug.h>

#if USE_X11

#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_progGlobals.h>
#include "../workCycle/workCyclePriv.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <string.h>
#include <limits.h>

namespace clipboard_private
{

static bool selection_ready = false;

void set_selection_ready() { selection_ready = true; }

} // namespace clipboard_private

namespace clipboard
{

using namespace clipboard_private;

bool get_clipboard_ansi_text(char *, int) { return false; }
bool set_clipboard_ansi_text(const char *) { return false; }

bool get_clipboard_utf8_text(char *dest, int buf_size)
{
  Display *display = workcycle_internal::get_root_display();
  Atom XA_CLIPBOARD = XInternAtom(display, "CLIPBOARD", 0);
  if (XA_CLIPBOARD == None)
    return false;

  Window window = *(Window *)win32_get_main_wnd();
  Atom format = XInternAtom(display, "UTF8_STRING", False);
  Window owner = XGetSelectionOwner(display, XA_CLIPBOARD);
  Atom selection;

  if ((owner == None) || (owner == window))
  {
    owner = DefaultRootWindow(display);
    selection = XA_CUT_BUFFER0;
  }
  else
  {
    selection = XInternAtom(display, "DAG_SEL_PROP", False);
    XConvertSelection(display, XA_CLIPBOARD, format, selection, window, CurrentTime);
    owner = window;

    int waitStart = get_time_msec();
    selection_ready = false;
    while (!selection_ready)
    {
      workcycle_internal::idle_loop();
      if ((get_time_msec() - waitStart) > 1000 && !selection_ready) // timeout
      {
        selection_ready = false;
        set_clipboard_utf8_text("");
        return false;
      }
    }
  }

  Atom selnType;
  int selnFormat;
  unsigned long nbytes;
  unsigned long overflow;
  unsigned char *src;
  if (XGetWindowProperty(display, owner, selection, 0, INT_MAX / 4, False, format, &selnType, &selnFormat, &nbytes, &overflow, &src) ==
      Success)
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
    XDeleteProperty(display, owner, selection);
    XFlush(display);
    return result;
  }

  return false;
}

bool set_clipboard_utf8_text(const char *text)
{
  Display *display = workcycle_internal::get_root_display();
  Atom XA_CLIPBOARD = XInternAtom(display, "CLIPBOARD", 0);
  Window window = *(Window *)win32_get_main_wnd();
  Atom format = XInternAtom(display, "UTF8_STRING", False);
  XChangeProperty(display, DefaultRootWindow(display), XA_CUT_BUFFER0, format, 8, PropModeReplace, (const unsigned char *)text,
    strlen(text));
  if (XGetSelectionOwner(display, XA_CLIPBOARD) != window)
    XSetSelectionOwner(display, XA_CLIPBOARD, window, CurrentTime);
  if (XGetSelectionOwner(display, XA_PRIMARY) != window)
    XSetSelectionOwner(display, XA_PRIMARY, window, CurrentTime);
  return true;
}

bool set_clipboard_bmp_image(TexPixel32 * /*im*/, int /*wd*/, int /*ht*/, int /*stride*/)
{
  // no implementation yet
  return false;
}


} // namespace clipboard

#else
namespace clipboard
{
bool get_clipboard_ansi_text(char *, int) { return false; }
bool set_clipboard_ansi_text(const char *) { return false; }
bool get_clipboard_utf8_text(char *, int) { return false; }
bool set_clipboard_utf8_text(const char *) { return false; }
bool set_clipboard_bmp_image(TexPixel32 * /*im*/, int /*wd*/, int /*ht*/, int /*stride*/) { return false; }
} // namespace clipboard
namespace clipboard_private
{
void set_selection_ready() {}
} // namespace clipboard_private
#endif

#define EXPORT_PULL dll_pull_osapiwrappers_clipboard
#include <supp/exportPull.h>
