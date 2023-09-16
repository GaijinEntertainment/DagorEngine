#include <libTools/dagFileRW/geomMeshHelper.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_mesh.h>
#include <EASTL/hash_set.h>
#include <hash/mum_hash.h>

GeomMeshHelper::GeomMeshHelper(IMemAlloc *mem) : verts(mem), faces(mem), edges(mem) {}


void GeomMeshHelper::init(Mesh &m)
{
  verts = m.getVert();

  faces.resize(m.getFace().size());
  for (int i = 0; i < faces.size(); ++i)
  {
    faces[i].v[0] = m.getFace()[i].v[0];
    faces[i].v[1] = m.getFace()[i].v[1];
    faces[i].v[2] = m.getFace()[i].v[2];
  }

  buildEdges();
  calcBox();
}


void GeomMeshHelper::calcBox()
{
  localBox.setempty();

  for (int i = 0; i < verts.size(); ++i)
    localBox += verts[i];
}

void GeomMeshHelper::buildEdges()
{
  clear_and_shrink(edges);
  edges.reserve(faces.size() * 3);

  struct EdgeHasher
  {
    size_t __forceinline operator()(const Edge &p) const
    {
      static_assert(sizeof(Edge) == sizeof(uint64_t));
      uint64_t key = uint64_t(p.v[0]) | (uint64_t(p.v[1]) << 32);
      return mum_hash64(key, 0);
    }
  };

  eastl::hash_set<Edge, EdgeHasher> edge_map;
  edge_map.reserve(faces.size() * 3);

  for (auto &face : faces)
  {
    for (int vi = 0; vi < 3; ++vi)
    {
      int v0 = face.v[vi];
      int v1 = face.v[(vi + 1) % 3];

      if (v0 > v1)
        eastl::swap(v0, v1);

      G_ASSERT(edge_map.size() == edges.size());
      auto insert_pair = edge_map.emplace(Edge(v0, v1));
      if (insert_pair.second)
        edges.push_back(Edge(v0, v1));
    }
  }

  edges.shrink_to_fit();
}

void GeomMeshHelper::save(DataBlock &blk)
{
  blk.setInt("numVerts", verts.size());
  blk.setInt("numFaces", faces.size());

  for (int i = 0; i < verts.size(); ++i)
    blk.addPoint3("vert", verts[i]);

  for (int i = 0; i < faces.size(); ++i)
    blk.addIPoint3("face", IPoint3(faces[i].v[0], faces[i].v[1], faces[i].v[2]));
}


void GeomMeshHelper::load(const DataBlock &blk)
{
  clear_and_shrink(verts);
  clear_and_shrink(faces);
  clear_and_shrink(edges);

  verts.reserve(blk.getInt("numVerts", 0));
  faces.reserve(blk.getInt("numFaces", 0));

  int nameId = blk.getNameId("vert");

  for (int i = 0; i < blk.paramCount(); ++i)
  {
    if (blk.getParamType(i) != DataBlock::TYPE_POINT3 || blk.getParamNameId(i) != nameId)
      continue;
    verts.push_back(blk.getPoint3(i));
  }

  nameId = blk.getNameId("face");

  for (int i = 0; i < blk.paramCount(); ++i)
  {
    if (blk.getParamType(i) != DataBlock::TYPE_IPOINT3 || blk.getParamNameId(i) != nameId)
      continue;
    IPoint3 v = blk.getIPoint3(i);
    faces.push_back(Face(v[0], v[1], v[2]));
  }

  buildEdges();
  calcBox();
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void GeomMeshHelperDagObject::save(DataBlock &blk)
{
  blk.setStr("class", "multiobj");
  blk.setStr("name", name);
  blk.setPoint3("wtm0", wtm.getcol(0));
  blk.setPoint3("wtm1", wtm.getcol(1));
  blk.setPoint3("wtm2", wtm.getcol(2));
  blk.setPoint3("wtm3", wtm.getcol(3));
  mesh.save(*blk.addBlock("mesh"));
}
