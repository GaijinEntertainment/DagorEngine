//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_color.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <memory/dag_mem.h>
#include <util/dag_stdint.h>

class Bitarray;

struct Face
{
  uint32_t v[3];
  uint32_t smgr; // smoothing group
  uint16_t mat;  // sub-material index, ignored if not multi-material
  uint16_t forceAlignBy32BitDummy;

  void set(uint32_t v0, uint32_t v1, uint32_t v2, uint16_t m = 0, uint32_t sg = 1)
  {
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    mat = m;
    smgr = sg;
    forceAlignBy32BitDummy = 0;
  }
};

struct TFace
{
  uint32_t t[3];
};

#define NUMMESHTCH 10

class TVertTab : public Tab<Point2>
{
public:
  TVertTab() = default;
  TVertTab(const typename Tab<Point2>::allocator_type &allocator) : Tab<Point2>(allocator) {}
  template <typename V, bool s = dag::is_same<Point2 const, typename V::value_type const>::is_true>
  TVertTab &operator=(const V &v)
  {
    Tab<Point2>::operator=(v);
    return *this;
  }
};

class TFaceTab : public Tab<TFace>
{
public:
  TFaceTab() = default;
  TFaceTab(const typename Tab<TFace>::allocator_type &allocator) : Tab<TFace>(allocator) {}
  template <typename V, bool s = dag::is_same<TFace const, typename V::value_type const>::is_true>
  TFaceTab &operator=(const V &v)
  {
    Tab<TFace>::operator=(v);
    return *this;
  }
};

class Tabint : public Tab<int>
{
public:
  Tabint() = default;
  Tabint(const typename Tab<int>::allocator_type &allocator) : Tab<int>(allocator) {}
  template <typename V, bool s = dag::is_same<int const, typename V::value_type const>::is_true>
  Tabint &operator=(const V &v)
  {
    Tab<int>::operator=(v);
    return *this;
  }
};


class F2V_Map
{
public:
  Tab<int> data;
  Tab<int> index;

  F2V_Map() : data(tmpmem_ptr()), index(tmpmem_ptr()) {}

  int size() { return index.size(); }
  int get_face_num(int v_ind)
  {
    if (v_ind + 1 < index.size())
      return index[v_ind + 1] - index[v_ind];
    return data.size() - index[v_ind];
  }
  // !Attention! no checks
  int get_face_ij(int v_ind, int j) { return data[index[v_ind] + j]; }
};

// class for faceindices (face_id*3 + vi)
class VertToFaceVertMap : public F2V_Map
{
public:
  int vertCount() { return size(); }
  int getNumFaces(int v_ind) { return get_face_num(v_ind); }
  int getVertFaceIndex(int v_ind, int j) { return get_face_ij(v_ind, j); }
};

struct FaceNGr
{
  uint32_t v[3];
  FaceNGr() = default;
  uint32_t &operator[](int i) { return v[i]; }
  const uint32_t &operator[](int i) const { return v[i]; }
};

#define FACEFLG_EDGE0    1
#define FACEFLG_EDGE1    2
#define FACEFLG_EDGE2    4
#define FACEFLG_ALLEDGES (FACEFLG_EDGE0 | FACEFLG_EDGE1 | FACEFLG_EDGE2)
#define FACEFLG_HIDDEN   8

class MeshData
{
public:
  DAG_DECLARE_NEW(tmpmem)

  Tab<Point3> vert;           // vertices
  Tab<Face> face;             // faces
  TVertTab tvert[NUMMESHTCH]; // texture coordinates for each mapping channel
  TFaceTab tface[NUMMESHTCH]; // TC faces for each mapping channel
  Tab<Color4> cvert;          // vertex colors
  TFaceTab cface;             // vertex color faces

  Tab<int> ntab;        // internal normals data
  Tab<int> ngr;         // internal normal groups data
  Tab<FaceNGr> facengr; // normal group indices for each vertex of each face
  Tab<Point3> facenorm; // face normals
  Tab<Point3> vertnorm; // normals for each normal group (vertex normals)
  Tab<uint8_t> faceflg;

  enum
  {
    CHT_UNKNOWN,
    CHT_FLOAT1,
    CHT_FLOAT2,
    CHT_FLOAT3,
    CHT_FLOAT4,
    CHT_E3DCOLOR,
  };

  struct ExtraChannel
  {
    Tab<TFace> fc;
    Tab<uint8_t> vt;
    uint8_t type, vsize, usg, usgi;


