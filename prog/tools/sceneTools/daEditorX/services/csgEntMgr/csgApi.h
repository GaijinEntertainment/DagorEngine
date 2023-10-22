#pragma once

#include <math/dag_mesh.h>
#include <math/dag_TMatrix.h>

struct BasicCSG
{
  enum OpType
  {
    UNKNOWN,
    UNION,
    INTERSECTION,
    A_MINUS_B,
    B_MINUS_A,
    SYMMETRIC_DIFFERENCE,
  };

  virtual ~BasicCSG() {}
  virtual void *create_poly(const TMatrix *wtm, const Mesh &m, int node_id, bool only_closed) = 0;
  virtual void delete_poly(void *poly) = 0;

  virtual void *op(OpType op_tp, void *a_, const TMatrix *atm, void *b_, const TMatrix *btm) = 0;

  virtual void convert(void *poly_in, Mesh &mesh) = 0;
};

extern BasicCSG *make_new_csg(int tc_cnt);
extern void delete_csg(BasicCSG *csg);
