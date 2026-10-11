// User-mode sampling profiler: suspends the main thread of a child process at a
// fixed interval and records its RIP as an image-relative RVA.
// usage: sprof <out.txt> <interval_us> <target_cpu> <sampler_cpu> <exe> [args...]
// The child runs in the current directory. Output: "rva count" lines, sorted by rva.
#include <windows.h>
#include <winternl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

typedef NTSTATUS(NTAPI *NtQIP)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

static std::wstring quote(const wchar_t *s)
{
  std::wstring r = L"\"";
  r += s;
  r += L"\"";
  return r;
}

int wmain(int argc, wchar_t **argv)
{
  if (argc < 6)
  {
    fwprintf(stderr, L"usage: sprof <out.txt> <interval_us> <target_cpu> <sampler_cpu> <exe> [args...]\n");
    return 2;
  }
  const wchar_t *outPath = argv[1];
  double intervalUs = _wtof(argv[2]);
  int targetCpu = _wtoi(argv[3]);
  int samplerCpu = _wtoi(argv[4]);
  std::wstring cmd = quote(argv[5]);
  for (int i = 6; i < argc; i++)
    cmd += L" " + quote(argv[i]);

  SetThreadAffinityMask(GetCurrentThread(), DWORD_PTR(1) << samplerCpu);
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

  STARTUPINFOW si = {sizeof(si)};
  PROCESS_INFORMATION pi = {};
  std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
  cmdBuf.push_back(0);
  if (!CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, TRUE, CREATE_SUSPENDED, nullptr, nullptr, &si, &pi))
  {
    fwprintf(stderr, L"CreateProcess failed %lu\n", GetLastError());
    return 1;
  }
  SetProcessAffinityMask(pi.hProcess, DWORD_PTR(1) << targetCpu);

  NtQIP qip = (NtQIP)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess");
  PROCESS_BASIC_INFORMATION pbi = {};
  qip(pi.hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), nullptr);
  ULONG_PTR imageBase = 0;
  ReadProcessMemory(pi.hProcess, (char *)pbi.PebBaseAddress + 0x10, &imageBase, sizeof(imageBase), nullptr);

  LARGE_INTEGER freq, t0, now;
  QueryPerformanceFrequency(&freq);
  const LONGLONG step = LONGLONG(intervalUs * freq.QuadPart / 1e6);

  std::unordered_map<ULONG_PTR, unsigned> hist;
  unsigned total = 0, outside = 0;
  ResumeThread(pi.hThread);
  QueryPerformanceCounter(&t0);
  LONGLONG next = t0.QuadPart + step;
  CONTEXT ctx;
  for (;;)
  {
    do
      QueryPerformanceCounter(&now);
    while (now.QuadPart < next);
    next += step;
    if (WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0)
      break;
    if (SuspendThread(pi.hThread) == (DWORD)-1)
      break;
    ctx.ContextFlags = CONTEXT_CONTROL;
    if (GetThreadContext(pi.hThread, &ctx))
    {
      ULONG_PTR rva = ctx.Rip - imageBase;
      if (ctx.Rip < imageBase || rva >= 0x10000000)
        outside++;
      else
        hist[rva]++;
      total++;
    }
    ResumeThread(pi.hThread);
  }
  DWORD code = 0;
  GetExitCodeProcess(pi.hProcess, &code);

  std::vector<std::pair<ULONG_PTR, unsigned>> v(hist.begin(), hist.end());
  std::sort(v.begin(), v.end());
  FILE *f = _wfopen(outPath, L"w");
  fprintf(f, "# samples %u outside %u exit %lu\n", total, outside, code);
  for (auto &e : v)
    fprintf(f, "%llx %u\n", (unsigned long long)e.first, e.second);
  fclose(f);
  fwprintf(stderr, L"samples %u (outside image %u), exit %lu\n", total, outside, code);
  return 0;
}
