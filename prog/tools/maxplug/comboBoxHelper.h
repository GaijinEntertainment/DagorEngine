#pragma once
#include <windowsx.h>

class ComboBoxHelper
{
public:
  static void AddItemWithData(HWND hwnd, const TCHAR *title, LPARAM data)
  {
    const int index = ComboBox_AddString(hwnd, title);
    ComboBox_SetItemData(hwnd, index, data);
  }

  static int GetItemIndexByData(HWND hwnd, LPARAM data)
  {
    const int itemCount = ComboBox_GetCount(hwnd);
    for (int i = 0; i < itemCount; ++i)
      if (ComboBox_GetItemData(hwnd, i) == data)
        return i;

    return -1;
  }

  static LPARAM GetSelectedItemData(HWND hwnd, LPARAM default_value)
  {
    const int index = ComboBox_GetCurSel(hwnd);
    return index >= 0 ? ComboBox_GetItemData(hwnd, index) : default_value;
  }
};
