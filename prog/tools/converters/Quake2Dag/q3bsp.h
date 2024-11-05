// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// ############################################################################
// ##                                                                        ##
// ##  Q3BSP.H                                                               ##
// ##                                                                        ##
// ##  Class to load Quake3 BSP files, interpret them, organize them into    ##
// ##  a triangle mesh, and save them out in various ASCII file formats.     ##
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

#include "stl.h"   // common STL definitions
#include "q3def.h" // include quake3 data structures.
#include "stringdict.h"
#include "vector.h"

// Loads a quake3 bsp file.
class Quake3BSP
{
public:
  Quake3BSP(const StringRef &fname, const StringRef &loc, const StringRef &code, bool lm);

  ~Quake3BSP(void);


  VertexMesh *GetVertexMesh(void) const { return mMesh; };


private:
  void ReadFaces(const void *mem);                           // load all faces (suraces) in the bsp
  void ReadVertices(const void *mem);                        // load all vertex data.
  void ReadLightmaps(const char *location, const void *mem); // load all lightmap data.
  void ReadElements(const void *mem);                        // load indices for indexed primitives
  void ReadShaders(const void *mem);                         // names of the 'shaders' (i.e. texture)

  void BuildVertexBuffers(void);

  bool mOk;                       // quake BSP properly loaded.
  StringRef mName;                // name of quake BSP
  StringRef mCodeName;            // short reference code for BSP
  QuakeHeader mHeader;            // header of quake BSP loaded.
  QuakeFaceVector mFaces;         // all faces
  QuakeVertexVector mVertices;    // all vertices.
  ShaderReferenceVector mShaders; // shader references
  UShortVector mElements;         // indices for draw primitives.
  Rect3d<float> mBound;
  VertexMesh *mMesh; // organized mesh
};
