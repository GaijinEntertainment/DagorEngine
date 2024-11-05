// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <math/dag_math3d.h>
#include <math/dag_color.h>
#include <libTools/dagFileRW/dagFileExport.h>
#include <debug/dag_debug.h>
#include <generic/dag_tab.h>
#include <math/dag_mesh.h>
#include <osApiWrappers/dag_direct.h>

// ############################################################################
// ##                                                                        ##
// ##  VFORMAT.CPP                                                           ##
// ##                                                                        ##
// ##  Defines a LightMapVertex, which is a vertex with two U/V channels.    ##
// ##  Also defines a class to organize a triangle soup into an ordered mesh ##
// ##                                                                        ##
// ##  OpenSourced 12/5/2000 by John W. Ratcliff                             ##
// ##                                                                        ##
// ##  No warranty expressed or implied.                                     ##
// ##                                                                        ##
// ##  Part of the Q3BSP project, which converts a Quake 3 BSP file into a   ##
// ##  polygon mesh.                                                         ##
// ############################################################################
// ##                                                                        ##
// ##  Contact John W. Ratcliff at jratcliff@verant.com                      ##
// ############################################################################


#include "vformat.h"

LightMapVertex VertexLess::mFind;
VertexVector *VertexLess::mList = 0;

bool VertexLess::operator()(int v1, int v2) const
{

  const LightMapVertex &a = Get(v1);
  const LightMapVertex &b = Get(v2);

  if (a.GetX() < b.GetX())
    return true;
  if (a.GetX() > b.GetX())
    return false;

  if (a.GetY() < b.GetY())
    return true;
  if (a.GetY() > b.GetY())
    return false;

  if (a.GetZ() < b.GetZ())
    return true;
  if (a.GetZ() > b.GetZ())
    return false;

  if (a.mTexel1.x < b.mTexel1.x)
    return true;
  if (a.mTexel1.x > b.mTexel1.x)
    return false;

  if (a.mTexel1.y < b.mTexel1.y)
    return true;
  if (a.mTexel1.y > b.mTexel1.y)
    return false;

  if (a.mTexel2.x < b.mTexel2.x)
    return true;
  if (a.mTexel2.x > b.mTexel2.x)
    return false;

  if (a.mTexel2.y < b.mTexel2.y)
    return true;
  if (a.mTexel2.y > b.mTexel2.y)
    return false;


  return false;
};


VertexMesh::~VertexMesh(void)
{
  VertexSectionMap::iterator i;
  for (i = mSections.begin(); i != mSections.end(); ++i)
  {
    VertexSection *section = (*i).second;
    delete section;
  }
}


void VertexMesh::AddTri(const StringRef &name, const LightMapVertex &v1, const LightMapVertex &v2, const LightMapVertex &v3)
{
  VertexSection *section;

  if (name == mLastName)
  {
    section = mLastSection;
  }
  else
  {
    VertexSectionMap::iterator found;
    found = mSections.find(name);
    if (found != mSections.end())
    {
      section = mLastSection = (*found).second;
      mLastName = name;
    }
    else
    {
      mLastSection = section = new VertexSection(name);
      mSections[name] = section;
      mLastName = name;
    }
  }

  assert(section);

  section->AddTri(v1, v2, v3);

  mBound.MinMax(v1.mPos);
  mBound.MinMax(v2.mPos);
  mBound.MinMax(v3.mPos);
}


void VertexSection::AddTri(const LightMapVertex &v1, const LightMapVertex &v2, const LightMapVertex &v3)
{
  mBound.MinMax(v1.mPos);
  mBound.MinMax(v2.mPos);
  mBound.MinMax(v3.mPos);

  AddPoint(v1);
  AddPoint(v2);
  AddPoint(v3);
}


void VertexSection::AddPoint(const LightMapVertex &p)
{
  unsigned short idx = (unsigned short)mPoints.GetVertex(p);
  mIndices.push_back(idx);
};