    ExtraChannel();

    void setmem(IMemAlloc *);

    bool resize_verts(int num, int type);

    bool resize_verts(int num) { return resize_verts(num, type); }

    int numverts()
    {
      if (vsize == 0)
        return 0;
      return vt.size() / vsize;
    }

    void save(class IGenSave &cb);
    void load(class IGenLoad &cb);
  };

  Tab<ExtraChannel> extra;
  MeshData();
  // erase n faces from mesh starting at at
  void erasefaces(int at, int n);

  void removeFacesFast(int at, int n);
  void removeFacesFast(const Bitarray &used_faces);

  // set number of mesh faces;
  // empty mapping channels are not affected
  bool setnumfaces(int);

  // set number of mesh faces;
  // empty mapping channels in Mesh m are not affected
  bool setNumFaces(int, const MeshData &m, bool set0);
  // validate mesh;
  // returns 0 if any problems were found
  int check_mesh();

  // returns extra channel number, or -1 if not found
  int find_extra_channel(int usage, int usage_index) const;
  // adds extra channel
  // returns -1 on error, or if such channel already exists,
  //   or index to extra[] otherwise
  // NOTE: channel faces are resized, but not initialized!
  int add_extra_channel(int type, int usage, int usage_index);

  // calculate normal groups (ntab, ngr and facengr)
  bool calc_ngr();
  // calculate face normals
  bool calc_facenorms();
  // calculate vertex normals from ntab, ngr and facengr;
  // calls calc_facenorms() if no face normals present in the mesh
  bool calc_vertnorms();

  // reorder faces using index array;
  // fi is index array;
  // f0 is starting face;
  // nf in number of faces to reorder or -1 to reorder all faces starting at f0
  bool reorder_faces(int *fi, int f0 = 0, int nf = -1);
  // sort faces by material index; uses reorder_faces()
  bool sort_faces_by_mat();
  // calculate mesh radius relative to the specified point
  real calc_mesh_rad(Point3 &c);
  // flip face normals;
  // f0 is starting face;
  // nf in number of faces to flip or -1 to flip all faces starting at f0
  void flip_normals(int f0 = 0, int nf = -1);
  // remove unused vertices
  // and welding equal ones with squared threshold sqThreshold. If sqThreshold<0 - do not weld
  void kill_unused_verts(float sqThreshold = -1.0);
  // remove degenerate faces (faces with two or more equal vertex indices)
  void kill_bad_faces();
  // optimize mapping by removing unused texture coordinate vertices
  // and welding equal ones with squared threshold sqThreshold. If sqThreshold<0 - do not weld
  void optimize_tverts(float sqThreshold = 0.0);
  bool optimize_extra(int id, float sqThreshold);
  // add mapping channel if it doesn't exist
  int force_mapping(int ch);

  // clamp materials with max_val
  void clampMaterials(uint16_t max_val, int sf = 0, int numf = -1);

  // build vertex-to-face map;
  // sf is starting face;
  // numf is number of faces to build map for or -1 to build it for all faces
  // starting at sf;
  // returns false if not enough memory
  bool get_vert2facemap(Tab<Tabint> &map, int sf = 0, int numf = -1);

  // build vertex-to-face map (fast for multiple calls )
  bool get_vert2facemap_fast(F2V_Map &map, int sf = 0, int numf = -1);

  // build map that maps each vertex to list of indices (face_index*3+in_face_vertex)
  void buildVertToFaceVertMap(VertToFaceVertMap &map);

  // setmem
  void setmem(IMemAlloc *m);

  // calculate data for tangents & binormal & place it in new extra channel. if channel is exists,
  // no work will be processed. return false, if any error occurs
  bool createTangentsData(int usage, int usage_index);
  bool createTextureSpaceData(int usage, int usage_index_du, int usage_index_dv);

  void attach(const MeshData &mesh);
  bool areNormalsComputed();          // return true, if they are computed, false if imported. should be removed later
  void transform(const TMatrix &wtm); // transform vertices and normals

  void saveData(class IGenSave &cb);
  void loadData(class IGenLoad &cb);

  // build vertex-to-face map (fast for multiple calls )
  static void buildVertToFaceVertMap(F2V_Map &map, const Face *face, int fnum, int vnum, int faceIndMul, int vertIndMul);
};
DAG_DECLARE_RELOCATABLE(MeshData);

class Mesh : public MeshData
{
public:
  DAG_DECLARE_NEW(tmpmem)

