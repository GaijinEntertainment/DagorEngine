// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/hash_map.h>

#include <shaders/dag_renderScene.h>
#include <shaders/dag_renderStateId.h>
#include "scriptSElem.h"
#include "shadersBinaryData.h"

namespace shaderbindump
{
ScriptedShadersBinDumpOwner shadersBinDump;
#ifdef SHADERS_ALLOW_2_BINDUMP
static ScriptedShadersBinDumpOwner shadersBinDumpExp;
#endif
ScriptedShadersBinDump emptyDump;

static const ScriptedShadersBinDump *null_dump()
{
  static const ScriptedShadersBinDump *mapped_null_dump = nullptr;
  if (!mapped_null_dump)
  {
    static bindump::MemoryWriter static_writer;
    bindump::Master<shader_layout::ScriptedShadersBinDump> shaders_dump;
    shaders_dump.shaderNameMap.resize(1);
    shaders_dump.shaderNameMap[0] = "?null?";
    shaders_dump.classes.resize(2);
    shaders_dump.classes[0].name = shaders_dump.shaderNameMap[0].getElementAddress(0);
    shaders_dump.classes[0].name.setCount(shaders_dump.shaderNameMap[0].size());
    shaders_dump.classes[1].name = shaders_dump.shaderNameMap[0].getElementAddress(0);
    shaders_dump.classes[1].name.setCount(shaders_dump.shaderNameMap[0].size());
    shaders_dump.classes[1].code.resize(1);
    bindump::streamWrite(shaders_dump, static_writer);
    mapped_null_dump = bindump::map<shader_layout::ScriptedShadersBinDump>(static_writer.mData.data());
  }
  return mapped_null_dump;
}

const ShaderClass &null_shader_class(bool with_code) { return null_dump()->classes[with_code ? 1 : 0]; }
const ShaderCode &null_shader_code() { return null_shader_class(true).code[0]; }
} // namespace shaderbindump

ScriptedShadersBinDumpOwner &shBinDumpExOwner(bool main)
{
#ifdef SHADERS_ALLOW_2_BINDUMP
  return main ? shaderbindump::shadersBinDump : shaderbindump::shadersBinDumpExp;
#else
  G_FAST_ASSERT(main); // Validate sec shdump
  G_UNUSED(main);
  return shaderbindump::shadersBinDump;
#endif
}
