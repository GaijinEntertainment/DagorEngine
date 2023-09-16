#include <startup/dag_restart.h>
#include <3d/dag_materialData.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <stdio.h>
#include <stdlib.h>

//==========================================================================//
Tab<String> all_names(midmem_ptr());

void analyzeMaterial(MaterialData *m, char *nname)
{
  if (m->className && (strstr(m->className, "hdr")))
  {
    for (int i = 0; i < MAXMATTEXNUM; ++i)
    {
      const char *tex_name = ::get_managed_texture_name(m->mtex[i]);
      if (tex_name)
      {
        int j;
        for (j = 0; j < all_names.size(); ++j)
        {
          if (stricmp(all_names[j], tex_name) == 0)
            break;
        }
        if (j >= all_names.size())
          all_names.push_back() = tex_name;
        printf("texture Name %s MaterialData=<%s> class=<%s> on node =<%s>\n", tex_name, (const char *)m->matName,
          (const char *)m->className, nname);
      }
    }
  }
}

void analyzeNode(class Node &node)
{
  if (!&node)
    return;
  // materials
  int num = 0;

  if (node.mat)
  {
    num = node.mat->subMatCount();
    if (!num)
      num = 1;
  }

  if (num)
  {
    for (int i = 0; i < num; ++i)
    {
      MaterialData *sm = node.mat->getSubMat(i);
      if (!sm)
        continue;

      analyzeMaterial((MaterialData *)sm, node.name);
    }
  }

  for (int i = 0; i < node.child.size(); ++i)
    analyzeNode(*node.child[i]);
}

int DagorWinMain(bool debugmode)
{
  startup_game(RESTART_ALL);

  fprintf(stderr, "\n HDR texture analyze Tool v0.01\n"
                  "Copyright (C) Gaijin Games KFT, 2023\n\n");

  if (__argc < 2)
  {
    fprintf(stderr, "usage analyze.exe dag");
    exit(0);
  }

  AScene sc;
  if (!::load_ascene(__argv[1], sc, LASF_MATNAMES | LASF_NULLMATS, false))
  {
    fprintf(stderr, "\nNot Done.\n");
    exit(0);
  }
  analyzeNode(*sc.root);
  printf("======================================================\n");
  for (int j = 0; j < all_names.size(); ++j)
    printf("texture <%s>\n", (char *)all_names[j]);

  fprintf(stderr, "\nDone.\n");

  return 0;
}
