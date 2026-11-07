// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shaderVarDump.h"
#include "jsonDumpCommon.h"

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <dag/dag_vectorMap.h>
#include <util/dag_console.h>

#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderVarType.h>
#include <shaders/dag_shaderVariableInfo.h>


// 64 bytes enough for any type (TMatrix4 = 16 floats)
// in arrays each array element is a separate global var,
// so their changes are also handled
using ShVarValueSlot = eastl::array<uint8_t, 64>;

struct ShVarInfo
{
  int id;
  int type;
  const char *name;
  ShVarValueSlot value;
};

struct ShVarChange
{
  uint64_t varIdx;
  ShVarValueSlot before;
  ShVarValueSlot after;
};


struct ShadervarDumpState
{
  dag::Vector<ShVarInfo> globvars{};

  dag::VectorMap<const D3DRESID, String> managedNames{};
  dag::VectorMap<const D3dResource *, String> resourceNames{};

  dag::VectorMap<dafg::intermediate::NodeIndex, dag::Vector<ShVarChange>> nodesChanges{};

  String customFilename{};
  bool captureRequested = false;
  bool captureActive = false;
} static state;


static const char *shvar_type_name(const int type)
{
  switch (type)
  {
    case SHVT_INT: return "int";
    case SHVT_REAL: return "float";
    case SHVT_INT4: return "int4";
    case SHVT_COLOR4: return "float4";
    case SHVT_FLOAT4X4: return "float4x4";
    case SHVT_FLOAT4x3: return "float4x3";
    case SHVT_TEXTURE: return "texture";
    case SHVT_BUFFER: return "buffer";
    case SHVT_SAMPLER: return "sampler";
    case SHVT_TLAS: return "tlas";
    default: return "unknown";
  }
}


static void read_shvar_value(const int shvar_id, const int shvar_type, ShVarValueSlot &slot)
{
  uint8_t *data = slot.data();
  memset(data, 0, slot.size());

  switch (shvar_type)
  {
    case SHVT_INT:
    {
      const int v = ShaderGlobal::get_int(shvar_id);
      memcpy(data, &v, sizeof(v));
      break;
    }
    case SHVT_REAL:
    {
      const float v = ShaderGlobal::get_float(shvar_id);
      memcpy(data, &v, sizeof(v));
      break;
    }
    case SHVT_INT4:
    {
      const IPoint4 v = ShaderGlobal::get_int4(shvar_id);
      memcpy(data, &v, sizeof(v));
      break;
    }
    case SHVT_COLOR4:
    {
      const Color4 v = ShaderGlobal::get_float4(shvar_id);
      memcpy(data, &v, sizeof(v));
      break;
    }
    case SHVT_FLOAT4X4:
    {
      const TMatrix4 v = ShaderGlobal::get_float4x4(shvar_id);
      memcpy(data, &v, sizeof(v));
      break;
    }
    case SHVT_FLOAT4x3:
    {
      const TMatrix v = ShaderGlobal::get_float4x3(shvar_id);
      memcpy(data, &v, sizeof(v));
      break;
    }
    case SHVT_TEXTURE:
    {
      const TEXTUREID textureId = ShaderGlobal::get_tex(shvar_id);
      const BaseTexture *texturePtr = ShaderGlobal::get_tex_ptr(shvar_id);
      const char *nameManaged = get_managed_res_name(textureId);
      const char *nameTexture = texturePtr ? texturePtr->getTexName() : nullptr;

      memcpy(data + 0, &textureId, sizeof(textureId));
      memcpy(data + 8, &texturePtr, sizeof(texturePtr));
      state.managedNames.insert(textureId).first->second = nameManaged;
      state.resourceNames.insert(texturePtr).first->second = nameTexture;

      break;
    }
    case SHVT_BUFFER:
    {
      const D3DRESID bufferId = ShaderGlobal::get_buf(shvar_id);
      const Sbuffer *bufferPtr = ShaderGlobal::get_buf_ptr(shvar_id);
      const char *nameManaged = get_managed_res_name(bufferId);
      const char *nameBuffer = bufferPtr ? bufferPtr->getBufName() : nullptr;

      memcpy(data + 0, &bufferId, sizeof(bufferId));
      memcpy(data + 8, &bufferPtr, sizeof(bufferPtr));
      state.managedNames.insert(bufferId).first->second = nameManaged;
      state.resourceNames.insert(bufferPtr).first->second = nameBuffer;

      break;
    }
    case SHVT_SAMPLER:
    {
      const d3d::SamplerHandle v = ShaderGlobal::get_sampler(shvar_id);
      memcpy(data, &v, sizeof(v));
      break;
    }
    case SHVT_TLAS:
    {
      const RaytraceTopAccelerationStructure *v = ShaderVariableInfo(shvar_id).get_tlas();
      memcpy(data, &v, sizeof(v));
      break;
    }
    default: break;
  }
}


