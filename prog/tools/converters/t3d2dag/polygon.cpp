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

#include "polygon.h"

Polygon::Polygon(const int size)
{
  vertices = new Vertex[size];
  nVertices = size;
  flags = PF_NONE;
}

Polygon::~Polygon() { delete vertices; }

void Polygon::insertVertex(const unsigned int index, const Vertex &v)
{
  nVertices++;
  Vertex *newVertices = new Vertex[nVertices];
  unsigned int i;
  for (i = 0; i < index; i++)
  {
    newVertices[i] = vertices[i];
  }
  newVertices[index] = v;
  for (i = index + 1; i < nVertices; i++)
  {
    newVertices[i] = vertices[i - 1];
  }
  delete vertices;
  vertices = newVertices;
}

void Polygon::setFlags(const PolygonFlags Flags) { flags = Flags; }

void Polygon::setFlags(const PolygonFlags Flags, bool value)
{
  if (value)
  {
    flags |= Flags;
  }
  else
  {
    flags &= ~Flags;
  }
}

void Polygon::setTexCoordSystem(const Vertex &Origin, const Vertex &S, const Vertex &T)
{
  origin = Origin;
  s = S / (scaleS = length(S));
  t = T / (scaleT = length(T));
}

class Polygon *Polygon::create(const int size) const { return new class Polygon(size); }


void Polygon::initialize(class Polygon *poly)
{
  poly->material = material;
  poly->s = s;
  poly->t = t;
  poly->scaleS = scaleS;
  poly->scaleT = scaleT;
  poly->origin = origin;
  poly->flags = flags;
}

void Polygon::copyVertex(Polygon *&poly, unsigned int destIndex, unsigned int srcIndex)
{
  poly->vertices[destIndex] = vertices[srcIndex];
}

void Polygon::setInterpolatedVertex(Polygon *&poly, unsigned int destIndex, int srcIndex0, int srcIndex1, float k)
{
  poly->vertices[destIndex] = vertices[srcIndex0] + k * (vertices[srcIndex1] - vertices[srcIndex0]);
}

void Polygon::finalize() { plane = Plane(vertices[0], vertices[1], vertices[2]); }

bool Polygon::similar(Polygon *poly)
{
  if (nVertices == poly->nVertices)
  {
    for (unsigned int i = 0; i < nVertices; i++)
    {
      if (vertices[0] == poly->vertices[i])
      {
        int dir = 0;

        if (vertices[1] == poly->vertices[(i + 1) % nVertices])
        {
          dir = 1;
        }
        else if (vertices[nVertices - 1] == poly->vertices[(i + 1) % nVertices])
        {
          dir = -1;
        }

        if (dir)
        {
          unsigned int j, offset = i + nVertices;

          for (j = 0; j < nVertices; j++)
          {
            if (lengthSqr(vertices[j] - poly->vertices[offset % nVertices]) > 0.01f)
              break;
            offset--;
          }

          if (j == nVertices)
            return true;
        }
      }
    }
  }

  return false;
}

RELATION Polygon::polygonRelation(const Polygon &other)
{
  RELATION rel;
  unsigned int i, front, back;

  front = back = 0;

  for (i = 0; i < other.nVertices; i++)
  {
    rel = plane.getVertexRelation(other.vertices[i]);
    if (rel == FRONT)
      front++;
    if (rel == BACK)
      back++;
  }
  if (front > 0)
  {
    if (back > 0)
      return CUTS;
    else
      return FRONT;
  }
  else
  {
    if (back > 0)
      return BACK;
    else
      return PLANAR;
  }
}

RELATION Polygon::split(const Plane &splitPlane, Polygon *&back, Polygon *&front)
{
  RELATION rel, prev, rel0, rel1, relprev0;
  unsigned int i, j;
  int split0, split1, offset;
  unsigned int nBack, nFront;
  float k;

  // To make g++ happy ... not really needed
  rel0 = rel1 = relprev0 = FRONT;

  nFront = nBack = 0;
  split0 = -1, split1 = -1;

  prev = splitPlane.getVertexRelation(vertices[nVertices - 1]);

  for (i = 0; i < nVertices; i++)
  {
    rel = splitPlane.getVertexRelation(vertices[i]);

    if (rel == BACK)
      nBack++;
    if (rel == FRONT)
      nFront++;

    if (rel != prev)
    {
      if (prev != PLANAR)
      {
        if (split0 < 0)
        {
          split0 = i;
          rel0 = rel;
          relprev0 = prev;
        }
        else
        {
          split1 = i;
          rel1 = rel;
        }
      }
    }

    prev = rel;
  }

  if (nBack == 0)
  {
    back = front = NULL;
    if (nFront == 0)
    {
      return PLANAR;
    }
    else
    {
      return FRONT;
    }
  }

  if (nFront == 0)
  {
    back = front = NULL;
    return BACK;
  }

  if (relprev0 == FRONT)
  {
    int temp = nFront;
    nFront = nBack;
    nBack = temp;
  }

  back = create(nBack + 2);
  initialize(back);
  front = create(nFront + 2);
  initialize(front);


  offset = (rel0 == PLANAR) ? split0 + 1 : split0;
  for (i = 0; i < nFront; i++)
  {
    copyVertex(front, i, offset + i);
  }

  offset = (rel1 == PLANAR) ? split1 + 1 : split1;
  for (i = 0; i < nBack; i++)
  {
    copyVertex(back, i, (offset + i) % nVertices);
  }

  j = (split0 == 0) ? nVertices - 1 : split0 - 1;

  k = splitPlane.getPlaneHitInterpolationConstant(vertices[j], vertices[split0]);
  setInterpolatedVertex(front, nFront + 1, j, split0, k);
  setInterpolatedVertex(back, nBack, j, split0, k);

  k = splitPlane.getPlaneHitInterpolationConstant(vertices[split1 - 1], vertices[split1]);
  setInterpolatedVertex(front, nFront, split1 - 1, split1, k);
  setInterpolatedVertex(back, nBack + 1, split1 - 1, split1, k);

  back->finalize();
  front->finalize();

  if (relprev0 == FRONT)
  {
    Polygon *temp = back;
    back = front;
    front = temp;
  }


  return CUTS;
}

