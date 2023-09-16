// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _GAIJIN_TOOLS_CONVERT_H
#define _GAIJIN_TOOLS_CONVERT_H
#pragma once

#include <generic/dag_tab.h>

template <class Face>
static void flipFaces(Face *face, int fcount)
{
  for (int i = 0; i < fcount; ++i)
  {
    unsigned *vp = (unsigned *)&face[i];
    unsigned v = vp[1];
    vp[1] = vp[2];
    vp[2] = v;
  }
}

static void invertV(Point2 *tvert, int tcount)
{
  for (int i = 0; i < tcount; ++i)
    tvert[i].y = 1 - tvert[i].y;
}

#pragma pack(push, 1)
struct DagBigFaceT
{
  unsigned int t[3];
  unsigned smgr;
  unsigned short mat;
};
#pragma pack(pop)

template <class Face, class Vert>
int optimize_verts(Face *tface, int fcount, Tab<Vert> &tvert, real eps, Tab<int> *degenerate = NULL)
{
  int i;
  Tab<int> nv(tmpmem), map(tmpmem);
  nv.resize(tvert.size());
  nv.resize(0);
  map.resize(tvert.size());
  char chg = 0;
  for (i = 0; i < tvert.size(); ++i)
  {
    int j;
    for (j = 0; j < fcount; ++j)
    {
      unsigned *vp = (unsigned *)&tface[j];
      if (vp[0] == i || vp[1] == i || vp[2] == i)
        break;
    }
    if (j >= fcount)
    {
      map[i] = 0;
      chg = 1;
      continue;
    }

    for (j = 0; j < nv.size(); ++j)
      if (lengthSq(tvert[nv[j]] - tvert[i]) < eps)
      {
        chg = 1;
        break;
      }

    if (j >= nv.size())
    {
      j = nv.size();
      nv.push_back(i);
    }

    map[i] = j;
  }
  if (chg)
  {
    for (i = 0; i < nv.size(); ++i)
      if (nv[i] != i)
        tvert[i] = tvert[nv[i]];
    tvert.resize(nv.size());
    tvert.shrink_to_fit();
    for (i = 0; i < fcount; ++i)
    {
      for (int v = 0; v < 3; ++v)
      {
        unsigned *vp = (unsigned *)&tface[i];
        vp[v] = map[vp[v]];
      }
      // tface[i].t[1]=map[tface[i].t[1]];
      // tface[i].t[2]=map[tface[i].t[2]];
    }
  }
  if (degenerate)
    for (i = fcount - 1; i >= 0; --i)
    {
      unsigned *vp = (unsigned *)&tface[i];
      if (vp[0] == vp[1] || vp[0] == vp[2] || vp[1] == vp[2])
      {
        degenerate->push_back(i);
      }
    }
  return 1;
}

#endif