// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define __UNLIMITED_BASE_PATH     1
#define __SUPPRESS_BLK_VALIDATION 1
#include <startup/dag_mainCon.inc.cpp>

#include <libTools/util/strUtil.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/dagFileRW/textureNameResolver.h>
#include <debug/dag_debug.h>
#include <stdio.h>


static void initDagor(const char *base_path);

class PassTextureNameResolver : public ITextureNameResolver
{
public:
  virtual bool resolveTextureName(const char *src_name, String &out_str) { return false; }
};

int DagorWinMain(bool debugmode)
{
  int argc = __argc;
  char **argv = __argv;

  if (argc < 3)
  {
    printf("RI merge utility\n");
    printf("Copyright (c) Gaijin Games KFT, 2023\n");
    printf("Usage: rimerge <in_rendinst.lod00.dag> <in_collision.dag> [out_rendinst.lod00.dag]\n");
    return -1;
  }

  PassTextureNameResolver resolve;
  set_global_tex_name_resolver(&resolve);

  StaticGeometryContainer vis_cont, coll_cont;
  if (!vis_cont.loadFromDag(argv[1], NULL, true))
  {
    printf("error: couldn't load rendinst DAG file \"%s\"\n", argv[1]);
    return -1;
  }
  if (!coll_cont.loadFromDag(argv[2], NULL, true))
  {
    printf("error: couldn't load collision DAG file \"%s\"\n", argv[2]);
    return -1;
  }

  for (int i = vis_cont.nodes.size() - 1; i >= 0; i--)
  {
    vis_cont.nodes[i]->flags &= ~StaticGeometryNode::FLG_COLLIDABLE;
    vis_cont.nodes[i]->script.removeParam("collision");
    vis_cont.nodes[i]->script.removeParam("collidable");
  }
  for (int i = coll_cont.nodes.size() - 1; i >= 0; i--)
  {
    coll_cont.nodes[i]->flags &= ~StaticGeometryNode::FLG_RENDERABLE;
    coll_cont.nodes[i]->script.removeParam("renderable");
    if (coll_cont.nodes[i]->flags & StaticGeometryNode::FLG_COLLIDABLE)
    {
      vis_cont.addNode(coll_cont.nodes[i]);
      erase_items(coll_cont.nodes, i, 1);
    }
  }
  vis_cont.optimize(true, true);
  vis_cont.exportDag(argc > 3 ? argv[3] : argv[1], false);

  return 0;
}
