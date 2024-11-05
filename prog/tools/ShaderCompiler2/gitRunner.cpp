// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gitRunner.h"
#include <util/dag_simpleString.h>
#include <debug/dag_log.h>
#include <EASTL/fixed_string.h>
#include <EASTL/utility.h>
#include <EASTL/optional.h>

#define CHECK_RET(error, ret, msg, ...) \
  if (error)                            \
  {                                     \
    logerr(msg, __VA_ARGS__);           \
    return ret;                         \
  }

static constexpr int OUTPUT_SIZE = 1024;
static constexpr int PATH_RESERVE_SIZE = 128;

#if _TARGET_PC_WIN
#include <windows.h>
static constexpr int PIPE_SIZE = OUTPUT_SIZE;
static constexpr int PIPE_MAX_READ_TRIES = 3;
static constexpr int CHILD_PROCESS_TIMEOUT = 3000; // ms

static eastl::optional<PROCESS_INFORMATION> create_child_process(const char *cmd, HANDLE &hInReadPipe, HANDLE &hOutWritePipe)
{
  PROCESS_INFORMATION piProcInfo;
  STARTUPINFO siStartInfo;

  ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
  ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
  siStartInfo.cb = sizeof(STARTUPINFO);
  siStartInfo.hStdError = hOutWritePipe;
  siStartInfo.hStdOutput = hOutWritePipe;
  siStartInfo.hStdInput = hInReadPipe;
  siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

  const auto bSuccess = CreateProcess(NULL, (char *)cmd, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);
  if (!bSuccess)
    return eastl::nullopt;

  return piProcInfo;
}

static int read_from_pipe(HANDLE hOutReadPipe, eastl::fixed_string<char, OUTPUT_SIZE> &output)
{
  DWORD dwRead;
  CHAR chBuf[PIPE_SIZE];
  BOOL bSuccess = FALSE;
  int tries = 0;
  for (; tries < PIPE_MAX_READ_TRIES; ++tries)
  {
    bSuccess = ReadFile(hOutReadPipe, chBuf, PIPE_SIZE, &dwRead, NULL);
    if (!bSuccess || dwRead == 0)
      break;
    chBuf[dwRead] = '\0';
    output.append_sprintf("%s", chBuf);
  }
  if (tries == PIPE_MAX_READ_TRIES)
  {
    logerr("Too large output in stdout pipe");
    return 0;
  }
  return 1;
}

static void wait_pipe_data(const PROCESS_INFORMATION &piProcInfo, HANDLE hOutReadPipe)
{
  DWORD bytesAvail = 0;
  int tries = 0;
  while (!bytesAvail && tries++ < PIPE_MAX_READ_TRIES)
  {
    if (WaitForSingleObject(piProcInfo.hProcess, CHILD_PROCESS_TIMEOUT) != WAIT_OBJECT_0)
      PeekNamedPipe(hOutReadPipe, NULL, 0, NULL, &bytesAvail, NULL);
    else
      break;
  }
}

static int wait_child_process_to_finish(const PROCESS_INFORMATION &piProcInfo)
{
  if (WaitForSingleObject(piProcInfo.hProcess, CHILD_PROCESS_TIMEOUT) != WAIT_OBJECT_0)
    logerr("Failed to wait child process to finish");

  CloseHandle(piProcInfo.hThread);

  unsigned long ret{};
  const auto bStatus = GetExitCodeProcess(piProcInfo.hProcess, &ret);

  CloseHandle(piProcInfo.hProcess);
  return bStatus ? ret : 0;
}

class PipeHandle
{
public:
  PipeHandle() {}
  ~PipeHandle()
  {
    if (shouldCloseRead)
      CloseHandle(readHandle);
    if (shouldCloseWrite)
      CloseHandle(writeHandle);
  }

  HANDLE &getReadHandle() { return readHandle; }
  HANDLE &getWriteHandle() { return writeHandle; }
  void setAutoClose() { shouldCloseRead = shouldCloseWrite = true; };
  void closeRead()
  {
    if (eastl::exchange(shouldCloseRead, false))
      CloseHandle(readHandle);
  }
  void closeWrite()
  {
    if (eastl::exchange(shouldCloseWrite, false))
      CloseHandle(writeHandle);
  }

private:
  HANDLE readHandle;
  HANDLE writeHandle;
  bool shouldCloseRead = false;
  bool shouldCloseWrite = false;
};

