#include <stdio.h>
#include <process.h>
#include <windows.h>

int wmain(int argc, wchar_t **argv)
{
  enum { MAX_ARG = 256 };
  wchar_t *new_argv[MAX_ARG];
  wchar_t exe_nm[512] = L"";

  if (argc+4 > MAX_ARG)
  {
    printf("ERR: too many args (%d)\n", argc);
    return 1;
  }

  // derive start path
  GetModuleFileNameW(GetModuleHandle(NULL), exe_nm, 512);
  if (wchar_t *p = wcsrchr(exe_nm, '\\'))
    p[1] = 0;
  wcscat(exe_nm, L"astcenc-impl.exe");

  new_argv[0] = exe_nm;
  for (int i = 1; i < argc; i ++)
    new_argv[i] = argv[i];
  if (argc > 1)
  {
    new_argv[argc] = L"-silentmode";
    new_argv[argc+1] = L"-j";
    new_argv[argc+2] = L"1";
    new_argv[argc+3] = nullptr;
  }
  else
    new_argv[argc] = nullptr;

  if (0)
  {
    wprintf(L"%s -> exec (%s", argv[0], new_argv[0]);
    for (int i = 1; new_argv[i]; i ++)
      wprintf(L", %s", new_argv[i]);
    printf(")\n");
  }
  return _wspawnvp(_P_WAIT, new_argv[0], new_argv);
}
