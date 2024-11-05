// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// ############################################################################
// ##                                                                        ##
// ##  PATCH.H                                                               ##
// ##                                                                        ##
// ##  Converts a Quake3 Bezier patch into a hard coded polygon mesh.        ##
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

class PatchSurface
{
public:
  PatchSurface(const LightMapVertex *control_points, int npoints, int controlx, int controly);
  ~PatchSurface(void);

  const LightMapVertex *GetVerts(void) const { return mPoints; };
  const unsigned short *GetIndices(void) const { return mIndices; };
  int GetIndiceCount(void) const { return mCount; };

private:
  bool FindSize(int controlx, int controly, const LightMapVertex *cp, int &sizex, int &sizey) const;
  void FillPatch(int controlx, int controly, int sizex, int sizey, LightMapVertex *points);
  void FillCurve(int numcp, int size, int stride, LightMapVertex *p);

  int FindLevel(const Vector3d<float> &cv0, const Vector3d<float> &cv1, const Vector3d<float> &cv2) const;

  int mCount;
  LightMapVertex *mPoints;  // vertices produced.
  unsigned short *mIndices; // indices into those vertices
};
