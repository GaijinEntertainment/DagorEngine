// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <max.h>
#include <assert.h>


#define MakeRefByID(x, y, z) ReplaceReference(y, z)

#ifdef NDEBUG
#define verify(x) x
#else
#define verify(x) assert(x)
#endif

#if defined(MAX_RELEASE_R26) && MAX_RELEASE >= MAX_RELEASE_R26
inline BitArray &mesh_face_sel(Mesh &m) { return m.FaceSel(); }
inline BitArray &mesh_vert_sel(Mesh &m) { return m.VertSel(); }
#else
inline BitArray &mesh_face_sel(Mesh &m) { return m.faceSel; }
inline BitArray &mesh_vert_sel(Mesh &m) { return m.vertSel; }
#endif
