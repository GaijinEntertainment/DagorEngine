// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shadersBinaryData.h>
#include <shaders/shUtils.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zstdIo.h>
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
#include <EASTL/sort.h>
#include <generic/dag_span.h>
#include <util/dag_strUtil.h>
#include <shaderBlobDisassembler/disasm.h>
#define USE_SHA1_HASH 0

#if _TARGET_PC_WIN
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
#else
extern int __argc;
extern char **__argv;

static void disassembleShader(dag::ConstSpan<uint32_t> native_code, const char *out_fn = nullptr) {}
#endif

static void showUsage()
{
  printf("\nUsage:\n"
         "  shaderInfo-dev.exe <in_shdump.bin> [-shader:<name>] [-outfile:<FILE>] [-outdir:<DIR>]\n"
         "    [-preset:MIN|BRIEF|TABLES|DETAIL|FULL](default:DETAIL)\n"
         "    [-globvars] [-nstblocks] [-maxreg] [-shaders] [-asm] [-stcode]\n"
         "    [-sortby:NAME|PASS](default:NAME)\n"
         "    [-disasm:dx11|dx12|spirv|metal|ps4|ps5]\n"
         "    [-headeronly] [-locvars] [-stinit] [-varsummary] [-stvartable]\n"
         "    [-codes] [-stvarmap] [-vtxchnl] [-dynvartable] [-variants]\n");
}
static void dumpCurrentShaders(const char *single_shader, shaderbindump::DumpDetails details, shaderbindump::SortType sortby,
  const char *sh_dir);

eastl::optional<shader_blob_disasm::Target> disasmTarget = {};