static int execute_cmd(const char *cmd, eastl::fixed_string<char, OUTPUT_SIZE> &output)
{
  PipeHandle hInPipe, hOutPipe;
  SECURITY_ATTRIBUTES saAttr;
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = NULL;

  CHECK_RET(!CreatePipe(&hOutPipe.getReadHandle(), &hOutPipe.getWriteHandle(), &saAttr, PIPE_SIZE), 0,
    "Failed to create stdout pipe for cmd %s", cmd);
  hOutPipe.setAutoClose();
  CHECK_RET(!SetHandleInformation(hOutPipe.getReadHandle(), HANDLE_FLAG_INHERIT, 0), 0,
    "Failed to set stdout handle information for cmd %s", cmd);
  CHECK_RET(!CreatePipe(&hInPipe.getReadHandle(), &hInPipe.getWriteHandle(), &saAttr, PIPE_SIZE), 0,
    "Failed to create stdin pipe for cmd %s", cmd);
  hInPipe.setAutoClose();
  CHECK_RET(!SetHandleInformation(hInPipe.getWriteHandle(), HANDLE_FLAG_INHERIT, 0), 0,
    "Failed to set stdin handle information for cmd %s", cmd);
  const auto procInfo = create_child_process(cmd, hInPipe.getReadHandle(), hOutPipe.getWriteHandle());
  CHECK_RET(!procInfo.has_value(), 0, "Failed to create child process for cmd %s", cmd);

  wait_pipe_data(procInfo.value(), hOutPipe.getReadHandle());
  hOutPipe.closeWrite(); // Need to close write handle before reading from pipe
  CHECK_RET(!read_from_pipe(hOutPipe.getReadHandle(), output), 0, "Failed to read from stdout pipe for cmd %s", cmd);
  const auto ret = wait_child_process_to_finish(procInfo.value());
  return ret;
}
#elif _TARGET_PC_LINUX | _TARGET_PC_MACOSX
#include <stdio.h>

#if _TARGET_PC_MACOSX
#include <AvailabilityMacros.h>
#elif _TARGET_PC_LINUX
#include <sys/wait.h>
#endif

static constexpr int PIPE_SIZE = OUTPUT_SIZE;

static int execute_cmd(const char *cmd, eastl::fixed_string<char, OUTPUT_SIZE> &output)
{
  FILE *pipe = popen(cmd, "r");

  CHECK_RET(!pipe, 0, "Failed to open pipe for command %s", cmd);
  char buffer[PIPE_SIZE];

  while (fgets(buffer, PIPE_SIZE, pipe) != nullptr)
    output.append_sprintf("%s", buffer);

  int ret = pclose(pipe);
  return WEXITSTATUS(ret);
}
#else
static int execute_cmd(const char *cmd, eastl::fixed_string<char, OUTPUT_SIZE> &output) { return 0; }
#endif

bool check_modified_files(dag::ConstSpan<SimpleString> file_paths)
{
  eastl::string gitDiffCmd{};
  gitDiffCmd.reserve(PATH_RESERVE_SIZE * file_paths.size());
  gitDiffCmd.sprintf("git diff --exit-code --quiet ");

  for (const auto &filePath : file_paths)
    gitDiffCmd.append_sprintf("%s ", filePath.c_str());

  eastl::fixed_string<char, OUTPUT_SIZE> output{};
  const auto ret = execute_cmd(gitDiffCmd.c_str(), output);
  if (ret != 0)
    return true;
  output.clear();
  execute_cmd("git rev-list --count @{u}..HEAD", output);
  return output.empty() || output[0] != '0';
}

int64_t get_git_timestamp(dag::ConstSpan<SimpleString> file_paths)
{
  eastl::string gitLogCmd{};
  gitLogCmd.reserve(PATH_RESERVE_SIZE * file_paths.size());
  gitLogCmd.sprintf("git log -1 --format=%%ct ");
  for (const auto &filePath : file_paths)
    gitLogCmd.append_sprintf("%s ", filePath.c_str());

  eastl::fixed_string<char, OUTPUT_SIZE> output{};
  if (execute_cmd(gitLogCmd.c_str(), output) == 0)
  {
    int64_t timestamp = 0;
#if _TARGET_PC_LINUX
    sscanf(output.c_str(), "%ld", &timestamp);
#else
    sscanf(output.c_str(), "%lld", &timestamp);
#endif
    return timestamp;
  }
  return 0;
}

int64_t get_git_files_last_commit_timestamp(dag::ConstSpan<SimpleString> file_paths)
{
  if (check_modified_files(file_paths))
    return 0;

  return get_git_timestamp(file_paths);
}
