#include <shaders/dag_shaderVar.h>
#include <math/dag_color.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_roDataBlock.h>
#include <shaders/dag_shaders.h>
#include <EASTL/string.h>
#include <ska_hash_map/flat_hash_map2.hpp>


bool dgs_all_shader_vars_optionals = false;

struct SavedVar
{
  SavedVar(int val)
  {
    type = SHVT_INT;
    intVal = val;
  }
  SavedVar(float val)
  {
    type = SHVT_REAL;
    floatVal = val;
  }
  SavedVar(const Color4 &val)
  {
    type = SHVT_COLOR4;
    colorVal = val;
  }

  int type;
  union
  {
    int intVal;
    float floatVal;
    Color4 colorVal;
  };
};

static ska::flat_hash_map<eastl::string, SavedVar> saved_vars;

void ShaderGlobal::set_vars_from_blk(const DataBlock &blk, bool loadTexures, bool allowAddTextures)
{
  if (!blk.getBool("__enable_group", true))
    return;

  for (int i = 0; i < blk.paramCount(); ++i)
  {
    const char *name = blk.getParamName(i);
    int id = VariableMap::getVariableId(name);
    if (!VariableMap::isGlobVariablePresent(id))
      continue;

    switch (blk.getParamType(i))
    {
      case DataBlock::TYPE_BOOL:
        saved_vars.emplace(name, ShaderGlobal::get_int(id));
        ShaderGlobal::set_int(id, blk.getBool(i) ? 1 : 0);
        break;

      case DataBlock::TYPE_INT:
        saved_vars.emplace(name, ShaderGlobal::get_int(id));
        ShaderGlobal::set_int(id, blk.getInt(i));
        break;

      case DataBlock::TYPE_REAL:
        saved_vars.emplace(name, ShaderGlobal::get_real(id));
        ShaderGlobal::set_real(id, blk.getReal(i));
        break;

      case DataBlock::TYPE_POINT3:
      {
        saved_vars.emplace(name, ShaderGlobal::get_color4(id));
        Point3 v = blk.getPoint3(i);
        ShaderGlobal::set_color4(id, Color4(v.x, v.y, v.z));
      }
      break;

      case DataBlock::TYPE_POINT4:
        saved_vars.emplace(name, ShaderGlobal::get_color4(id));
        ShaderGlobal::set_color4(id, Color4::xyzw(blk.getPoint4(i)));
        break;

      case DataBlock::TYPE_E3DCOLOR:
      {
        saved_vars.emplace(name, ShaderGlobal::get_color4(id));
        real mul = blk.getReal(String("__mul_") + blk.getParamName(i), 1.0f);
        Color4 v = color4(blk.getE3dcolor(i));
        v.r *= mul;
        v.g *= mul;
        v.b *= mul;
        ShaderGlobal::set_color4(id, v);
      }
      break;

      case DataBlock::TYPE_STRING:
      {
        if (loadTexures)
        {
          // debug("set_vars_from_blk: %s %s\n", blk.getParamName(i), blk.getStr(i));

          int gvid = get_shader_glob_var_id(blk.getParamName(i), true);
          TEXTUREID last_tid = ShaderGlobal::get_tex_fast(gvid);
          ShaderGlobal::set_texture_fast(gvid, BAD_TEXTUREID);
          if (gvid < 0)
            logwarn("cannot set tex <%s> to var <%s> - shader globvar is missing", blk.getStr(i), blk.getParamName(i));
          else
          {
            TEXTUREID tid = get_managed_texture_id(String(64, "%s*", blk.getStr(i)));
            if (tid == BAD_TEXTUREID)
              tid = get_managed_texture_id(blk.getStr(i));

            if (tid == BAD_TEXTUREID && allowAddTextures)
              tid = add_managed_texture(blk.getStr(i));

            if (tid == BAD_TEXTUREID)
              logwarn("cannot set neigher <%s*> nor <%s> tex to var <%s> - texture is missing", blk.getStr(i), blk.getParamName(i));
            else
            {
              if (last_tid != tid)
                debug("set tex <%s> to globvar <%s>, %d->%d", blk.getStr(i), blk.getParamName(i), last_tid, tid);
              ShaderGlobal::set_texture_fast(gvid, tid);
            }
          }
        }
      }
      break;
    }
  }

  for (int i = 0; i < blk.blockCount(); ++i)
    ShaderGlobal::set_vars_from_blk(*blk.getBlock(i), loadTexures);
}

void ShaderGlobal::set_vars_from_blk(const RoDataBlock &blk)
{
  if (!blk.getBool("__enable_group", true))
    return;

  for (int i = 0; i < blk.paramCount(); ++i)
  {
    const char *name = blk.getParamName(i);
    int id = VariableMap::getVariableId(name);
    if (!VariableMap::isGlobVariablePresent(id))
      continue;

    switch (blk.getParamType(i))
    {
      case RoDataBlock::TYPE_BOOL:
        saved_vars.emplace(name, ShaderGlobal::get_int(id));
        ShaderGlobal::set_int(id, blk.getBool(i) ? 1 : 0);
        break;

      case RoDataBlock::TYPE_INT:
        saved_vars.emplace(name, ShaderGlobal::get_int(id));
        ShaderGlobal::set_int(id, blk.getInt(i));
        break;

      case RoDataBlock::TYPE_REAL:
        saved_vars.emplace(name, ShaderGlobal::get_real(id));
        ShaderGlobal::set_real(id, blk.getReal(i));
        break;

      case RoDataBlock::TYPE_POINT3:
      {
        saved_vars.emplace(name, ShaderGlobal::get_color4(id));
        Point3 v = blk.getPoint3(i);
        ShaderGlobal::set_color4(id, Color4(v.x, v.y, v.z));
      }
      break;

      case RoDataBlock::TYPE_POINT4:
        saved_vars.emplace(name, ShaderGlobal::get_color4(id));
        ShaderGlobal::set_color4(id, Color4::xyzw(blk.getPoint4(i)));
        break;

      case RoDataBlock::TYPE_E3DCOLOR:
      {
        saved_vars.emplace(name, ShaderGlobal::get_color4(id));
        real mul = blk.getReal(String("__mul_") + blk.getParamName(i), 1.0f);
        Color4 v = color4(blk.getE3dcolor(i));
        v.r *= mul;
        v.g *= mul;
        v.b *= mul;
        ShaderGlobal::set_color4(id, v);
      }
      break;
    }
  }

  for (int i = 0; i < blk.blockCount(); ++i)
    ShaderGlobal::set_vars_from_blk(*blk.getBlock(i));
}

void ShaderGlobal::reset_vars_from_blk()
{
  for (const auto &var : saved_vars)
  {
    int id = VariableMap::getVariableId(var.first.c_str());
    if (!VariableMap::isGlobVariablePresent(id))
      continue;

    switch (var.second.type)
    {
      case SHVT_INT: ShaderGlobal::set_int(id, var.second.intVal); break;

      case SHVT_REAL: ShaderGlobal::set_real(id, var.second.floatVal); break;

      case SHVT_COLOR4: ShaderGlobal::set_color4(id, var.second.colorVal); break;
    }
  }
}
