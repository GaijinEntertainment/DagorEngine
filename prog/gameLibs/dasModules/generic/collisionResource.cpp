// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotCollisionResource.h>
#include <rendInst/rendInstExtra.h>


DAS_BASE_BIND_ENUM_98(CollisionNode::BehaviorFlag, BehaviorFlag, TRACEABLE, PHYS_COLLIDABLE, SOLID, FLAG_ALLOW_HOLE,
  FLAG_DAMAGE_REQUIRED, FLAG_CUT_REQUIRED, FLAG_CHECK_SIDE, FLAG_ALLOW_BULLET_DECAL, FLAG_ALLOW_SPLASH_HOLE);

DAS_BASE_BIND_ENUM_98(CollisionNode::NodeFlag, CollisionNodeFlag, NONE, IDENT, TRANSLATE, ORTHONORMALIZED, ORTHOUNIFORM);

DAS_BASE_BIND_ENUM_98(CollisionResourceNodeType, CollisionResourceNodeType, COLLISION_NODE_TYPE_MESH, COLLISION_NODE_TYPE_POINTS,
  COLLISION_NODE_TYPE_BOX, COLLISION_NODE_TYPE_SPHERE, COLLISION_NODE_TYPE_CAPSULE, COLLISION_NODE_TYPE_CONVEX,
  NUM_COLLISION_NODE_TYPES);


namespace bind_dascript
{

struct CollisionNodeAnnotation : das::ManagedStructureAnnotation<CollisionNode, false>
{
  CollisionNodeAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("CollisionNode", ml)
  {
    cppName = " ::CollisionNode";
    addField<DAS_BIND_MANAGED_FIELD(tm)>("tm");
    addField<DAS_BIND_MANAGED_FIELD(modelBBox)>("modelBBox");
    addField<DAS_BIND_MANAGED_FIELD(boundingSphere)>("boundingSphere");
    addField<DAS_BIND_MANAGED_FIELD(capsule)>("capsule");
    addField<DAS_BIND_MANAGED_FIELD(physMatId)>("physMatId");
    addProperty<DAS_BIND_MANAGED_PROP(getNodeIdAsInt)>("geomNodeId", "getNodeIdAsInt");
    addField<DAS_BIND_MANAGED_FIELD(flags)>("flags");
    addField<DAS_BIND_MANAGED_FIELD(insideOfNode)>("insideOfNode");
    addField<DAS_BIND_MANAGED_FIELD(nodeIndex)>("nodeIndex");
    addField<DAS_BIND_MANAGED_FIELD(type)>("nodeType", "type");
    addField<DAS_BIND_MANAGED_FIELD(behaviorFlags)>("behaviorFlags");
    addFieldEx("name", "name", offsetof(CollisionNode, name), das::makeType<char *>(ml));
  }
};

struct CollisionResourceAnnotation : das::ManagedStructureAnnotation<CollisionResource, false>
{
  CollisionResourceAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("CollisionResource", ml)
  {
    cppName = " ::CollisionResource";
    addField<DAS_BIND_MANAGED_FIELD(vFullBBox)>("vFullBBox");
    addField<DAS_BIND_MANAGED_FIELD(boundingSphereRad)>("boundingSphereRad");
    addFieldEx("boundingSphereCenter", "vBoundingSphere", offsetof(CollisionResource, vBoundingSphere),
      das::makeType<das::float3>(ml));
    addField<DAS_BIND_MANAGED_FIELD(boundingBox)>("boundingBox");
  }
};

struct IntersectedNodeAnnotation : das::ManagedStructureAnnotation<IntersectedNode, false>
{
  IntersectedNodeAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("IntersectedNode", ml)
  {
    cppName = " ::IntersectedNode";
    addField<DAS_BIND_MANAGED_FIELD(normal)>("normal");
    addField<DAS_BIND_MANAGED_FIELD(intersectionT)>("intersectionT");
    addField<DAS_BIND_MANAGED_FIELD(intersectionPos)>("intersectionPos");
    addField<DAS_BIND_MANAGED_FIELD(collisionNodeId)>("collisionNodeId");
  }
};

class CollRes final : public das::Module
{
public:
  CollRes() : das::Module("CollRes")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("math"));
    addBuiltinDependency(lib, require("ecs"));
    addBuiltinDependency(lib, require("AnimV20"));
    addBuiltinDependency(lib, require("GeomNodeTree"));

    addEnumeration(das::make_smart<EnumerationCollisionResourceNodeType>());
    addAnnotation(das::make_smart<CollisionNodeAnnotation>(lib));
    addAnnotation(das::make_smart<CollisionResourceAnnotation>(lib));
    addAnnotation(das::make_smart<IntersectedNodeAnnotation>(lib));
    das::typeFactory<CollResIntersectionsType>::make(lib);
    das::typeFactory<CollResHitNodesType>::make(lib);
    das::addCtorAndUsing<CollResHitNodesType>(*this, lib, "CollResHitNodesType", "::CollResHitNodesType");