int DagorWinMain(bool debugmode)
{
  printf("Shader Info Tool v1.2\n"
         "Copyright (C) Gaijin Games KFT, 2023\n");

  if (__argc < 2)
  {
    showUsage();
    return 1;
  }

  const char *single_shader = nullptr;
  const char *output_log_fn = "shader_dump";
  const char *output_dir = nullptr;

  shaderbindump::DumpDetails userOutputDetails = {};

  auto outputPreset = shaderbindump::DumpDetails::Preset::DEFAULT;
  auto sortType = shaderbindump::SortType::DEFAULT;


  for (int i = 2; i < __argc; i++)
    if (strncmp(__argv[i], "-shader:", 8) == 0)
      single_shader = __argv[i] + 8;
    else if (strncmp(__argv[i], "-outfile:", 9) == 0)
      output_log_fn = __argv[i] + 9;
    else if (strncmp(__argv[i], "-outdir:", 8) == 0)
      output_dir = __argv[i] + 8;

    else if (strncmp(__argv[i], "-preset:", 8) == 0)
    {
      const char *presetName = __argv[i] + 8;
      if (strcmp(presetName, "MIN") == 0)
        outputPreset = shaderbindump::DumpDetails::Preset::MINIMAL;
      else if (strcmp(presetName, "BRIEF") == 0)
        outputPreset = shaderbindump::DumpDetails::Preset::BRIEF;
      else if (strcmp(presetName, "TABLES") == 0)
        outputPreset = shaderbindump::DumpDetails::Preset::TABLES;
      else if (strcmp(presetName, "DETAIL") == 0)
        outputPreset = shaderbindump::DumpDetails::Preset::DETAIL;
      else if (strcmp(presetName, "FULL") == 0)
        outputPreset = shaderbindump::DumpDetails::Preset::FULL;
      else
      {
        printf("ERR: unsupported preset <%s>\n"
               "  supported presets: MIN BRIEF TABLES DETAIL FULL\n",
          presetName);
        return 1;
      }
    }
    else if (strncmp(__argv[i], "-sortby:", 8) == 0)
    {
      const char *sortMode = __argv[i] + 8;
      if (strcmp(sortMode, "NAME") == 0)
        sortType = shaderbindump::SortType::NAME;
      else if (strcmp(sortMode, "PASS") == 0)
        sortType = shaderbindump::SortType::PASS;
      else
      {
        printf("ERR: unsupported sort mode <%s>\n"
               "  supported modes: NAME PASS\n",
          sortMode);
        return 1;
      }
    }
    else if (strncmp(__argv[i], "-disasm:", 8) == 0)
    {
      char const *target = __argv[i] + 8;
      for (size_t i = 0; char const *opt : shader_blob_disasm::TARGET_NAMES)
      {
        if (strcmp(target, opt) == 0)
        {
          disasmTarget.emplace(shader_blob_disasm::Target(i));
          break;
        }
        ++i;
      }
      if (!disasmTarget)
      {
        eastl::string supportedTargets{};
        for (char const *opt : shader_blob_disasm::TARGET_NAMES)
        {
          if (!supportedTargets.empty())
            supportedTargets.append(" ");
          supportedTargets.append(opt);
        }
        printf("ERR: unsupported disasm target <%s>\n"
               "  supported targets: %s\n",
          target, supportedTargets.c_str());
        return 1;
      }
    }

    // overall output options
    else if (strcmp(__argv[i], "-globvars") == 0)
      userOutputDetails.globalVars = true;
    else if (strcmp(__argv[i], "-nstblocks") == 0)
      userOutputDetails.namedStateBlocks = true;
    else if (strcmp(__argv[i], "-maxreg") == 0)
      userOutputDetails.maxregCount = true;
    else if (strcmp(__argv[i], "-shaders") == 0)
      userOutputDetails.shaders = true;
    else if (strcmp(__argv[i], "-asm") == 0)
      userOutputDetails.assembly = true;
    else if (strcmp(__argv[i], "-stcode") == 0)
      userOutputDetails.stcode = true;

    // shader output options
    else if (strcmp(__argv[i], "-headeronly") == 0)
      userOutputDetails.shaderDetails.headerOnly = true;
    else if (strcmp(__argv[i], "-locvars") == 0)
      userOutputDetails.shaderDetails.localVars = true;
    else if (strcmp(__argv[i], "-stinit") == 0)
      userOutputDetails.shaderDetails.staticInit = true;
    else if (strcmp(__argv[i], "-varsummary") == 0)
      userOutputDetails.shaderDetails.varSummary = true;
    else if (strcmp(__argv[i], "-stvartable") == 0)
      userOutputDetails.shaderDetails.staticVariantTable = true;
    else if (strcmp(__argv[i], "-codes") == 0)
      userOutputDetails.shaderDetails.codeBlocks = true;
    else if (strcmp(__argv[i], "-stvarmap") == 0)
      userOutputDetails.shaderDetails.stvarmap = true;
    else if (strcmp(__argv[i], "-vtxchnl") == 0)
      userOutputDetails.shaderDetails.vertexChannels = true;
    else if (strcmp(__argv[i], "-dynvartable") == 0)
      userOutputDetails.shaderDetails.dynamicVariantTable = true;
    else if (strcmp(__argv[i], "-variants") == 0)
      userOutputDetails.shaderDetails.dumpVariants = true;

    else
    {
      printf("ERR: unsupported switch <%s>\n", __argv[i]);
      showUsage();
      return 1;
    }


  // if none of output options are specified - use output preset (stated or default)
  // if at least one is specified - use user output options
  shaderbindump::DumpDetails outputDetails =
    userOutputDetails == shaderbindump::DumpDetails() ? shaderbindump::DumpDetails(outputPreset) : userOutputDetails;
  outputDetails.validate();


  FullFileLoadCB crd(__argv[1]);
  if (!crd.fileHandle)
  {
    printf("ERR: can't open <%s>\n", __argv[1]);
    return 1;
  }

  start_classic_debug_system(output_log_fn);
  debug_enable_timestamps(false);

  if (!shBinDumpOwner().loadFromFile(crd, df_length(crd.fileHandle)))
  {
    printf("ERR: can't read shaders binary dump from <%s>\n", __argv[1]);
    return 1;
  }

  printf("loaded binary dump %s (memSize=%uK)\n"
         "%d shaders, %d total vars, %d glob var\n"
         "dumping contents to: \"%s\"\n"
         "  globals:[%s %s %s %s %s %s]\n",
    __argv[1], (uint32_t)(shBinDumpOwner().getDumpSize() >> 10), (int)shBinDump().classes.size(), (int)shBinDump().varMap.size(),
    (int)shBinDump().globVars.v.size(), output_log_fn, outputDetails.globalVars ? "GLOB_VARS" : "",
    outputDetails.namedStateBlocks ? "NAMED_STBLOCKS" : "", outputDetails.maxregCount ? "MAXREG_CNT" : "",
    outputDetails.shaders ? "SHADERS" : "", outputDetails.assembly ? "ASM" : "", outputDetails.stcode ? "STCODE" : "");

  if (outputDetails.shaders)
  {
    printf("  shaders:[%s %s %s %s %s %s %s %s %s %s]\n", outputDetails.shaderDetails.headerOnly ? "HEADER" : "",
      outputDetails.shaderDetails.localVars ? "LOCAL_VARS" : "", outputDetails.shaderDetails.staticInit ? "ST_INIT" : "",
      outputDetails.shaderDetails.varSummary ? "VARIANT_SUMMARY" : "",
      outputDetails.shaderDetails.staticVariantTable ? "ST_VARIANT_TABLE" : "", outputDetails.shaderDetails.codeBlocks ? "CODES" : "",
      outputDetails.shaderDetails.stvarmap ? "ST_VAR_MAP" : "", outputDetails.shaderDetails.vertexChannels ? "VERTEX_CHANNELS" : "",
      outputDetails.shaderDetails.dynamicVariantTable ? "DYN_VARIANT_TABLE" : "",
      outputDetails.shaderDetails.dumpVariants ? "VARIANTS" : "");
  }

  if (single_shader)
    printf("(dumping only contents of <%s> shader)\n", single_shader);
  if (output_dir)
    printf("(dumping with content-like names to folder: %s/ )\n", output_dir);

  int t0 = get_time_msec();
  dumpCurrentShaders(single_shader, outputDetails, sortType, output_dir);
  printf("\ndumped for %.1f seconds\n", (get_time_msec() - t0) / 1000.0f);

  return 0;
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

// have to duplicate it here because drv3d_commonCode can't be included as drv3d_null redefines some of code
// can be removed after drv3d_null references drv3d_commonCode
const uint32_t *ShaderSource::uncompress(Tab<uint8_t> &tmpbuf) const
{
  G_ASSERT(!compressedData.empty());
  G_ASSERT(dictionary || compressedData.size() <= uncompressedSize);
  tmpbuf.resize(uncompressedSize);
  if (dictionary == nullptr)
    eastl::copy(compressedData.begin(), compressedData.end(), (uint8_t *)tmpbuf.data());
  else
  {
    ZSTD_DCtx_s *dctx = zstd_create_dctx(true); // tmp for framemem
    uint32_t decompressed_size = zstd_decompress_with_dict(dctx, tmpbuf.data(), tmpbuf.size(), compressedData.data(),
      compressedData.size(), (const ZSTD_DDict_s *)dictionary);
    zstd_destroy_dctx(dctx);
    G_ASSERT(decompressed_size <= tmpbuf.size());
  }
  return (const uint32_t *)tmpbuf.data();
}

dag::ConstSpan<uint32_t> uncompressShader(const ShaderSource &source, Tab<uint8_t> &tmpbuf)
{
  return make_span_const(source.uncompress(tmpbuf), source.uncompressedSize / 4);
}

static void dumpCurrentShaders(const char *single_shader, shaderbindump::DumpDetails details, shaderbindump::SortType sortby,
  const char *sh_dir)
{
  debug("--- START ---\n");

  struct RelevantShaderIds
  {
    eastl::vector_set<int> vpr;
    eastl::vector_set<int> fsh;
  };
  eastl::optional<RelevantShaderIds> filter{};

  if (single_shader)
  {
    filter.emplace();
    bool shaderfound = false;
    for (int i = 0; i < shBinDump().classes.size(); i++)
      if (strcmp(single_shader, (const char *)shBinDump().classes[i].name) == 0)
      {
        shaderfound = true;
        shaderbindump::dumpShaderInfo(shBinDump(), shBinDump().classes[i], details.shaderDetails);
        for (auto const &code : shBinDump().classes[i].code)
        {
          for (auto const &pass : code.passes)
          {
            if (pass.rpass->vprId >= 0)
              filter->vpr.insert(pass.rpass->vprId);
            if (pass.rpass->fshId >= 0)
              filter->fsh.insert(pass.rpass->fshId);
          }
        }
        break;
      }
    if (!shaderfound)
      debug("shader \"%s\" not found!\n", single_shader);
  }


  if (details.globalVars)
  {
    debug("global vars (%d):", shBinDump().globVars.v.size());
    shaderbindump::dumpVars(shBinDump(), shBinDump().globVars, nullptr);
    debug("");
  }


  if (details.namedStateBlocks)
  {
    debug("named state blocks (%d):", shBinDump().blockNameMap.size());
    for (int i = 0; i < shBinDump().blockNameMap.size(); i++)
    {
      debug("  block <%s>: uidMask=%04X uidVal=%04X, stcode=%d", (const char *)shBinDump().blockNameMap[i],
        shBinDump().blocks[i].uidMask, shBinDump().blocks[i].uidVal, shBinDump().blocks[i].stcodeId);

      const shader_layout::blk_word_t *sb = &shBinDump().blocks[i].suppBlockUid.get();
      if (!shBinDump().blocks[i].suppBlkMask || *sb == shader_layout::BLK_WORD_FULLMASK)
      {
        debug("    suppMask=%04X, no other blocks supported!", shBinDump().blocks[i].suppBlkMask);
      }
      else
      {
        debug_("    suppMask=%04X, supported block codes:", shBinDump().blocks[i].suppBlkMask);
        while (*sb != shader_layout::BLK_WORD_FULLMASK)
        {
          debug_(" %04X", *sb);
          sb++;
        }
        debug("");
      }
    }
    debug("");
  }


  if (details.maxregCount)
    debug("maxreg count = %d\n", shBinDump().maxRegSize);


  if (details.shaders && !single_shader)
  {
    dag::Vector<eastl::pair<uint32_t, uint32_t>> sorted;
    bool sort = sortby != shaderbindump::SortType::NAME;
    if (sort)
    {
      sorted.resize(shBinDump().classes.size());
      for (int i = 0; i < shBinDump().classes.size(); i++)
      {
        uint32_t totalPasses = 0;
        uint32_t totalUniquePasses = 0;
        shaderbindump::getTotalPasses(shBinDump().classes[i], totalPasses, totalUniquePasses);
        sorted[i] = {i, totalUniquePasses};
      }

      eastl::sort(sorted.begin(), sorted.end(),
        [](const eastl::pair<uint32_t, uint32_t> &a, const eastl::pair<uint32_t, uint32_t> &b) { return a.second > b.second; });
    }

    debug("shaders (%d):", shBinDump().classes.size());
    for (int i = 0; i < shBinDump().classes.size(); i++)
      shaderbindump::dumpShaderInfo(shBinDump(), shBinDump().classes[sort ? sorted[i].first : i], details.shaderDetails);
    debug("");
  }


  const auto vprIdCount = shBinDump().vprCount;
  const auto fshIdCount = shBinDump().fshCount;

  if (!details.assembly && !disasmTarget && !sh_dir)
    debug("\n******* %d vertex shaders\n******* %d pixel shaders", vprIdCount, fshIdCount);
  else if (sh_dir)
  {
    ShaderBytecode tmpbuf;
    debug("");
    dd_mkdir(String::mk_str_cat(sh_dir, "/vs"));

    auto writeStrToFile = [](auto const &str, String const &fn) {
      file_ptr_t fp = df_open(fn.str(), DF_WRITE | DF_CREATE);
      G_ASSERT_RETURN(fp, );
      df_write(fp, str.c_str(), str.size());
      df_close(fp);
    };

    for (int i = 0; i < vprIdCount; i++)
    {
      if (filter && filter->vpr.find(i) == filter->vpr.end())
        continue;
      ShaderSource src = shBinDumpOwner().getCode(i, ShaderCodeType::VERTEX);
      dag::ConstSpan<uint32_t> code = uncompressShader(src, tmpbuf);
      String sha1 = calc_sha1(code);
      debug("Vertex shader --v%d-- %s/vs/%s (%d bytes)", i, sh_dir, sha1, data_size(tmpbuf));
      FullFileSaveCB cwr(mk_str_cat3(sh_dir, "/vs/", sha1));
      cwr.write(tmpbuf.data(), data_size(tmpbuf));
      if (details.assembly)
        disassembleShader(code, mk_str_cat4(sh_dir, "/vs/", sha1, ".asm"));
      if (disasmTarget)
      {
        writeStrToFile(
          shader_blob_disasm::disassembleShaderBlob(
            dag::ConstSpan<uint8_t>{(uint8_t const *)code.data(), code.size() * elem_size(code)}, src.metadata, *disasmTarget),
          mk_str_cat4(sh_dir, "/vs/", sha1, ".disasm"));
      }
    }

    dd_mkdir(String::mk_str_cat(sh_dir, "/ps"));
    for (int i = 0; i < fshIdCount; i++)
    {
      if (filter && filter->fsh.find(i) == filter->fsh.end())
        continue;
      ShaderSource src = shBinDumpOwner().getCode(i, ShaderCodeType::PIXEL);
      dag::ConstSpan<uint32_t> code = uncompressShader(src, tmpbuf);
      String sha1 = calc_sha1(code);
      debug("Pixel shader --p%d-- %s/ps/%s (%d bytes)", i, sh_dir, sha1, data_size(tmpbuf));
      FullFileSaveCB cwr(mk_str_cat3(sh_dir, "/ps/", sha1));
      cwr.write(tmpbuf.data(), data_size(tmpbuf));
      if (details.assembly)
        disassembleShader(code, mk_str_cat4(sh_dir, "/ps/", sha1, ".asm"));
      if (disasmTarget)
      {
        writeStrToFile(
          shader_blob_disasm::disassembleShaderBlob(
            dag::ConstSpan<uint8_t>{(uint8_t const *)code.data(), code.size() * elem_size(code)}, src.metadata, *disasmTarget),
          mk_str_cat4(sh_dir, "/ps/", sha1, ".disasm"));
      }
    }
  }
  else
  {
    ShaderBytecode tmpbuf;
    for (int i = 0; i < vprIdCount; i++)
    {
      if (filter && filter->vpr.find(i) == filter->vpr.end())
        continue;
      debug("\n******* Vertex shader --v%d--", i);
      ShaderSource src = shBinDumpOwner().getCode(i, ShaderCodeType::VERTEX);
      dag::ConstSpan<uint32_t> code = uncompressShader(src, tmpbuf);
      if (details.assembly)
        disassembleShader(code);
      if (disasmTarget)
      {
        debug("%s",
          shader_blob_disasm::disassembleShaderBlob(
            dag::ConstSpan<uint8_t>{(uint8_t const *)code.data(), code.size() * elem_size(code)}, src.metadata, *disasmTarget)
            .c_str());
      }
    }

    for (int i = 0; i < fshIdCount; i++)
    {
      if (filter && filter->fsh.find(i) == filter->fsh.end())
        continue;
      debug("\n******* Pixel shader --p%d--", i);
      ShaderSource src = shBinDumpOwner().getCode(i, ShaderCodeType::PIXEL);
      dag::ConstSpan<uint32_t> code = uncompressShader(src, tmpbuf);
      if (details.assembly)
        disassembleShader(uncompressShader(src, tmpbuf));
      if (disasmTarget)
      {
        debug("%s",
          shader_blob_disasm::disassembleShaderBlob(
            dag::ConstSpan<uint8_t>{(uint8_t const *)code.data(), code.size() * elem_size(code)}, src.metadata, *disasmTarget)
            .c_str());
      }
    }
  }

  if (!details.stcode)
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
    struct Record
    {
      eastl::vector_set<eastl::string> classes{};
      eastl::vector_set<eastl::string> blocks{};
      bool isStatic = false;
    };
    eastl::vector<Record> recordsByStcode;
    recordsByStcode.resize(shBinDump().stcode.size());
    for (auto &sh_class : shBinDump().classes)
      for (auto &sh_code : sh_class.code)
        for (auto &pass : sh_code.passes)
        {
          if (pass.rpass->stcodeId != 0xFFFF)
          {
            G_ASSERT(!recordsByStcode[pass.rpass->stcodeId].isStatic);
            recordsByStcode[pass.rpass->stcodeId].classes.insert(eastl::string(sh_class.name.data()));
          }
          if (pass.rpass->stblkcodeId != 0xFFFF)
          {
            G_ASSERT(recordsByStcode[pass.rpass->stblkcodeId].blocks.empty());
            G_ASSERT(recordsByStcode[pass.rpass->stblkcodeId].isStatic || recordsByStcode[pass.rpass->stblkcodeId].classes.empty());
            recordsByStcode[pass.rpass->stblkcodeId].classes.insert(eastl::string(sh_class.name.data()));
            recordsByStcode[pass.rpass->stblkcodeId].isStatic = true;
          }
        }
    for (auto &sh_block : shBinDump().blocks)
      if (sh_block.stcodeId != -1 && sh_block.nameId != -1)
      {
        G_ASSERT(!recordsByStcode[sh_block.stcodeId].isStatic);
        recordsByStcode[sh_block.stcodeId].blocks.insert(eastl::string(shBinDump().blockNameMap[sh_block.nameId].c_str()));
      }

    for (int i = 0; i < shBinDump().stcode.size(); i++)
    {
      if (single_shader && recordsByStcode[i].classes.find(eastl::string(single_shader)) == recordsByStcode[i].classes.end())
        continue;

      debug("\n******* State code shader --s%d-- (%s)", i, recordsByStcode[i].isStatic ? "static" : "dynamic");
      if (shBinDump().stcode[i].size())
      {
        ShUtils::shcod_dump(shBinDump().stcode[i], &shBinDump().globVars, nullptr, nullptr, &shBinDump(), {}, false);
        if (!recordsByStcode[i].classes.empty())
        {
          debug("\nUsed by shaders:");
          for (const eastl::string &name : recordsByStcode[i].classes)
            debug("  %s", name);
        }
        if (!recordsByStcode[i].blocks.empty())
        {
          debug("\nUsed by blocks:");
          for (const eastl::string &name : recordsByStcode[i].blocks)
            debug("  %s", name);
        }
      }
    }
  }

  debug("--- END ---");
}
