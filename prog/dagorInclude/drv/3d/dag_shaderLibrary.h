//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_shaderLibraryObject.h>

namespace d3d
{
/// Creates a new shader library.
/// The creation of a shader library may be a time consuming process (several hundred milliseconds) as device drivers compile the
/// generic shader IL into device specific machine code, this function will block until this process is completed.
/// Returns either a new ShaderLibrary object or InvalidShaderLibrary to indicate there was and error. In case of an error, driver
/// should log the cause for the error into the error log. In case of a device reset, the shader library becomes unusable and has to be
/// destroyed and recreated.
ShaderLibrary create_shader_library(const ShaderLibraryCreateInfo &slci);
/// Destroys a shader library.
/// A shader library can be destroyed when other object reference shaders of the shader library.
void destroy_shader_library(ShaderLibrary library);
} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>

namespace d3d
{
inline ShaderLibrary create_shader_library(const ShaderLibraryCreateInfo &slci) { return d3di.create_shader_library(slci); }
inline void destroy_shader_library(ShaderLibrary library) { d3di.destroy_shader_library(library); }
} // namespace d3d
#endif