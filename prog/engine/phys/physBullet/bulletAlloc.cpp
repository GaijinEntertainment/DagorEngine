// Copyright (C) Gaijin Games KFT.  All rights reserved.

// This is rather hacky - we have to defeat class operator new/delete defines in order to use in-place operator new
// To consider: use btAlignedAllocator somehow instead?
#include <LinearMath/btScalar.h>
#undef BT_DECLARE_ALIGNED_ALLOCATOR
#define BT_DECLARE_ALIGNED_ALLOCATOR()

#include <phys/dag_physDecl.h>
#include <phys/dag_bulletPhysics.h>
#include <osApiWrappers/dag_critSec.h>
#include <memory/dag_fixedBlockAllocator.h>

#define MEMCPY_COLL(DEST, SRC, TYPE) memcpy((void *)static_cast<TYPE *>(DEST), (void *)static_cast<TYPE *>(SRC), sizeof(TYPE))

// These two are called directly by Bullet lib
void *btAlignedAllocInternal(size_t size, int alignment)
{
  G_FAST_ASSERT(alignment <= 16);
  return memalloc(size);
}
void btAlignedFreeInternal(void *ptr) { memfree_anywhere(ptr); }

struct BTCompoundShapeWithDbvt final : public btCompoundShape
{
  btDbvt inlineDynamicAabbTree;
  BTCompoundShapeWithDbvt() = delete;
  void constructInlineExtra()
  {
    if (m_dynamicAabbTree)
    {
      m_dynamicAabbTree->~btDbvt();
      btAlignedFree(m_dynamicAabbTree);
    }
    btCompoundShape::m_dynamicAabbTree = new (&inlineDynamicAabbTree, _NEW_INPLACE) btDbvt();
    // Copy-Paste from btCompoundShape::createAabbTreeFromChildren()
    for (int index = 0; index < m_children.size(); index++)
    {
      btCompoundShapeChild &child = m_children[index];

      // extend the local aabbMin/aabbMax
      btVector3 localAabbMin, localAabbMax;
      child.m_childShape->getAabb(child.m_transform, localAabbMin, localAabbMax);

      const btDbvtVolume bounds = btDbvtVolume::FromMM(localAabbMin, localAabbMax);
      child.m_node = m_dynamicAabbTree->insert(bounds, (void *)(intptr_t)index);
    }
  }
  ~BTCompoundShapeWithDbvt() { btCompoundShape::m_dynamicAabbTree = nullptr; }
};

struct BTRigidWithInlineCollShape final : public btRigidBody
{
  union
  {
    btSphereShape sphere;
    btCapsuleShape capsule;
    BTCompoundShapeWithDbvt compound;
    btConvexHullShape convHull;
    btScaledBvhTriangleMeshShape scaledTriMesh;
  };
  BTRigidWithInlineCollShape(const btRigidBody::btRigidBodyConstructionInfo &ctor_info) : btRigidBody(ctor_info)
  {
    btCollisionShape *shape = btRigidBody::getCollisionShape();
    // Note: ideally we would wanted to use eastl::move() here, but unfortunately Bullet doesn't provides ctors
    // with move semantic for collision shapes therefore we have to use hacky memcpy instead
    if (!shape)
      return;
    switch (shape->getShapeType())
    {
      case CONVEX_HULL_SHAPE_PROXYTYPE: MEMCPY_COLL(&convHull, shape, btConvexHullShape); break; //-V512
      case COMPOUND_SHAPE_PROXYTYPE:
        MEMCPY_COLL(&compound, shape, btCompoundShape); //-V512
        compound.constructInlineExtra();
        for (int i = 0, nc = compound.getNumChildShapes(); i < nc; ++i)
        {
          btCollisionShape *c = compound.getChildShape(i);
          if (c->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE && !static_cast<btBvhTriangleMeshShape *>(c)->getOptimizedBvh())
            static_cast<btBvhTriangleMeshShape *>(c)->buildOptimizedBvh();
        }
        break;
      case TRIANGLE_MESH_SHAPE_PROXYTYPE: G_FAST_ASSERT(0); break; // should been allocated within BTRigidWithInlineTriMesh
      case SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE: MEMCPY_COLL(&scaledTriMesh, shape, btScaledBvhTriangleMeshShape); break;
      case CAPSULE_SHAPE_PROXYTYPE: MEMCPY_COLL(&capsule, shape, btCapsuleShape); break; //-V512
      case SPHERE_SHAPE_PROXYTYPE: MEMCPY_COLL(&sphere, shape, btSphereShape); break;    //-V512
      default: return;                                                                   // unknown shape type, ignore
    }
    btRigidBody::setCollisionShape(&sphere);
    memfree(shape, defaultmem); // dtor is intentionally not called
  }
  ~BTRigidWithInlineCollShape()
  {
    btCollisionShape *shape = btRigidBody::getCollisionShape();
    switch ((shape == &sphere) ? shape->getShapeType() : -1)
    {
      case CONVEX_HULL_SHAPE_PROXYTYPE: convHull.~btConvexHullShape(); break;
      case COMPOUND_SHAPE_PROXYTYPE: compound.~BTCompoundShapeWithDbvt(); break;
      case SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE: scaledTriMesh.~btScaledBvhTriangleMeshShape(); break;
      case CAPSULE_SHAPE_PROXYTYPE: capsule.~btCapsuleShape(); break;
      case SPHERE_SHAPE_PROXYTYPE: sphere.~btSphereShape(); break;
      default: delete shape;
    }
  }
};

