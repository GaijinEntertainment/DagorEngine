// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shadersBinaryData.h>
#include <shaders/shUtils.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <debug/dag_debug.h>
#include <debug/dag_logSys.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_string.h>
#include <stdio.h>
#include <stdlib.h>
#include <EASTL/string.h>
#include <EASTL/vector_set.h>
#include <EASTL/vector.h>
#include <generic/dag_span.h>

static void showUsage()
{
  printf("\nUsage:\n  shaderInfo-dev.exe <in_shdump.bin> [out] [-asm] [-variants] [-stcode] [-shader:<name>]\n");
}
static void dumpCurrentShaders(bool dump_asm, bool dump_variants, bool dump_stcode, const char *single_shader);

int DagorWinMain(bool debugmode)
{
  printf("Shader Info Tool v1.2\n"
         "Copyright (C) Gaijin Games KFT, 2023\n");

  if (__argc < 2)
  {
    showUsage();
    return 1;
  }
  const char *output_log_fn = "shader_dump";
  bool out_asm = false;
  bool out_variants = false;
  bool out_stcode = false;
  const char *single_shader = nullptr;
  for (int i = 2; i < __argc; i++)
    if (strcmp(__argv[i], "-asm") == 0)
      out_asm = true;
    else if (strcmp(__argv[i], "-variants") == 0)
      out_variants = true;
    else if (strcmp(__argv[i], "-stcode") == 0)
      out_stcode = true;
    else if (strncmp(__argv[i], "-shader:", 8) == 0)
      single_shader = __argv[i] + 8;
    else if (__argv[i][0] != '-')
      output_log_fn = __argv[i];
    else
    {
      printf("ERR: unsupported switch  <%s>", __argv[i]);
      return 1;
    }
  start_classic_debug_system(output_log_fn);
  debug_enable_timestamps(false);

  FullFileLoadCB crd(__argv[1]);
  if (!crd.fileHandle)
  {
    printf("ERR: can't open <%s>", __argv[1]);
    return 1;
  }
  if (!shBinDumpOwner().load(crd, df_length(crd.fileHandle)))
  {
    printf("ERR: can't read shaders binary dump from <%s>", __argv[1]);
    return 1;
  }

  printf("loaded binary dump %s (memSize=%uK), dumping contents to: \"%s\" [VARS SHADERS %s %s %s], %d shaders, %d globvars\n",
    __argv[1], (uint32_t)(shBinDumpOwner().getDumpSize() >> 10), output_log_fn, out_variants ? "VARIANT_TABLES" : "",
    out_asm ? "ASM" : "", out_stcode ? "STCODE" : "", (int)shBinDump().classes.size(), shBinDump().globVars.v.size());
  if (single_shader)
    printf("(dumping only contents of <%s> shader)\n", single_shader);

  int t0 = get_time_msec();
  dumpCurrentShaders(out_asm, out_variants, out_stcode, single_shader);
  printf("\ndumped for %.1f seconds\n", (get_time_msec() - t0) / 1000.0f);
  return 0;
}

#include <d3dcompiler.h>
static void disassembleShader(dag::ConstSpan<uint32_t> native_code)
{
  if (native_code.empty())
    return;
  ID3DBlob *disassembly = nullptr;
  HRESULT hr = E_FAIL;
  if (native_code.size() * 4 == native_code[0] + 12)
  { // DX11 uncompressed shaders
    hr = D3DDisassemble(native_code.data() + 3, data_size(native_code) - 12, 0, nullptr, &disassembly);
  }
  else if (native_code[3] == _MAKE4C('SH.z'))
  {
    debug("Outdated compressed shader format [SH.z] that is no longer supported");
    return;
  }
  else if (native_code[0] == _MAKE4C('SVu3'))
  { // SpirV shaders
  }
  else if (native_code[0] == _MAKE4C('sx12'))
  { // DX12 shaders (DXIL or DXBC)
  }

  if (hr == S_OK && disassembly && disassembly->GetBufferPointer() && *(char *)disassembly->GetBufferPointer())
    debug("%s", disassembly->GetBufferPointer());
  else
  {
    String dump_str(native_code.size() * 3 + 64, "  [%d words]\n", native_code.size());
    for (const uint32_t *c = native_code.data(), *c_e = c + native_code.size(); c < c_e; c += 16)
      if (c + 16 <= c_e)
        dump_str.aprintf(0, "  %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X\n", c[0], c[1], c[2],
          c[3], c[4], c[5], c[6], c[7], c[8], c[9], c[10], c[11], c[12], c[13], c[14], c[15]);
      else
      {
        dump_str.aprintf(0, "  %08X", c[0]);
        for (c++; c < c_e; c++)
          dump_str.aprintf(0, " %08X", c[0]);
        dump_str.aprintf(0, "\n");
      }
    debug_(dump_str);
  }

  if (disassembly)
    disassembly->Release();
}