/*void VertexMesh::SaveVRML(const StdString &name,  // base file name
              bool tex1) const           // texture channel 1=(true)
{
  if ( mSections.size() )
  {
    StdString oname = name+".wrl";
    FILE *fph = fopen(oname.c_str(),"wb");

    if ( fph )
    {
      fprintf(fph,"#VRML V1.0 ascii\n");
      fprintf(fph,"Separator {\n");
      fprintf(fph,"  ShapeHints {\n");
      fprintf(fph,"    shapeType SOLID\n");
      fprintf(fph,"    vertexOrdering COUNTERCLOCKWISE\n");
      fprintf(fph,"    faceType CONVEX\n");
      fprintf(fph,"  }\n");

      VertexSectionMap::const_iterator i;

      for (i=mSections.begin(); i!=mSections.end(); ++i)
      {
        (*i).second->SaveVRML(fph,tex1);
      }

      fprintf(fph,"}\n");

      fclose(fph);
    }
  }
}


void VertexSection::SaveVRML(FILE *fph,bool tex1)
{
  // save it into a VRML file!
  static int itemcount=1;

  fprintf(fph,"DEF item%d Separator {\n",itemcount++);
  fprintf(fph,"Translation { translation 0 0 0 }\n");
  fprintf(fph,"Material {\n");
  fprintf(fph,"  ambientColor 0.1791 0.06536 0.06536\n");
  fprintf(fph,"  diffuseColor 0.5373 0.1961 0.1961\n");
  fprintf(fph,"  specularColor 0.9 0.9 0.9\n");
  fprintf(fph,"  shininess 0.25\n");
  fprintf(fph,"  transparency 0\n");
  fprintf(fph,"}\n");
  fprintf(fph,"Texture2 {\n");


  const char *foo = mName;
  char scratch[256];

  if ( !tex1 )
  {
    while ( *foo && *foo != '+' ) foo++;
    char *dest = scratch;
    if ( *foo == '+' )
    {
      foo++;
      while ( *foo ) *dest++ = *foo++;
    }
    *dest = 0;
  }
  else
  {
    char *dest = scratch;
    while ( *foo && *foo != '+' ) *dest++ = *foo++;
    *dest = 0;
  }

  if ( tex1 )
    fprintf(fph,"  filename %c%s.tga%c\n",0x22,scratch,0x22);
  else
    fprintf(fph,"  filename %c%s.bmp%c\n",0x22,scratch,0x22);

  fprintf(fph,"}\n");

  mPoints.SaveVRML(fph,tex1);

  int tcount = mIndices.size()/3;

  if ( 1 )
  {
    fprintf(fph,"  IndexedFaceSet { coordIndex [\n");
    UShortVector::iterator j= mIndices.begin();
    for (int i=0; i<tcount; i++)
    {
      int i1 = *j;
      j++;
      int i2 = *j;
      j++;
      int i3 = *j;
      j++;
      if ( i == (tcount-1) )
        fprintf(fph,"  %d, %d, %d, -1]\n",i1,i2,i3);
      else
        fprintf(fph,"  %d, %d, %d, -1,\n",i1,i2,i3);
    }
  }

  if ( 1 )
  {
    fprintf(fph,"  textureCoordIndex [\n");
    UShortVector::iterator j= mIndices.begin();
    for (int i=0; i<tcount; i++)
    {
      int i1 = *j;
      j++;
      int i2 = *j;
      j++;
      int i3 = *j;
      j++;
      if ( i == (tcount-1) )
        fprintf(fph,"  %d, %d, %d, -1]\n",i1,i2,i3);
      else
        fprintf(fph,"  %d, %d, %d, -1,\n",i1,i2,i3);
    }
  }
  fprintf(fph,"  }\n");
  fprintf(fph,"}\n");
};


void VertexPool::SaveVRML(FILE *fph,bool tex1)
{
  if ( 1 )
  {
    fprintf(fph,"  Coordinate3 { point [\n");
    int count = mVtxs.size();
    for (int i=0; i<count; i++)
    {
      const LightMapVertex &vtx = mVtxs[i];
      if ( i == (count-1) )
        fprintf(fph,"  %f %f %f]\n",vtx.mPos.x,vtx.mPos.y,vtx.mPos.z);
      else
        fprintf(fph,"  %f %f %f,\n",vtx.mPos.x,vtx.mPos.y,vtx.mPos.z);
    }
    fprintf(fph,"   }\n");
  }
  if ( 1 )
  {
    fprintf(fph,"  TextureCoordinate2 { point [\n");
    int count = mVtxs.size();

    for (int i=0; i<count; i++)
    {
      const LightMapVertex &vtx = mVtxs[i];

      if ( !tex1 ) // if saving second U/V channel.
      {
        if ( i == (count-1) )
          fprintf(fph,"  %f %f]\n",vtx.mTexel2.x,1.0f-vtx.mTexel2.y);
        else
          fprintf(fph,"  %f %f,\n",vtx.mTexel2.x,1.0f-vtx.mTexel2.y);
      }
      else
      {
        if ( i == (count-1) )
          fprintf(fph,"  %f %f]\n",vtx.mTexel1.x,1.0f-vtx.mTexel1.y);
        else
          fprintf(fph,"  %f %f,\n",vtx.mTexel1.x,1.0f-vtx.mTexel1.y);
      }
    }

    fprintf(fph,"   }\n");
  }
}*/

