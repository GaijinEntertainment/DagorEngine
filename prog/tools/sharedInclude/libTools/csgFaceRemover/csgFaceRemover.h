#pragma once
class TMatrix;
class Mesh;
class BBox3;

class BaseCSGRemoval
{
public:
  virtual ~BaseCSGRemoval() {}
  virtual void *create_poly(const TMatrix *wtm, const Mesh &m, int node_id, bool only_closed, BBox3 &box) = 0;
  virtual void delete_poly(void *poly) = 0;
  virtual void *makeUnion(void *a_, void *b_) = 0;
  // virtual void convert(void *poly_in, Mesh& mesh) = 0;

  virtual int removeUnusedFaces(Mesh &mesh, void *poly_in, int node_id) = 0; // return number of removed faces
};

extern BaseCSGRemoval *make_new_csg();
extern void delete_csg(BaseCSGRemoval *csg);
