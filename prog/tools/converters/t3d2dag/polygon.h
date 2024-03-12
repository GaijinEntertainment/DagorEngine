/***********      .---.         .-"-.      *******************\
* -------- *     /   ._.       / ´ ` \     * ---------------- *
* Author's *     \_  (__\      \_°v°_/     * humus@rogers.com *
*   note   *     //   \\       //   \\     * ICQ #47010716    *
* -------- *    ((     ))     ((     ))    * ---------------- *
*          ****--""---""-------""---""--****                  ********\
* This file is a part of the work done by Humus. You are free to use  *
* the code in any way you like, modified, unmodified or copy'n'pasted *
* into your own work. However, I expect you to respect these points:  *
*  @ If you use this file and its contents unmodified, or use a major *
*    part of this file, please credit the author and leave this note. *
*  @ For use in anything commercial, please request my approval.      *
*  @ Share your work and ideas too as much as you can.                *
\*********************************************************************/

#ifndef _POLYGON_H_
#define _POLYGON_H_

// #include "../Renderer.h"
#include "math/plane.h"

typedef unsigned int PolygonFlags;

#define PF_NONE        0
#define PF_DOUBLESIDED 0x1
#define PF_UNLIT       0x2
#define PF_TRANSLUCENT 0x4
#define PF_NONBLOCKING 0x8
#define PF_PORTAL      0x10
#define PF_ADD         0x11

class MaterialData;

class Polygon
{
protected:
  Vertex *vertices;
  unsigned int nVertices;

  Plane plane;
  Vertex origin, s, t;
  float scaleS, scaleT;

  PolygonFlags flags;
  MaterialData *material;

public:
  Polygon() {}
  Polygon(const int size);
  virtual ~Polygon();

  PolygonFlags getFlags() const { return flags; }
  const Plane &getPlane() const { return plane; }

  void setVertex(const unsigned int index, const Vertex &v) { vertices[index] = v; }
  void insertVertex(const unsigned int index, const Vertex &v);

  unsigned int getnVertices() const { return nVertices; }
  const Vertex &getVertex(const int index) const { return vertices[index]; }
  const Vertex &getLastVertex() const { return vertices[nVertices - 1]; }
  const Vertex *getVertices() const { return vertices; }

  void setFlags(const PolygonFlags Flags);
  void setFlags(const PolygonFlags Flags, bool value);

  void setTexCoordSystem(const Vertex &Origin, const Vertex &S, const Vertex &T);
  const Vertex &getOrigin() const { return origin; }
  const Vertex &getS() const { return s; }
  const Vertex &getT() const { return t; }
  float getScaleS() const { return scaleS; }
  float getScaleT() const { return scaleT; }

  void setMaterial(MaterialData *mat) { material = mat; }
  MaterialData *getMaterial() const { return material; }

  virtual class Polygon *create(const int size) const;
  virtual void initialize(class Polygon *poly);
  void copyVertex(Polygon *&poly, unsigned int destIndex, unsigned int srcIndex);
  void setInterpolatedVertex(Polygon *&poly, unsigned int destIndex, int srcIndex0, int srcIndex1, float k);
  void finalize();

  bool similar(Polygon *poly);

  RELATION polygonRelation(const Polygon &other);
  RELATION split(const Plane &splitPlane, Polygon *&back, Polygon *&front);
  bool linePassesThrough(const Vertex &v0, const Vertex &v1, Vertex *result = NULL);
};

class Polygon *polyCreator(const int size);

#endif // _POLYGON_H_
