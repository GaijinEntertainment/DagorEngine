#include "fileDropHandler.h"
#include "main.h"
#include <daRg/dag_guiScene.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <sqrat/sqratArray.h>
#include <sqrat/sqratFunction.h>

#if _TARGET_PC_WIN
#include <windows.h>
#include <shellapi.h>
#endif

static Sqrat::Function file_drop_handler;

class FileDropHandlerWndProcComponent : public IWndProcComponent
{
  virtual RetCode process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result) override
  {
#if _TARGET_PC_WIN
    if (msg == WM_DROPFILES)
      return handleFileDrop(wParam);
#endif

    return PROCEED_OTHER_COMPONENTS;
  }

  RetCode handleFileDrop(uintptr_t wParam)
  {
#if _TARGET_PC_WIN
    darg::IGuiScene *guiScene = get_ui_scene();
    if (!guiScene)
      return PROCEED_OTHER_COMPONENTS;

    if (file_drop_handler.IsNull())
      return PROCEED_OTHER_COMPONENTS;

    HDROP hdrop = (HDROP)wParam;
    const int fileCount = DragQueryFileA(hdrop, -1, nullptr, 0);
    if (fileCount <= 0)
      return PROCEED_OTHER_COMPONENTS;

    HSQUIRRELVM vm = guiScene->getScriptVM();
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
#else
    return PROCEED_OTHER_COMPONENTS;
#endif
  }
};

static FileDropHandlerWndProcComponent file_drop_handler_wnd_proc_component;

void init_file_drop_handler(Sqrat::Object func)
{
#if _TARGET_PC_WIN
  G_ASSERT(file_drop_handler.IsNull());

  DragAcceptFiles((HWND)win32_get_main_wnd(), TRUE);
  add_wnd_proc_component(&file_drop_handler_wnd_proc_component);
  file_drop_handler = Sqrat::Function(func.GetVM(), Sqrat::Object(func.GetVM()), func);
#endif
}

void release_file_drop_handler()
{
  file_drop_handler.Release();
  del_wnd_proc_component(&file_drop_handler_wnd_proc_component);
}
