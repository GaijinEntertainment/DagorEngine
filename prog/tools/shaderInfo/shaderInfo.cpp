// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shadersBinaryData.h>
#include <shaders/shUtils.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
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
#include <util/dag_strUtil.h>
#define USE_SHA1_HASH 0

static void showUsage()
{
  printf("\nUsage:\n  shaderInfo-dev.exe <in_shdump.bin> [out] [-asm] [-shaders:<DIR>] [-variants] [-stcode] [-shader:<name>]\n");
}
static void dumpCurrentShaders(bool dump_asm, bool dump_variants, bool dump_stcode, const char *single_shader, const char *sh_dir);

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
  const char *out_shaders = nullptr;
  bool out_variants = false;
  bool out_stcode = false;
  const char *single_shader = nullptr;
  for (int i = 2; i < __argc; i++)
    if (strcmp(__argv[i], "-asm") == 0)
      out_asm = true;
    else if (strncmp(__argv[i], "-shaders:", 9) == 0)
      out_shaders = __argv[i] + 9;
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

  printf("loaded binary dump %s (memSize=%uK), dumping contents to: \"%s\" [VARS SHADERS %s %s %s], %d shaders, %d total vars, %d "
         "globvars\n",
    __argv[1], (uint32_t)(shBinDumpOwner().getDumpSize() >> 10), output_log_fn, out_variants ? "VARIANT_TABLES" : "",
    out_asm ? "ASM" : "", out_stcode ? "STCODE" : "", (int)shBinDump().classes.size(), (int)shBinDump().varMap.size(),
    (int)shBinDump().globVars.v.size());
  if (out_shaders)
    printf("(dumping shaders with content-like names to folder: %s/ )\n", out_shaders);
  if (single_shader)
    printf("(dumping only contents of <%s> shader)\n", single_shader);

  int t0 = get_time_msec();
  dumpCurrentShaders(out_asm, out_variants, out_stcode, single_shader, out_shaders);
  printf("\ndumped for %.1f seconds\n", (get_time_msec() - t0) / 1000.0f);
  return 0;
}

#include <d3dcompiler.h>
static void disassembleShader(dag::ConstSpan<uint32_t> native_code, const char *out_fn = nullptr)
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
  {
    if (out_fn)
    {
      FullFileSaveCB cwr(out_fn);
      cwr.write(disassembly->GetBufferPointer(), disassembly->GetBufferSize());
    }
    else
      debug("%s", disassembly->GetBufferPointer());
  }
  else if (!out_fn)
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

static String mk_str_cat3(const char *s1, const char *s2, const char *s3)
{
  String s;
  s.setStrCat3(s1, s2, s3);
  return s;
}

static String mk_str_cat4(const char *s1, const char *s2, const char *s3, const char *s4)
{
  String s;
  s.setStrCat4(s1, s2, s3, s4);
  return s;
}

#if USE_SHA1_HASH
#include <hash/sha1.h>
#define HASH_SIZE    20
#define HASH_CONTEXT sha1_context
#define HASH_UPDATE  sha1_update
#define HASH_INIT    sha1_starts
#define HASH_FINISH  sha1_finish

#else
#include <hash/BLAKE3/blake3.h>

#define HASH_SIZE    32
inline void blake3_finalize_32(const blake3_hasher *h, unsigned char *hash) { blake3_hasher_finalize(h, hash, HASH_SIZE); }
#define HASH_CONTEXT blake3_hasher
#define HASH_UPDATE  blake3_hasher_update
#define HASH_INIT    blake3_hasher_init
#define HASH_FINISH  blake3_finalize_32
#endif
static String calc_sha1(dag::ConstSpan<uint32_t> native_code)
{
  HASH_CONTEXT sha1;
  unsigned char srcSha1[HASH_SIZE];

  HASH_INIT(&sha1);
  HASH_UPDATE(&sha1, (const unsigned char *)native_code.data(), data_size(native_code));
  HASH_FINISH(&sha1, srcSha1);

  String s;
  data_to_str_hex(s, srcSha1, sizeof(srcSha1));
  return s;
}

static void dumpCurrentShaders(bool dump_asm, bool dump_variants, bool dump_stcode, const char *single_shader, const char *sh_dir)
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

  const auto vprIdCount = shBinDump().vprCount;
  const auto fshIdCount = shBinDump().fshCount;

  if (!dump_asm && !sh_dir)
    debug("\n******* %d vertex shaders\n******* %d pixel shaders", vprIdCount, fshIdCount);
  else if (sh_dir)
  {
    ShaderBytecode tmpbuf;
    debug("");
    dd_mkdir(String::mk_str_cat(sh_dir, "/vs"));
    for (int i = 0; i < vprIdCount; i++)
    {
      String sha1 = calc_sha1(shBinDumpOwner().getCode(i, ShaderCodeType::VERTEX, tmpbuf));
      debug("Vertex shader --v%d-- %s/vs/%s (%d bytes)", i, sh_dir, sha1, data_size(tmpbuf));
      FullFileSaveCB cwr(mk_str_cat3(sh_dir, "/vs/", sha1));
      cwr.write(tmpbuf.data(), data_size(tmpbuf));
      if (dump_asm)
        disassembleShader(tmpbuf, mk_str_cat4(sh_dir, "/vs/", sha1, ".asm"));
    }

    dd_mkdir(String::mk_str_cat(sh_dir, "/ps"));
    for (int i = 0; i < fshIdCount; i++)
    {
      String sha1 = calc_sha1(shBinDumpOwner().getCode(i, ShaderCodeType::PIXEL, tmpbuf));
      debug("Pixel shader --p%d-- %s/ps/%s (%d bytes)", i, sh_dir, sha1, data_size(tmpbuf));
      FullFileSaveCB cwr(mk_str_cat3(sh_dir, "/ps/", sha1));
      cwr.write(tmpbuf.data(), data_size(tmpbuf));
      if (dump_asm)
        disassembleShader(tmpbuf, mk_str_cat4(sh_dir, "/ps/", sha1, ".asm"));
    }
  }
  else
  {
    ShaderBytecode tmpbuf;
    for (int i = 0; i < vprIdCount; i++)
    {
      debug("\n******* Vertex shader --v%d--", i);

      disassembleShader(shBinDumpOwner().getCode(i, ShaderCodeType::VERTEX, tmpbuf));
    }

    for (int i = 0; i < fshIdCount; i++)
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
