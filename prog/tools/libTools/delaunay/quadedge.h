#pragma once

#include "geom.h"

namespace delaunay
{

class Triangle;

class Edge : public Labelled
{
private:
  Edge *qnext, *qprev;

  Edge(Edge *prev);

protected:
  Vec2 *data;
  Edge *next;

  Triangle *lface;

public:
  DAG_DECLARE_NEW(delmem)

  Edge();
  Edge(const Edge &);
  ~Edge();

  //
  // Primitive methods
  //
  Edge *Onext() const { return next; }
  Edge *Sym() const { return qnext->qnext; }
  Edge *Rot() const { return qnext; }
  Edge *invRot() const { return qprev; }

  //
  // Synthesized methods
  //
  Edge *Oprev() const { return Rot()->Onext()->Rot(); }
  Edge *Dnext() const { return Sym()->Onext()->Sym(); }
  Edge *Dprev() const { return invRot()->Onext()->invRot(); }

  Edge *Lnext() const { return invRot()->Onext()->Rot(); }
  Edge *Lprev() const { return Onext()->Sym(); }
  Edge *Rnext() const { return Rot()->Onext()->invRot(); }
  Edge *Rprev() const { return Sym()->Onext(); }


  Vec2 &Org() const { return *data; }
  Vec2 &Dest() const { return *Sym()->data; }

  Triangle *Lface() const { return lface; }
  void set_Lface(Triangle *t) { lface = t; }

  void EndPoints(Vec2 &org, Vec2 &dest)
  {
    data = &org;
    Sym()->data = &dest;
  }

  //
  // The fundamental topological operator
  friend void splice(Edge *a, Edge *b);
};


inline boolean rightOf(const Vec2 &x, const Edge *e) { return rightOf(x, e->Org(), e->Dest()); }

inline boolean leftOf(const Vec2 &x, const Edge *e) { return leftOf(x, e->Org(), e->Dest()); }


#ifdef IOSTREAMH
inline ostream &operator<<(ostream &out, const Edge *e) { return out << "{ " << e->Org() << " ---> " << e->Dest() << " }"; }
#endif
}; // namespace delaunay
