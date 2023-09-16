//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_resPtr.h>
#include <3d/dag_textureIDHolder.h>
#include <EASTL/unique_ptr.h>

class PostFxRenderer;
class ComputeShaderElement;
class GenericTonemapLUT
{
public:
  enum class HDROutput
  {
    LDR,
    HDR
  };
  bool init(const char *lut_name, const char *render_shader_name, const char *compute_shader_name, HDROutput hdr, int lut_size = 32);
  bool perform();
  const UniqueTexHolder &getLUT() const { return lut; }
  const int getLUTSize() const { return lutSize; }
  bool isValid() const { return renderLUT || computeLUT; }

  GenericTonemapLUT();
  ~GenericTonemapLUT();

protected:
  GenericTonemapLUT &operator=(const GenericTonemapLUT &) = delete;
  GenericTonemapLUT(const GenericTonemapLUT &) = delete;
  int lutSize = 32;
  UniqueTexHolder lut;
  eastl::unique_ptr<ComputeShaderElement> computeLUT;
  eastl::unique_ptr<PostFxRenderer> renderLUT;
};
