// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>

namespace texgen
{
struct Graph;

// Pulls the das modules texgen_run_script needs (builtin, math, strings, texgen). Call it before
// das::Module::Initialize(); the host owns Initialize and Shutdown.
void texgen_das_pull_modules();

// Compiles and runs a texgen script and moves the graph that its `[export] def graph()` builds in texgen_graph() of the
// calling thread into out. The script sees only the builtin, texgen, math and strings modules and the generated module
// `texgen_nodes` (nodes_module_das_text; null serves an empty module), every other require fails, and it can set only
// the gen2 and indenting options. script_text null reads script_path_or_name from the disk. False on a compile,
// simulate or run error, or when the graph does not validate; one entry per problem is appended to errors, with
// "file:line: " when the problem has a line.
bool texgen_run_script(const char *script_path_or_name, const char *script_text, const char *nodes_module_das_text, Graph &out,
  eastl::string &errors);
} // namespace texgen
