#pragma once
class TMatrix;
class Mesh;

class BaseCSG
{
public:
  virtual ~BaseCSG() {}
  virtual void *create_poly(const TMatrix *wtm, const Mesh &m, int node_id, bool only_closed, BBox3 &box) = 0;
  virtual bool hasOpenManifolds(void *poly) = 0;
  virtual void delete_poly(void *poly) = 0;
  virtual void *op(void *a_, void *b_) = 0;
  // virtual void convert(void *poly_in, Mesh& mesh) = 0;

  virtual void removeUnusedFaces(Mesh &mesh, void *poly_in, int node_id) = 0;
};

extern BaseCSG *make_new_csg();
extern void delete_csg(BaseCSG *csg);
