//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <memory/dag_memBase.h>
#include <ioSys/dag_genIo.h>
#include <util/dag_globDef.h>
#include <stdio.h>
#include <string.h>            // memcpy
#include <math/dag_mathBase.h> // min/max
#include <util/dag_stdint.h>
#include <util/dag_safeArg.h>
#include <memory/dag_framemem.h>

#include "httpserver.h"
namespace ecs
{
class EntityManager;
}
namespace webui
{

// cross platform and implements only write function (also resizes if need to)
class YAMemSave : public IGenSave
{
public:
  char *mem;
  IMemAlloc *allocator;
  size_t bufSize;
  size_t offset;

  YAMemSave(size_t buf_size = 16 << 10, IMemAlloc *allocator_ = framemem_ptr()) : bufSize(buf_size), offset(0), allocator(allocator_)
  {
    mem = (char *)allocator->alloc(bufSize);
    mem[0] = 0;
  }

  ~YAMemSave() { memfree(mem, allocator); }

  int tell()
  {
    G_ASSERT(0);
    return -1;
  }
  void seekto(int) { G_ASSERT(0); }
  void seektoend(int) { G_ASSERT(0); }
  const char *getTargetName()
  {
    G_ASSERT(0);
    return "";
  }
  void beginBlock() { G_ASSERT(0); }
  void endBlock(unsigned) { G_ASSERT(0); }
  int getBlockLevel()
  {
    G_ASSERT(0);
    return 0;
  }
  void flush() {}

  void write(const void *ptr, int size)
  {
    if (offset + size >= bufSize)
    {
      bufSize = (offset + size) * 2;
      mem = (char *)allocator->realloc((void *)mem, bufSize);
    }
    memcpy(mem + offset, ptr, size);
    offset += size;
  }

#define DSA_OVERLOADS_PARAM_DECL
#define DSA_OVERLOADS_PARAM_PASS
  DECLARE_DSA_OVERLOADS_FAMILY(inline void printf, inline void vprintf, vprintf);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

  void finishStr() { mem[min(offset, bufSize - 1)] = 0; }

  void forget()
  {
    mem = NULL;
    offset = bufSize = 0;
  }
};

inline void YAMemSave::vprintf(const char *f, const DagorSafeArg *arg, int anum)
{
  int w = DagorSafeArg::print_fmt(mem + offset, (int)(bufSize - offset), f, arg, anum);
  offset += w;
  if (offset >= (bufSize - bufSize / 4))
  {
    bufSize *= 2;
    mem = (char *)allocator->realloc((void *)mem, bufSize);
  }

  finishStr();
}

extern HttpPlugin dagor_http_plugins[];
extern HttpPlugin squirrel_http_plugins[];
extern HttpPlugin webview_http_plugins[];

extern HttpPlugin tvos_http_plugins[];
extern HttpPlugin shader_http_plugin;
extern HttpPlugin screenshot_http_plugin;
extern HttpPlugin profiler_http_plugin;
extern HttpPlugin ecsviewer_http_plugin;
extern HttpPlugin auto_drey_http_plugin;
extern HttpPlugin editvar_http_plugin;
extern HttpPlugin color_pipette_http_plugin;
extern HttpPlugin colorpicker_http_plugin;
extern HttpPlugin curveeditor_http_plugin;
extern HttpPlugin cookie_http_plugin;
extern HttpPlugin colorgradient_http_plugin;
extern HttpPlugin rendinst_colors_http_plugin;

void init_dmviewer_plugin();
void init_ecsviewer_plugin();

void set_ecsviewer_entity_manager(ecs::EntityManager *);

}; // namespace webui
