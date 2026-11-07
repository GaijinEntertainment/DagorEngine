// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <util/dag_bitArray.h>
#include <math/dag_e3dColor.h>

class CollisionResource;

struct NodeMaterialFaces
{
  int matId;
  uint32_t faces;
};

// Which phys materials a loaded collision resource holds, per node and as a union, with face counts.
// A node without faces (a primitive, or a mesh the load dropped) lists its one material with 0 faces.
class PhysMatLegend
{
public:
  void build(const CollisionResource &res);
  void clear();
  dag::ConstSpan<NodeMaterialFaces> nodeMaterials(int node_id) const;
  dag::ConstSpan<NodeMaterialFaces> resourceMaterials() const { return materialRows; }

  // Filter: one set of checked materials for the whole resource; build() checks every material.
  bool isVisible(int mat_id) const;
  void setRowVisible(int row, bool v);
  void setAllVisible(bool v);
  bool hasVisibleGeometry(int node_id) const; // some material of the node is checked

  // Not authored: the bf_class picks the hue, or the head of the material name when it declares none, and the material
  // picks one of the four shades of that hue. build() gives one hue to one cluster of the resource, so two shades of a
  // hue mean one family while no cluster there names more than four materials; the fifth one has to leave its hue.
  // Bright red is reserved for a face with no material (PHYSMAT_INVALID or PHYSMAT_DEFAULT).
  E3DCOLOR colorOf(int mat_id) const;
  static const char *nameOf(int mat_id); // "none" for PHYSMAT_INVALID or an unknown id

private:
  void assignColors();

  dag::Vector<dag::Vector<NodeMaterialFaces>> perNodeMaterials; // sorted by name
  dag::Vector<NodeMaterialFaces> materialRows;                  // sorted by name
  dag::Vector<E3DCOLOR> matColor;                               // by mat_id + 1, as matVisible
  Bitarray matVisible;                                          // by mat_id + 1, so PHYSMAT_INVALID sits at 0
};
