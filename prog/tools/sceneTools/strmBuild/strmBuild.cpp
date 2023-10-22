#include <scene/dag_loadLevel.h>
#include <shaders/dag_renderScene.h>
// #include <3d/dag_drv3d.h>
#include <startup/dag_startupTex.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_string.h>
#include <debug/dag_logSys.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

unsigned int lightmap_quality = 0;

void __cdecl ctrl_break_handler(int)
{
  printf("Cancelling...\n");
  quit_game(0);
}


static void showUsage()
{
  printf("\n"
         "Usage:\n"
         "  strmBuild-dev.exe [-debug] <streaming.ctrl.blk> <shaders_prefix> [baseDataFolder]\n");
}


int DagorWinMain(bool debugmode)
{
  printf("strmBuild v1.0\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");

  signal(SIGINT, ctrl_break_handler);

  if (__argc > 1)
  {
    if (stricmp(__argv[1], "-h") == 0 || stricmp(__argv[1], "-H") == 0 || stricmp(__argv[1], "/?") == 0)
    {
      showUsage();
      return 1;
    }
    else if (stricmp(__argv[1], "-debug") == 0)
    {
      memmove(&__argv[1], &__argv[2], sizeof(*__argv) * (__argc - 2));
      __argc--;
      start_classic_debug_system("debug");
    }
  }

  // get options
  if (__argc < 3)
  {
    showUsage();
    return 1;
  }

  const char *strm_blk_fn = __argv[1];
  const char *sh_file = __argv[2];
  const char *strm_base_folder = __argc > 3 ? __argv[3] : ".";
  DataBlock blk;
  if (!blk.load(strm_blk_fn))
  {
    printf("ERR: cannot load %s\n", strm_blk_fn);
    return 2;
  }
  // if (!::load_shaders_bindump(sh_file, FSHVER_30))
  // if (!::load_shaders_bindump(sh_file, FSHVER_20A))
  // if (!::load_shaders_bindump(sh_file, FSHVER_R300))
  //{
  //  printf("ERR: cannot load shaders: %s\n", sh_file);
  //  return 3;
  //}
  set_default_sym_texture_factory();
  // void* n = NULL;
  // d3d::init_video(NULL,NULL,NULL,0,n,NULL,NULL,NULL,NULL);

  for (int i = 0; i < blk.blockCount(); i++)
  {
    DataBlock &b = *blk.getBlock(i);
    const char *bin_fn = b.getStr("stream", NULL);
    if (bin_fn)
    {
      String strm(260, "%s/%s", strm_base_folder, bin_fn);

      class ScnExtractor : public IBinaryDumpLoaderClient
      {
      public:
        Tab<RenderScene *> scn;

        ScnExtractor() : scn(tmpmem) {}

        virtual bool bdlGetPhysicalWorldBbox(BBox3 &bbox) { return false; }
        virtual bool bdlConfirmLoading(unsigned, int tag) { return tag == _MAKE4C('SCN') || tag == _MAKE4C('DxP2'); }
        virtual void bdlTextureMap(unsigned, dag::ConstSpan<TEXTUREID> texId) {}
        virtual void bdlEnviLoaded(unsigned, RenderScene *envi) {}
        virtual void bdlSceneLoaded(unsigned, RenderScene *scene) { scn.push_back(scene); }
        virtual void bdlBinDumpLoaded(unsigned bindump_id) {}

        virtual bool bdlCustomLoad(unsigned, int, IGenLoad &, dag::ConstSpan<TEXTUREID>) { return true; }

        virtual const char *bdlGetBasePath() { return NULL; }
      };

      BBox3 box;
      box.setempty();

      ScnExtractor client;
      BinaryDump *bin_dump = load_binary_dump(strm, client);
      if (!bin_dump)
      {
        printf("ERR: cannot read bindump: %s\n", strm.str());
        return 4;
      }

      // calculate bounding box
      for (int j = 0; j < client.scn.size(); j++)
        for (int k = 0; k < client.scn[j]->obj.size(); k++)
        {
          BBox3 b2 = client.scn[j]->obj[k].bbox;
          b2.lim[0].y = 0;
          b2.lim[1].y = 0;
          box += b2;
        }

      unload_binary_dump(bin_dump, true);

      if (box.isempty())
      {
        b.removeParam("center");
        b.removeParam("rad");
      }
      else
      {
        printf("  %-30s sph.c=%9.3f,%9.3f,%9.3f   sph.r=%7.3f\n", bin_fn, P3D(box.center()), length(box.lim[1] - box.lim[0]) / 2);
        b.setPoint3("center", box.center());
        b.setReal("rad", length(box.lim[1] - box.lim[0]) / 2);
      }
    }
  }
  if (!blk.saveToTextFile(strm_blk_fn))
  {
    printf("ERR: cannot save streamg BLK: %s\n", sh_file);
    return 5;
  }

  return 0;
}

#define __UNLIMITED_BASE_PATH     1
#define __SUPPRESS_BLK_VALIDATION 1
#define __DEBUG_FILEPATH          NULL
#include <startup/dag_mainCon.inc.cpp>
