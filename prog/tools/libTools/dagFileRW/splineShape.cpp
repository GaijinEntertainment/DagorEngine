#include <util/dag_globDef.h>
#include <libTools/dagFileRW/splineShape.h>

void DagSpline::transform(TMatrix &tm)
{
  for (int i = 0; i < knot.size(); ++i)
    knot[i].transform(tm);
}

SplineShape::~SplineShape()
{
  for (int i = 0; i < spl.size(); ++i)
    if (spl[i])
      delete (spl[i]);
}

void SplineShape::transform(TMatrix &tm)
{
  for (int i = 0; i < spl.size(); ++i)
    if (spl[i])
      spl[i]->transform(tm);
}