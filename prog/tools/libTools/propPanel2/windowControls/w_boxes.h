#pragma once

#include "../c_window_controls.h"

class IMenu;

class WComboBox : public WindowControlBase
{
public:
  WComboBox(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, const Tab<String> &vals,
    int index, bool sorted);

  int getSelectedIndex() const { return mSelectIndex; }
  int getSelectedText(char *buffer, int buflen) const;
  int getSelectedItemData() const;
  void setSelectedIndex(int index);
  void setSelectedText(const char buffer[]);
  void setSelectedByItemData(int data);
  void setStrings(const Tab<String> &vals, bool set_item_data = false);
  virtual unsigned getHeight();

  virtual intptr_t onControlCommand(unsigned notify_code, unsigned elem_id);

private:
  int mSelectIndex;
};


class WListBox : public WindowControlBase
{
public:
  WListBox(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, const Tab<String> &vals,
    int index, bool allow_popup_menu = false);
  ~WListBox();

  int getSelectedIndex() const { return mSelectIndex; }
  int getSelectedText(char *buffer, int buflen) const;
  void setSelectedIndex(int index);
  void setStrings(const Tab<String> &vals);
  void renameSelected(const char *name);
  int addString(const char *value);
  void removeString(int idx);

  virtual intptr_t onControlCommand(unsigned notify_code, unsigned elem_id);
  virtual intptr_t onLButtonDClick(long x, long y);
  virtual intptr_t onLButtonUp(long x, long y);
  virtual intptr_t onRButtonUp(long x, long y) override;

  void setPopupMenuShow(bool is_show) { showPopupMenu = is_show; }
  IMenu *getPopupMenu() const { return menu; }

private:
  int mSelectIndex;
  IMenu *menu;
  bool showPopupMenu;
  bool dcEndWait;
};


class WMultiSelListBox : public WindowControlBase
{
public:
  WMultiSelListBox(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, const Tab<String> &vals);

  int getCount() const;
  void setFilter(const char value[]);

  int getStrings(Tab<String> &vals);
  void setStrings(const Tab<String> &vals);

  int getSelection(Tab<int> &sels);
  void setSelection(const Tab<int> &sels);

  virtual intptr_t onControlCommand(unsigned notify_code, unsigned elem_id);
  virtual intptr_t onLButtonDClick(long x, long y);
  virtual intptr_t onLButtonUp(long x, long y);

private:
  int updateSelection();

  Tab<int> mSelIndexes;
  Tab<String> mLines;
  bool dcEndWait;
};