  Mesh();
  // get data functions
  int getVertCount() const { return vert.size(); }
  int getFaceCount() const { return face.size(); }
  dag::ConstSpan<Point3> getVert() const { return vert; }
  dag::ConstSpan<Point3> getVertNorm() const { return vertnorm; }
  dag::ConstSpan<Color4> getCVert() const { return cvert; }
  dag::ConstSpan<Point2> getTVert(int i) const { return tvert[i]; }

  dag::ConstSpan<Face> getFace() const { return face; }
  dag::ConstSpan<FaceNGr> getNormFace() const { return facengr; }
  dag::ConstSpan<TFace> getCFace() const { return cface; }
  dag::ConstSpan<TFace> getTFace(int i) const { return tface[i]; }
  dag::ConstSpan<uint8_t> getFaceFlg() const { return faceflg; }
  const ExtraChannel &getExtra(int c) const { return extra[c]; }

  uint16_t getFaceMaterial(int faceId) const { return face[faceId].mat; }
  void setFaceMaterial(int faceId, uint16_t mat) { face[faceId].mat = mat; }

  uint32_t getFaceSMGR(int faceId) const { return face[faceId].smgr; }
  void setFaceSMGR(int faceId, uint32_t smgr) { face[faceId].smgr = smgr; }

  // getMeshData for building data explicitly
  MeshData &getMeshData() { return *this; }

  bool hasMatId(int mat) const
  {
    for (int i = 0; i < face.size(); i++)
      if (face[i].mat == mat)
        return true;
    return false;
  }

  // erase n faces from mesh starting at at
  void erasefaces(int at, int n) { MeshData::erasefaces(at, n); }

  void removeFacesFast(int at, int n) { MeshData::removeFacesFast(at, n); }
  void removeFacesFast(const Bitarray &used_faces) { MeshData::removeFacesFast(used_faces); }

  // set number of mesh faces;
  // empty mapping channels are not affected
  bool setnumfaces(int n) { return MeshData::setnumfaces(n); }

  // set number of mesh faces;
  // empty mapping channels in Mesh m are not affected
  bool setNumFaces(int n, const MeshData &m, bool set0) { return MeshData::setNumFaces(n, m, set0); }
  // validate mesh;
  // returns 0 if any problems were found
  int check_mesh() { return MeshData::check_mesh(); }

  // returns extra channel number, or -1 if not found
  int find_extra_channel(int usage, int usage_index) const { return MeshData::find_extra_channel(usage, usage_index); }
  // adds extra channel
  // returns -1 on error, or if such channel already exists,
  //   or index to extra[] otherwise
  // NOTE: channel faces are resized, but not initialized!
  int add_extra_channel(int type, int usage, int usage_index) { return MeshData::add_extra_channel(type, usage, usage_index); }

  // calculate normal groups (ntab, ngr and facengr)
  bool calc_ngr() { return MeshData::calc_ngr(); }
  // calculate face normals
  bool calc_facenorms() { return MeshData::calc_facenorms(); }
  // calculate vertex normals from ntab, ngr and facengr;
  // calls calc_facenorms() if no face normals present in the mesh
  bool calc_vertnorms() { return MeshData::calc_vertnorms(); }

  // reorder faces using index array;
  // fi is index array;
  // f0 is starting face;
  // nf in number of faces to reorder or -1 to reorder all faces starting at f0
  bool reorder_faces(int *fi, int f0 = 0, int nf = -1) { return MeshData::reorder_faces(fi, f0, nf); }
  // sort faces by material index; uses reorder_faces()
  bool sort_faces_by_mat() { return MeshData::sort_faces_by_mat(); }
  // calculate mesh radius relative to the specified point
  real calc_mesh_rad(Point3 &c) { return MeshData::calc_mesh_rad(c); }
  // flip face normals;
  // f0 is starting face;
  // nf in number of faces to flip or -1 to flip all faces starting at f0
  void flip_normals(int f0 = 0, int nf = -1) { MeshData::flip_normals(f0, nf); }
  // remove unused vertices
  // and welding equal ones with squared threshold sqThreshold. If sqThreshold<0 - do not weld
  void kill_unused_verts(float sqThreshold = -1.0) { MeshData::kill_unused_verts(sqThreshold); }
  // remove degenerate faces (faces with two or more equal vertex indices)
  void kill_bad_faces() { MeshData::kill_bad_faces(); }
  // optimize mapping by removing unused texture coordinate vertices
  // and welding equal ones with squared threshold sqThreshold. If sqThreshold<0 - do not weld
  void optimize_tverts(float sqThreshold = 0.0) { MeshData::optimize_tverts(sqThreshold); }