struct BTTriMeshShapeWithInlineBvh final : public btBvhTriangleMeshShape
{
  btOptimizedBvh bvh;
  btTriangleIndexVertexArray meshIface;
  btIndexedMesh meshPart;

  BTTriMeshShapeWithInlineBvh() = delete;
  void constructInlineExtra()
  {
    auto meshInterface = static_cast<btTriangleIndexVertexArray *>(m_meshInterface);
    memcpy((void *)&meshIface, (void *)meshInterface, sizeof(meshIface));
    if (meshInterface->getNumSubParts() == 1)
    {
      memcpy(&meshPart, &meshInterface->getIndexedMeshArray()[0], sizeof(meshPart));
      meshIface.getIndexedMeshArray().initializeFromBuffer(&meshPart, 1, 1);
    }
    memfree(meshInterface, defaultmem); // dtor is intentionally not called (it's internal data moved to meshIface)
    m_meshInterface = &meshIface;
    (new (&bvh, _NEW_INPLACE) btOptimizedBvh())->build(&meshIface, usesQuantizedAabbCompression(), m_localAabbMin, m_localAabbMax);
    setOptimizedBvh(&bvh, meshIface.getScaling());
  }
};

struct BTRigidWithInlineTriMesh final : public btRigidBody
{
  union
  {
    BTTriMeshShapeWithInlineBvh triMesh;
  };
  BTRigidWithInlineTriMesh(const btRigidBody::btRigidBodyConstructionInfo &ctor_info) : btRigidBody(ctor_info)
  {
    btCollisionShape *shape = btRigidBody::getCollisionShape();
    G_FAST_ASSERT(shape->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE);
    MEMCPY_COLL(&triMesh, shape, btBvhTriangleMeshShape);
    triMesh.constructInlineExtra();
    btRigidBody::setCollisionShape(&triMesh);
    memfree(shape, defaultmem); // dtor is intentionally not called
  }
  ~BTRigidWithInlineTriMesh()
  {
    G_FAST_ASSERT(btRigidBody::getCollisionShape() == &triMesh);
    triMesh.~BTTriMeshShapeWithInlineBvh();
  }
};


static constexpr size_t rigidBodyInitialChunkSize = 128;
static FixedBlockAllocator btRigidAllocator(sizeof(BTRigidWithInlineCollShape), rigidBodyInitialChunkSize);
static FixedBlockAllocator btRigidTriMeshAllocator(sizeof(BTRigidWithInlineTriMesh), rigidBodyInitialChunkSize);
static WinCritSec btRigidBodyAllocatorMutex;
static int numAllocatedRigidBodies = 0;

btRigidBody *PhysWorld::create_bt_rigidbody(const btRigidBody::btRigidBodyConstructionInfo &ctor_info)
{
  G_STATIC_ASSERT(alignof(btRigidBody) <= 16);
  bool isTriMesh = (ctor_info.m_collisionShape ? ctor_info.m_collisionShape->getShapeType() : -1) == TRIANGLE_MESH_SHAPE_PROXYTYPE;
  btRigidBodyAllocatorMutex.lock();
  void *retptr = (isTriMesh ? btRigidTriMeshAllocator : btRigidAllocator).allocateOneBlock();
  ++numAllocatedRigidBodies;
  btRigidBodyAllocatorMutex.unlock();
  if (isTriMesh)
    return new (retptr, _NEW_INPLACE) BTRigidWithInlineTriMesh(ctor_info);
  else
    return new (retptr, _NEW_INPLACE) BTRigidWithInlineCollShape(ctor_info);
}

void PhysWorld::destroy_bt_collobject(btRigidBody *obj)
{
  if (!obj)
    return;
  const btCollisionShape *collShape = obj->getCollisionShape();
  bool isTriMesh = collShape && collShape->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE;
  if (isTriMesh)
    static_cast<BTRigidWithInlineTriMesh *>(obj)->~BTRigidWithInlineTriMesh();
  else
    static_cast<BTRigidWithInlineCollShape *>(obj)->~BTRigidWithInlineCollShape();
  btRigidBodyAllocatorMutex.lock();
  G_VERIFY((isTriMesh ? btRigidTriMeshAllocator : btRigidAllocator).freeOneBlock(obj));
  G_VERIFYF(--numAllocatedRigidBodies >= 0, "Mismatched create/destroy?");
  btRigidBodyAllocatorMutex.unlock();
}

void free_all_bt_collobjects_memory()
{
  G_ASSERTF(numAllocatedRigidBodies == 0, "%d leaked rigids", numAllocatedRigidBodies);
  btRigidBodyAllocatorMutex.lock();
  btRigidAllocator.clear();
  btRigidTriMeshAllocator.clear();
  numAllocatedRigidBodies = 0;
  btRigidBodyAllocatorMutex.unlock();
}
