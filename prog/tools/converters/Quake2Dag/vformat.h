// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// ############################################################################
// ##                                                                        ##
// ##  VFORMAT.H                                                             ##
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


#include "vector.h"
#include "stringdict.h"
#include "rect.h"


class LightMapVertex
{
public:
  LightMapVertex(void){};

  LightMapVertex(float x, float y, float z, float u1, float v1, float u2, float v2)
  {
    mPos.Set(x, y, z);
    mTexel1.Set(u1, v1);
    mTexel2.Set(u2, v2);
  }


  void GetPos(Vector3d<float> &pos) const { pos = mPos; };
  const Vector3d<float> &GetPos(void) const { return mPos; };

  float GetX(void) const { return mPos.x; };
  float GetY(void) const { return mPos.y; };
  float GetZ(void) const { return mPos.z; };

  void Lerp(const LightMapVertex &a, const LightMapVertex &b, float p)
  {
    mPos.Lerp(a.mPos, b.mPos, p);
    mTexel1.Lerp(a.mTexel1, b.mTexel1, p);
    mTexel2.Lerp(a.mTexel2, b.mTexel2, p);
  };

  void Set(int index, const float *pos, const float *texel1, const float *texel2)
  {
    const float *p = &pos[index * 3];

    const float *tv1 = &texel1[index * 2];
    const float *tv2 = &texel2[index * 2];

    mPos.x = p[0];
    mPos.y = p[1];
    mPos.z = p[2];
    mTexel1.x = tv1[0];
    mTexel1.y = tv1[1];
    mTexel2.x = tv2[0];
    mTexel2.y = tv2[1];
  };

  Vector3d<float> mPos;
  Vector2d<float> mTexel1;
  Vector2d<float> mTexel2;
};


typedef std::vector<LightMapVertex> VertexVector;

class VertexLess
{
public:
  bool operator()(int v1, int v2) const;

  static void SetSearch(const LightMapVertex &match, VertexVector *list)
  {
    mFind = match;
    mList = list;
  };

private:
  const LightMapVertex &Get(int index) const
  {
    if (index == -1)
      return mFind;
    VertexVector &vlist = *mList;
    return vlist[index];
  }
  static LightMapVertex mFind; // vertice to locate.
  static VertexVector *mList;
};

typedef std::set<int, VertexLess> VertexSet;

class VertexPool
{
public:
  int GetVertex(const LightMapVertex &vtx)
  {
    VertexLess::SetSearch(vtx, &mVtxs);
    VertexSet::iterator found;
    found = mVertSet.find(-1);
    if (found != mVertSet.end())
    {
      return *found;
    }
    int idx = mVtxs.size();
    assert(idx >= 0 && idx < 65536);
    mVtxs.push_back(vtx);
    mVertSet.insert(idx);
    return idx;
  };

  void GetPos(int idx, Vector3d<float> &pos) const { pos = mVtxs[idx].mPos; }

  const LightMapVertex &Get(int idx) const { return mVtxs[idx]; };

  int GetSize(void) const { return mVtxs.size(); };

  void Clear(int reservesize) // clear the vertice pool.
  {
    mVertSet.clear();
    mVtxs.clear();
    mVtxs.reserve(reservesize);
  };

  const VertexVector &GetVertexList(void) const { return mVtxs; };

  void Set(const LightMapVertex &vtx) { mVtxs.push_back(vtx); }

  int GetVertexCount(void) const { return mVtxs.size(); };

  bool GetVertex(int i, Vector3d<float> &vect) const
  {
    vect = mVtxs[i].mPos;
    return true;
  };


  void SaveVRML(FILE *fph, bool tex1);

  VertexSet mVertSet; // ordered list.
  VertexVector mVtxs; // set of vertices.

private:
};


class VertexSection
{
public:
  VertexSection(const StringRef &name)
  {
    mName = name;
    mBound.InitMinMax();
  };

  void AddTri(const LightMapVertex &v1, const LightMapVertex &v2, const LightMapVertex &v3);


  void SaveVRML(FILE *fph, bool tex1);
  StringRef mName;
  VertexPool mPoints;
  UShortVector mIndices;

private:
  void AddPoint(const LightMapVertex &p);

  Rect3d<float> mBound;
};

typedef std::map<StringRef, VertexSection *> VertexSectionMap;

class VertexMesh
{
public:
  VertexMesh(void)
  {
    mLastSection = 0;
    mBound.InitMinMax();
  };

  ~VertexMesh(void);

  void AddTri(const StringRef &name, const LightMapVertex &v1, const LightMapVertex &v2, const LightMapVertex &v3);

  void SaveVRML(const StdString &name, // base file name
    bool tex1) const;                  // texture channel 1=(true)

  void saveDag(const StdString &name, const char *lm) const;

private:
  StringRef mLastName;
  VertexSection *mLastSection;
  VertexSectionMap mSections;
  Rect3d<float> mBound; // bounding region for whole mesh
};