    addEnumeration(das::make_smart<EnumerationBehaviorFlag>());
    addEnumeration(das::make_smart<EnumerationCollisionNodeFlag>());

    das::addExtern<DAS_BIND_FUN(collnode_get_name)>(*this, lib, "collnode_get_name", das::SideEffects::none,
      "bind_dascript::collnode_get_name");

    das::addExtern<DAS_BIND_FUN(collres_traceray)>(*this, lib, "collres_traceray", das::SideEffects::modifyArgumentAndAccessExternal,
      "bind_dascript::collres_traceray");
    das::addExtern<DAS_BIND_FUN(collres_traceray2)>(*this, lib, "collres_traceray", das::SideEffects::modifyArgumentAndAccessExternal,
      "bind_dascript::collres_traceray2");
    das::addExtern<DAS_BIND_FUN(collres_traceray_out_mat)>(*this, lib, "collres_traceray",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::collres_traceray_out_mat");
    das::addExtern<DAS_BIND_FUN(collres_traceray_out_intersections)>(*this, lib, "collres_traceray",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::collres_traceray_out_intersections");
    das::addExtern<DAS_BIND_FUN(collres_traceray_out_intersections2)>(*this, lib, "collres_traceray",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::collres_traceray_out_intersections2");
    das::addExtern<DAS_BIND_FUN(collres_traceCapsule_out_intersections)>(*this, lib, "collres_traceCapsule",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::collres_traceCapsule_out_intersections");
    das::addExtern<DAS_BIND_FUN(collres_capsuleHit_out_intersections)>(*this, lib, "collres_capsuleHit",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::collres_capsuleHit_out_intersections");
    das::addExtern<DAS_BIND_FUN(collres_rayhit)>(*this, lib, "collres_rayhit", das::SideEffects::accessExternal,
      "bind_dascript::collres_rayhit");

    das::addExtern<DAS_BIND_FUN(collres_get_node_index_by_name)>(*this, lib, "collres_get_node_index_by_name", das::SideEffects::none,
      "bind_dascript::collres_get_node_index_by_name");

    das::addExtern<DAS_BIND_FUN(collres_get_collision_node_tm)>(*this, lib, "collres_get_collision_node_tm",
      das::SideEffects::modifyArgument, "bind_dascript::collres_get_collision_node_tm");

    das::addExtern<DAS_BIND_FUN(collres_get_node)>(*this, lib, "collres_get_node", das::SideEffects::none,
      "bind_dascript::collres_get_node");

    das::addExtern<DAS_BIND_FUN(collres_get_nodesCount)>(*this, lib, "collres_get_nodesCount", das::SideEffects::none,
      "bind_dascript::collres_get_nodesCount");

    das::addExtern<DAS_BIND_FUN(rendinst::getRIGenExtraCollRes)>(*this, lib, "get_rigen_extra_coll_res",
      das::SideEffects::accessExternal, "rendinst::getRIGenExtraCollRes");

    das::addExtern<DAS_BIND_FUN(rendinst::getRIGenExtraCollBb)>(*this, lib, "get_rigen_extra_coll_bb",
      das::SideEffects::accessExternal, "rendinst::getRIGenExtraCollBb");

    das::addExtern<DAS_BIND_FUN(test_collres_intersection)>(*this, lib, "test_collres_intersection",
      das::SideEffects::modifyArgumentAndExternal, "bind_dascript::test_collres_intersection");

    das::addExtern<DAS_BIND_FUN(test_collres_intersection_out_intersections)>(*this, lib, "test_collres_intersection",
      das::SideEffects::modifyArgumentAndExternal, "bind_dascript::test_collres_intersection_out_intersections");

    das::addExtern<DAS_BIND_FUN(get_collnode_geom)>(*this, lib, "get_collnode_geom", das::SideEffects::invoke,
      "bind_dascript::get_collnode_geom");

    das::addExtern<DAS_BIND_FUN(collres_check_grid_available)>(*this, lib, "collres_check_grid_available",
      das::SideEffects::accessExternal, "bind_dascript::collres_check_grid_available");

    using method_setBsphereCenterNode = DAS_CALL_MEMBER(CollisionResource::setBsphereCenterNode);
    das::addExtern<DAS_CALL_METHOD(method_setBsphereCenterNode)>(*this, lib, "collres_setBsphereCenterNode",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(CollisionResource::setBsphereCenterNode));

    using method_getGridSize = DAS_CALL_MEMBER(CollisionResource::getGridSize);
    das::addExtern<DAS_CALL_METHOD(method_getGridSize)>(*this, lib, "collres_getGridSize", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(CollisionResource::getGridSize));

    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotCollisionResource.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(CollRes, bind_dascript);
