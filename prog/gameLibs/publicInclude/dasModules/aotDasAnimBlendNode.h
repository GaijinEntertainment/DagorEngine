//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <daScript/simulate/aot.h>
#include <dasModules/aotAnimchar.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>


MAKE_TYPE_FACTORY(AnimPostBlendCtrlContext, AnimV20::AnimPostBlendCtrl::Context);

static constexpr uint32_t BLEND_NODE_CONTEXT_INVALID_OFFSET = eastl::numeric_limits<uint32_t>::max();

struct AnimPostBlendControllerDasContext
{
  eastl::string name; // controller class name as used in *.animTree.blk (block name)
  uint64_t mangledNameHash;
  das::StructInfo *structInfo = nullptr;
  das::Context *context = nullptr;

  das::SimFunction *ctor = nullptr;
  uint32_t gen = 1u; // bumped on every (re)compile, drives hot-reload of the das instance if enabled
  uint32_t createNodeOffset = BLEND_NODE_CONTEXT_INVALID_OFFSET;
  uint32_t initOffset = BLEND_NODE_CONTEXT_INVALID_OFFSET;
  uint32_t processOffset = BLEND_NODE_CONTEXT_INVALID_OFFSET;
  uint32_t advanceOffset = BLEND_NODE_CONTEXT_INVALID_OFFSET;
  uint32_t setDefaultStateOffset = BLEND_NODE_CONTEXT_INVALID_OFFSET;
  uint32_t thisCtrlOffset = BLEND_NODE_CONTEXT_INVALID_OFFSET; // hidden AnimPostBlendCtrl? back-pointer field
  uint32_t srcBlkOffset = BLEND_NODE_CONTEXT_INVALID_OFFSET;   // hidden DataBlock? pointing at the kept ctrl blk

  explicit AnimPostBlendControllerDasContext(eastl::string name_, uint64_t mnh) : name(eastl::move(name_)), mangledNameHash(mnh) {}

  void reset();
  void reload(uint64_t mangled_name_hash);
  void updateContext(das::Context *ctx, const das::StructurePtr &st);
};

using DasPostBlendControllerContexts = ska::flat_hash_map<eastl::string, eastl::unique_ptr<AnimPostBlendControllerDasContext>>;

extern das::mutex dasPostBlendControllerContextsMutex;
extern DasPostBlendControllerContexts dasPostBlendControllerContexts;

// [anim_post_blend_controller(name="...")] structure annotation. Turns a das class into a post-blend
// controller class that AnimResManagerV20::add_bn() can instantiate by block name.
struct AnimDasPostBlendControlerAnnotation : das::StructureAnnotation
{
  AnimDasPostBlendControlerAnnotation() : das::StructureAnnotation("anim_post_blend_controller") {}
  static bool internalField(const eastl::string &field_name);

  bool touch(const das::StructurePtr &st, das::ModuleGroup & /*libGroup*/, const das::AnnotationArgumentList &args,
    das::string &err) override;

  bool look(const das::StructurePtr &st, das::ModuleGroup & /*libGroup*/, const das::AnnotationArgumentList &args,
    das::string & /*err*/) override;

  void complete(das::Context *context, const das::StructurePtr &) override;
};

void register_das_blend_node_creator();
