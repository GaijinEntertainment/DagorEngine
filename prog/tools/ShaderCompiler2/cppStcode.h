// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "cppStcodeAssembly.h"
#include "shHardwareOpt.h"
#include "processes.h"
#include <generic/dag_expected.h>

extern bool compile_cpp_stcode;
extern bool cpp_stcode_unity_build;
extern StcodeShader g_cppstcode;

bool set_stcode_arch_from_arg(const char *str);

#define RET_IF_SHOULD_NOT_COMPILE(_ret) \
  do                                    \
  {                                     \
    if (!compile_cpp_stcode)            \
      return _ret;                      \
  } while (0)

void prepare_stcode_directory(const char *dest_dir);

void save_compiled_cpp_stcode(StcodeShader &&cpp_shader);
void save_stcode_dll_main(StcodeInterface &&cpp_interface);
void save_stcode_global_vars(StcodeGlobalVars &&cpp_globvars);

enum class StcodeMakeTaskError
{
  FAILED,
  DISABLED
};

dag::Expected<proc::ProcessTask, StcodeMakeTaskError> make_stcode_compilation_task(const char *out_dir, const char *lib_name,
  const ShVariantName &variant);
