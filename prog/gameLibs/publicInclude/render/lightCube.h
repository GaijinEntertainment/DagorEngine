//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>

class BaseTexture;
typedef BaseTexture CubeTexture;
struct Color4;

namespace light_probe
{
class Cube;
void init();  // global inti
void close(); // global close

Cube *create(); // name can be null

void destroy(Cube *);
bool init(Cube *a, const char *name, int spec_size, unsigned fmt);
bool update(Cube *h, CubeTexture *cubTex, int face_start = 0, int face_count = 6, int from_mip = 0, int mips_count = 12);
const ManagedTex *getManagedTex(Cube *h);
const Color4 *getSphHarm(Cube *h);
bool calcDiffuse(Cube *h, CubeTexture *cubTex, float gamma, float scale, bool force_recalc);
bool hasNans(Cube *h);

inline Cube *create(const char *name, int spec_size, unsigned fmt) // name can be null
{
  Cube *cube = create();
  if (!init(cube, name, spec_size, fmt))
  {
    destroy(cube);
    return NULL;
  }
  return cube;
}

}; // namespace light_probe
