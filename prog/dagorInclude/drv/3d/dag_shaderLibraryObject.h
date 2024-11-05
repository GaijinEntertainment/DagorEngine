//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_inttypes.h>
#include <generic/dag_span.h>

/// Opaque type of a shader library object.
struct ShaderLibraryType;
/// Shader library object handle type.
using ShaderLibrary = ShaderLibraryType *;
/// Value of a invalid handle to ShaderLibrary object, eg NULL.
inline constexpr ShaderLibrary InvalidShaderLibrary = nullptr;

/// Create info structure that contains all the data to create a new shader library that can provide shaders to create pipeline
/// objects.
struct ShaderLibraryCreateInfo
{
  /// Optional debug name, drivers may use this to enhance debug information.
  /// This may be null.
  const char *debugName = nullptr;
  /// Drivers may need to translate indices into names back and forth.
  /// This has to match the exported shaders of the library defined by driverBinary.
  dag::ConstSpan<const char *> nameTable;
  /// Pointer to driver specific binary of a shader library.
  /// Can not be an empty span and stored data must be a valid binary for the current d3d driver.
  dag::ConstSpan<const uint8_t> driverBinary;
  /// If this library may be used to create a pipeline that is expandable, then this has to be set to true otherwise the create of
  /// pipelines that supposed to be expandable and use the library created without this setting may fail.
  bool mayBeUsedByExpandablePipeline = false;
};

/// This references a shader in a shader library.
/// A valid reference is when 'library' is not InvalidShaderLibrary and is a handle to a alive library object and 'index' is a index is
/// a valid index into the shader library, where the value is less than the number of shaders stored in that shader library referenced
/// with the 'library' member. A value of InvalidShaderLibrary of the member library indicates a 'null reference' (null pointer). Any
/// other combination of values that are not a valid reference or a 'null reference' will result in undefined behavior. Drivers may be
/// able to validate any or none of the inputs.
struct ShaderInLibraryReference
{
  /// Source library
  ShaderLibrary library;
  /// Index into the library that references the shader.
  uint32_t index;
};