static void write_int_array(rapidjson::PrettyWriter<rapidjson::StringBuffer> &w, const void *data, int count)
{
  int *v = (int *)data;
  w.StartArray();
  for (int i = 0; i < count; ++i)
    w.Int(v[i]);
  w.EndArray();
}

static void write_float_array(rapidjson::PrettyWriter<rapidjson::StringBuffer> &w, const void *data, int count)
{
  float *v = (float *)data;
  w.StartArray();
  for (int i = 0; i < count; ++i)
    w.Double(double(v[i]));
  w.EndArray();
}

static void write_shvar_value(rapidjson::PrettyWriter<rapidjson::StringBuffer> &w, const int shvar_type, const ShVarValueSlot &slot)
{
  const uint8_t *data = slot.data();

  switch (shvar_type)
  {
    case SHVT_INT:
    {
      w.Int(*(int *)data);
      break;
    }
    case SHVT_REAL:
    {
      w.Double(double(*(float *)data));
      break;
    }
    case SHVT_INT4:
    {
      write_int_array(w, data, sizeof(IPoint4) / sizeof(int));
      break;
    }
    case SHVT_COLOR4:
    {
      write_float_array(w, data, sizeof(Color4) / sizeof(float));
      break;
    }
    case SHVT_FLOAT4X4:
    {
      write_float_array(w, data, sizeof(TMatrix4) / sizeof(float));
      break;
    }
    case SHVT_FLOAT4x3:
    {
      write_float_array(w, data, sizeof(TMatrix) / sizeof(float));
      break;
    }
    case SHVT_TEXTURE:
    {
      const TEXTUREID textureId = *(TEXTUREID *)(data + 0);
      const BaseTexture *texturePrt = *(BaseTexture **)(data + 8);
      const String &managedName = state.managedNames.at_key(textureId);
      const String &textureName = state.resourceNames.at_key(texturePrt);

      w.StartObject();
      {
        w.Key("id");
        w.Uint((unsigned)textureId);

        w.Key("ptr");
        w.Uint64((uint64_t)texturePrt);

        w.Key("managedName");
        managedName.empty() ? w.Null() : w.String(managedName);

        w.Key("textureName");
        textureName.empty() ? w.Null() : w.String(textureName);
      }
      w.EndObject();
      break;
    }
    case SHVT_BUFFER:
    {
      const D3DRESID bufferId = *(D3DRESID *)(data + 0);
      const Sbuffer *bufferPrt = *(Sbuffer **)(data + 8);
      const String &managedName = state.managedNames.at_key(bufferId);
      const String &bufferName = state.resourceNames.at_key(bufferPrt);

      w.StartObject();
      {
        w.Key("id");
        w.Uint((unsigned)bufferId);

        w.Key("ptr");
        w.Uint64((uint64_t)bufferPrt);

        w.Key("managedName");
        managedName.empty() ? w.Null() : w.String(managedName);

        w.Key("bufferName");
        bufferName.empty() ? w.Null() : w.String(bufferName);
      }
      w.EndObject();
      break;
    }
    case SHVT_SAMPLER:
    case SHVT_TLAS:
    {
      w.Uint64(*(uint64_t *)data);
      break;
    }
    default:
    {
      w.Null();
      break;
    }
  }
}


