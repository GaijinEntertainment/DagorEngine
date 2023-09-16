#include <stdlib.h>
#include <iostream>
using namespace std;

#include "quadedge.h"

namespace delaunay
{

Edge::Edge(const Edge &)
{
  cerr << "Edge: Edge assignments are forbidden." << endl;
  exit(1);
}


Edge::Edge(Edge *prev)
{
  qnext = NULL;
  qprev = prev;
  prev->qnext = this;

  lface = NULL;
  token = 0;
}

Edge::Edge()
{
  qprev = qnext = NULL;
  Edge *e0 = this;
  Edge *e1 = new (delmem) Edge(e0);
  Edge *e2 = new (delmem) Edge(e1);
  Edge *e3 = new (delmem) Edge(e2);

  qprev = e3;
  e3->qnext = e0;

  e0->next = e0;
  e1->next = e3;
  e2->next = e2;
  e3->next = e1;

  lface = NULL;
  token = 0;
}

Edge::~Edge()
{
  if (qnext)
  {
    Edge *e1 = qnext;
    Edge *e2 = qnext->qnext;
    Edge *e3 = qprev;

#ifdef SAFETY
    qnext = NULL;

    token = -69;
    e1->token = -69;
    e2->token = -69;
    e3->token = -69;
#endif
    e1->qnext = NULL;
    e2->qnext = NULL;
    e3->qnext = NULL;

    delete_obj(e1);
    delete_obj(e2);
    delete_obj(e3);
  }
}


void splice(Edge *a, Edge *b)
{
  Edge *alpha = a->Onext()->Rot();
  Edge *beta = b->Onext()->Rot();

  Edge *t1 = b->Onext();
  Edge *t2 = a->Onext();
  Edge *t3 = beta->Onext();
  Edge *t4 = alpha->Onext();

  a->next = t1;
  b->next = t2;
  alpha->next = t3;
  beta->next = t4;
}

}; // namespace delaunay
