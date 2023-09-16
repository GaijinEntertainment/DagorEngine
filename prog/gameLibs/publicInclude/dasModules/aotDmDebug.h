//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/aotDagorMath.h>
#include <dasModules/aotEcs.h>
#include <damageModel/dmObject.h>
#include <damageModel/debug/dmDbgTools.h>
#include <damageModel/debug/dmDbgRender.h>

DAS_BIND_ENUM_CAST(dm::dbg::Tool)
DAS_BASE_BIND_ENUM(dm::dbg::Tool, DmDebugTool, CAMSHOT, TRACE_STATS, ARMOR_STRENGTH, EVENT_LOG, EVENT_RENDER, DAMAGE_PROCESSING_STATS,
  HIT_STATS, MULTITRACE_LOG, MULTITRACE_RENDER, PROTECTION_MAP, SONIC_WAVE, DAMAGE_AREA);

namespace bind_dascript
{
inline bool is_dm_dbg_tool_enabled(int tool_id) { return dm::dbg::is_tool_enabled(tool_id); }

inline void dm_dbg_render_clear_all() { dm::dbg::render::clear(); }

inline void dm_dbg_render_clear(int id, int minor_id) { dm::dbg::render::clear(id, minor_id, dm::ObjectDescriptor()); }

inline void dm_dbg_render_add_line_3d(int id, int minor_id, const das::float3 &p0, const das::float3 &p1, const E3DCOLOR &color)
{
  dm::dbg::render::add_line_3d(id, minor_id, dm::ObjectDescriptor(), reinterpret_cast<const Point3 &>(p0),
    reinterpret_cast<const Point3 &>(p1), color);
}

inline void dm_dbg_render_add_sph(int id, int minor_id, const das::float3 &p, float radius, const E3DCOLOR &color)
{
  dm::dbg::render::add_sph(id, minor_id, dm::ObjectDescriptor(), reinterpret_cast<const Point3 &>(p), radius, color);
}

inline void dm_dbg_render_add_capsule(int id, int minor_id, const das::float3 &p0, const das::float3 &p1, float radius,
  const E3DCOLOR &color)
{
  dm::dbg::render::add_capsule(id, minor_id, dm::ObjectDescriptor(), reinterpret_cast<const Point3 &>(p0),
    reinterpret_cast<const Point3 &>(p1), radius, color);
}
} // namespace bind_dascript
