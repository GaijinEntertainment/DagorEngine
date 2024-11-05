// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_dataBlock.h>
#include <scene/dag_loadLevelVer.h>
#include <generic/dag_smallTab.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

void __cdecl ctrl_break_handler(int) { quit_game(0); }

static void print_header()
{
  printf("SCNF tex replace utility v1.0\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");
}


int DagorWinMain(bool debugmode)
{
  if (__argc < 3)
  {
    print_header();
    printf("usage: scnUtil-dev.exe <input.scn> -exp:<tex_list.blk>\n"
           "       scnUtil-dev.exe <inout.scn> -imp:<tex_list.blk>\n\n");
    return -1;
  }
  signal(SIGINT, ctrl_break_handler);

  FullFileLoadCB crd(__argv[1]);
  if (!crd.fileHandle)
  {
    printf("ERROR: cannot open <%s>\n", __argv[1]);
    return 13;
  }
  if (crd.readInt() != _MAKE4C('scnf'))
  {
    printf("ERROR: 'scnf' lable not found in <%s>\n", __argv[1]);
    return 13;
  }

  Tab<String> tex_names(tmpmem);
  crd.beginBlock();
  tex_names.resize(crd.readInt());
  for (int i = 0; i < tex_names.size(); i++)
  {
    tex_names[i].resize(crd.readIntP<1>() + 1);
    crd.read(tex_names[i].data(), tex_names[i].size() - 1);
    tex_names[i][tex_names[i].size() - 1] = 0;
  }
  crd.endBlock();

  if (strnicmp(__argv[2], "-exp:", 5) == 0)
  {
    DataBlock blk;
    blk.setInt("count", tex_names.size());
    for (int i = 0; i < tex_names.size(); i++)
      blk.addStr(String(16, "tex%d", i), tex_names[i]);

    if (!blk.saveToTextFile(__argv[2] + 5))
    {
      printf("ERROR: cannot write <%s>\n", __argv[2] + 5);
      return 13;
    }
  }
  else if (strnicmp(__argv[2], "-imp:", 5) == 0)
  {
    DataBlock blk;
    if (!blk.load(__argv[2] + 5))
    {
      printf("ERROR: cannot write <%s>\n", __argv[2] + 5);
      return 13;
    }
    if (blk.getInt("count", 0) != tex_names.size())
    {
      printf("ERROR: inconsistent texture count: %d!=%d\n", blk.getInt("count", 0), tex_names.size());
      return 13;
    }
    int changed = 0;
    for (int i = 0; i < tex_names.size(); i++)
      if (!blk.getStr(String(16, "tex%d", i), NULL))
      {
        printf("ERROR: missing tex%d in <%s>\n", i, __argv[2] + 5);
        return 13;
      }
      else if (stricmp(blk.getStr(String(16, "tex%d", i), NULL), tex_names[i]) != 0)
        changed++;

    if (!changed)
    {
      printf("all %d texture names are the same, no writing needed\n", tex_names.size());
      return 0;
    }
    SmallTab<char, TmpmemAlloc> scn2_data;
    clear_and_resize(scn2_data, df_length(crd.fileHandle) - crd.tell());
    crd.read(scn2_data.data(), data_size(scn2_data));
    crd.close();

    FullFileSaveCB cwr(__argv[1]);
    if (!cwr.fileHandle)
    {
      printf("ERROR: cannot write <%s>\n", __argv[1]);
      return 13;
    }

    cwr.writeInt(_MAKE4C('scnf'));
    cwr.beginBlock();
    cwr.writeInt(tex_names.size());
    for (int i = 0; i < tex_names.size(); i++)
    {
      const char *tex = blk.getStr(String(16, "tex%d", i), NULL);
      int len = strlen(tex);
      G_ASSERT(len < 256);
      cwr.writeIntP<1>(len);
      cwr.write(tex, len);
    }
    while (cwr.tell() & 15)
      cwr.writeIntP<1>(0);
    cwr.endBlock();
    cwr.writeTabData(scn2_data);
    cwr.close();
    printf("changed %d of %d textures in %s\n", changed, tex_names.size(), __argv[1]);
  }
  else
  {
    printf("ERROR: unknown op <%s>\n", __argv[2]);
    return 13;
  }

  return 0;
}
