#pragma once

#include <spirv2Metal/spirv_msl.hpp>
#include "metalShaderType.h"

#include <util/dag_string.h>

class CompilerMSLlocal : public spirv_cross::CompilerMSL
{
public:
  CompilerMSLlocal(std::vector<uint32_t> spirv) : CompilerMSL(spirv) {}

  bool validate(std::stringstream &errors);

private:
  uint32_t getTypeSize(const spirv_cross::SPIRType &type, uint32_t &align) const;
};

inline void setupMSLCompiler(spirv_cross::CompilerMSL &compiler, bool use_ios_token)
{
  spirv_cross::CompilerGLSL::Options options_glsl;
  spirv_cross::CompilerMSL::Options options;
  if (use_ios_token)
  {
    options.platform = spirv_cross::CompilerMSL::Options::iOS;
    options.set_msl_version(2, 3);
  }
  else
    options.set_msl_version(2, 2);
  options.pad_fragment_output_components = true;
  options.enable_decoration_binding = true;
  options.force_native_arrays = true;
  options.ios_support_base_vertex_instance = true;
  // options_glsl.vertex.flip_vert_y = true;
  compiler.set_msl_options(options);
  compiler.set_common_options(options_glsl);

  compiler.set_enabled_interface_variables(compiler.get_active_interface_variables());
}