bool Polygon::linePassesThrough(const Vertex &v0, const Vertex &v1, Vertex *result)
{
  float k;
  Vertex p, g0, g1, g2; //,f;

  // Find interpolation constant
  k = plane.normal * (v0 - v1);
  if (k == 0.0f)
    return false;
  k = plane.distance(v0) / k;

  // If not within the points, then it doesn't even hit the plane
  if (k <= 0 || k >= 1)
    return false;

  // Find the point it hits the polygons plane, p is the actual point, f is relative first vertex
  p = v0 + k * (v1 - v0);
  // f = p - vertices[0];


  g1 = vertices[1] - vertices[0];
  for (unsigned int i = 2; i < nVertices; i++)
  {
    /*g0 = g1;
    g1 = vertices[i] - vertices[0];

    a = length(cross(g0,f)) + length(cross(g1,f)) + length(cross(g0 - g1, f - g1));
    area = length(cross(g0, g1));

    // There must be a better way than comparing areas ...
    // but it does at least work ...
    if (fabs(a - area) < 1){
      result = p;
      return true;
    }
    if (fabsf(a0 + a1 + a2 - 1) < 0.1f) return true;
    */


    // Solve the P = a0 * P0 + a1 * P1 + a2 * P2 equation
    g0 = vertices[0];
    g1 = vertices[i - 1];
    g2 = vertices[i];


    float a0, a1, a2, M[9];
    float invDetM =
      1.0f / (g0.x * (g1.y * g2.z - g1.z * g2.y) - g1.x * (g0.y * g2.z - g0.z * g2.y) + g2.x * (g0.y * g1.z - g0.z * g1.y));

    M[0] = (g1.y * g2.z - g1.z * g2.y) * invDetM;
    M[1] = -(g1.x * g2.z - g1.z * g2.x) * invDetM;
    M[2] = (g1.x * g2.y - g1.y * g2.x) * invDetM;

    M[3] = -(g0.y * g2.z - g0.z * g2.y) * invDetM;
    M[4] = (g0.x * g2.z - g0.z * g2.x) * invDetM;
    M[5] = -(g0.x * g2.y - g0.y * g2.x) * invDetM;

    M[6] = (g0.y * g1.z - g0.z * g1.y) * invDetM;
    M[7] = -(g0.x * g1.z - g0.z * g1.x) * invDetM;
    M[8] = (g0.x * g1.y - g0.y * g1.x) * invDetM;

    a0 = M[0] * p.x + M[1] * p.y + M[2] * p.z;
    a1 = M[3] * p.x + M[4] * p.y + M[5] * p.z;
    a2 = M[6] * p.x + M[7] * p.y + M[8] * p.z;


    /*		g0 = g1;
        g1 = vertices[i] - vertices[0];

        float a0,a1,a2;
        float invDetM, detM = g0.x * g1.y - g0.y * g1.x;
        if (detM != 0){
          invDetM = 1.0f / detM;
          a0 = (g1.y * f.x - g1.x * f.y) * invDetM;
          a1 = (g0.x * f.y - g0.y * f.x) * invDetM;
        } else {
          detM = g0.x * g1.z - g0.z * g1.x;
          invDetM = 1.0f / detM;
          if (detM != 0){
            a0 = (g1.z * f.x - g1.x * f.z) * invDetM;
            a1 = (g0.x * f.z - g0.z * f.x) * invDetM;
          } else {
            invDetM = 1.0f / (g0.y * g1.z - g0.z * g1.y);
            a0 = (g1.z * f.y - g1.y * f.z) * invDetM;
            a1 = (g0.y * f.z - g0.z * f.y) * invDetM;
          }
        }
        a2 = 1.0f - a0 - a1;
    */


    // if (a0 >= -0.01f && a0 <= 1.01f && a1 >= -0.01 && a1 <= 1.01f && a2 >= -0.01f && a2 <= 1.01f) return true;
    if (a0 >= 0 && a0 <= 1 && a1 >= 0 && a1 <= 1 && a2 >= 0 && a2 <= 1)
    {
      if (result != NULL)
        *result = p;
      return true;
    }
  }

  return false;
}

class Polygon *polyCreator(const int size) { return new class Polygon(size); }
