// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/dagFileRW/dagExporter.h>
#include <libTools/dtx/dtx.h>
#include <image/dag_loadImage.h>
#include <image/dag_texPixel.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_oaHashNameMap.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include <libTools/util/fileUtils.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/dagFileRW/textureNameResolver.h>


struct ObjectData
{
  ObjectData() {}

public:
  TMatrix tm;
  SimpleString name;
};

static String res_base;
static DataBlock *texref_blk = NULL, *texslot_blk = NULL;
static DataBlock *obGroupsBlk = NULL;
static bool missing_added = false;
static FastNameMapEx texres_names;
static OAHashNameMap<true> fileNm;
static Point2 boxX(999999, 0), boxY(999999, 0);


static inline void addLandObject(const char *name, const TMatrix &tm, const char *type = NULL)
{
  if (!obGroupsBlk)
    return;

  DataBlock *objectBlk = obGroupsBlk->addNewBlock("node");

  objectBlk->setStr("name", name);
  if (type)
    objectBlk->setStr("type", type);
  objectBlk->setTm("tm", tm);
}

static int processName(const char *name)
{
  const int len = strlen(name);
  for (int i = len - 1; i > 0; i--)
    if (name[i] == '_')
      return i;

  return len;
}

static void processNode(Node &n)
{
  DataBlock nb;
  char name[260];
  _snprintf(name, 260, "%.*s", processName(n.name), n.name.str());

  TMatrix tm = n.wtm;
  Point3 sp(tm.getcol(1));
  tm.setcol(1, tm.getcol(2));
  tm.setcol(2, sp);

  sp = tm.getcol(3);
  if (sp.x < boxX.x)
    boxX.x = sp.x;
  if (sp.x > boxX.y)
    boxX.y = sp.x;

  if (sp.z < boxY.x)
    boxY.x = sp.z;
  if (sp.z > boxY.y)
    boxY.y = sp.z;

  const char *objName = name;
  const char *type = NULL;
  if (*objName == '*')
  {
    objName = "position";
    type = name + 1; // Skip '*'
  }

  if (*objName == '+')
  {
    if (n.obj && n.obj->isSubOf(OCID_MESHHOLDER))
    {
      const MeshHolderObj *curObject = static_cast<MeshHolderObj *>(n.obj);
      if (curObject->mesh)
      {
        BBox3 box;
        for (int i = 0; i < curObject->mesh->getVertCount(); ++i)
          box += Point3::xzy(curObject->mesh->getVert()[i]);
        tm.setcol(3, tm.getcol(3) + box.center());
        Point3 boxWidth = box.width();
        tm.setcol(0, normalize(tm.getcol(0)) * boxWidth.x * 0.5f);
        tm.setcol(1, normalize(tm.getcol(1)) * boxWidth.y * 0.5f);
        tm.setcol(2, normalize(tm.getcol(2)) * boxWidth.z * 0.5f);
      }
    }
  }

  addLandObject(objName, tm, type);

  n.script = NULL;
}

static int getFilePath(const char *file)
{
  for (int i = strlen(file) - 1; i > 0; i--)
    if ((file[i] == '\\') || (file[i] == '/'))
      return i;
  return 0;
}

static bool processDag(const char *fn, const char *base_path)
{
  AScene scene;

  load_ascene(fn, scene);
  if (!scene.root)
    return false;
  scene.root->calc_wtm();

  scene.root->invalidate_wtm();
  scene.root->calc_wtm();

  // process nodes
  for (int i = 0; i < scene.root->child.size(); i++)
    processNode(*scene.root->child[i]);

  return true;
}

bool generate_res_db(const char *src_dag_file, const char *land_class_file)
{
  char base_path[260];
  dd_get_fname_location(base_path, src_dag_file);
  dd_simplify_fname_c(base_path);
  dd_append_slash_c(base_path);

  DataBlock objectsGroupBlk;
  obGroupsBlk = &objectsGroupBlk;
  processDag(src_dag_file, base_path);
  objectsGroupBlk.saveToTextFile(land_class_file);
  obGroupsBlk = NULL;

  return true;
}
