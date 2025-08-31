//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/registry.h>
#include <render/daFrameGraph/primitive.h>

namespace dafg
{

/**
 * \brief Request a node to render full screen postFx with a shader.
 * \param shader_name The name of the shader to use.
 * \param registry The node's declaration registry.
 */
inline void postFx(const char *shader_name, dafg::Registry &registry)
{
  registry.draw(shader_name, DrawPrimitive::TRIANGLE_LIST).primitiveCount(1).instanceCount(1);
}

} // namespace dafg