void VertexMesh::saveDag(const StdString &name, const char *lm) const
{
  DagSaver dag;
  if (!dag.start_save_dag(name.c_str()))
  {
    printf("Could not save to file <%s>", name.c_str());
  }
  Tab<String> textures(tmpmem);
  Tab<String> textures_raw(tmpmem);

  VertexSectionMap::const_iterator i;
  for (i = mSections.begin(); i != mSections.end(); ++i)
  {
    char scratch[256];
    char scratch1[256];
    const char *foo = (*i).second->mName;

    char *dest = scratch;
    while (*foo && *foo != '+')
      *dest++ = *foo++;
    *dest = 0;
    dest = scratch1;
    if (*foo == '+')
    {
      foo++;
      while (*foo)
        *dest++ = *foo++;
    }
    *dest = 0;
    textures.push_back(String(128, "%s.tga", scratch));
    textures_raw.push_back(String(128, "%s", dd_get_fname(scratch)));
    if (lm)
      textures.push_back(String(128, "%s%s.bmp", lm, scratch1));
  }

  Tab<const char *> texCharNames(tmpmem);
  texCharNames.resize(textures.size());
  for (int tex = 0; tex < textures.size(); ++tex)
    texCharNames[tex] = textures[tex];
  dag.save_textures(texCharNames.size(), &texCharNames[0]);

  int mat = 0;
  for (i = mSections.begin(); i != mSections.end(); ++i, ++mat)
  {
    DagExpMater mater;
    int matid = mat;
    mater.name = String(128, "%s_%s", (const char *)(*i).second->mName, (const char *)textures_raw[matid]);
    if (lm)
      matid *= 2;
    mater.clsname = lm ? "simple_lm" : "simple";
    mater.script = lm ? "" : "lighting=lightmap";

    mater.amb = Color3(1, 1, 1);
    mater.diff = Color3(1, 1, 1);
    mater.spec = Color3(1, 1, 1);
    mater.emis = Color3(0, 0, 0);

    memset(mater.texid, 0xFF, sizeof(mater.texid));
    mater.texid[0] = matid;
    if (lm)
      mater.texid[6] = matid + 1;
    dag.save_mater(mater);
  }
  dag.start_save_nodes();
  int j = 0;
  for (i = mSections.begin(); i != mSections.end(); ++i)
  {
    unsigned flags = 0;
    flags |= DAG_NF_RENDERABLE;
    flags |= DAG_NF_CASTSHADOW;
    flags |= DAG_NF_RCVSHADOW;

    // TODO: add node script for backcompatibility (local,world,cast shadows on self, etc);

    if (dag.start_save_node(String(128, "%s", (const char *)(*i).second->mName), TMatrix::IDENT, flags))
    {
      uint16_t matId = j;
      dag.save_node_mats(1, &matId);
      DagBigMapChannel mapch[NUMMESHTCH];
      int mapChannels = 0;

      Tab<DagBigTFace> tface(tmpmem);
      int ii = 0;
      tface.resize((*i).second->mIndices.size() / 3);
      for (int f = 0; f < tface.size(); ++f)
      {
        tface[f].t[0] = (*i).second->mIndices[ii + 0];
        tface[f].t[1] = (*i).second->mIndices[ii + 2];
        tface[f].t[2] = (*i).second->mIndices[ii + 1];
        ii += 3;
      }
      Tab<Point2> tvert(tmpmem);
      tvert.resize((*i).second->mPoints.mVtxs.size());
      for (int t = 0; t < tvert.size(); ++t)
      {
        tvert[t].x = (*i).second->mPoints.mVtxs[t].mTexel2.x;
        tvert[t].y = 1.0f - (*i).second->mPoints.mVtxs[t].mTexel2.y;
      }
      Tab<Point2> tvert1(tmpmem);
      tvert1.resize((*i).second->mPoints.mVtxs.size());
      for (int t = 0; t < tvert1.size(); ++t)
      {
        tvert1[t].x = (*i).second->mPoints.mVtxs[t].mTexel1.x;
        tvert1[t].y = 1.0f - (*i).second->mPoints.mVtxs[t].mTexel1.y;
      }
      for (int m = 0; m < NUMMESHTCH; ++m)
      {
        if (!((*i).second->mPoints.mVtxs.size()))
          continue;

        mapch[mapChannels].numtv = (*i).second->mPoints.mVtxs.size();
        mapch[mapChannels].num_tv_coords = 2;
        mapch[mapChannels].channel_id = m + 1;
        mapch[mapChannels].tverts = (m && lm) ? &tvert[0][0] : &tvert1[0][0];

        mapch[mapChannels].tface = (DagBigTFace *)&tface[0];
        ++mapChannels;
      }

      Tab<DagBigFace> fc(tmpmem);
      fc.resize((*i).second->mIndices.size() / 3);

      ii = 0;
      for (int f = 0; f < fc.size(); ++f)
      {
        fc[f].v[0] = (*i).second->mIndices[ii + 0];
        fc[f].v[1] = (*i).second->mIndices[ii + 2];
        fc[f].v[2] = (*i).second->mIndices[ii + 1];
        fc[f].smgr = 0;
        fc[f].mat = 0;
        ii += 3;
      }

      // TODO: ad dnode script for backcompatibility
      // String nodeScript;
      // dag.save_node_script();

      Tab<Point3> vert(tmpmem);
      vert.resize((*i).second->mPoints.mVtxs.size());
      for (int v = 0; v < vert.size(); ++v)
      {
        vert[v].x = (*i).second->mPoints.mVtxs[v].mPos.x;
        vert[v].z = (*i).second->mPoints.mVtxs[v].mPos.y;
        vert[v].y = (*i).second->mPoints.mVtxs[v].mPos.z;
      }
      dag.save_dag_bigmesh(vert.size(), &vert[0], fc.size(), &fc[0], mapChannels, mapch);
      dag.end_save_node();
    }
    j++;
  }

  dag.end_save_nodes();
  dag.end_save_dag();
}