static void dumpCurrentShaders(bool dump_asm, bool dump_variants, bool dump_stcode, const char *single_shader)
{
  if (single_shader)
  {
    for (int i = 0; i < shBinDump().classes.size(); i++)
      if (strcmp(single_shader, (const char *)shBinDump().classes[i].name) == 0)
        shaderbindump::dumpShaderInfo(shBinDump().classes[i], dump_variants);
    debug("");
  }

  debug("global vars (%d):", shBinDump().globVars.v.size());
  shaderbindump::dumpVars(shBinDump().globVars);

  debug("named state blocks(%d):", shBinDump().blockNameMap.size());
  for (int i = 0; i < shBinDump().blockNameMap.size(); i++)
  {
    debug("  block <%s>: uidMask=%04X uidVal=%04X, stcode=%d", (const char *)shBinDump().blockNameMap[i],
      shBinDump().blocks[i].uidMask, shBinDump().blocks[i].uidVal, shBinDump().blocks[i].stcodeId);

    const shader_layout::blk_word_t *sb = &shBinDump().blocks[i].suppBlockUid.get();
    if (!shBinDump().blocks[i].suppBlkMask || *sb == shader_layout::BLK_WORD_FULLMASK)
      debug("     suppMask=%04X, no other blocks supported!", shBinDump().blocks[i].suppBlkMask);
    else
    {
      debug_("     suppMask=%04X, supported block codes:", shBinDump().blocks[i].suppBlkMask);
      while (*sb != shader_layout::BLK_WORD_FULLMASK)
      {
        debug_(" %04X", *sb);
        sb++;
      }
      debug("");
    }
    debug("");
  }
  debug("");
  if (!single_shader)
    for (int i = 0; i < shBinDump().classes.size(); i++)
      shaderbindump::dumpShaderInfo(shBinDump().classes[i], dump_variants);

  debug("\nmaxreg count = %d\n", shBinDump().maxRegSize);

  if (!dump_asm)
    debug("\n******* %d vertex shaders\n******* %d pixel shaders", shBinDump().vprId.size(), shBinDump().fshId.size());
  else
  {
    ShaderBytecode tmpbuf;
    for (int i = 0; i < shBinDump().vprId.size(); i++)
    {
      debug("\n******* Vertex shader --v%d--", i);

      disassembleShader(shBinDumpOwner().getCode(i, ShaderCodeType::VERTEX, tmpbuf));
    }

    for (int i = 0; i < shBinDump().fshId.size(); i++)
    {
      debug("\n******* Pixel shader --p%d--", i);

      disassembleShader(shBinDumpOwner().getCode(i, ShaderCodeType::PIXEL, tmpbuf));
    }
  }

  if (!dump_stcode)
  {
    uint64_t totalBytes = 0;
    for (int i = 0; i < shBinDump().stcode.size(); i++)
    {
      const dag::ConstSpan<int> &prog = shBinDump().stcode[i];
      totalBytes += data_size(prog);
    }
    debug("\n******* %d stcode programs, total mem %lfkb", shBinDump().stcode.size(), (double)totalBytes / 1024.0);
  }
  else
  {
    eastl::vector<eastl::vector_set<eastl::string>> classesByStcode;
    classesByStcode.resize(shBinDump().stcode.size());
    for (auto &sh_class : shBinDump().classes)
      for (auto &sh_code : sh_class.code)
        for (auto &pass : sh_code.passes)
          if (pass.rpass->stcodeId != 0xFFFF)
            classesByStcode[pass.rpass->stcodeId].insert(eastl::string(sh_class.name.data()));

    for (int i = 0; i < shBinDump().stcode.size(); i++)
    {
      if (single_shader && classesByStcode[i].find(eastl::string(single_shader)) == classesByStcode[i].end())
        continue;

      debug("\n******* State code shader --s%d--", i);
      if (shBinDump().stcode[i].size())
      {
        ShUtils::shcod_dump(shBinDump().stcode[i], &shBinDump().globVars, NULL, {}, false);
        debug("\nUsed by shaders:");
        for (const eastl::string &name : classesByStcode[i])
          debug("  %s", name);
      }
    }
  }
}
