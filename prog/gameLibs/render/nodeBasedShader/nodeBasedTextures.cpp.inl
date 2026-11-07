// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_resPtr.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <render/nodeBasedShader.h>
#include "nodeBasedTextures.h"

struct NBSTexSlot
{
  SharedTex texture;
  d3d::SamplerHandle sampler = d3d::INVALID_SAMPLER_HANDLE;
};

static constexpr int SLOT_COUNT = 6;
static constexpr int SYSTEM_COUNT = static_cast<int>(NodeBasedShaderType::COUNT);
static eastl::array<eastl::array<NBSTexSlot, SLOT_COUNT>, SYSTEM_COUNT> nbs_tex_slots;

namespace var
{
static eastl::array<ShaderVariableInfo, SLOT_COUNT> nbs_tex_slot_vars = {
  ShaderVariableInfo("nbs_slot_tex_0", true),
  ShaderVariableInfo("nbs_slot_tex_1", true),
  ShaderVariableInfo("nbs_slot_tex_2", true),
  ShaderVariableInfo("nbs_slot_tex_3", true),
  ShaderVariableInfo("nbs_slot_tex_4", true),
  ShaderVariableInfo("nbs_slot_tex_5", true),
};
static eastl::array<ShaderVariableInfo, SLOT_COUNT> nbs_sampler_slot_vars = {
  ShaderVariableInfo("nbs_slot_tex_0_samplerstate", true),
  ShaderVariableInfo("nbs_slot_tex_1_samplerstate", true),
  ShaderVariableInfo("nbs_slot_tex_2_samplerstate", true),
  ShaderVariableInfo("nbs_slot_tex_3_samplerstate", true),
  ShaderVariableInfo("nbs_slot_tex_4_samplerstate", true),
  ShaderVariableInfo("nbs_slot_tex_5_samplerstate", true),
};
} // namespace var

static eastl::array<NBSTexSlot, SLOT_COUNT> &get_nbs_slots_for_type(NodeBasedShaderType type)
{
  if (type == NodeBasedShaderType::COUNT)
  {
    logerr("Texture slots for type %d are not handled correctly, defaulting to volfog", int(type));
    return nbs_tex_slots[0];
  }

  return nbs_tex_slots[static_cast<int>(type)];
}

static bool nbs_type_from_name(const ecs::string &name, NodeBasedShaderType &out_type)
{
  for (NodeBasedShaderType type : {NodeBasedShaderType::Fog, NodeBasedShaderType::EnviCover, NodeBasedShaderType::Clouds})
  {
    if (get_shader_suffix(type) == name.c_str())
    {
      out_type = type;
      return true;
    }
  }
  return false;
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void add_nbs_texture_es(const ecs::Event &, const ecs::string &nbs_texture__system, const ecs::string &nbs_texture__name,
  const ecs::string &nbs_texture__sampler, const int nbs_texture__slot)
{
  NodeBasedShaderType type;
  if (!nbs_type_from_name(nbs_texture__system, type))
  {
    logerr("Could not identify system: %s, in nbs slot texture:%s", nbs_texture__system.c_str(), nbs_texture__name.c_str());
    return;
  }

  if (nbs_texture__slot < 0 || nbs_texture__slot >= SLOT_COUNT)
  {
    logerr("Invalid slot number %d, for nbs slot texture: %s", nbs_texture__slot, nbs_texture__name.c_str());
    return;
  }

  eastl::array<NBSTexSlot, SLOT_COUNT> &slots = get_nbs_slots_for_type(type);
  if (slots[nbs_texture__slot].texture)
  {
    logerr("Tried to bind texture %s to slot %d, but that slot is already bound to %s", nbs_texture__name, nbs_texture__slot,
      slots[nbs_texture__slot].texture->getTexName());
    return;
  }

  NBSTexSlot slot;
  slot.texture = dag::get_tex_gameres(nbs_texture__name.c_str());
  if (!slot.texture)
  {
    logerr("Could not find texture: %s, for nbs slot texture.", nbs_texture__name.c_str());
    return;
  }

  if (!nbs_texture__sampler.empty())
  {
    TextureMetaData metaData;
    String enc(0, "?%s", nbs_texture__sampler.c_str());
    if (!metaData.decodeData(enc.c_str(), false))
    {
      logerr("Could not decode sampler description: %s, for nbs slot texture: %s", nbs_texture__sampler.c_str(),
        nbs_texture__name.c_str());
      return;
    }

    slot.sampler = d3d::request_sampler(get_sampler_info(metaData, true));
  }

  TEXTUREID id = slot.texture.getTexId();
  mark_managed_textures_important({&id, 1});
  prefetch_managed_texture(id);

  slots[nbs_texture__slot] = eastl::move(slot);
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
static void remove_nbs_texture_es(const ecs::Event &, const ecs::string &nbs_texture__system, int nbs_texture__slot)
{
  NodeBasedShaderType type;
  if (!nbs_type_from_name(nbs_texture__system, type) || nbs_texture__slot < 0 || nbs_texture__slot >= SLOT_COUNT)
    return;

  eastl::array<NBSTexSlot, SLOT_COUNT> &slots = get_nbs_slots_for_type(type);
  slots[nbs_texture__slot] = NBSTexSlot{};
}

namespace node_based_texture_slot
{
void bindSlots(NodeBasedShaderType type)
{
  const eastl::array<NBSTexSlot, SLOT_COUNT> &slots = get_nbs_slots_for_type(type);
  for (int slotIdx = 0; slotIdx < SLOT_COUNT; slotIdx++)
  {
    const NBSTexSlot &slot = slots[slotIdx];
    ShaderGlobal::set_texture(var::nbs_tex_slot_vars[slotIdx], slot.texture);

    d3d::SamplerHandle sampler = slot.sampler;
    if (sampler == d3d::INVALID_SAMPLER_HANDLE)
      sampler = get_texture_separate_sampler(slot.texture.getTexId());
    ShaderGlobal::set_sampler(var::nbs_sampler_slot_vars[slotIdx], sampler);
  }
}
} // namespace node_based_texture_slot

namespace nodebasedshaderutils
{
String getBoundNBSTextureNameMap(NodeBasedShaderType type)
{
  const eastl::array<NBSTexSlot, SLOT_COUNT> &slots = get_nbs_slots_for_type(type);
  String result("{");
  for (int i = 0; i < SLOT_COUNT; i++)
  {
    const NBSTexSlot &slot = slots[i];
    if (slot.texture)
    {
      result.aprintf(32, "\"nbs_slot_tex_%d\" : \"%d: ", i, i);
      result += slot.texture->getTexName();
      result += "\"";
    }
    else
    {
      result.aprintf(32, "\"nbs_slot_tex_%d\" : \"%d: unused\"", i, i);
    }

    if (i != SLOT_COUNT - 1)
      result += ", ";
  }
  result += "}";
  return result;
}
} // namespace nodebasedshaderutils
