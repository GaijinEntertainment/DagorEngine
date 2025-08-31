// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// ############################################################################
// ##                                                                        ##
// ##  Q3DEF.H                                                               ##
// ##                                                                        ##
// ##  Defines various Quake 3 BSP data structures.                          ##
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

#include "stl.h"
#include "stringdict.h"
#include <string.h>
#include "vector.h"
#include "rect.h"
#include "plane.h"
#include "vformat.h"

typedef std::vector<Plane> PlaneVector;

// Face types in Quake3
enum FaceType
{
  FACETYPE_NORMAL = 1,
  FACETYPE_MESH = 2,
  FACETYPE_TRISURF = 3,
  FACETYPE_FLARE = 4
};

// Reference to a type of shader.
class ShaderReference
{
public:
  ShaderReference(const unsigned char *mem)
  {
    memcpy(mName, mem, 64);
    memcpy(mUnknown, &mem[64], sizeof(int) * 2);
  }
  void GetTextureName(char *tname);

private:
  char mName[64];  // hard coded by this size, the shader name.
  int mUnknown[2]; // unknown 2 integer data in shader reference.
};

typedef std::vector<ShaderReference> ShaderReferenceVector;

/* BSP lumps in the order they appear in the header */
enum QuakeLumps
{
  Q3_ENTITIES = 0,
  Q3_SHADERREFS,
  Q3_PLANES,
  Q3_NODES,
  Q3_LEAFS,
  Q3_LFACES,
  Q3_LBRUSHES, // leaf brushes
  Q3_MODELS,
  Q3_BRUSHES,
  Q3_BRUSH_SIDES,
  Q3_VERTS,
  Q3_ELEMS,
  Q3_FOG,
  Q3_FACES,
  Q3_LIGHTMAPS,
  Q3_LIGHTGRID,
  Q3_VISIBILITY,
  NUM_LUMPS
};

class QuakeLump
{
public:
  int GetFileOffset(void) const { return mFileOffset; };
  int GetFileLength(void) const { return mFileLength; };

private:
  // Exactly comforms to raw data in Quake3 BSP file.
  int mFileOffset; // offset address of 'lump'
  int mFileLength; // file length of lump.
};

class QuakeHeader
{
public:
  bool SetHeader(const void *mem); // returns true if valid quake header.
  const void *LumpInfo(QuakeLumps lump, const void *mem, int &lsize, int &lcount);

private:
  // Exactly conforms to raw data in Quake3 BSP file.
  int mId;      // id number.
  int mVersion; // version number.
  QuakeLump mLumps[NUM_LUMPS];
};

class QuakeNode
{
public:
  QuakeNode(void) {}
  QuakeNode(const int *node);
  int GetLeftChild(void) const { return mLeftChild; };
  int GetRightChild(void) const { return mRightChild; };
  const Rect3d<float> &GetBound(void) const { return mBound; };
  int GetPlane(void) const { return mPlane; };

private:
  int mPlane;           // index to dividing plane.
  int mLeftChild;       // index to left child node.
  int mRightChild;      // index to right child node.
  Rect3d<float> mBound; // bounding box for node.
};

typedef std::vector<QuakeNode> QuakeNodeVector;

class QuakeLeaf
{
public:
  QuakeLeaf(void) {}
  QuakeLeaf(const int *leaf);
  const Rect3d<float> &GetBound(void) const { return mBound; };
  int GetCluster(void) const { return mCluster; };
  int GetFirstFace(void) const { return mFirstFace; };
  int GetFaceCount(void) const { return mFaceCount; };

private:
  int mCluster;         // which visibility cluster we are in.
  int mArea;            // unknown ?
  Rect3d<float> mBound; // bounding box for leaf node.
  int mFirstFace;       // index to first face.
  int mFaceCount;       // number of faces.
  int mFirstUnknown;    // unknown 'first' indicator.
  int mNumberUnknowns;  // number of unknown thingies.
};

typedef std::vector<QuakeLeaf> QuakeLeafVector;


class QuakeVertex
{
public:
  QuakeVertex(void) {}
  QuakeVertex(const int *vert);

  void Get(LightMapVertex &vtx) const;

  void Get(Vector3d<float> &p) const { p = mPos; };

  //**private:
  Vector3d<float> mPos;
  Vector2d<float> mTexel1;
  Vector2d<float> mTexel2;
  Vector3d<float> mNormal;
  unsigned int mColor;
};

typedef std::vector<QuakeVertex> QuakeVertexVector;

class QuakeModel
{
public:
  QuakeModel(void) {}
  QuakeModel(const int *mem);

private:
  Rect3d<float> mBound;
  int mFirstFace;
  int mFcount;
  int mFirstUnknown;
  int mUcount;
};

typedef std::vector<QuakeModel> QuakeModelVector;

class QuakeFace
{
public:
  QuakeFace(void) {}
  QuakeFace(const int *face);
  ~QuakeFace(void);

  void Build(const UShortVector &elements, const QuakeVertexVector &vertices, ShaderReferenceVector &shaders, const StringRef &name,
    const StringRef &code, VertexMesh &mesh);


private:
  int mFrameNo;
  unsigned int mShader; // 'shader' numer used by this face.
  int mUnknown;         // Unknown integer in the face specification.
  FaceType mType;       // type of face.
  int mFirstVertice;    // index into vertex list.
  int mVcount;          // number of vertices.
  int mFirstElement;    // start of indexed list
  int mEcount;          // number of elements.
  int mLightmap;        // lightmap index
  int mOffsetX;
  int mOffsetY;
  int mSizeX;
  int mSizeY;
  Vector3d<float> mOrig;
  Rect3d<float> mBound;
  Vector3d<float> mNormal;
  int mControlX;
  int mControlY;
};

typedef std::vector<QuakeFace> QuakeFaceVector;
