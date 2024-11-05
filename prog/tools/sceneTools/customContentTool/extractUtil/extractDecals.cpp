// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/util/conLogWriter.h>
#include <regExp/regExp.h>
#include <libTools/util/strUtil.h>
#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <assets/assetMsgPipe.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_findFiles.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <debug/dag_logSys.h>
#include <hash/md5.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#undef ERROR
#include <libTools/util/setupTexStreaming.h>

static ConsoleLogWriter conlog;

class ConsoleMsgPipe : public NullMsgPipe
{
public:
  virtual void onAssetMgrMessage(int msg_t, const char *msg, DagorAsset *a, const char *asset_src_fpath)
  {
    updateErrCount(msg_t);
    if (msg_t == REMARK)
      return;

    if (asset_src_fpath)
      conlog.addMessage((ILogWriter::MessageType)msg_t, "%s (file %s)", msg, asset_src_fpath);
    else if (a)
      conlog.addMessage((ILogWriter::MessageType)msg_t, "%s (file %s)", msg, a->getSrcFilePath());
    else
      conlog.addMessage((ILogWriter::MessageType)msg_t, msg);
  }
};

static void __cdecl ctrl_break_handler(int) { quit_game(0); }

static void print_title()
{
  printf("Extract decals tool v1.0\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");
}

int DagorWinMain(bool debugmode)
{
  DataBlock::fatalOnMissingFile = false;
  DataBlock::fatalOnLoadFailed = false;
  DataBlock::fatalOnBadVarType = false;
  DataBlock::fatalOnMissingVar = false;
  DataBlock::parseIncludesAsParams = false;

  signal(SIGINT, ctrl_break_handler);
  Tab<char *> argv(tmpmem);
  bool quiet = false;
  const char *out_list_fn = NULL;

  for (int i = 1; i < __argc; i++)
  {
    if (__argv[i][0] != '-')
      argv.push_back(__argv[i]);
    else if (stricmp(&__argv[i][1], "q") == 0)
      quiet = true;
    else
    {
      print_title();
      printf("ERR: unknown option <%s>\n", __argv[i]);
      return 1;
    }
  }

  if (argv.size() != 3)
  {
    print_title();
    printf("usage: extractDecals-dev.exe <app_dir> <decal_blk> <output_folder>\n");
    return -1;
  }

  if (!quiet)
    print_title();

  DataBlock decalsBlk;
  if (!decalsBlk.load(argv[1]))
  {
    printf("ERR: cannot load %s\n", argv[1]);
    return 13;
  }

  DataBlock appblk;
  DagorAssetMgr assetMgr;
  ConsoleMsgPipe pipe;
  Tab<String> base_prefix;

  {
    assetMgr.setMsgPipe(&pipe);

    String app_blk_fn(0, "%s/application.blk", argv[0]);
    if (!appblk.load(app_blk_fn))
    {
      printf("ERR: cannot load %s\n", app_blk_fn.str());
      return 13;
    }
    appblk.setStr("appDir", argv[0]);
    DataBlock::setRootIncludeResolver(argv[0]);
    ::load_tex_streaming_settings(app_blk_fn, NULL, true);

    // load asset base
    const DataBlock &blk = *appblk.getBlockByNameEx("assets");
    int base_nid = blk.getNameId("base");

    assetMgr.setupAllowedTypes(*blk.getBlockByNameEx("types"), blk.getBlockByName("export"));
    for (int i = 0; i < blk.paramCount(); i++)
      if (blk.getParamNameId(i) == base_nid && blk.getParamType(i) == DataBlock::TYPE_STRING)
      {
        base_prefix.push_back().printf(260, "%s/%s", argv[0], blk.getStr(i));
        dd_simplify_fname_c(base_prefix.back());
        assetMgr.loadAssetsBase(base_prefix.back(), "global");
      }
  }

  for (int i = 0; i < decalsBlk.blockCount(); i++)
  {
    DataBlock &blk = *decalsBlk.getBlock(i);

    String fn(0, "%s/%s.blk", argv[2], blk.getBlockName());

    String nm(blk.getBlockName());
    DagorAsset *a = assetMgr.findAsset(nm, assetMgr.getTexAssetTypeId());
    if (!a)
    {
      printf("ERR: missing tex asset <%s>\n", nm.str());
      continue;
    }
    SimpleString tex_fn(a->getTargetFilePath());
    bool found = false;
    for (int m = 0; m < base_prefix.size(); m++)
      if (strnicmp(tex_fn, base_prefix[m], strlen(base_prefix[m])) == 0)
      {
        blk.setStr("tex", tex_fn + strlen(base_prefix[m]));
        found = true;
        break;
      }
    if (!found)
    {
      printf("ERR: failed to resolve tex asset %s (%s)\n", tex_fn.str(), nm.str());
      continue;
    }

    dd_mkpath(fn);
    blk.saveToTextFile(fn);
    printf("  %s\n", fn.str());
  }

  return 0;
}
