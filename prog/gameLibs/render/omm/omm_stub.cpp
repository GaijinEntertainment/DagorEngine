// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/omm.h>

#include "shaders/omm_texcoord_formats.hlsli"

#include <debug/dag_assert.h>

namespace render::omm
{

static void assert_stub_call(const char *function) { G_ASSERTF(false, "render::omm stub called: %s", function); }

bool init(Context &) { return false; }

void shutdown(Context &) { assert_stub_call("shutdown"); }

bool begin_bake(Context &, const BakeInput &, BakeHandle &)
{
  assert_stub_call("begin_bake");
  return false;
}

bool has_free_bake_slot(const Context &)
{
  assert_stub_call("has_free_bake_slot");
  return false;
}

bool is_bake_ready(Context &, BakeHandle)
{
  assert_stub_call("is_bake_ready");
  return false;
}

ConsumeBakeResult consume_bake(Context &, BakeHandle, BakeResult &, BakeStats *)
{
  assert_stub_call("consume_bake");
  return ConsumeBakeResult::Failed;
}

bool wait_bake(Context &, BakeHandle, BakeResult &, BakeStats *)
{
  assert_stub_call("wait_bake");
  return false;
}

void discard_bake(Context &, BakeHandle) { assert_stub_call("discard_bake"); }

void clear_result(BakeResult &result)
{
  assert_stub_call("clear_result");
  debug_unregister_bake_result(result);
  result.arrayData.close();
  result.descArray.close();
  result.indexBuffer.close();
  result.arrayBuildDescs.clear();
  result.blasLinkageDescs.clear();
  result.indexFormat = IndexFormat::UINT32;
  result.indexCount = 0;
  result.arrayDataSizeInBytes = 0;
  result.descArraySizeInBytes = 0;
  result.indexBufferSizeInBytes = 0;
}

DebugBakeSource make_debug_bake_source(const BakeInput &, TEXTUREID) { return {}; }

void debug_register_bake_result(const BakeResult &, const DebugBakeResultInfo &) {}

void debug_adopt_bake_result(BakeResult &&, const DebugBakeResultInfo &) {}

void debug_unregister_bake_result(const BakeResult &) {}

void debug_shutdown() {}

raytrace::OpacityMicroMapTriangleArrayBuildInfo make_array_build_info(const BakeResult &, Sbuffer *, uint32_t, uint32_t,
  RaytraceBuildFlags)
{
  assert_stub_call("make_array_build_info");
  return {};
}

RaytraceGeometryDescription::OpacityMicroMapLinkage make_geometry_linkage(const BakeResult &, RaytraceOpacityMicroMapTriangleArray *)
{
  assert_stub_call("make_geometry_linkage");
  return {};
}

} // namespace render::omm