namespace dafg::shadervar_dump
{

void request(const char *custom_filename)
{
  state.customFilename = custom_filename;
  state.captureRequested = true;
}

void begin_frame(const intermediate::Graph &irGraph)
{
  if (!state.captureRequested)
    return;

  TIME_PROFILE(dafg_dump_shvar_begin_frame)

  state.captureRequested = false;
  state.captureActive = true;

  const int globvarCount = VariableMap::getGlobalVariablesCount();

  state.globvars.clear();
  state.globvars.reserve(globvarCount);
  for (int varIdx = 0; varIdx < globvarCount; ++varIdx)
  {
    const char *varName = VariableMap::getGlobalVariableName(varIdx);
    const int varId = varName ? VariableMap::getVariableId(varName) : -1;
    const int varType = varId >= 0 ? ShaderGlobal::get_var_type(varId) : -1;
    if (varId < 0 || varType < 0)
      continue;

    auto &newInfo = state.globvars.push_back({varId, varType, varName, {}});
    read_shvar_value(varId, varType, newInfo.value);
  }

  state.nodesChanges.clear();
  state.nodesChanges.reserve(irGraph.nodes.totalKeys());
}

void after_node(const intermediate::NodeIndex node_idx)
{
  if (!state.captureActive)
    return;

  TIME_PROFILE(dafg_dump_shvar_after_node)

  ShVarValueSlot newValue;
  dag::Vector<ShVarChange> nodeChanges;
  for (uint64_t varIdx = 0; varIdx < state.globvars.size(); ++varIdx)
  {
    auto &info = state.globvars[varIdx];

    read_shvar_value(info.id, info.type, newValue);
    if (newValue != info.value)
      nodeChanges.push_back({varIdx, info.value, newValue});

    info.value = newValue;
  }
  if (!nodeChanges.empty())
    state.nodesChanges.insert({node_idx, eastl::move(nodeChanges)});
}

void end_frame(const intermediate::Graph &irGraph)
{
  if (!state.captureActive)
    return;

  TIME_PROFILE(dafg_dump_shvar_end_frame)

  state.captureActive = false;

  if (state.nodesChanges.empty())
  {
    console::print_d("daFG: no shadervar changes detected");
    return;
  }

  console::print_d("daFG: %d nodes with shadervar changes", state.nodesChanges.size());

  rapidjson::StringBuffer sb;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> w(sb);
  w.SetIndent(' ', 2);

  w.StartObject();
  for (const auto &[nodeIdx, nodeChanges] : state.nodesChanges)
  {
    w.Key(irGraph.nodeNames[nodeIdx].c_str());

    w.StartArray();
    for (const auto &change : nodeChanges)
    {
      const auto &info = state.globvars[change.varIdx];

      w.StartObject();
      {
        w.Key("id");
        w.Int(info.id);

        w.Key("name");
        w.String(info.name);

        w.Key("type");
        w.String(shvar_type_name(info.type));

        w.Key("before");
        write_shvar_value(w, info.type, change.before);

        w.Key("after");
        write_shvar_value(w, info.type, change.after);
      }
      w.EndObject();
    }
    w.EndArray();
  }
  w.EndObject();


  String &filename = state.customFilename;
  if (filename.empty())
  {
    DagorDateTime dt;
    get_local_time(&dt);
    filename.printf(0, "dafgShaderVarDump_%02d-%02d-%02d", dt.hour, dt.minute, dt.second);
  }

  jsondump::write_to_file(filename, sb);
}

} // namespace dafg::shadervar_dump
