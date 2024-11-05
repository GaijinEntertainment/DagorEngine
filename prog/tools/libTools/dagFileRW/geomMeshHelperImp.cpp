// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/dagFileRW/geomMeshHelper.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <math/dag_mesh.h>
#include <osApiWrappers/dag_direct.h>


static void importDagNode(Node &n, Tab<GeomMeshHelperDagObject> &objs)
{
  if (n.parent)
  {
    GeomMeshHelperDagObject &o = objs.push_back();
    o.name = n.name;
    o.wtm = n.wtm;

    if (n.obj && n.obj->isSubOf(OCID_MESHHOLDER))
      if (Mesh *m = ((MeshHolderObj *)n.obj)->mesh)
        o.mesh.init(*m);
  }

  for (int i = 0; i < n.child.size(); ++i)
    ::importDagNode(*n.child[i], objs);
}


bool import_geom_mesh_helpers_dag(const char *filename, Tab<GeomMeshHelperDagObject> &objects)
{
  if (!dd_file_exist(filename))
    return false;

  AScene sc;
  if (!load_ascene(filename, sc, LASF_NOMATS, false))
    return false;
  if (!sc.root)
    return false;

  sc.root->calc_wtm();

  ::importDagNode(*sc.root, objects);

  return true;
}
