// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>

class Mesh;

namespace dacsg
{
struct Mesh;

// Plus a .mtl beside it: one material per color of a face's first corner; s: a face's lowest smoothing group.
// false: a uv channel not of dim 2, a normal channel not of dim 3, a color channel not of dim 4, or a file it cannot write.
// m must hold the Mesh layout of mesh_api.h, as an execute result does: mesh_contract_breach checks it, nothing here does.
bool write_obj(const Mesh &m, const char *path, eastl::string &err);
// Reads v (x y z; more numbers, w or a color, ignored), vt (u, then v or 0 when absent; w ignored), f (3 or more corners fanned,
// each v, v/vt, v//vn or v/vt/vn, a negative index from the end; an index names a v, vt or vn read above it), usemtl (the material
// index by name) and s (1 to 32, 0 or off); a # starts a comment anywhere, a backslash at a line end continues the statement.
// vn, vp, p, l, the free-form statements, g, o, mg, mtllib, maplib, usemap, call, csh and the render settings are ignored,
// so the mesh has no normals and DEFAULT_COLOR; any other word, or a malformed statement, is refused with its line number.
// It reads through df_read; the internal read_obj_through of io/readSeam.h takes the reader per call, with no global state.
// It also refuses a well-formed file with a line or a joined statement over 16 MiB, a NUL or more than 65536 materials.
// false: m holds an empty Mesh.
bool read_obj(const char *path, Mesh &m, eastl::string &err);

// Both swap v1 and v2: engine winding is opposite. to_dag_mesh overwrites out whole: every table, extra too, is cleared first.
// uv: tvert[0], uvN: tvert[N-1], color: cvert, normal: vertnorm and facengr; others dropped. smgr: a bit per smooth
// region of two or more faces, 0 for a lone face, the least shared bit when the region's vertices touch all 32; no vertex
// splits, so from_dag_mesh gives the triangles back.
// m must hold the Mesh layout of mesh_api.h, as an execute result does: mesh_contract_breach checks it, nothing here does.
void to_dag_mesh(const Mesh &m, ::Mesh &out);
// false: a face has a vertex index out of range; a channel with an index out of range is dropped with a warning
bool from_dag_mesh(const ::Mesh &in, Mesh &m, eastl::string &err);
} // namespace dacsg
