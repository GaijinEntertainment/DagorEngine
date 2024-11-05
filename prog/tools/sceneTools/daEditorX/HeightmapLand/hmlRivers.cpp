// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlPlugin.h"
#include <EditorCore/ec_IEditorCore.h>
#include <libTools/staticGeom/geomObject.h>

using editorcore_extapi::dagGeom;

void HmapLandPlugin::rebuildRivers()
{
  GeomObject *obj = dagGeom->newGeomObject(midmem);

  gatherStaticGeometry(*obj->getGeometryContainer(), StaticGeometryNode::FLG_RENDERABLE, false, 1);
  obj->notChangeVertexColors(true);
  dagGeom->geomObjectRecompile(*obj);
  obj->notChangeVertexColors(false);

  //== do something with geom object (render, etc.)


  dagGeom->deleteGeomObject(obj);
}
