#pragma once

#include "../c_window_controls.h"

const char IMAGE_LIST_COUNT = 3;


class WToolbar : public WindowControlBase
{
public:
  WToolbar(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, const char *caption);

  virtual ~WToolbar();

  intptr_t onControlCommand(unsigned notify_code, unsigned elem_id);

  void addButton(int id, const char *caption);
  void addCheck(int id, const char *caption);
  void addCheckGroup(int id, const char *caption);
  void addSep(int id);

  bool getItemCheck(int id);
  bool setItemEnabled(int id, bool enabled);
  bool setItemCheck(int id, bool value);


  void setItemText(int id, const char *caption);
  void setItemPictures(int id, const char *fname);

  void clear();

  unsigned getLastClickID();

protected:
  int prepareText(int id, const char *caption);
  int prepareImages(int id, const char *fname);
  int addTB(int id, int string_id, int bitmap_id, unsigned char style);

private:
  unsigned lastClickID;
  void *mHImageList[IMAGE_LIST_COUNT];
};
