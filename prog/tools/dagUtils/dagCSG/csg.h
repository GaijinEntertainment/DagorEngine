#pragma once

class BBox3;
class BaseCSG
{
public:
  enum OpType
  {
    UNKNOWN,
    UNION,
    INTERSECTION,
    A_MINUS_B,
    B_MINUS_A,
    SYMMETRIC_DIFFERENCE,
    SLICE_IN,
    SLICE_OUT,
  };
  virtual ~BaseCSG() {}
  virtual void *create_poly(const TMatrix *wtm, const Mesh &m, int node_id, bool only_closed, BBox3 &box) = 0;
  virtual void delete_poly(void *poly) = 0;
  virtual void *op(OpType op_tp, void *a_, const TMatrix *atm, void *b_, const TMatrix *btm) = 0;
  virtual void convert(void *poly_in, Mesh &mesh) = 0;
  virtual bool slice_and_classify(void *open_, const TMatrix *openTM, void *closed_, const TMatrix *closedTM, void *&open_in,
    void *&open_on_in, void *&open_out, void *&open_on_out) = 0;

  virtual void removeUnusedFaces(Mesh &mesh, void *poly_in, int node_id) = 0;
};

extern BaseCSG *make_new_csg(int tccnt);
extern void delete_csg(BaseCSG *csg);
