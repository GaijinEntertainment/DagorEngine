// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fileDropHandler.h"
#include <osApiWrappers/dag_wndProcComponent.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <sqrat/sqratArray.h>
#include <util/dag_string.h>

#if _TARGET_PC_WIN
#include <windows.h>
#include <shellapi.h>
#endif

namespace bindquirrel
{

#if _TARGET_PC_WIN

static Sqrat::Function file_drop_handler;

class FileDropHandlerWndProcComponent : public IWndProcComponent
{
  virtual RetCode process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result) override
  {
    G_UNUSED(hwnd);
    G_UNUSED(lParam);
    G_UNUSED(result);

    if (msg == WM_DROPFILES)
      return handleFileDrop(wParam);

    return PROCEED_OTHER_COMPONENTS;
  }

  RetCode handleFileDrop(uintptr_t wParam)
  {
    if (file_drop_handler.IsNull())
      return PROCEED_OTHER_COMPONENTS;

    HDROP hdrop = (HDROP)wParam;
    const int fileCount = DragQueryFileA(hdrop, -1, nullptr, 0);
    if (fileCount <= 0)
      return PROCEED_OTHER_COMPONENTS;

    HSQUIRRELVM vm = file_drop_handler.GetVM();
    Sqrat::Array files(vm);

    for (int i = 0; i < fileCount; ++i)
    {
      const int lengthWithoutNullTerminator = DragQueryFileA(hdrop, 0, nullptr, 0);
      if (lengthWithoutNullTerminator > 0)
      {
        String path;
        path.resize(lengthWithoutNullTerminator + 1);
        if (DragQueryFileA(hdrop, 0, path.begin(), lengthWithoutNullTerminator + 1))
          files.Append(path.c_str());
      }
    }

    file_drop_handler(files);

    return IMMEDIATE_RETURN;
  }
};

static FileDropHandlerWndProcComponent file_drop_handler_wnd_proc_component;

void init_file_drop_handler(Sqrat::Function func)
{
  G_ASSERT(file_drop_handler.IsNull());

  DragAcceptFiles((HWND)win32_get_main_wnd(), TRUE);
  add_wnd_proc_component(&file_drop_handler_wnd_proc_component);
  file_drop_handler = func;
}

void release_file_drop_handler()
{
  file_drop_handler.Release();
  del_wnd_proc_component(&file_drop_handler_wnd_proc_component);
}

#else

void init_file_drop_handler(Sqrat::Function func) { G_UNUSED(func); }
void release_file_drop_handler() {}

#endif

} // namespace bindquirrel