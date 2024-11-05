// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotGeomNodeTree.h>

#include "math/dag_geomNodeUtils.h"

namespace bind_dascript
{
struct GeomNodeTreeAnnotation : das::ManagedStructureAnnotation<GeomNodeTree, false>
{
  GeomNodeTreeAnnotation(das::ModuleLibrary &ml) : das::ManagedStructureAnnotation<GeomNodeTree, false>("GeomNodeTree", ml)
  {
    cppName = " ::GeomNodeTree";
    addProperty<DAS_BIND_MANAGED_PROP(nodeCount)>("nodeCount");
    addProperty<DAS_BIND_MANAGED_PROP(importantNodeCount)>("importantNodeCount");
  }
};

class GeomNodeTreeModule final : public das::Module
{
public:
  GeomNodeTreeModule() : das::Module("GeomNodeTree")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));
    addBuiltinDependency(lib, require("math"));

    addAnnotation(das::make_smart<GeomNodeTreeAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(geomtree_find_node_index)>(*this, lib, "geomtree_findNodeIndex", das::SideEffects::none,
      "bind_dascript::geomtree_find_node_index");
    das::addExtern<DAS_BIND_FUN(geomtree_get_node_tm), das::SimNode_ExtFuncCallRef, das::explicitConstArgFn>(*this, lib,
      "geomtree_getNodeTm", das::SideEffects::modifyArgument, "bind_dascript::geomtree_get_node_tm");
    das::addExtern<DAS_BIND_FUN(geomtree_get_node_wtm_rel), das::SimNode_ExtFuncCallRef, das::explicitConstArgFn>(*this, lib,
      "geomtree_getNodeWtmRel", das::SideEffects::modifyArgument, "bind_dascript::geomtree_get_node_wtm_rel");
    das::addExtern<DAS_BIND_FUN(geomtree_get_node_tm_c), das::SimNode_ExtFuncCallRef, das::explicitConstArgFn>(*this, lib,
      "geomtree_getNodeTm", das::SideEffects::none, "bind_dascript::geomtree_get_node_tm_c");
    das::addExtern<DAS_BIND_FUN(geomtree_get_node_wtm_rel_c), das::SimNode_ExtFuncCallRef, das::explicitConstArgFn>(*this, lib,
      "geomtree_getNodeWtmRel", das::SideEffects::none, "bind_dascript::geomtree_get_node_wtm_rel_c");
    das::addExtern<DAS_BIND_FUN(geomtree_get_node_tm_scalar)>(*this, lib, "geomtree_getNodeTmScalar", das::SideEffects::modifyArgument,
      "bind_dascript::geomtree_get_node_tm_scalar");
    das::addExtern<DAS_BIND_FUN(geomtree_set_node_wtm_rel_scalar)>(*this, lib, "geomtree_setNodeWtmRelScalar",
      das::SideEffects::modifyArgument, "bind_dascript::geomtree_set_node_wtm_rel_scalar");
    das::addExtern<DAS_BIND_FUN(geomtree_set_node_tm_scalar)>(*this, lib, "geomtree_setNodeTmScalar", das::SideEffects::modifyArgument,
      "bind_dascript::geomtree_set_node_tm_scalar");
    das::addExtern<DAS_BIND_FUN(geomtree_get_node_wpos)>(*this, lib, "geomtree_getNodeWpos", das::SideEffects::none,
      "bind_dascript::geomtree_get_node_wpos");
    das::addExtern<DAS_BIND_FUN(geomtree_get_node_wtm)>(*this, lib, "geomtree_getNodeWtmScalar", das::SideEffects::modifyArgument,
      "bind_dascript::geomtree_get_node_wtm");
    das::addExtern<DAS_BIND_FUN(geomtree_get_node_rel_wtm)>(*this, lib, "geomtree_getNodeWtmRelScalar",
      das::SideEffects::modifyArgument, "bind_dascript::geomtree_get_node_rel_wtm");
    das::addExtern<DAS_BIND_FUN(geomtree_set_node_wtm)>(*this, lib, "geomtree_setNodeWtmScalar", das::SideEffects::modifyArgument,
      "bind_dascript::geomtree_set_node_wtm");
    das::addExtern<DAS_BIND_FUN(geomtree_get_wtm_ofs)>(*this, lib, "geomtree_getWtmOfs", das::SideEffects::none,
      "bind_dascript::geomtree_get_wtm_ofs");
    das::addExtern<DAS_BIND_FUN(geomtree_get_node_wpos_rel)>(*this, lib, "geomtree_getNodeWposRel", das::SideEffects::none,
      "bind_dascript::geomtree_get_node_wpos_rel");
    das::addExtern<DAS_BIND_FUN(geomtree_calc_optimal_wofs)>(*this, lib, "geomtree_calc_optimal_wofs", das::SideEffects::none,
      "bind_dascript::geomtree_calc_optimal_wofs");
    das::addExtern<DAS_BIND_FUN(geomtree_change_root_pos)>(*this, lib, "geomtree_changeRootPos", das::SideEffects::modifyArgument,
      "bind_dascript::geomtree_change_root_pos");

    das::addExtern<DAS_BIND_FUN(geomtree_invalidateWtm)>(*this, lib, "geomtree_invalidateWtm", das::SideEffects::modifyArgument,
      "bind_dascript::geomtree_invalidateWtm");
    das::addExtern<DAS_BIND_FUN(geomtree_invalidateWtm0)>(*this, lib, "geomtree_invalidateWtm", das::SideEffects::modifyArgument,
      "bind_dascript::geomtree_invalidateWtm0");
    das::addExtern<DAS_BIND_FUN(geomtree_markNodeTmInvalid)>(*this, lib, "geomtree_markNodeTmInvalid",
      das::SideEffects::modifyArgument, "bind_dascript::geomtree_markNodeTmInvalid");
    das::addExtern<DAS_BIND_FUN(geomtree_validateTm)>(*this, lib, "geomtree_validateTm", das::SideEffects::modifyArgument,
      "bind_dascript::geomtree_validateTm");
    using method_calcWtm = das::das_call_member<void (GeomNodeTree::*)(), &GeomNodeTree::calcWtm>;
    das::addExtern<DAS_CALL_METHOD(method_calcWtm)>(*this, lib, "geomtree_calcWtm", das::SideEffects::modifyArgument,
      "das_call_member<void(GeomNodeTree::*)(), &GeomNodeTree::calcWtm>::invoke");
    das::addExtern<DAS_BIND_FUN(geomtree_getParentNodeIdx)>(*this, lib, "geomtree_getParentNodeIdx", das::SideEffects::none,
      "bind_dascript::geomtree_getParentNodeIdx");
    das::addExtern<DAS_BIND_FUN(geomtree_getNodeName)>(*this, lib, "geomtree_getNodeName", das::SideEffects::none,
      "bind_dascript::geomtree_getNodeName");
    das::addExtern<DAS_BIND_FUN(geomtree_getChildCount)>(*this, lib, "geomtree_getChildCount", das::SideEffects::none,
      "bind_dascript::geomtree_getChildCount");
    das::addExtern<DAS_BIND_FUN(geomtree_getChildNodeIdx)>(*this, lib, "geomtree_getChildNodeIdx", das::SideEffects::none,
      "bind_dascript::geomtree_getChildNodeIdx");

    das::addExtern<DAS_BIND_FUN(geomtree_get_node_wtm_rel_ptr)>(*this, lib, "get_node_wtm_rel_ptr", das::SideEffects::none,
      "bind_dascript::geomtree_get_node_wtm_rel_ptr");
    das::addExtern<DAS_BIND_FUN(GeomNodeTree::mat44f_to_TMatrix)>(*this, lib, "mat44f_to_TMatrix", das::SideEffects::modifyArgument,
      "GeomNodeTree::mat44f_to_TMatrix");

    using method_calcWorldBox = das::das_call_member<void (GeomNodeTree::*)(bbox3f &) const, &GeomNodeTree::calcWorldBox>;
    das::addExtern<DAS_CALL_METHOD(method_calcWorldBox)>(*this, lib, "geomtree_calcWorldBox", das::SideEffects::modifyArgument,
      "das_call_member<void(GeomNodeTree::*)(bbox3f &) const, &GeomNodeTree::calcWorldBox>::invoke");
    using method_calcWorldBox2 = das::das_call_member<void (GeomNodeTree::*)(BBox3 &) const, &GeomNodeTree::calcWorldBox>;
    das::addExtern<DAS_CALL_METHOD(method_calcWorldBox2)>(*this, lib, "geomtree_calcWorldBox", das::SideEffects::modifyArgument,
      "das_call_member<void(GeomNodeTree::*)(BBox3 &) const, &GeomNodeTree::calcWorldBox>::invoke");

    das::addExtern<DAS_BIND_FUN(geomtree_recalc_tm)>(*this, lib, "geomtree_recalcTm", das::SideEffects::modifyArgument,
      "bind_dascript::geomtree_recalc_tm");

    using method_setWtmOfs = DAS_CALL_MEMBER(::GeomNodeTree::setWtmOfs);
    das::addExtern<DAS_CALL_METHOD(method_setWtmOfs)>(*this, lib, "geomtree_setWtmOfs", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(::GeomNodeTree::setWtmOfs));

    das::addExtern<DAS_BIND_FUN(geomtree_calcWtmForBranch)>(*this, lib, "geomtree_calcWtmForBranch", das::SideEffects::modifyArgument,
      "bind_dascript::geomtree_calcWtmForBranch");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotGeomNodeTree.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(GeomNodeTreeModule, bind_dascript);
