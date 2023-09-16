#pragma once

#include "../c_window_controls.h"


class WTabPanel : public WindowControlBase
{
public:
  WTabPanel(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h);

  void addPage(int id, const char caption[]);
  void insertPage(int id, int id_after, const char caption[]);
  void deletePage(int id);
  void deleteAllPages();
  void setCurrentPage(int id);
  void setPageCaption(int id, const char caption[]);
  int getCurrentPageId() const;
  int getPageCount() const;

  virtual intptr_t onControlNotify(void *info);
  virtual intptr_t onControlDrawItem(void *info);

private:
  void insertPageByIndex(int id, int index, const char caption[]);
  int getIndexById(int id) const;
  int getIdByIndex(int index) const;
};
