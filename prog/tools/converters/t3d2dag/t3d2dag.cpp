#include <math/dag_math3d.h>

#include "t3dLoader.h"
#include <converters/convert.h>


#include <math/dag_mesh.h>
#include <math/dag_math3d.h>
#include <math/dag_color.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_tab.h>
#include <util/dag_bitArray.h>
#include <libTools/dagFileRW/dagFileExport.h>
#include <libTools/util/strUtil.h>
#include <startup/dag_restart.h>
#include <string.h>
#include <stdio.h>

class Polygon *polygonCreator(const int size) { return new Polygon(size); }

static bool polygonFilter(const int flags)
{
  if (flags & PF_Invisible)
    return false;
  return true;
}
static Point3 point3(const Vertex &v) { return Point3(v.x, v.y, v.z); }

static void polys2mesh(Set<Polygon *> &polygons, Mesh &mesh)
{
  BBox3 box;
  for (int i = 0; i < polygons.getSize(); ++i)
  {
    for (int j = 0; j < polygons[i]->getnVertices(); ++j)
    {
      Point2 tv;
      tv.x = point3(polygons[i]->getS() * polygons[i]->getScaleS()) * point3(polygons[i]->getVertex(j) - polygons[i]->getOrigin());
      tv.y = point3(polygons[i]->getT() * polygons[i]->getScaleT()) * point3(polygons[i]->getVertex(j) - polygons[i]->getOrigin());
      mesh.vert.push_back(point3(polygons[i]->getVertex(j)));
      mesh.tvert[0].push_back(tv);
      box += point3(polygons[i]->getVertex(j));
    }
    for (int j = 0; j < polygons[i]->getnVertices() - 2; ++j)
    {
      int fi = append_items(mesh.face, 1);
      mesh.face[fi].smgr = 1;
      mesh.face[fi].mat = 0;
      mesh.face[fi].v[0] = mesh.vert.size() - (polygons[i]->getnVertices()) + j;
      mesh.face[fi].v[polygons[i]->getFlags() & PF_ADD ? 2 : 1] = mesh.vert.size() - (polygons[i]->getnVertices()) + j + 1;
      mesh.face[fi].v[polygons[i]->getFlags() & PF_ADD ? 1 : 2] = mesh.vert.size() - 1;

      fi = append_items(mesh.tface[0], 1);
      mesh.tface[0][fi].t[0] = mesh.tvert[0].size() - (polygons[i]->getnVertices()) + j;
      mesh.tface[0][fi].t[polygons[i]->getFlags() & PF_ADD ? 2 : 1] = mesh.tvert[0].size() - (polygons[i]->getnVertices()) + j + 1;
      mesh.tface[0][fi].t[polygons[i]->getFlags() & PF_ADD ? 1 : 2] = mesh.tvert[0].size() - 1;
    }
  }
  Tab<int> degenerates(tmpmem);
  // G_ASSERT(sizeof(FaceT) == elem_size(mesh.face));
  optimize_verts(&mesh.face[0], mesh.face.size(), mesh.vert, length(box.width()) * 0.0005, &degenerates);
  optimize_verts(&mesh.tface[0][0], mesh.tface[0].size(), mesh.tvert[0], 0.0001);
  invertV(&mesh.tvert[0][0], mesh.tvert[0].size());

  for (int di = 0; di < degenerates.size(); ++di)
  {
    erase_items(mesh.face, degenerates[di], 1);
    erase_items(mesh.tface[0], degenerates[di], 1);
  }

  flipFaces(&mesh.face[0], mesh.face.size());
  flipFaces(&mesh.tface[0][0], mesh.tface[0].size());
}

