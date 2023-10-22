#pragma once

#include <libTools/staticGeomUi/nodeFlags.h>
#include <ioSys/dag_dataBlock.h>

static void readNorm(const DataBlock *blk, Point3 &normals_dir)
{
  normals_dir = normalize(blk->getPoint3("dir", Point3(0, 1, 0)));

  if (normals_dir.lengthSq() < 0.0001)
    normals_dir = Point3(0, 1, 0);
}

static unsigned readFlags(const DataBlock *blk, Point3 &normals_dir)
{
  if (blk)
  {
    unsigned flags = 0x00000000;

    if (blk->getBool("renderable", true))
      flags |= StaticGeometryNode::FLG_RENDERABLE;
    if (blk->getBool("collidable", true))
      flags |= StaticGeometryNode::FLG_COLLIDABLE;
    if (blk->getBool("cast_shadows", true))
      flags |= StaticGeometryNode::FLG_CASTSHADOWS;
    if (blk->getBool("cast_on_self", flags & StaticGeometryNode::FLG_CASTSHADOWS))
      flags |= StaticGeometryNode::FLG_CASTSHADOWS_ON_SELF;

    const char *normals = blk->getStr("normals", "");
    if (stricmp(normals, "local") == 0)
    {
      flags |= StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS;
      readNorm(blk, normals_dir);
    }
    else if (stricmp(normals, "world") == 0)
    {
      flags |= StaticGeometryNode::FLG_FORCE_WORLD_NORMALS;
      readNorm(blk, normals_dir);
    }
    else if (stricmp(normals, "spherical") == 0)
      flags |= StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS;
    return flags;
  }

  return StaticGeometryNode::FLG_RENDERABLE | StaticGeometryNode::FLG_COLLIDABLE | StaticGeometryNode::FLG_CASTSHADOWS |
         StaticGeometryNode::FLG_CASTSHADOWS_ON_SELF;
}
