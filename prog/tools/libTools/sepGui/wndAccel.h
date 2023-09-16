#pragma once

#include <generic/dag_tab.h>
#include <sepGui/wndCommon.h>

class WinManager;


struct AccelRecord
{
  unsigned cmd_id;
  unsigned v_key;
  bool ctrl;
  bool alt;
  bool shift;
  bool down;
};

class WinAccel
{
public:
  WinAccel(WinManager *manager);
  ~WinAccel();

  void addAccelerator(unsigned cmd_id, unsigned v_key, bool ctrl, bool alt, bool shift, bool down);
  void clear();

  long procAccel(int code, TSgWParam w_param, TSgLParam l_param);

private:
  WinManager *mManager;
  Tab<AccelRecord> mAccelArray;
  void *hhandle;
};
