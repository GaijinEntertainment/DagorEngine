#pragma once

#include "c_window_base.h"
#include "c_constants.h"

#include <propPanel2/c_window_event_handler.h>
#include <propPanel2/c_common.h>
#include <util/dag_string.h>
#include <math/dag_Point2.h>
#include <generic/dag_tab.h>
#include <debug/dag_debug.h>


void *load_bmp_picture(const char *lpwszFileName, unsigned final_w, unsigned final_h);
void *clone_bmp_picture(void *image);
unsigned get_alpha_color(void *hbitmap);

class WindowControlBase : public WindowBase
{
public:
  WindowControlBase(WindowControlEventHandler *event_handler, WindowBase *parent, const char class_name[], unsigned long ex_style,
    unsigned long style, const char caption[], int x, int y, int w, int h);

  virtual intptr_t onResize(unsigned new_width, unsigned new_height);
  virtual intptr_t onKeyDown(unsigned v_key, int flags) override;
  virtual intptr_t onClipboardCopy(DataBlock &blk);
  virtual intptr_t onClipboardPaste(const DataBlock &blk);
  virtual intptr_t onDropFiles(const dag::Vector<String> &files) override;

protected:
  WindowControlEventHandler *mEventHandler;
};