  // return false, if it is not possible (channel not exists or invalid)
  bool optimize_extra(int id, float sqThreshold) { return MeshData::optimize_extra(id, sqThreshold); }

  // add mapping channel if it doesn't exist
  int force_mapping(int ch) { return MeshData::force_mapping(ch); }

  // clamp materials with max_val
  void clampMaterials(uint16_t max_val, int sf = 0, int numf = -1) { MeshData::clampMaterials(max_val, sf, numf); }

  // build vertex-to-face map;
  // sf is starting face;
  // numf is number of faces to build map for or -1 to build it for all faces
  // starting at sf;
  // returns false if not enough memory
  bool get_vert2facemap(Tab<Tabint> &map, int sf = 0, int numf = -1) { return MeshData::get_vert2facemap(map, sf, numf); }

  // build vertex-to-face map (fast for multiple calls )
  bool get_vert2facemap_fast(F2V_Map &map, int sf = 0, int numf = -1) { return MeshData::get_vert2facemap_fast(map, sf, numf); }

  // build map that maps each vertex to list of indices (face_index*3+in_face_vertex)
  void buildVertToFaceVertMap(VertToFaceVertMap &map) { MeshData::buildVertToFaceVertMap(map); }

  // setmem
  void setmem(IMemAlloc *m) { MeshData::setmem(m); }

  // calculate data for tangents & binormal & place it in new extra channel. if channel is exists,
  // no work will be processed. return false, if any error occurs
  bool createTangentsData(int usage, int usage_index) { return MeshData::createTangentsData(usage, usage_index); }
  bool createTextureSpaceData(int usage, int usage_index_du, int usage_index_dv)
  {
    return MeshData::createTextureSpaceData(usage, usage_index_du, usage_index_dv);
  }

  bool areNormalsComputed() // return true, if they are computed, false if imported. should be removed later
  {
    return MeshData::areNormalsComputed();
  }
  void transform(const TMatrix &wtm) // transform vertices and normals
  {
    MeshData::transform(wtm);
  }

  void saveData(class IGenSave &cb) { MeshData::saveData(cb); }
  void loadData(class IGenLoad &cb) { MeshData::loadData(cb); }

  // split mesh in two by division plane
  void split(Mesh &neghalf, Mesh &poshalf, real A, real B, real C, real D, real eps);

  void attach(const Mesh &mesh) { MeshData::attach(mesh); }
};
DAG_DECLARE_RELOCATABLE(Mesh);


// base class for edges - vertex data is sorted by vertex number
struct Edge
{
  union
  {
    struct
    {
      int v0, v1;
    };
    int v[2];
  };
  Edge() {}
  Edge(int a, int b) { set(a, b); }
  void set(int a, int b)
  {
    if (a < b)
    {
      v0 = a;
      v1 = b;
    }
    else
    {
      v0 = b;
      v1 = a;
    }
  }
  int vertequal(int a, int b) { return (a == v0 && b == v1); }
  int hasvert(int a) { return a == v0 || a == v1; }
};


template <class E>
class EdgeTab : public Tab<E>
{
public:
  EdgeTab(IMemAlloc *m) : Tab<E>(m) {}
  EdgeTab(const typename Tab<E>::allocator_type &allocator) : Tab<E>(allocator) {}

  int find(int v0, int v1)
  {
    if (v0 > v1)
    {
      int a = v0;
      v0 = v1;
      v1 = a;
    }
    for (int i = 0; i < Tab<E>::size(); ++i)
      if ((Tab<E>::data() + i)->vertequal(v0, v1))
        return i;
    return -1;
  }
  int add(const E &e)
  {
    int i = find(e.v0, e.v1);
    if (i >= 0)
      return i;
    i = append_items(*this, 1, &e);
    return i;
  }
};


// class V must be able to convert itself to int - vertex number
template <class V>
class VertTab : public Tab<V>
{
public:
  VertTab(IMemAlloc *m) : Tab<V>(m) {}
  VertTab(const typename Tab<V>::allocator_type &allocator) : Tab<V>(allocator) {}
  int find(int v)
  {
    for (int i = 0; i < Tab<V>::num; ++i)
      if (Tab<V>::data[i] == v)
        return i;
    return -1;
  }
  void add(const V &v)
  {
    if (find(v) >= 0)
      return;
    append_items(*this, 1, &v);
  }
};