int DagorWinMain(bool debugmode)
{
  startup_game(RESTART_ALL);

  fprintf(stderr, "\n T3D to Dag Tool v0.01\n"
                  "Copyright (C) Gaijin Games KFT, 2023\n\n");

  String input[3];
  input[2] = "./";
  input[1] = "./";
  int inp = 0;

  for (int i = 1; i < __argc; ++i)
  {
    const char *s = __argv[i];

    if (*s == '-' || *s == '/')
    {
      ++s;
      // options
    }
    else
    {
      input[inp++] = s;
      if (inp >= 3)
        break;
    }
  }
  if (inp < 1)
  {
    fprintf(stderr, "Usage: t3d2dag file.t3d [file.dag] [/glue]\n");
    exit(1);
  }
  if (inp < 2)
    input[1] = input[0] + ".dag";
  bool glue = false;
  if ((__argc == 3 && strcmp(__argv[2], "/glue") == NULL) || (__argc == 2 && strcmp(__argv[1], "/glue") == NULL))
    glue = true;

  T3dLoader loader;
  Set<Polygon *> polygons;
  MaterialTable materialTable;
  Vertex displacement(0, 0, 0);
  loader.loadFromFile(input[0], polygons, materialTable, polyCreator, polygonFilter, displacement);
  fprintf(stderr, "Note: %d materials found.\n", materialTable.materialNames.getSize());
  fprintf(stderr, "Note: %d polygons loaded.\n", polygons.getSize());

  const int meshCount = glue ? 1 : materialTable.materialNames.getSize();
  Tab<Mesh> mesh(tmpmem);

  fprintf(stderr, "Note: %d materials found.\n", materialTable.materialNames.getSize());
  mesh.resize(meshCount + 1);
  if (glue)
  {
    polys2mesh(polygons, mesh[0]);
    fprintf(stderr, "Note: %d faces converted.\n", mesh[0].face.size());
  }
  else
    for (int i = 0; i < meshCount; i++)
    {
      Set<Polygon *> ps;
      for (int j = 0; j < polygons.getSize(); j++)
      {
        if (polygons[j]->getMaterial() && polygons[j]->getMaterial() == materialTable.materialNames[i]->material)
          ps.add(polygons[j]);
      }
      polys2mesh(ps, mesh[i]);
      fprintf(stderr, "%d faces from %d polygons to mesh #%i with mat '%s' converted.\n", mesh[i].face.size(), ps.getSize(), i,
        materialTable.materialNames[i]->name);
    }
  {
    Set<Polygon *> ps;
    for (int j = 0; j < polygons.getSize(); j++)
    {
      bool found = false;
      if (!glue)
      {
        for (int i = 0; i < meshCount; i++)
          if (polygons[j]->getMaterial() && polygons[j]->getMaterial() == materialTable.materialNames[i]->material)
            found = true;
      }
      polygons[j]->getMaterial()->name;
      if (!found)
        ps.add(polygons[j]);
    }
    polys2mesh(ps, mesh[meshCount]);
    fprintf(stderr, "Warning: %d faces from %d polygons to mesh wo mat converted.\n", mesh[meshCount].face.size(), ps.getSize());
  }
  fprintf(stderr, "\nNote: look debug for textures to decode by utpt16beta3");

  DagSaver sav;
  if (!sav.start_save_dag(input[1]))
    return false;
  try
  {
    Tab<const char *> texnames(tmpmem);
    texnames.resize(meshCount);
    for (int t = 0; t < meshCount; ++t)
    {
      char names[3][512];
      int cnm = 0, cind = 0;
      for (int i = 0; i <= strlen(materialTable.materialNames[t]->name); ++i)
      {
        names[cnm][cind++] = materialTable.materialNames[t]->name[i];
        if (materialTable.materialNames[t]->name[i] == '\0')
          break;
        if (materialTable.materialNames[t]->name[i] == '.')
        {
          names[cnm][cind - 1] = 0;
          cind = 0;
          cnm++;
        }
      }
      texnames[t] = strdup((char *)String(128, "./%s.utx-%s.%s-0.bmp", names[0], names[1], names[2]));
    }

    if (!sav.save_textures(texnames.size(), texnames.data()))
      throw 0;
    for (int i = 0; i < meshCount + 1; i++)
    {
      DagExpMater mater;
      mater.name = "mat";
      mater.clsname = "simple";
      mater.script = ""; // materialTable.materialNames[t]->material->script;
      mater.amb = Color3(0.9, 0.9, 0.9);
      mater.diff = Color3(0.9, 0.9, 0.9);
      mater.spec = Color3(0.9, 0.9, 0.9);
      // mater.emis = color3(material->emis);0.25
      memset(mater.texid, 0xFF, sizeof(mater.texid));
      for (int t = 0; t < DAGTEXNUM; ++t)
        mater.texid[t] = (t || i >= meshCount) ? DAGBADMATID : i;
      sav.save_mater(mater);
    }
    if (!sav.start_save_nodes())
      throw 0;

    for (int i = 0; i < meshCount + 1; i++)
    {
      TMatrix tm = TMatrix::IDENT;
      tm.setcol(0, tm.getcol(0) * 0.01);
      tm.setcol(1, tm.getcol(1) * 0.01);
      tm.setcol(2, tm.getcol(2) * 0.01);
      if (!sav.start_save_node("root", tm))
        throw 0;
      unsigned short matId = i;
      if (!sav.save_node_mats(1, &matId))
        throw 0;

      DagBigMapChannel ch[1];
      ch[0].tverts = &mesh[i].tvert[0][0].x;
      ch[0].tface = (DagBigTFace *)&mesh[i].tface[0][0];
      ch[0].numtv = mesh[i].tvert[0].size();
      ch[0].num_tv_coords = 2;
      ch[0].channel_id = 1;
      Tab<DagBigFace> fc(tmpmem);
      fc.resize(mesh[i].face.size());
      for (int f = 0; f < mesh[i].face.size(); ++f)
      {
        fc[f].mat = mesh[i].face[f].mat;
        fc[f].smgr = mesh[i].face[f].smgr;
        fc[f].v[0] = mesh[i].face[f].v[0];
        fc[f].v[1] = mesh[i].face[f].v[1];
        fc[f].v[2] = mesh[i].face[f].v[2];
      }
      if (!sav.save_dag_bigmesh(mesh[i].vert.size(), &mesh[i].vert[0], fc.size(), &fc[0], sizeof(ch) / sizeof(DagBigMapChannel), ch))
        throw 0;

      if (!sav.end_save_node())
        throw 0;
    }
    if (!sav.end_save_nodes())
      throw 0;
  }
  catch (int)
  {
    sav.end_save_dag();
    return false;
  }
  return sav.end_save_dag();

  fprintf(stderr, "\nDone.\n");

  return 0;
}
