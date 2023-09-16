// Copyright 2006-2015 Tobias Sargeant (tobias.sargeant@gmail.com).
//
// This file is part of the Carve CSG Library (http://carve-csg.com/)
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <carve/csg.hpp>

#include "csg_detail.hpp"

struct carve::csg::detail::Data {
  //        * @param[out] vmap A mapping from vertex pointer to intersection
  //        point.
  //        * @param[out] emap A mapping from edge pointer to intersection
  //        points.
  //        * @param[out] fmap A mapping from face pointer to intersection
  //        points.
  //        * @param[out] fmap_rev A mapping from intersection points to face
  //        pointers.
  // map from intersected vertex to intersection point.
  VVMap vmap;

  // map from intersected edge to intersection points.
  EIntMap emap;

  // map from intersected face to intersection points.
  FVSMap fmap;

  // map from intersection point to intersected faces.
  VFSMap fmap_rev;

  // created by divideEdges().
  // holds, for each edge, an ordered vector of inserted vertices.
  EVVMap divided_edges;

  // created by faceSplitEdges.
  FV2SMap face_split_edges;

  // mapping from vertex to edge for potentially intersected
  // faces. Saves building the vertex to edge map for all faces of
  // both meshes.
  VEVecMap vert_to_edges;
};
