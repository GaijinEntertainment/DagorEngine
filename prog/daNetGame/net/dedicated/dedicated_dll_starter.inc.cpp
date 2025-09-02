#if _TARGET_PC_WIN
#include <windows.h>
#include <osApiWrappers/dag_progGlobals.h>

extern int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR /*lpCmdLine*/, int nCmdShow);
extern "C" __declspec(dllexport) bool __cdecl start_dedicated_server()
{
  return WinMain((HINSTANCE)win32_get_instance(), NULL, nullptr, 0) == 0;
}

#elif _TARGET_PC_LINUX || _TARGET_PC_MACOSX || _TARGET_C1 || _TARGET_C2

extern int main(int argc, char *argv[]);

extern "C" __attribute__((visibility("default"))) bool __cdecl start_dedicated_server() { return main(0, nullptr) == 0; }

#endif
