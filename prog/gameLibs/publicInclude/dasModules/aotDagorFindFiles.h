//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <generic/dag_tab.h>
#include <ioSys/dag_findFiles.h>
#include <memory/dag_framemem.h>

namespace bind_dascript
{
inline int das_find_files_in_folder(const char *dir_path, const char *file_suffix_to_match, bool vromfs, bool realfs, bool subdirs,
  const das::TBlock<void, const das::TTemporary<const das::TArray<char *>>> &block, das::Context *context, das::LineInfoArg *at)
{
  Tab<SimpleString> list(framemem_ptr());
  const int result =
    find_files_in_folder(list, dir_path ? dir_path : "", file_suffix_to_match ? file_suffix_to_match : "", vromfs, realfs, subdirs);
  das::Array arr;
  arr.data = (char *)list.data();
  arr.size = list.size();
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
  return result;
}
inline int das_find_file_in_vromfs(const char *filename,
  const das::TBlock<void, const das::TTemporary<const das::TArray<char *>>> &block, das::Context *context, das::LineInfoArg *at)
{
  Tab<SimpleString> list(framemem_ptr());
  const int result = find_file_in_vromfs(list, filename ? filename : "");
  das::Array arr;
  arr.data = (char *)list.data();
  arr.size = list.size();
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
  return result;
}
} // namespace bind_dascript
