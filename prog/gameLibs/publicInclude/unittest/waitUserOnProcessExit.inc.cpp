// Copyright (C) Gaijin Games KFT.  All rights reserved.

// This file adds the following feature to executable:
//
// if environment variable DAGOR_WAIT_ON_PROCESS_EXIT=true
// then
// global singleton destructor will print message and wait for keyboard hit (or enter key pressed)
//
// This destructor will be executed after main() exit.
//
// The feature is useful during running process from Visual Studio, Visual Studio Code & etc
// It prevents from console disappear and allows to read last error messages, read unit test run result & etc


#include <cstdio>
#include <cstdlib>
#include <cstring>

#if _TARGET_PC_WIN
#include <conio.h>
#endif


namespace
{

constexpr const char *ENVIRONMENT_VARIABLE_NAME = "DAGOR_WAIT_ON_PROCESS_EXIT";


class WaitUserOnProcessExit
{
public:
  WaitUserOnProcessExit()
  {
    const char *const envVariableValue = std::getenv(ENVIRONMENT_VARIABLE_NAME);
    const bool envVariableIsSet = envVariableValue != nullptr && 0 == std::strcmp(envVariableValue, "true");

    waitEnabled = envVariableIsSet;

    if (waitEnabled)
    {
      printf("The process will wait for user keyboard input before its exit!\n");
    }
  }

  ~WaitUserOnProcessExit()
  {
    if (waitEnabled)
    {
      printf("The process is almost stopped!\n");
#if _TARGET_PC_WIN
      printf("Press any key to exit the process!\n");
      _getch();
#else
      printf("Press Enter to exit the process!\n");
      char ch = 0;
      const int result = scanf("%c", &ch);
#endif
    }
  }

  bool waitEnabled = false;
};


} // namespace


const WaitUserOnProcessExit wait_user_on_process_exit;
