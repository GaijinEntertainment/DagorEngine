#include "func.h"

#include <libTools/util/fileUtils.h>
#include <libTools/util/strUtil.h>

#include <stdio.h>
#include <stdlib.h>


#define MIN_ARG_COUNT 3


//==================================================================================================
#include <osApiWrappers/dag_direct.h>
int _cdecl main(int argc, char **argv)
{
  dd_get_fname(""); //== pull in directoryService.obj
  printf("Quake map to DagorEditor Prefabs convertion tool\n");
  printf("Copyright (c) Gaijin Games KFT, 2023\n");

  if (argc < MIN_ARG_COUNT)
  {
    ::show_usage();
    return -1;
  }

  bool showDebug = false;
  bool checkModel = true;
  String savePath;

  for (int i = MIN_ARG_COUNT; i < argc; ++i)
  {
    if (*argv[i] == '-')
    {
      switch (argv[i][1])
      {
        case 'd':
        case 'D': showDebug = true; break;

        case 'c':
        case 'C': checkModel = false; break;
      }
    }
    else
      savePath = argv[i];
  }

  ::init_dagor(argv[0]);

  String map;
  if (!::dag_read_file_to_mem(argv[1], map))
  {
    printf("error: couldn't open map file \"%s\"\n", argv[1]);
    return -1;
  }

  Tab<Prefab> prefabs(tmpmem);
  if (!::create_prefabs_list(map, prefabs, checkModel))
  {
    printf("error: couldn't analyze map file \"%s\"\n", argv[1]);
    return -1;
  }

  if (savePath.length())
    savePath = ::make_full_path(savePath, "staticobjects.plugin.blk");
  else
  {
    savePath = argv[1];
    ::location_from_path(savePath);
    savePath = ::make_full_path(savePath, "staticobjects.plugin.blk");
  }

  if (!::generate_file(savePath, prefabs, argv[2], showDebug))
  {
    printf("error: couldn't generate file \"%s\"\n", (const char *)savePath);
    return -1;
  }

  return 0;
}
