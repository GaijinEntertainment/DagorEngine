#include <rendInst/rendInstGen.h>
#include "riGen/riGenData.h"
#include "riGen/genObjUtil.h"
#include "riGen/riGenExtra.h"
#include "riGen/riUtil.h"

#include <util/dag_console.h>
#include <ioSys/dag_fileIo.h>
#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_gameResources.h>
#include <math/dag_math3d.h>
#include <math/dag_mathUtils.h>
#include <math/dag_wooray2d.h>
#include <math/dag_vecMathCompatibility.h>
#include <render/primitiveObjects.h>

namespace rendinst
{
typedef Point3 CollisionVertex;
static inline CollisionVertex to_coll_vert(vec4f v)
{
  CollisionVertex r;
  memcpy(&r, &v, sizeof(CollisionVertex));
  return r;
}
static inline CollisionVertex to_coll_vert(Point3 v) { return v; }
void dumpAllCollisions(IGenSave &cb)
{
  static const int COLLISION_BOX_INDICES_NUM = 36;
  static const int COLLISION_BOX_VERTICES_NUM = 8;
  carray<uint16_t, COLLISION_BOX_INDICES_NUM> boxIndices;
  create_cubic_indices(make_span((uint8_t *)boxIndices.data(), COLLISION_BOX_INDICES_NUM * sizeof(uint16_t)), 1, false);
  dag::Vector<uint16_t> indices;
  dag::Vector<Point3> vertices;
  cb.writeInt(riExtra.size()); // hint!
  for (auto &pool : riExtra)
  {
    CollisionResource *collRes = pool.collRes;
    if (!collRes || pool.isPosInst())
      continue;
    indices.clear();
    vertices.clear();
    for (int ni = 0, ne = collRes->getAllNodes().size(); ni < ne; ++ni)
    {
      CollisionNode *node = collRes->getNode(ni);
      if (!node || !node->checkBehaviorFlags(CollisionNode::TRACEABLE))
        continue;
      int firstVertex = vertices.size();
      size_t iAt = indices.size(), vAt = firstVertex;
      if (node->type == COLLISION_NODE_TYPE_BOX)
      {
        indices.resize(indices.size() + COLLISION_BOX_INDICES_NUM);
        vertices.resize(vertices.size() + COLLISION_BOX_VERTICES_NUM);

        for (int vertNo = 0; vertNo < COLLISION_BOX_VERTICES_NUM; ++vertNo, vAt++)
          vertices[vAt] = to_coll_vert(node->modelBBox.point(vertNo));

        for (int ii = 0; ii < COLLISION_BOX_INDICES_NUM; ++ii)
          indices[iAt + ii] = boxIndices[ii] + firstVertex;
      }
      else if (node->indices.size() > 0 && (node->type == COLLISION_NODE_TYPE_MESH || node->type == COLLISION_NODE_TYPE_CONVEX))
      {
        indices.resize(indices.size() + node->indices.size());
        vertices.resize(vertices.size() + node->vertices.size());
        G_ASSERT_CONTINUE(elem_size(node->vertices) == sizeof(vec4f));
        const vec4f *__restrict verts = (const vec4f *)node->vertices.data();
        mat44f nodeTm;
        v_mat44_make_from_43cu_unsafe(nodeTm, node->tm[0]);
        for (const vec4f *__restrict ve = verts + node->vertices.size(); verts != ve; ++verts, ++vAt)
          vertices[vAt] = to_coll_vert(v_mat44_mul_vec3p(nodeTm, *verts));
        for (int ii = 0, ie = node->indices.size(); ii < ie; ++ii)
          indices[iAt + ii] = node->indices[ii] + firstVertex;
      }
      else
        continue;
    }
    if (!indices.size())
      continue;
    cb.writeInt(indices.size());
    cb.write(indices.begin(), indices.size() * sizeof(*indices.data()));
    cb.writeInt(vertices.size());
    cb.write(vertices.begin(), vertices.size() * sizeof(*vertices.data()));

    cb.writeInt(pool.riTm.size());
    cb.write(pool.riTm.begin(), pool.riTm.size() * sizeof(mat43f));
  }
}
} // namespace rendinst

static bool ri_collision_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("ri", "dump_coll", 1, 2)
  {
    const char *fn = argc < 2 ? "ri_collisions.bin" : argv[1];
    console::print("dumping all collisions to %s", fn);
    FullFileSaveCB cb(fn);
    if (cb.fileHandle)
    {
      rendinst::dumpAllCollisions(cb);
      console::print("done");
    }
    else
      console::print_d("file %s can't be open for save", fn);
  }
  return found;
}

using namespace console;
REGISTER_CONSOLE_HANDLER(ri_collision_console_handler);
