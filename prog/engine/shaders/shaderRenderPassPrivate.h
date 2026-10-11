// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_shBindumps.h>

struct ScriptedShadersBinDumpOwner;

bool check_render_passes_against_main_dump(const ScriptedShadersBinDumpOwner &owner);

// The expected shape is the one the variant's own dump declares, so a same-named but reshaped pass
// from another loaded dump is not a match.
#if DAGOR_DBGLEVEL > 0
namespace ShaderRenderPass
{
void verify_active_pass(ShaderBindumpHandle dump, int pass_index, int expected_subpass, const char *shader_name);
void verify_active_pass_group(ShaderBindumpHandle dump, int group_index, const char *shader_name);
} // namespace ShaderRenderPass
#endif
