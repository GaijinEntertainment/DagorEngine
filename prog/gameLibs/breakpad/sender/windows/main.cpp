#include "../sender.h"

#include <vector>

#include <windows.h>
#include <shellapi.h>


int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, PWSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
  int argc = 0;
  wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), &argc);

  std::vector<char> storage(8192, '0');
  std::vector<char *> argv;
  argv.reserve(argc);

  char *b = storage.data();
  for (int i = 0, left = storage.size() - 1; left > 0 && i < argc; ++i)
  {
    int l = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, b, left, NULL, NULL);
    if (l > 0)
    {
      argv.push_back(b);
      l++;
      left -= l;
      b += l;
    }
  }

  return breakpad::process_report(argv.size(), argv.data());
}
