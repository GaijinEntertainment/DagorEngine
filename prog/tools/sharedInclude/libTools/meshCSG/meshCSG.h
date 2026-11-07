// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/daCSG/mesh_api.h>
#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <math/dag_TMatrix.h>
#include <util/dag_stdint.h>
#include <EASTL/string.h>

class Mesh;
class Bitarray;

namespace mesh_csg
{
enum class Op : uint8_t
{
  Union,
  Subtract,
  Intersect,
  Xor,
};

// Engine winding, as a .dag holds it; the caller keeps mesh and name alive for the call, the library copies the name
struct Input
{
  const Mesh *mesh = nullptr;
  TMatrix wtm = TMatrix::IDENT;
  const char *name = "";
};

struct Result
{
  static constexpr uint32_t NO_INPUT = ~0u;

  // after a false evaluate, faceInput and faceSource are empty and mesh is empty or the root mesh that failed validation
  // after a true one, mesh, faceInput and faceSource are empty when every input is refused or the result has no surface
  dacsg::Mesh mesh;                    // world space, daCSG winding; a face of an input keeps its source face's material id
  dag::Vector<uint32_t> faceInput;     // per face: input, a then b, or NO_INPUT for a face of no input
  dag::Vector<uint32_t> faceSource;    // per face: the face of that input, or NO_INPUT for a face of no input
  dag::Vector<eastl::string> refused;  // per input, a then b: why it is left out; empty: the input is accepted
  dag::Vector<eastl::string> warnings; // repairs and kernel fallbacks
  eastl::string error;                 // the reason of a false return
};

// (union of a) op (union of b), each outward shell of an input is an operand with its cavities;
// true: the graph executed on the accepted inputs
bool evaluate(dag::ConstSpan<Input> a, Op op, dag::ConstSpan<Input> b, Result &r);
// a minus b; false, the reason in r.error: the cut fails, or an input of a or a cutter of b before keep is refused,
// and r holds the evaluate without that input. A cutter from keep on that the library refuses is named in r.refused
// and leaves the call true. When the whole cut fails, the cutters from keep on go in one at a time;
// one that fails on top of those before it is refused with its failure and left out; r numbers the inputs a then b either way
bool subtract_in_turn(dag::ConstSpan<Input> a, dag::ConstSpan<Input> b, uint32_t keep, Result &r);
// dacsg::to_dag_mesh replaces out whole and always writes normals and smgr, in engine winding; face f of out is face f of r.mesh,
// so faceInput and faceSource index it; uv and color stay only where an accepted input had them; r of an evaluate that returned true
void to_engine_mesh(const Result &r, Mesh &out);
// face_count: input's mesh faces; a set bit keeps a face with a result piece; refused: all; accepted after a false evaluate: none
void surface_faces(const Result &r, uint32_t input, uint32_t face_count, Bitarray &used);
// keeps the faces that surface_faces marks for input, removes the rest; r of an evaluate that returned true
void keep_surface_faces(const Result &r, uint32_t input, Mesh &mesh);
} // namespace mesh_csg
