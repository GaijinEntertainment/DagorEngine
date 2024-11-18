// Copyright (C) Gaijin Games KFT.  All rights reserved.

// auto generated, do not modify!
#include "../publicInclude/spirv/traits_table.h"
using namespace spirv;
static const char *extension_name_table[] = //
  {"SPV_KHR_multiview", "SPV_KHR_ray_query", "SPV_INTEL_optnone", "SPV_NV_mesh_shader", "SPV_NV_ray_tracing", "SPV_INTEL_io_pipes",
    "SPV_INTEL_fpga_reg", "SPV_AMD_gcn_shader", "SPV_EXT_mesh_shader", "SPV_KHR_ray_tracing", "SPV_NV_shading_rate",
    "SPV_INTEL_subgroups", "SPV_INTEL_loop_fuse", "SPV_KHR_linkonce_odr", "SPV_GOOGLE_user_type", "SPV_KHR_device_group",
    "SPV_KHR_8bit_storage", "SPV_KHR_shader_clock", "SPV_KHR_quad_control", "SPV_KHR_shader_ballot", "SPV_KHR_ray_cull_mask",
    "SPV_AMD_shader_ballot", "SPV_ARM_core_builtins", "SPV_KHR_subgroup_vote", "SPV_KHR_16bit_storage", "SPV_KHR_expect_assume",
    "SPV_KHR_float_controls", "SPV_NV_viewport_array2", "SPV_INTEL_debug_module", "SPV_INTEL_fp_max_error", "SPV_AMDX_shader_enqueue",
    "SPV_NV_bindless_texture", "SPV_KHR_subgroup_rotate", "SPV_KHR_float_controls2", "SPV_INTEL_split_barrier",
    "SPV_EXT_opacity_micromap", "SPV_NV_raw_access_chains", "SPV_INTEL_media_block_io", "SPV_INTEL_vector_compute",
    "SPV_INTEL_blocking_pipes", "SPV_KHR_bit_instructions", "SPV_INTEL_cache_controls", "SPV_KHR_variable_pointers",
    "SPV_QCOM_image_processing", "SPV_NV_shader_sm_builtins", "SPV_EXT_shader_tile_image", "SPV_NV_cooperative_matrix",
    "SPV_INTEL_float_controls2", "SPV_INTEL_inline_assembly", "SPV_INTEL_runtime_aligned", "SPV_INTEL_long_composites",
    "SPV_QCOM_image_processing2", "SPV_EXT_shader_image_int64", "SPV_INTEL_fpga_dsp_control", "SPV_KHR_cooperative_matrix",
    "SPV_GOOGLE_decorate_string", "SPV_KHR_vulkan_memory_model", "SPV_KHR_post_depth_coverage", "SPV_INTEL_kernel_attributes",
    "SPV_INTEL_function_pointers", "SPV_EXT_descriptor_indexing", "SPV_INTEL_fp_fast_math_mode", "SPV_KHR_integer_dot_product",
    "SPV_INTEL_maximum_registers", "SPV_NV_stereo_view_rendering", "SPV_AMD_shader_fragment_mask", "SPV_NV_displacement_micromap",
    "SPV_INTEL_fpga_loop_controls", "SPV_KHR_terminate_invocation", "SPV_EXT_shader_stencil_export", "SPV_KHR_maximal_reconvergence",
    "SPV_INTEL_usm_storage_classes", "SPV_KHR_fragment_shading_rate", "SPV_NV_shader_image_footprint", "SPV_EXT_replicated_composites",
    "SPV_INTEL_bfloat16_conversion", "SPV_AMD_shader_trinary_minmax", "SPV_GOOGLE_hlsl_functionality1",
    "SPV_KHR_shader_draw_parameters", "SPV_EXT_fragment_fully_covered", "SPV_NV_ray_tracing_motion_blur",
    "SPV_INTEL_fpga_memory_accesses", "SPV_INTEL_fpga_buffer_location", "SPV_INTEL_fpga_latency_control",
    "SPV_EXT_physical_storage_buffer", "SPV_KHR_physical_storage_buffer", "SPV_AMD_texture_gather_bias_lod",
    "SPV_INTEL_variable_length_array", "SPV_EXT_shader_atomic_float_add", "SPV_INTEL_masked_gather_scatter",
    "SPV_INTEL_memory_access_aliasing", "SPV_INTEL_fpga_memory_attributes", "SPV_NV_shader_invocation_reorder",
    "SPV_NV_shader_atomic_fp16_vector", "SPV_NV_compute_shader_derivatives", "SPV_EXT_fragment_shader_interlock",
    "SPV_KHR_shader_atomic_counter_ops", "SPV_INTEL_fpga_cluster_attributes", "SPV_EXT_shader_atomic_float16_add",
    "SPV_KHR_no_integer_wrap_decoration", "SPV_NV_geometry_shader_passthrough", "SPV_NV_fragment_shader_barycentric",
    "SPV_NV_shader_subgroup_partitioned", "SPV_ARM_cooperative_matrix_layouts", "SPV_KHR_ray_tracing_position_fetch",
    "SPV_INTEL_fpga_argument_interfaces", "SPV_KHR_uniform_group_instructions", "SPV_KHR_fragment_shader_barycentric",
    "SPV_EXT_fragment_invocation_density", "SPV_AMD_gpu_shader_half_float_fetch", "SPV_AMD_shader_image_load_store_lod",
    "SPV_EXT_shader_viewport_index_layer", "SPV_EXT_demote_to_helper_invocation", "SPV_INTEL_shader_integer_functions2",
    "SPV_EXT_shader_atomic_float_min_max", "SPV_KHR_storage_buffer_storage_class", "SPV_NV_sample_mask_override_coverage",
    "SPV_INTEL_unstructured_loop_controls", "SPV_KHR_relaxed_extended_instruction", "SPV_KHR_subgroup_uniform_control_flow",
    "SPV_NVX_multiview_per_view_attributes", "SPV_INTEL_global_variable_host_access", "SPV_INTEL_arbitrary_precision_integers",
    "SPV_AMD_shader_explicit_vertex_parameter", "SPV_KHR_workgroup_memory_explicit_layout",
    "SPV_INTEL_arbitrary_precision_fixed_point", "SPV_INTEL_global_variable_fpga_decorations",
    "SPV_INTEL_device_side_avc_motion_estimation", "SPV_AMD_shader_early_and_late_fragment_tests",
    "SPV_INTEL_arbitrary_precision_floating_point", "SPV_INTEL_fpga_invocation_pipelining_attributes"};
static const size_t extension_name_table_len[] = //
  {17, 17, 17, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23,
    23, 23, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28,
    28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31, 32, 32, 32, 32, 33, 33, 33, 33, 33,
    34, 34, 34, 34, 34, 34, 34, 34, 35, 35, 35, 35, 35, 35, 35, 35, 36, 36, 36, 36, 37, 37, 37, 38, 40, 40, 41, 42, 43, 44, 44, 47};
Extension spirv::extension_name_to_id(const char *name, Id name_len)
{
  for (Id i = 0; i < static_cast<Id>(Extension::Count); ++i)
    if (string_equals(name, extension_name_table[i], name_len))
      return static_cast<Extension>(i);
  return Extension::Count;
};

dag::ConstSpan<char> spirv::extension_id_to_name(Extension ident)
{
  if (ident < Extension::Count)
    return make_span(extension_name_table[static_cast<unsigned>(ident)], extension_name_table_len[static_cast<unsigned>(ident)]);
  return make_span("<invalid extension id>", 9 + 13);
};

static const char *extended_grammar_name_table[] = //
  {"GLSL.std.450", "SPV_AMD_gcn_shader", "SPV_AMD_shader_ballot", "SPV_AMD_shader_trinary_minmax",
    "SPV_AMD_shader_explicit_vertex_parameter"};
static const size_t extended_grammar_name_table_len[] = //
  {12, 18, 21, 29, 40};
ExtendedGrammar spirv::extended_grammar_name_to_id(const char *name, Id name_len)
{
  for (Id i = 0; i < static_cast<Id>(ExtendedGrammar::Count); ++i)
    if (string_equals(name, extended_grammar_name_table[i], name_len))
      return static_cast<ExtendedGrammar>(i);
  return ExtendedGrammar::Count;
};

dag::ConstSpan<char> spirv::extended_grammar_id_to_name(ExtendedGrammar ident)
{
  if (ident < ExtendedGrammar::Count)
    return make_span(extended_grammar_name_table[static_cast<unsigned>(ident)],
      extended_grammar_name_table_len[static_cast<unsigned>(ident)]);
  return make_span("<invalid extended grammar id>", 16 + 13);
};

const char *spirv::name_of(SourceLanguage value)
{
  switch (value)
  {
    case SourceLanguage::Unknown: return "Unknown";
    case SourceLanguage::ESSL: return "ESSL";
    case SourceLanguage::GLSL: return "GLSL";
    case SourceLanguage::OpenCL_C: return "OpenCL_C";
    case SourceLanguage::OpenCL_CPP: return "OpenCL_CPP";
    case SourceLanguage::HLSL: return "HLSL";
    case SourceLanguage::CPP_for_OpenCL: return "CPP_for_OpenCL";
    case SourceLanguage::SYCL: return "SYCL";
    case SourceLanguage::HERO_C: return "HERO_C";
    case SourceLanguage::NZSL: return "NZSL";
    case SourceLanguage::WGSL: return "WGSL";
    case SourceLanguage::Slang: return "Slang";
    case SourceLanguage::Zig: return "Zig";
  }
  return "<unrecognized enum value of SourceLanguage>";
}

const char *spirv::name_of(ExecutionModel value)
{
  switch (value)
  {
    case ExecutionModel::Vertex: return "Vertex";
    case ExecutionModel::TessellationControl: return "TessellationControl";
    case ExecutionModel::TessellationEvaluation: return "TessellationEvaluation";
    case ExecutionModel::Geometry: return "Geometry";
    case ExecutionModel::Fragment: return "Fragment";
    case ExecutionModel::GLCompute: return "GLCompute";
    case ExecutionModel::Kernel: return "Kernel";
    case ExecutionModel::TaskNV: return "TaskNV";
    case ExecutionModel::MeshNV: return "MeshNV";
    case ExecutionModel::RayGenerationKHR: return "RayGenerationKHR";
    case ExecutionModel::IntersectionKHR: return "IntersectionKHR";
    case ExecutionModel::AnyHitKHR: return "AnyHitKHR";
    case ExecutionModel::ClosestHitKHR: return "ClosestHitKHR";
    case ExecutionModel::MissKHR: return "MissKHR";
    case ExecutionModel::CallableKHR: return "CallableKHR";
    case ExecutionModel::TaskEXT: return "TaskEXT";
    case ExecutionModel::MeshEXT: return "MeshEXT";
  }
  return "<unrecognized enum value of ExecutionModel>";
}

const char *spirv::name_of(AddressingModel value)
{
  switch (value)
  {
    case AddressingModel::Logical: return "Logical";
    case AddressingModel::Physical32: return "Physical32";
    case AddressingModel::Physical64: return "Physical64";
    case AddressingModel::PhysicalStorageBuffer64: return "PhysicalStorageBuffer64";
  }
  return "<unrecognized enum value of AddressingModel>";
}

const char *spirv::name_of(MemoryModel value)
{
  switch (value)
  {
    case MemoryModel::Simple: return "Simple";
    case MemoryModel::GLSL450: return "GLSL450";
    case MemoryModel::OpenCL: return "OpenCL";
    case MemoryModel::Vulkan: return "Vulkan";
  }
  return "<unrecognized enum value of MemoryModel>";
}

const char *spirv::name_of(ExecutionMode value)
{
  switch (value)
  {
    case ExecutionMode::Invocations: return "Invocations";
    case ExecutionMode::SpacingEqual: return "SpacingEqual";
    case ExecutionMode::SpacingFractionalEven: return "SpacingFractionalEven";
    case ExecutionMode::SpacingFractionalOdd: return "SpacingFractionalOdd";
    case ExecutionMode::VertexOrderCw: return "VertexOrderCw";
    case ExecutionMode::VertexOrderCcw: return "VertexOrderCcw";
    case ExecutionMode::PixelCenterInteger: return "PixelCenterInteger";
    case ExecutionMode::OriginUpperLeft: return "OriginUpperLeft";
    case ExecutionMode::OriginLowerLeft: return "OriginLowerLeft";
    case ExecutionMode::EarlyFragmentTests: return "EarlyFragmentTests";
    case ExecutionMode::PointMode: return "PointMode";
    case ExecutionMode::Xfb: return "Xfb";
    case ExecutionMode::DepthReplacing: return "DepthReplacing";
    case ExecutionMode::DepthGreater: return "DepthGreater";
    case ExecutionMode::DepthLess: return "DepthLess";
    case ExecutionMode::DepthUnchanged: return "DepthUnchanged";
    case ExecutionMode::LocalSize: return "LocalSize";
    case ExecutionMode::LocalSizeHint: return "LocalSizeHint";
    case ExecutionMode::InputPoints: return "InputPoints";
    case ExecutionMode::InputLines: return "InputLines";
    case ExecutionMode::InputLinesAdjacency: return "InputLinesAdjacency";
    case ExecutionMode::Triangles: return "Triangles";
    case ExecutionMode::InputTrianglesAdjacency: return "InputTrianglesAdjacency";
    case ExecutionMode::Quads: return "Quads";
    case ExecutionMode::Isolines: return "Isolines";
    case ExecutionMode::OutputVertices: return "OutputVertices";
    case ExecutionMode::OutputPoints: return "OutputPoints";
    case ExecutionMode::OutputLineStrip: return "OutputLineStrip";
    case ExecutionMode::OutputTriangleStrip: return "OutputTriangleStrip";
    case ExecutionMode::VecTypeHint: return "VecTypeHint";
    case ExecutionMode::ContractionOff: return "ContractionOff";
    case ExecutionMode::Initializer: return "Initializer";
    case ExecutionMode::Finalizer: return "Finalizer";
    case ExecutionMode::SubgroupSize: return "SubgroupSize";
    case ExecutionMode::SubgroupsPerWorkgroup: return "SubgroupsPerWorkgroup";
    case ExecutionMode::SubgroupsPerWorkgroupId: return "SubgroupsPerWorkgroupId";
    case ExecutionMode::LocalSizeId: return "LocalSizeId";
    case ExecutionMode::LocalSizeHintId: return "LocalSizeHintId";
    case ExecutionMode::NonCoherentColorAttachmentReadEXT: return "NonCoherentColorAttachmentReadEXT";
    case ExecutionMode::NonCoherentDepthAttachmentReadEXT: return "NonCoherentDepthAttachmentReadEXT";
    case ExecutionMode::NonCoherentStencilAttachmentReadEXT: return "NonCoherentStencilAttachmentReadEXT";
    case ExecutionMode::SubgroupUniformControlFlowKHR: return "SubgroupUniformControlFlowKHR";
    case ExecutionMode::PostDepthCoverage: return "PostDepthCoverage";
    case ExecutionMode::DenormPreserve: return "DenormPreserve";
    case ExecutionMode::DenormFlushToZero: return "DenormFlushToZero";
    case ExecutionMode::SignedZeroInfNanPreserve: return "SignedZeroInfNanPreserve";
    case ExecutionMode::RoundingModeRTE: return "RoundingModeRTE";
    case ExecutionMode::RoundingModeRTZ: return "RoundingModeRTZ";
    case ExecutionMode::EarlyAndLateFragmentTestsAMD: return "EarlyAndLateFragmentTestsAMD";
    case ExecutionMode::StencilRefReplacingEXT: return "StencilRefReplacingEXT";
    case ExecutionMode::CoalescingAMDX: return "CoalescingAMDX";
    case ExecutionMode::MaxNodeRecursionAMDX: return "MaxNodeRecursionAMDX";
    case ExecutionMode::StaticNumWorkgroupsAMDX: return "StaticNumWorkgroupsAMDX";
    case ExecutionMode::ShaderIndexAMDX: return "ShaderIndexAMDX";
    case ExecutionMode::MaxNumWorkgroupsAMDX: return "MaxNumWorkgroupsAMDX";
    case ExecutionMode::StencilRefUnchangedFrontAMD: return "StencilRefUnchangedFrontAMD";
    case ExecutionMode::StencilRefGreaterFrontAMD: return "StencilRefGreaterFrontAMD";
    case ExecutionMode::StencilRefLessFrontAMD: return "StencilRefLessFrontAMD";
    case ExecutionMode::StencilRefUnchangedBackAMD: return "StencilRefUnchangedBackAMD";
    case ExecutionMode::StencilRefGreaterBackAMD: return "StencilRefGreaterBackAMD";
    case ExecutionMode::StencilRefLessBackAMD: return "StencilRefLessBackAMD";
    case ExecutionMode::QuadDerivativesKHR: return "QuadDerivativesKHR";
    case ExecutionMode::RequireFullQuadsKHR: return "RequireFullQuadsKHR";
    case ExecutionMode::OutputLinesEXT: return "OutputLinesEXT";
    case ExecutionMode::OutputPrimitivesEXT: return "OutputPrimitivesEXT";
    case ExecutionMode::DerivativeGroupQuadsNV: return "DerivativeGroupQuadsNV";
    case ExecutionMode::DerivativeGroupLinearNV: return "DerivativeGroupLinearNV";
    case ExecutionMode::OutputTrianglesEXT: return "OutputTrianglesEXT";
    case ExecutionMode::PixelInterlockOrderedEXT: return "PixelInterlockOrderedEXT";
    case ExecutionMode::PixelInterlockUnorderedEXT: return "PixelInterlockUnorderedEXT";
    case ExecutionMode::SampleInterlockOrderedEXT: return "SampleInterlockOrderedEXT";
    case ExecutionMode::SampleInterlockUnorderedEXT: return "SampleInterlockUnorderedEXT";
    case ExecutionMode::ShadingRateInterlockOrderedEXT: return "ShadingRateInterlockOrderedEXT";
    case ExecutionMode::ShadingRateInterlockUnorderedEXT: return "ShadingRateInterlockUnorderedEXT";
    case ExecutionMode::SharedLocalMemorySizeINTEL: return "SharedLocalMemorySizeINTEL";
    case ExecutionMode::RoundingModeRTPINTEL: return "RoundingModeRTPINTEL";
    case ExecutionMode::RoundingModeRTNINTEL: return "RoundingModeRTNINTEL";
    case ExecutionMode::FloatingPointModeALTINTEL: return "FloatingPointModeALTINTEL";
    case ExecutionMode::FloatingPointModeIEEEINTEL: return "FloatingPointModeIEEEINTEL";
    case ExecutionMode::MaxWorkgroupSizeINTEL: return "MaxWorkgroupSizeINTEL";
    case ExecutionMode::MaxWorkDimINTEL: return "MaxWorkDimINTEL";
    case ExecutionMode::NoGlobalOffsetINTEL: return "NoGlobalOffsetINTEL";
    case ExecutionMode::NumSIMDWorkitemsINTEL: return "NumSIMDWorkitemsINTEL";
    case ExecutionMode::SchedulerTargetFmaxMhzINTEL: return "SchedulerTargetFmaxMhzINTEL";
    case ExecutionMode::MaximallyReconvergesKHR: return "MaximallyReconvergesKHR";
    case ExecutionMode::FPFastMathDefault: return "FPFastMathDefault";
    case ExecutionMode::StreamingInterfaceINTEL: return "StreamingInterfaceINTEL";
    case ExecutionMode::RegisterMapInterfaceINTEL: return "RegisterMapInterfaceINTEL";
    case ExecutionMode::NamedBarrierCountINTEL: return "NamedBarrierCountINTEL";
    case ExecutionMode::MaximumRegistersINTEL: return "MaximumRegistersINTEL";
    case ExecutionMode::MaximumRegistersIdINTEL: return "MaximumRegistersIdINTEL";
    case ExecutionMode::NamedMaximumRegistersINTEL: return "NamedMaximumRegistersINTEL";
  }
  return "<unrecognized enum value of ExecutionMode>";
}

const char *spirv::name_of(StorageClass value)
{
  switch (value)
  {
    case StorageClass::UniformConstant: return "UniformConstant";
    case StorageClass::Input: return "Input";
    case StorageClass::Uniform: return "Uniform";
    case StorageClass::Output: return "Output";
    case StorageClass::Workgroup: return "Workgroup";
    case StorageClass::CrossWorkgroup: return "CrossWorkgroup";
    case StorageClass::Private: return "Private";
    case StorageClass::Function: return "Function";
    case StorageClass::Generic: return "Generic";
    case StorageClass::PushConstant: return "PushConstant";
    case StorageClass::AtomicCounter: return "AtomicCounter";
    case StorageClass::Image: return "Image";
    case StorageClass::StorageBuffer: return "StorageBuffer";
    case StorageClass::TileImageEXT: return "TileImageEXT";
    case StorageClass::NodePayloadAMDX: return "NodePayloadAMDX";
    case StorageClass::NodeOutputPayloadAMDX: return "NodeOutputPayloadAMDX";
    case StorageClass::CallableDataKHR: return "CallableDataKHR";
    case StorageClass::IncomingCallableDataKHR: return "IncomingCallableDataKHR";
    case StorageClass::RayPayloadKHR: return "RayPayloadKHR";
    case StorageClass::HitAttributeKHR: return "HitAttributeKHR";
    case StorageClass::IncomingRayPayloadKHR: return "IncomingRayPayloadKHR";
    case StorageClass::ShaderRecordBufferKHR: return "ShaderRecordBufferKHR";
    case StorageClass::PhysicalStorageBuffer: return "PhysicalStorageBuffer";
    case StorageClass::HitObjectAttributeNV: return "HitObjectAttributeNV";
    case StorageClass::TaskPayloadWorkgroupEXT: return "TaskPayloadWorkgroupEXT";
    case StorageClass::CodeSectionINTEL: return "CodeSectionINTEL";
    case StorageClass::DeviceOnlyINTEL: return "DeviceOnlyINTEL";
    case StorageClass::HostOnlyINTEL: return "HostOnlyINTEL";
  }
  return "<unrecognized enum value of StorageClass>";
}

const char *spirv::name_of(Dim value)
{
  switch (value)
  {
    case Dim::Dim1D: return "1D";
    case Dim::Dim2D: return "2D";
    case Dim::Dim3D: return "3D";
    case Dim::Cube: return "Cube";
    case Dim::Rect: return "Rect";
    case Dim::Buffer: return "Buffer";
    case Dim::SubpassData: return "SubpassData";
    case Dim::TileImageDataEXT: return "TileImageDataEXT";
  }
  return "<unrecognized enum value of Dim>";
}

const char *spirv::name_of(SamplerAddressingMode value)
{
  switch (value)
  {
    case SamplerAddressingMode::None: return "None";
    case SamplerAddressingMode::ClampToEdge: return "ClampToEdge";
    case SamplerAddressingMode::Clamp: return "Clamp";
    case SamplerAddressingMode::Repeat: return "Repeat";
    case SamplerAddressingMode::RepeatMirrored: return "RepeatMirrored";
  }
  return "<unrecognized enum value of SamplerAddressingMode>";
}

const char *spirv::name_of(SamplerFilterMode value)
{
  switch (value)
  {
    case SamplerFilterMode::Nearest: return "Nearest";
    case SamplerFilterMode::Linear: return "Linear";
  }
  return "<unrecognized enum value of SamplerFilterMode>";
}

const char *spirv::name_of(ImageFormat value)
{
  switch (value)
  {
    case ImageFormat::Unknown: return "Unknown";
    case ImageFormat::Rgba32f: return "Rgba32f";
    case ImageFormat::Rgba16f: return "Rgba16f";
    case ImageFormat::R32f: return "R32f";
    case ImageFormat::Rgba8: return "Rgba8";
    case ImageFormat::Rgba8Snorm: return "Rgba8Snorm";
    case ImageFormat::Rg32f: return "Rg32f";
    case ImageFormat::Rg16f: return "Rg16f";
    case ImageFormat::R11fG11fB10f: return "R11fG11fB10f";
    case ImageFormat::R16f: return "R16f";
    case ImageFormat::Rgba16: return "Rgba16";
    case ImageFormat::Rgb10A2: return "Rgb10A2";
    case ImageFormat::Rg16: return "Rg16";
    case ImageFormat::Rg8: return "Rg8";
    case ImageFormat::R16: return "R16";
    case ImageFormat::R8: return "R8";
    case ImageFormat::Rgba16Snorm: return "Rgba16Snorm";
    case ImageFormat::Rg16Snorm: return "Rg16Snorm";
    case ImageFormat::Rg8Snorm: return "Rg8Snorm";
    case ImageFormat::R16Snorm: return "R16Snorm";
    case ImageFormat::R8Snorm: return "R8Snorm";
    case ImageFormat::Rgba32i: return "Rgba32i";
    case ImageFormat::Rgba16i: return "Rgba16i";
    case ImageFormat::Rgba8i: return "Rgba8i";
    case ImageFormat::R32i: return "R32i";
    case ImageFormat::Rg32i: return "Rg32i";
    case ImageFormat::Rg16i: return "Rg16i";
    case ImageFormat::Rg8i: return "Rg8i";
    case ImageFormat::R16i: return "R16i";
    case ImageFormat::R8i: return "R8i";
    case ImageFormat::Rgba32ui: return "Rgba32ui";
    case ImageFormat::Rgba16ui: return "Rgba16ui";
    case ImageFormat::Rgba8ui: return "Rgba8ui";
    case ImageFormat::R32ui: return "R32ui";
    case ImageFormat::Rgb10a2ui: return "Rgb10a2ui";
    case ImageFormat::Rg32ui: return "Rg32ui";
    case ImageFormat::Rg16ui: return "Rg16ui";
    case ImageFormat::Rg8ui: return "Rg8ui";
    case ImageFormat::R16ui: return "R16ui";
    case ImageFormat::R8ui: return "R8ui";
    case ImageFormat::R64ui: return "R64ui";
    case ImageFormat::R64i: return "R64i";
  }
  return "<unrecognized enum value of ImageFormat>";
}

const char *spirv::name_of(ImageChannelOrder value)
{
  switch (value)
  {
    case ImageChannelOrder::R: return "R";
    case ImageChannelOrder::A: return "A";
    case ImageChannelOrder::RG: return "RG";
    case ImageChannelOrder::RA: return "RA";
    case ImageChannelOrder::RGB: return "RGB";
    case ImageChannelOrder::RGBA: return "RGBA";
    case ImageChannelOrder::BGRA: return "BGRA";
    case ImageChannelOrder::ARGB: return "ARGB";
    case ImageChannelOrder::Intensity: return "Intensity";
    case ImageChannelOrder::Luminance: return "Luminance";
    case ImageChannelOrder::Rx: return "Rx";
    case ImageChannelOrder::RGx: return "RGx";
    case ImageChannelOrder::RGBx: return "RGBx";
    case ImageChannelOrder::Depth: return "Depth";
    case ImageChannelOrder::DepthStencil: return "DepthStencil";
    case ImageChannelOrder::sRGB: return "sRGB";
    case ImageChannelOrder::sRGBx: return "sRGBx";
    case ImageChannelOrder::sRGBA: return "sRGBA";
    case ImageChannelOrder::sBGRA: return "sBGRA";
    case ImageChannelOrder::ABGR: return "ABGR";
  }
  return "<unrecognized enum value of ImageChannelOrder>";
}

const char *spirv::name_of(ImageChannelDataType value)
{
  switch (value)
  {
    case ImageChannelDataType::SnormInt8: return "SnormInt8";
    case ImageChannelDataType::SnormInt16: return "SnormInt16";
    case ImageChannelDataType::UnormInt8: return "UnormInt8";
    case ImageChannelDataType::UnormInt16: return "UnormInt16";
    case ImageChannelDataType::UnormShort565: return "UnormShort565";
    case ImageChannelDataType::UnormShort555: return "UnormShort555";
    case ImageChannelDataType::UnormInt101010: return "UnormInt101010";
    case ImageChannelDataType::SignedInt8: return "SignedInt8";
    case ImageChannelDataType::SignedInt16: return "SignedInt16";
    case ImageChannelDataType::SignedInt32: return "SignedInt32";
    case ImageChannelDataType::UnsignedInt8: return "UnsignedInt8";
    case ImageChannelDataType::UnsignedInt16: return "UnsignedInt16";
    case ImageChannelDataType::UnsignedInt32: return "UnsignedInt32";
    case ImageChannelDataType::HalfFloat: return "HalfFloat";
    case ImageChannelDataType::Float: return "Float";
    case ImageChannelDataType::UnormInt24: return "UnormInt24";
    case ImageChannelDataType::UnormInt101010_2: return "UnormInt101010_2";
    case ImageChannelDataType::UnsignedIntRaw10EXT: return "UnsignedIntRaw10EXT";
    case ImageChannelDataType::UnsignedIntRaw12EXT: return "UnsignedIntRaw12EXT";
  }
  return "<unrecognized enum value of ImageChannelDataType>";
}

const char *spirv::name_of(FPRoundingMode value)
{
  switch (value)
  {
    case FPRoundingMode::RTE: return "RTE";
    case FPRoundingMode::RTZ: return "RTZ";
    case FPRoundingMode::RTP: return "RTP";
    case FPRoundingMode::RTN: return "RTN";
  }
  return "<unrecognized enum value of FPRoundingMode>";
}

const char *spirv::name_of(FPDenormMode value)
{
  switch (value)
  {
    case FPDenormMode::Preserve: return "Preserve";
    case FPDenormMode::FlushToZero: return "FlushToZero";
  }
  return "<unrecognized enum value of FPDenormMode>";
}

const char *spirv::name_of(QuantizationModes value)
{
  switch (value)
  {
    case QuantizationModes::TRN: return "TRN";
    case QuantizationModes::TRN_ZERO: return "TRN_ZERO";
    case QuantizationModes::RND: return "RND";
    case QuantizationModes::RND_ZERO: return "RND_ZERO";
    case QuantizationModes::RND_INF: return "RND_INF";
    case QuantizationModes::RND_MIN_INF: return "RND_MIN_INF";
    case QuantizationModes::RND_CONV: return "RND_CONV";
    case QuantizationModes::RND_CONV_ODD: return "RND_CONV_ODD";
  }
  return "<unrecognized enum value of QuantizationModes>";
}

const char *spirv::name_of(FPOperationMode value)
{
  switch (value)
  {
    case FPOperationMode::IEEE: return "IEEE";
    case FPOperationMode::ALT: return "ALT";
  }
  return "<unrecognized enum value of FPOperationMode>";
}

const char *spirv::name_of(OverflowModes value)
{
  switch (value)
  {
    case OverflowModes::WRAP: return "WRAP";
    case OverflowModes::SAT: return "SAT";
    case OverflowModes::SAT_ZERO: return "SAT_ZERO";
    case OverflowModes::SAT_SYM: return "SAT_SYM";
  }
  return "<unrecognized enum value of OverflowModes>";
}

const char *spirv::name_of(LinkageType value)
{
  switch (value)
  {
    case LinkageType::Export: return "Export";
    case LinkageType::Import: return "Import";
    case LinkageType::LinkOnceODR: return "LinkOnceODR";
  }
  return "<unrecognized enum value of LinkageType>";
}

const char *spirv::name_of(AccessQualifier value)
{
  switch (value)
  {
    case AccessQualifier::ReadOnly: return "ReadOnly";
    case AccessQualifier::WriteOnly: return "WriteOnly";
    case AccessQualifier::ReadWrite: return "ReadWrite";
  }
  return "<unrecognized enum value of AccessQualifier>";
}

const char *spirv::name_of(HostAccessQualifier value)
{
  switch (value)
  {
    case HostAccessQualifier::NoneINTEL: return "NoneINTEL";
    case HostAccessQualifier::ReadINTEL: return "ReadINTEL";
    case HostAccessQualifier::WriteINTEL: return "WriteINTEL";
    case HostAccessQualifier::ReadWriteINTEL: return "ReadWriteINTEL";
  }
  return "<unrecognized enum value of HostAccessQualifier>";
}

const char *spirv::name_of(FunctionParameterAttribute value)
{
  switch (value)
  {
    case FunctionParameterAttribute::Zext: return "Zext";
    case FunctionParameterAttribute::Sext: return "Sext";
    case FunctionParameterAttribute::ByVal: return "ByVal";
    case FunctionParameterAttribute::Sret: return "Sret";
    case FunctionParameterAttribute::NoAlias: return "NoAlias";
    case FunctionParameterAttribute::NoCapture: return "NoCapture";
    case FunctionParameterAttribute::NoWrite: return "NoWrite";
    case FunctionParameterAttribute::NoReadWrite: return "NoReadWrite";
    case FunctionParameterAttribute::RuntimeAlignedINTEL: return "RuntimeAlignedINTEL";
  }
  return "<unrecognized enum value of FunctionParameterAttribute>";
}

const char *spirv::name_of(Decoration value)
{
  switch (value)
  {
    case Decoration::RelaxedPrecision: return "RelaxedPrecision";
    case Decoration::SpecId: return "SpecId";
    case Decoration::Block: return "Block";
    case Decoration::BufferBlock: return "BufferBlock";
    case Decoration::RowMajor: return "RowMajor";
    case Decoration::ColMajor: return "ColMajor";
    case Decoration::ArrayStride: return "ArrayStride";
    case Decoration::MatrixStride: return "MatrixStride";
    case Decoration::GLSLShared: return "GLSLShared";
    case Decoration::GLSLPacked: return "GLSLPacked";
    case Decoration::CPacked: return "CPacked";
    case Decoration::BuiltIn: return "BuiltIn";
    case Decoration::NoPerspective: return "NoPerspective";
    case Decoration::Flat: return "Flat";
    case Decoration::Patch: return "Patch";
    case Decoration::Centroid: return "Centroid";
    case Decoration::Sample: return "Sample";
    case Decoration::Invariant: return "Invariant";
    case Decoration::Restrict: return "Restrict";
    case Decoration::Aliased: return "Aliased";
    case Decoration::Volatile: return "Volatile";
    case Decoration::Constant: return "Constant";
    case Decoration::Coherent: return "Coherent";
    case Decoration::NonWritable: return "NonWritable";
    case Decoration::NonReadable: return "NonReadable";
    case Decoration::Uniform: return "Uniform";
    case Decoration::UniformId: return "UniformId";
    case Decoration::SaturatedConversion: return "SaturatedConversion";
    case Decoration::Stream: return "Stream";
    case Decoration::Location: return "Location";
    case Decoration::Component: return "Component";
    case Decoration::Index: return "Index";
    case Decoration::Binding: return "Binding";
    case Decoration::DescriptorSet: return "DescriptorSet";
    case Decoration::Offset: return "Offset";
    case Decoration::XfbBuffer: return "XfbBuffer";
    case Decoration::XfbStride: return "XfbStride";
    case Decoration::FuncParamAttr: return "FuncParamAttr";
    case Decoration::FPRoundingMode: return "FPRoundingMode";
    case Decoration::FPFastMathMode: return "FPFastMathMode";
    case Decoration::LinkageAttributes: return "LinkageAttributes";
    case Decoration::NoContraction: return "NoContraction";
    case Decoration::InputAttachmentIndex: return "InputAttachmentIndex";
    case Decoration::Alignment: return "Alignment";
    case Decoration::MaxByteOffset: return "MaxByteOffset";
    case Decoration::AlignmentId: return "AlignmentId";
    case Decoration::MaxByteOffsetId: return "MaxByteOffsetId";
    case Decoration::NoSignedWrap: return "NoSignedWrap";
    case Decoration::NoUnsignedWrap: return "NoUnsignedWrap";
    case Decoration::WeightTextureQCOM: return "WeightTextureQCOM";
    case Decoration::BlockMatchTextureQCOM: return "BlockMatchTextureQCOM";
    case Decoration::BlockMatchSamplerQCOM: return "BlockMatchSamplerQCOM";
    case Decoration::ExplicitInterpAMD: return "ExplicitInterpAMD";
    case Decoration::NodeSharesPayloadLimitsWithAMDX: return "NodeSharesPayloadLimitsWithAMDX";
    case Decoration::NodeMaxPayloadsAMDX: return "NodeMaxPayloadsAMDX";
    case Decoration::TrackFinishWritingAMDX: return "TrackFinishWritingAMDX";
    case Decoration::PayloadNodeNameAMDX: return "PayloadNodeNameAMDX";
    case Decoration::OverrideCoverageNV: return "OverrideCoverageNV";
    case Decoration::PassthroughNV: return "PassthroughNV";
    case Decoration::ViewportRelativeNV: return "ViewportRelativeNV";
    case Decoration::SecondaryViewportRelativeNV: return "SecondaryViewportRelativeNV";
    case Decoration::PerPrimitiveEXT: return "PerPrimitiveEXT";
    case Decoration::PerViewNV: return "PerViewNV";
    case Decoration::PerTaskNV: return "PerTaskNV";
    case Decoration::PerVertexKHR: return "PerVertexKHR";
    case Decoration::NonUniform: return "NonUniform";
    case Decoration::RestrictPointer: return "RestrictPointer";
    case Decoration::AliasedPointer: return "AliasedPointer";
    case Decoration::HitObjectShaderRecordBufferNV: return "HitObjectShaderRecordBufferNV";
    case Decoration::BindlessSamplerNV: return "BindlessSamplerNV";
    case Decoration::BindlessImageNV: return "BindlessImageNV";
    case Decoration::BoundSamplerNV: return "BoundSamplerNV";
    case Decoration::BoundImageNV: return "BoundImageNV";
    case Decoration::SIMTCallINTEL: return "SIMTCallINTEL";
    case Decoration::ReferencedIndirectlyINTEL: return "ReferencedIndirectlyINTEL";
    case Decoration::ClobberINTEL: return "ClobberINTEL";
    case Decoration::SideEffectsINTEL: return "SideEffectsINTEL";
    case Decoration::VectorComputeVariableINTEL: return "VectorComputeVariableINTEL";
    case Decoration::FuncParamIOKindINTEL: return "FuncParamIOKindINTEL";
    case Decoration::VectorComputeFunctionINTEL: return "VectorComputeFunctionINTEL";
    case Decoration::StackCallINTEL: return "StackCallINTEL";
    case Decoration::GlobalVariableOffsetINTEL: return "GlobalVariableOffsetINTEL";
    case Decoration::CounterBuffer: return "CounterBuffer";
    case Decoration::UserSemantic: return "UserSemantic";
    case Decoration::UserTypeGOOGLE: return "UserTypeGOOGLE";
    case Decoration::FunctionRoundingModeINTEL: return "FunctionRoundingModeINTEL";
    case Decoration::FunctionDenormModeINTEL: return "FunctionDenormModeINTEL";
    case Decoration::RegisterINTEL: return "RegisterINTEL";
    case Decoration::MemoryINTEL: return "MemoryINTEL";
    case Decoration::NumbanksINTEL: return "NumbanksINTEL";
    case Decoration::BankwidthINTEL: return "BankwidthINTEL";
    case Decoration::MaxPrivateCopiesINTEL: return "MaxPrivateCopiesINTEL";
    case Decoration::SinglepumpINTEL: return "SinglepumpINTEL";
    case Decoration::DoublepumpINTEL: return "DoublepumpINTEL";
    case Decoration::MaxReplicatesINTEL: return "MaxReplicatesINTEL";
    case Decoration::SimpleDualPortINTEL: return "SimpleDualPortINTEL";
    case Decoration::MergeINTEL: return "MergeINTEL";
    case Decoration::BankBitsINTEL: return "BankBitsINTEL";
    case Decoration::ForcePow2DepthINTEL: return "ForcePow2DepthINTEL";
    case Decoration::StridesizeINTEL: return "StridesizeINTEL";
    case Decoration::WordsizeINTEL: return "WordsizeINTEL";
    case Decoration::TrueDualPortINTEL: return "TrueDualPortINTEL";
    case Decoration::BurstCoalesceINTEL: return "BurstCoalesceINTEL";
    case Decoration::CacheSizeINTEL: return "CacheSizeINTEL";
    case Decoration::DontStaticallyCoalesceINTEL: return "DontStaticallyCoalesceINTEL";
    case Decoration::PrefetchINTEL: return "PrefetchINTEL";
    case Decoration::StallEnableINTEL: return "StallEnableINTEL";
    case Decoration::FuseLoopsInFunctionINTEL: return "FuseLoopsInFunctionINTEL";
    case Decoration::MathOpDSPModeINTEL: return "MathOpDSPModeINTEL";
    case Decoration::AliasScopeINTEL: return "AliasScopeINTEL";
    case Decoration::NoAliasINTEL: return "NoAliasINTEL";
    case Decoration::InitiationIntervalINTEL: return "InitiationIntervalINTEL";
    case Decoration::MaxConcurrencyINTEL: return "MaxConcurrencyINTEL";
    case Decoration::PipelineEnableINTEL: return "PipelineEnableINTEL";
    case Decoration::BufferLocationINTEL: return "BufferLocationINTEL";
    case Decoration::IOPipeStorageINTEL: return "IOPipeStorageINTEL";
    case Decoration::FunctionFloatingPointModeINTEL: return "FunctionFloatingPointModeINTEL";
    case Decoration::SingleElementVectorINTEL: return "SingleElementVectorINTEL";
    case Decoration::VectorComputeCallableFunctionINTEL: return "VectorComputeCallableFunctionINTEL";
    case Decoration::MediaBlockIOINTEL: return "MediaBlockIOINTEL";
    case Decoration::StallFreeINTEL: return "StallFreeINTEL";
    case Decoration::FPMaxErrorDecorationINTEL: return "FPMaxErrorDecorationINTEL";
    case Decoration::LatencyControlLabelINTEL: return "LatencyControlLabelINTEL";
    case Decoration::LatencyControlConstraintINTEL: return "LatencyControlConstraintINTEL";
    case Decoration::ConduitKernelArgumentINTEL: return "ConduitKernelArgumentINTEL";
    case Decoration::RegisterMapKernelArgumentINTEL: return "RegisterMapKernelArgumentINTEL";
    case Decoration::MMHostInterfaceAddressWidthINTEL: return "MMHostInterfaceAddressWidthINTEL";
    case Decoration::MMHostInterfaceDataWidthINTEL: return "MMHostInterfaceDataWidthINTEL";
    case Decoration::MMHostInterfaceLatencyINTEL: return "MMHostInterfaceLatencyINTEL";
    case Decoration::MMHostInterfaceReadWriteModeINTEL: return "MMHostInterfaceReadWriteModeINTEL";
    case Decoration::MMHostInterfaceMaxBurstINTEL: return "MMHostInterfaceMaxBurstINTEL";
    case Decoration::MMHostInterfaceWaitRequestINTEL: return "MMHostInterfaceWaitRequestINTEL";
    case Decoration::StableKernelArgumentINTEL: return "StableKernelArgumentINTEL";
    case Decoration::HostAccessINTEL: return "HostAccessINTEL";
    case Decoration::InitModeINTEL: return "InitModeINTEL";
    case Decoration::ImplementInRegisterMapINTEL: return "ImplementInRegisterMapINTEL";
    case Decoration::CacheControlLoadINTEL: return "CacheControlLoadINTEL";
    case Decoration::CacheControlStoreINTEL: return "CacheControlStoreINTEL";
  }
  return "<unrecognized enum value of Decoration>";
}

const char *spirv::name_of(BuiltIn value)
{
  switch (value)
  {
    case BuiltIn::Position: return "Position";
    case BuiltIn::PointSize: return "PointSize";
    case BuiltIn::ClipDistance: return "ClipDistance";
    case BuiltIn::CullDistance: return "CullDistance";
    case BuiltIn::VertexId: return "VertexId";
    case BuiltIn::InstanceId: return "InstanceId";
    case BuiltIn::PrimitiveId: return "PrimitiveId";
    case BuiltIn::InvocationId: return "InvocationId";
    case BuiltIn::Layer: return "Layer";
    case BuiltIn::ViewportIndex: return "ViewportIndex";
    case BuiltIn::TessLevelOuter: return "TessLevelOuter";
    case BuiltIn::TessLevelInner: return "TessLevelInner";
    case BuiltIn::TessCoord: return "TessCoord";
    case BuiltIn::PatchVertices: return "PatchVertices";
    case BuiltIn::FragCoord: return "FragCoord";
    case BuiltIn::PointCoord: return "PointCoord";
    case BuiltIn::FrontFacing: return "FrontFacing";
    case BuiltIn::SampleId: return "SampleId";
    case BuiltIn::SamplePosition: return "SamplePosition";
    case BuiltIn::SampleMask: return "SampleMask";
    case BuiltIn::FragDepth: return "FragDepth";
    case BuiltIn::HelperInvocation: return "HelperInvocation";
    case BuiltIn::NumWorkgroups: return "NumWorkgroups";
    case BuiltIn::WorkgroupSize: return "WorkgroupSize";
    case BuiltIn::WorkgroupId: return "WorkgroupId";
    case BuiltIn::LocalInvocationId: return "LocalInvocationId";
    case BuiltIn::GlobalInvocationId: return "GlobalInvocationId";
    case BuiltIn::LocalInvocationIndex: return "LocalInvocationIndex";
    case BuiltIn::WorkDim: return "WorkDim";
    case BuiltIn::GlobalSize: return "GlobalSize";
    case BuiltIn::EnqueuedWorkgroupSize: return "EnqueuedWorkgroupSize";
    case BuiltIn::GlobalOffset: return "GlobalOffset";
    case BuiltIn::GlobalLinearId: return "GlobalLinearId";
    case BuiltIn::SubgroupSize: return "SubgroupSize";
    case BuiltIn::SubgroupMaxSize: return "SubgroupMaxSize";
    case BuiltIn::NumSubgroups: return "NumSubgroups";
    case BuiltIn::NumEnqueuedSubgroups: return "NumEnqueuedSubgroups";
    case BuiltIn::SubgroupId: return "SubgroupId";
    case BuiltIn::SubgroupLocalInvocationId: return "SubgroupLocalInvocationId";
    case BuiltIn::VertexIndex: return "VertexIndex";
    case BuiltIn::InstanceIndex: return "InstanceIndex";
    case BuiltIn::CoreIDARM: return "CoreIDARM";
    case BuiltIn::CoreCountARM: return "CoreCountARM";
    case BuiltIn::CoreMaxIDARM: return "CoreMaxIDARM";
    case BuiltIn::WarpIDARM: return "WarpIDARM";
    case BuiltIn::WarpMaxIDARM: return "WarpMaxIDARM";
    case BuiltIn::SubgroupEqMask: return "SubgroupEqMask";
    case BuiltIn::SubgroupGeMask: return "SubgroupGeMask";
    case BuiltIn::SubgroupGtMask: return "SubgroupGtMask";
    case BuiltIn::SubgroupLeMask: return "SubgroupLeMask";
    case BuiltIn::SubgroupLtMask: return "SubgroupLtMask";
    case BuiltIn::BaseVertex: return "BaseVertex";
    case BuiltIn::BaseInstance: return "BaseInstance";
    case BuiltIn::DrawIndex: return "DrawIndex";
    case BuiltIn::PrimitiveShadingRateKHR: return "PrimitiveShadingRateKHR";
    case BuiltIn::DeviceIndex: return "DeviceIndex";
    case BuiltIn::ViewIndex: return "ViewIndex";
    case BuiltIn::ShadingRateKHR: return "ShadingRateKHR";
    case BuiltIn::BaryCoordNoPerspAMD: return "BaryCoordNoPerspAMD";
    case BuiltIn::BaryCoordNoPerspCentroidAMD: return "BaryCoordNoPerspCentroidAMD";
    case BuiltIn::BaryCoordNoPerspSampleAMD: return "BaryCoordNoPerspSampleAMD";
    case BuiltIn::BaryCoordSmoothAMD: return "BaryCoordSmoothAMD";
    case BuiltIn::BaryCoordSmoothCentroidAMD: return "BaryCoordSmoothCentroidAMD";
    case BuiltIn::BaryCoordSmoothSampleAMD: return "BaryCoordSmoothSampleAMD";
    case BuiltIn::BaryCoordPullModelAMD: return "BaryCoordPullModelAMD";
    case BuiltIn::FragStencilRefEXT: return "FragStencilRefEXT";
    case BuiltIn::CoalescedInputCountAMDX: return "CoalescedInputCountAMDX";
    case BuiltIn::ShaderIndexAMDX: return "ShaderIndexAMDX";
    case BuiltIn::ViewportMaskNV: return "ViewportMaskNV";
    case BuiltIn::SecondaryPositionNV: return "SecondaryPositionNV";
    case BuiltIn::SecondaryViewportMaskNV: return "SecondaryViewportMaskNV";
    case BuiltIn::PositionPerViewNV: return "PositionPerViewNV";
    case BuiltIn::ViewportMaskPerViewNV: return "ViewportMaskPerViewNV";
    case BuiltIn::FullyCoveredEXT: return "FullyCoveredEXT";
    case BuiltIn::TaskCountNV: return "TaskCountNV";
    case BuiltIn::PrimitiveCountNV: return "PrimitiveCountNV";
    case BuiltIn::PrimitiveIndicesNV: return "PrimitiveIndicesNV";
    case BuiltIn::ClipDistancePerViewNV: return "ClipDistancePerViewNV";
    case BuiltIn::CullDistancePerViewNV: return "CullDistancePerViewNV";
    case BuiltIn::LayerPerViewNV: return "LayerPerViewNV";
    case BuiltIn::MeshViewCountNV: return "MeshViewCountNV";
    case BuiltIn::MeshViewIndicesNV: return "MeshViewIndicesNV";
    case BuiltIn::BaryCoordKHR: return "BaryCoordKHR";
    case BuiltIn::BaryCoordNoPerspKHR: return "BaryCoordNoPerspKHR";
    case BuiltIn::FragSizeEXT: return "FragSizeEXT";
    case BuiltIn::FragInvocationCountEXT: return "FragInvocationCountEXT";
    case BuiltIn::PrimitivePointIndicesEXT: return "PrimitivePointIndicesEXT";
    case BuiltIn::PrimitiveLineIndicesEXT: return "PrimitiveLineIndicesEXT";
    case BuiltIn::PrimitiveTriangleIndicesEXT: return "PrimitiveTriangleIndicesEXT";
    case BuiltIn::CullPrimitiveEXT: return "CullPrimitiveEXT";
    case BuiltIn::LaunchIdKHR: return "LaunchIdKHR";
    case BuiltIn::LaunchSizeKHR: return "LaunchSizeKHR";
    case BuiltIn::WorldRayOriginKHR: return "WorldRayOriginKHR";
    case BuiltIn::WorldRayDirectionKHR: return "WorldRayDirectionKHR";
    case BuiltIn::ObjectRayOriginKHR: return "ObjectRayOriginKHR";
    case BuiltIn::ObjectRayDirectionKHR: return "ObjectRayDirectionKHR";
    case BuiltIn::RayTminKHR: return "RayTminKHR";
    case BuiltIn::RayTmaxKHR: return "RayTmaxKHR";
    case BuiltIn::InstanceCustomIndexKHR: return "InstanceCustomIndexKHR";
    case BuiltIn::ObjectToWorldKHR: return "ObjectToWorldKHR";
    case BuiltIn::WorldToObjectKHR: return "WorldToObjectKHR";
    case BuiltIn::HitTNV: return "HitTNV";
    case BuiltIn::HitKindKHR: return "HitKindKHR";
    case BuiltIn::CurrentRayTimeNV: return "CurrentRayTimeNV";
    case BuiltIn::HitTriangleVertexPositionsKHR: return "HitTriangleVertexPositionsKHR";
    case BuiltIn::HitMicroTriangleVertexPositionsNV: return "HitMicroTriangleVertexPositionsNV";
    case BuiltIn::HitMicroTriangleVertexBarycentricsNV: return "HitMicroTriangleVertexBarycentricsNV";
    case BuiltIn::IncomingRayFlagsKHR: return "IncomingRayFlagsKHR";
    case BuiltIn::RayGeometryIndexKHR: return "RayGeometryIndexKHR";
    case BuiltIn::WarpsPerSMNV: return "WarpsPerSMNV";
    case BuiltIn::SMCountNV: return "SMCountNV";
    case BuiltIn::WarpIDNV: return "WarpIDNV";
    case BuiltIn::SMIDNV: return "SMIDNV";
    case BuiltIn::HitKindFrontFacingMicroTriangleNV: return "HitKindFrontFacingMicroTriangleNV";
    case BuiltIn::HitKindBackFacingMicroTriangleNV: return "HitKindBackFacingMicroTriangleNV";
    case BuiltIn::CullMaskKHR: return "CullMaskKHR";
  }
  return "<unrecognized enum value of BuiltIn>";
}

const char *spirv::name_of(Scope value)
{
  switch (value)
  {
    case Scope::CrossDevice: return "CrossDevice";
    case Scope::Device: return "Device";
    case Scope::Workgroup: return "Workgroup";
    case Scope::Subgroup: return "Subgroup";
    case Scope::Invocation: return "Invocation";
    case Scope::QueueFamily: return "QueueFamily";
    case Scope::ShaderCallKHR: return "ShaderCallKHR";
  }
  return "<unrecognized enum value of Scope>";
}

const char *spirv::name_of(GroupOperation value)
{
  switch (value)
  {
    case GroupOperation::Reduce: return "Reduce";
    case GroupOperation::InclusiveScan: return "InclusiveScan";
    case GroupOperation::ExclusiveScan: return "ExclusiveScan";
    case GroupOperation::ClusteredReduce: return "ClusteredReduce";
    case GroupOperation::PartitionedReduceNV: return "PartitionedReduceNV";
    case GroupOperation::PartitionedInclusiveScanNV: return "PartitionedInclusiveScanNV";
    case GroupOperation::PartitionedExclusiveScanNV: return "PartitionedExclusiveScanNV";
  }
  return "<unrecognized enum value of GroupOperation>";
}

const char *spirv::name_of(KernelEnqueueFlags value)
{
  switch (value)
  {
    case KernelEnqueueFlags::NoWait: return "NoWait";
    case KernelEnqueueFlags::WaitKernel: return "WaitKernel";
    case KernelEnqueueFlags::WaitWorkGroup: return "WaitWorkGroup";
  }
  return "<unrecognized enum value of KernelEnqueueFlags>";
}

const char *spirv::name_of(Capability value)
{
  switch (value)
  {
    case Capability::Matrix: return "Matrix";
    case Capability::Shader: return "Shader";
    case Capability::Geometry: return "Geometry";
    case Capability::Tessellation: return "Tessellation";
    case Capability::Addresses: return "Addresses";
    case Capability::Linkage: return "Linkage";
    case Capability::Kernel: return "Kernel";
    case Capability::Vector16: return "Vector16";
    case Capability::Float16Buffer: return "Float16Buffer";
    case Capability::Float16: return "Float16";
    case Capability::Float64: return "Float64";
    case Capability::Int64: return "Int64";
    case Capability::Int64Atomics: return "Int64Atomics";
    case Capability::ImageBasic: return "ImageBasic";
    case Capability::ImageReadWrite: return "ImageReadWrite";
    case Capability::ImageMipmap: return "ImageMipmap";
    case Capability::Pipes: return "Pipes";
    case Capability::Groups: return "Groups";
    case Capability::DeviceEnqueue: return "DeviceEnqueue";
    case Capability::LiteralSampler: return "LiteralSampler";
    case Capability::AtomicStorage: return "AtomicStorage";
    case Capability::Int16: return "Int16";
    case Capability::TessellationPointSize: return "TessellationPointSize";
    case Capability::GeometryPointSize: return "GeometryPointSize";
    case Capability::ImageGatherExtended: return "ImageGatherExtended";
    case Capability::StorageImageMultisample: return "StorageImageMultisample";
    case Capability::UniformBufferArrayDynamicIndexing: return "UniformBufferArrayDynamicIndexing";
    case Capability::SampledImageArrayDynamicIndexing: return "SampledImageArrayDynamicIndexing";
    case Capability::StorageBufferArrayDynamicIndexing: return "StorageBufferArrayDynamicIndexing";
    case Capability::StorageImageArrayDynamicIndexing: return "StorageImageArrayDynamicIndexing";
    case Capability::ClipDistance: return "ClipDistance";
    case Capability::CullDistance: return "CullDistance";
    case Capability::ImageCubeArray: return "ImageCubeArray";
    case Capability::SampleRateShading: return "SampleRateShading";
    case Capability::ImageRect: return "ImageRect";
    case Capability::SampledRect: return "SampledRect";
    case Capability::GenericPointer: return "GenericPointer";
    case Capability::Int8: return "Int8";
    case Capability::InputAttachment: return "InputAttachment";
    case Capability::SparseResidency: return "SparseResidency";
    case Capability::MinLod: return "MinLod";
    case Capability::Sampled1D: return "Sampled1D";
    case Capability::Image1D: return "Image1D";
    case Capability::SampledCubeArray: return "SampledCubeArray";
    case Capability::SampledBuffer: return "SampledBuffer";
    case Capability::ImageBuffer: return "ImageBuffer";
    case Capability::ImageMSArray: return "ImageMSArray";
    case Capability::StorageImageExtendedFormats: return "StorageImageExtendedFormats";
    case Capability::ImageQuery: return "ImageQuery";
    case Capability::DerivativeControl: return "DerivativeControl";
    case Capability::InterpolationFunction: return "InterpolationFunction";
    case Capability::TransformFeedback: return "TransformFeedback";
    case Capability::GeometryStreams: return "GeometryStreams";
    case Capability::StorageImageReadWithoutFormat: return "StorageImageReadWithoutFormat";
    case Capability::StorageImageWriteWithoutFormat: return "StorageImageWriteWithoutFormat";
    case Capability::MultiViewport: return "MultiViewport";
    case Capability::SubgroupDispatch: return "SubgroupDispatch";
    case Capability::NamedBarrier: return "NamedBarrier";
    case Capability::PipeStorage: return "PipeStorage";
    case Capability::GroupNonUniform: return "GroupNonUniform";
    case Capability::GroupNonUniformVote: return "GroupNonUniformVote";
    case Capability::GroupNonUniformArithmetic: return "GroupNonUniformArithmetic";
    case Capability::GroupNonUniformBallot: return "GroupNonUniformBallot";
    case Capability::GroupNonUniformShuffle: return "GroupNonUniformShuffle";
    case Capability::GroupNonUniformShuffleRelative: return "GroupNonUniformShuffleRelative";
    case Capability::GroupNonUniformClustered: return "GroupNonUniformClustered";
    case Capability::GroupNonUniformQuad: return "GroupNonUniformQuad";
    case Capability::ShaderLayer: return "ShaderLayer";
    case Capability::ShaderViewportIndex: return "ShaderViewportIndex";
    case Capability::UniformDecoration: return "UniformDecoration";
    case Capability::CoreBuiltinsARM: return "CoreBuiltinsARM";
    case Capability::TileImageColorReadAccessEXT: return "TileImageColorReadAccessEXT";
    case Capability::TileImageDepthReadAccessEXT: return "TileImageDepthReadAccessEXT";
    case Capability::TileImageStencilReadAccessEXT: return "TileImageStencilReadAccessEXT";
    case Capability::CooperativeMatrixLayoutsARM: return "CooperativeMatrixLayoutsARM";
    case Capability::FragmentShadingRateKHR: return "FragmentShadingRateKHR";
    case Capability::SubgroupBallotKHR: return "SubgroupBallotKHR";
    case Capability::DrawParameters: return "DrawParameters";
    case Capability::WorkgroupMemoryExplicitLayoutKHR: return "WorkgroupMemoryExplicitLayoutKHR";
    case Capability::WorkgroupMemoryExplicitLayout8BitAccessKHR: return "WorkgroupMemoryExplicitLayout8BitAccessKHR";
    case Capability::WorkgroupMemoryExplicitLayout16BitAccessKHR: return "WorkgroupMemoryExplicitLayout16BitAccessKHR";
    case Capability::SubgroupVoteKHR: return "SubgroupVoteKHR";
    case Capability::StorageBuffer16BitAccess: return "StorageBuffer16BitAccess";
    case Capability::UniformAndStorageBuffer16BitAccess: return "UniformAndStorageBuffer16BitAccess";
    case Capability::StoragePushConstant16: return "StoragePushConstant16";
    case Capability::StorageInputOutput16: return "StorageInputOutput16";
    case Capability::DeviceGroup: return "DeviceGroup";
    case Capability::MultiView: return "MultiView";
    case Capability::VariablePointersStorageBuffer: return "VariablePointersStorageBuffer";
    case Capability::VariablePointers: return "VariablePointers";
    case Capability::AtomicStorageOps: return "AtomicStorageOps";
    case Capability::SampleMaskPostDepthCoverage: return "SampleMaskPostDepthCoverage";
    case Capability::StorageBuffer8BitAccess: return "StorageBuffer8BitAccess";
    case Capability::UniformAndStorageBuffer8BitAccess: return "UniformAndStorageBuffer8BitAccess";
    case Capability::StoragePushConstant8: return "StoragePushConstant8";
    case Capability::DenormPreserve: return "DenormPreserve";
    case Capability::DenormFlushToZero: return "DenormFlushToZero";
    case Capability::SignedZeroInfNanPreserve: return "SignedZeroInfNanPreserve";
    case Capability::RoundingModeRTE: return "RoundingModeRTE";
    case Capability::RoundingModeRTZ: return "RoundingModeRTZ";
    case Capability::RayQueryProvisionalKHR: return "RayQueryProvisionalKHR";
    case Capability::RayQueryKHR: return "RayQueryKHR";
    case Capability::RayTraversalPrimitiveCullingKHR: return "RayTraversalPrimitiveCullingKHR";
    case Capability::RayTracingKHR: return "RayTracingKHR";
    case Capability::TextureSampleWeightedQCOM: return "TextureSampleWeightedQCOM";
    case Capability::TextureBoxFilterQCOM: return "TextureBoxFilterQCOM";
    case Capability::TextureBlockMatchQCOM: return "TextureBlockMatchQCOM";
    case Capability::TextureBlockMatch2QCOM: return "TextureBlockMatch2QCOM";
    case Capability::Float16ImageAMD: return "Float16ImageAMD";
    case Capability::ImageGatherBiasLodAMD: return "ImageGatherBiasLodAMD";
    case Capability::FragmentMaskAMD: return "FragmentMaskAMD";
    case Capability::StencilExportEXT: return "StencilExportEXT";
    case Capability::ImageReadWriteLodAMD: return "ImageReadWriteLodAMD";
    case Capability::Int64ImageEXT: return "Int64ImageEXT";
    case Capability::ShaderClockKHR: return "ShaderClockKHR";
    case Capability::ShaderEnqueueAMDX: return "ShaderEnqueueAMDX";
    case Capability::QuadControlKHR: return "QuadControlKHR";
    case Capability::SampleMaskOverrideCoverageNV: return "SampleMaskOverrideCoverageNV";
    case Capability::GeometryShaderPassthroughNV: return "GeometryShaderPassthroughNV";
    case Capability::ShaderViewportIndexLayerEXT: return "ShaderViewportIndexLayerEXT";
    case Capability::ShaderViewportMaskNV: return "ShaderViewportMaskNV";
    case Capability::ShaderStereoViewNV: return "ShaderStereoViewNV";
    case Capability::PerViewAttributesNV: return "PerViewAttributesNV";
    case Capability::FragmentFullyCoveredEXT: return "FragmentFullyCoveredEXT";
    case Capability::MeshShadingNV: return "MeshShadingNV";
    case Capability::ImageFootprintNV: return "ImageFootprintNV";
    case Capability::MeshShadingEXT: return "MeshShadingEXT";
    case Capability::FragmentBarycentricKHR: return "FragmentBarycentricKHR";
    case Capability::ComputeDerivativeGroupQuadsNV: return "ComputeDerivativeGroupQuadsNV";
    case Capability::FragmentDensityEXT: return "FragmentDensityEXT";
    case Capability::GroupNonUniformPartitionedNV: return "GroupNonUniformPartitionedNV";
    case Capability::ShaderNonUniform: return "ShaderNonUniform";
    case Capability::RuntimeDescriptorArray: return "RuntimeDescriptorArray";
    case Capability::InputAttachmentArrayDynamicIndexing: return "InputAttachmentArrayDynamicIndexing";
    case Capability::UniformTexelBufferArrayDynamicIndexing: return "UniformTexelBufferArrayDynamicIndexing";
    case Capability::StorageTexelBufferArrayDynamicIndexing: return "StorageTexelBufferArrayDynamicIndexing";
    case Capability::UniformBufferArrayNonUniformIndexing: return "UniformBufferArrayNonUniformIndexing";
    case Capability::SampledImageArrayNonUniformIndexing: return "SampledImageArrayNonUniformIndexing";
    case Capability::StorageBufferArrayNonUniformIndexing: return "StorageBufferArrayNonUniformIndexing";
    case Capability::StorageImageArrayNonUniformIndexing: return "StorageImageArrayNonUniformIndexing";
    case Capability::InputAttachmentArrayNonUniformIndexing: return "InputAttachmentArrayNonUniformIndexing";
    case Capability::UniformTexelBufferArrayNonUniformIndexing: return "UniformTexelBufferArrayNonUniformIndexing";
    case Capability::StorageTexelBufferArrayNonUniformIndexing: return "StorageTexelBufferArrayNonUniformIndexing";
    case Capability::RayTracingPositionFetchKHR: return "RayTracingPositionFetchKHR";
    case Capability::RayTracingNV: return "RayTracingNV";
    case Capability::RayTracingMotionBlurNV: return "RayTracingMotionBlurNV";
    case Capability::VulkanMemoryModel: return "VulkanMemoryModel";
    case Capability::VulkanMemoryModelDeviceScope: return "VulkanMemoryModelDeviceScope";
    case Capability::PhysicalStorageBufferAddresses: return "PhysicalStorageBufferAddresses";
    case Capability::ComputeDerivativeGroupLinearNV: return "ComputeDerivativeGroupLinearNV";
    case Capability::RayTracingProvisionalKHR: return "RayTracingProvisionalKHR";
    case Capability::CooperativeMatrixNV: return "CooperativeMatrixNV";
    case Capability::FragmentShaderSampleInterlockEXT: return "FragmentShaderSampleInterlockEXT";
    case Capability::FragmentShaderShadingRateInterlockEXT: return "FragmentShaderShadingRateInterlockEXT";
    case Capability::ShaderSMBuiltinsNV: return "ShaderSMBuiltinsNV";
    case Capability::FragmentShaderPixelInterlockEXT: return "FragmentShaderPixelInterlockEXT";
    case Capability::DemoteToHelperInvocation: return "DemoteToHelperInvocation";
    case Capability::DisplacementMicromapNV: return "DisplacementMicromapNV";
    case Capability::RayTracingOpacityMicromapEXT: return "RayTracingOpacityMicromapEXT";
    case Capability::ShaderInvocationReorderNV: return "ShaderInvocationReorderNV";
    case Capability::BindlessTextureNV: return "BindlessTextureNV";
    case Capability::RayQueryPositionFetchKHR: return "RayQueryPositionFetchKHR";
    case Capability::AtomicFloat16VectorNV: return "AtomicFloat16VectorNV";
    case Capability::RayTracingDisplacementMicromapNV: return "RayTracingDisplacementMicromapNV";
    case Capability::RawAccessChainsNV: return "RawAccessChainsNV";
    case Capability::SubgroupShuffleINTEL: return "SubgroupShuffleINTEL";
    case Capability::SubgroupBufferBlockIOINTEL: return "SubgroupBufferBlockIOINTEL";
    case Capability::SubgroupImageBlockIOINTEL: return "SubgroupImageBlockIOINTEL";
    case Capability::SubgroupImageMediaBlockIOINTEL: return "SubgroupImageMediaBlockIOINTEL";
    case Capability::RoundToInfinityINTEL: return "RoundToInfinityINTEL";
    case Capability::FloatingPointModeINTEL: return "FloatingPointModeINTEL";
    case Capability::IntegerFunctions2INTEL: return "IntegerFunctions2INTEL";
    case Capability::FunctionPointersINTEL: return "FunctionPointersINTEL";
    case Capability::IndirectReferencesINTEL: return "IndirectReferencesINTEL";
    case Capability::AsmINTEL: return "AsmINTEL";
    case Capability::AtomicFloat32MinMaxEXT: return "AtomicFloat32MinMaxEXT";
    case Capability::AtomicFloat64MinMaxEXT: return "AtomicFloat64MinMaxEXT";
    case Capability::AtomicFloat16MinMaxEXT: return "AtomicFloat16MinMaxEXT";
    case Capability::VectorComputeINTEL: return "VectorComputeINTEL";
    case Capability::VectorAnyINTEL: return "VectorAnyINTEL";
    case Capability::ExpectAssumeKHR: return "ExpectAssumeKHR";
    case Capability::SubgroupAvcMotionEstimationINTEL: return "SubgroupAvcMotionEstimationINTEL";
    case Capability::SubgroupAvcMotionEstimationIntraINTEL: return "SubgroupAvcMotionEstimationIntraINTEL";
    case Capability::SubgroupAvcMotionEstimationChromaINTEL: return "SubgroupAvcMotionEstimationChromaINTEL";
    case Capability::VariableLengthArrayINTEL: return "VariableLengthArrayINTEL";
    case Capability::FunctionFloatControlINTEL: return "FunctionFloatControlINTEL";
    case Capability::FPGAMemoryAttributesINTEL: return "FPGAMemoryAttributesINTEL";
    case Capability::FPFastMathModeINTEL: return "FPFastMathModeINTEL";
    case Capability::ArbitraryPrecisionIntegersINTEL: return "ArbitraryPrecisionIntegersINTEL";
    case Capability::ArbitraryPrecisionFloatingPointINTEL: return "ArbitraryPrecisionFloatingPointINTEL";
    case Capability::UnstructuredLoopControlsINTEL: return "UnstructuredLoopControlsINTEL";
    case Capability::FPGALoopControlsINTEL: return "FPGALoopControlsINTEL";
    case Capability::KernelAttributesINTEL: return "KernelAttributesINTEL";
    case Capability::FPGAKernelAttributesINTEL: return "FPGAKernelAttributesINTEL";
    case Capability::FPGAMemoryAccessesINTEL: return "FPGAMemoryAccessesINTEL";
    case Capability::FPGAClusterAttributesINTEL: return "FPGAClusterAttributesINTEL";
    case Capability::LoopFuseINTEL: return "LoopFuseINTEL";
    case Capability::FPGADSPControlINTEL: return "FPGADSPControlINTEL";
    case Capability::MemoryAccessAliasingINTEL: return "MemoryAccessAliasingINTEL";
    case Capability::FPGAInvocationPipeliningAttributesINTEL: return "FPGAInvocationPipeliningAttributesINTEL";
    case Capability::FPGABufferLocationINTEL: return "FPGABufferLocationINTEL";
    case Capability::ArbitraryPrecisionFixedPointINTEL: return "ArbitraryPrecisionFixedPointINTEL";
    case Capability::USMStorageClassesINTEL: return "USMStorageClassesINTEL";
    case Capability::RuntimeAlignedAttributeINTEL: return "RuntimeAlignedAttributeINTEL";
    case Capability::IOPipesINTEL: return "IOPipesINTEL";
    case Capability::BlockingPipesINTEL: return "BlockingPipesINTEL";
    case Capability::FPGARegINTEL: return "FPGARegINTEL";
    case Capability::DotProductInputAll: return "DotProductInputAll";
    case Capability::DotProductInput4x8Bit: return "DotProductInput4x8Bit";
    case Capability::DotProductInput4x8BitPacked: return "DotProductInput4x8BitPacked";
    case Capability::DotProduct: return "DotProduct";
    case Capability::RayCullMaskKHR: return "RayCullMaskKHR";
    case Capability::CooperativeMatrixKHR: return "CooperativeMatrixKHR";
    case Capability::ReplicatedCompositesEXT: return "ReplicatedCompositesEXT";
    case Capability::BitInstructions: return "BitInstructions";
    case Capability::GroupNonUniformRotateKHR: return "GroupNonUniformRotateKHR";
    case Capability::FloatControls2: return "FloatControls2";
    case Capability::AtomicFloat32AddEXT: return "AtomicFloat32AddEXT";
    case Capability::AtomicFloat64AddEXT: return "AtomicFloat64AddEXT";
    case Capability::LongCompositesINTEL: return "LongCompositesINTEL";
    case Capability::OptNoneINTEL: return "OptNoneINTEL";
    case Capability::AtomicFloat16AddEXT: return "AtomicFloat16AddEXT";
    case Capability::DebugInfoModuleINTEL: return "DebugInfoModuleINTEL";
    case Capability::BFloat16ConversionINTEL: return "BFloat16ConversionINTEL";
    case Capability::SplitBarrierINTEL: return "SplitBarrierINTEL";
    case Capability::FPGAClusterAttributesV2INTEL: return "FPGAClusterAttributesV2INTEL";
    case Capability::FPGAKernelAttributesv2INTEL: return "FPGAKernelAttributesv2INTEL";
    case Capability::FPMaxErrorINTEL: return "FPMaxErrorINTEL";
    case Capability::FPGALatencyControlINTEL: return "FPGALatencyControlINTEL";
    case Capability::FPGAArgumentInterfacesINTEL: return "FPGAArgumentInterfacesINTEL";
    case Capability::GlobalVariableHostAccessINTEL: return "GlobalVariableHostAccessINTEL";
    case Capability::GlobalVariableFPGADecorationsINTEL: return "GlobalVariableFPGADecorationsINTEL";
    case Capability::GroupUniformArithmeticKHR: return "GroupUniformArithmeticKHR";
    case Capability::MaskedGatherScatterINTEL: return "MaskedGatherScatterINTEL";
    case Capability::CacheControlsINTEL: return "CacheControlsINTEL";
    case Capability::RegisterLimitsINTEL: return "RegisterLimitsINTEL";
  }
  return "<unrecognized enum value of Capability>";
}

const char *spirv::name_of(RayQueryIntersection value)
{
  switch (value)
  {
    case RayQueryIntersection::RayQueryCandidateIntersectionKHR: return "RayQueryCandidateIntersectionKHR";
    case RayQueryIntersection::RayQueryCommittedIntersectionKHR: return "RayQueryCommittedIntersectionKHR";
  }
  return "<unrecognized enum value of RayQueryIntersection>";
}

const char *spirv::name_of(RayQueryCommittedIntersectionType value)
{
  switch (value)
  {
    case RayQueryCommittedIntersectionType::RayQueryCommittedIntersectionNoneKHR: return "RayQueryCommittedIntersectionNoneKHR";
    case RayQueryCommittedIntersectionType::RayQueryCommittedIntersectionTriangleKHR:
      return "RayQueryCommittedIntersectionTriangleKHR";
    case RayQueryCommittedIntersectionType::RayQueryCommittedIntersectionGeneratedKHR:
      return "RayQueryCommittedIntersectionGeneratedKHR";
  }
  return "<unrecognized enum value of RayQueryCommittedIntersectionType>";
}

const char *spirv::name_of(RayQueryCandidateIntersectionType value)
{
  switch (value)
  {
    case RayQueryCandidateIntersectionType::RayQueryCandidateIntersectionTriangleKHR:
      return "RayQueryCandidateIntersectionTriangleKHR";
    case RayQueryCandidateIntersectionType::RayQueryCandidateIntersectionAABBKHR: return "RayQueryCandidateIntersectionAABBKHR";
  }
  return "<unrecognized enum value of RayQueryCandidateIntersectionType>";
}

const char *spirv::name_of(PackedVectorFormat value)
{
  switch (value)
  {
    case PackedVectorFormat::PackedVectorFormat4x8Bit: return "PackedVectorFormat4x8Bit";
  }
  return "<unrecognized enum value of PackedVectorFormat>";
}

const char *spirv::name_of(CooperativeMatrixLayout value)
{
  switch (value)
  {
    case CooperativeMatrixLayout::RowMajorKHR: return "RowMajorKHR";
    case CooperativeMatrixLayout::ColumnMajorKHR: return "ColumnMajorKHR";
    case CooperativeMatrixLayout::RowBlockedInterleavedARM: return "RowBlockedInterleavedARM";
    case CooperativeMatrixLayout::ColumnBlockedInterleavedARM: return "ColumnBlockedInterleavedARM";
  }
  return "<unrecognized enum value of CooperativeMatrixLayout>";
}

const char *spirv::name_of(CooperativeMatrixUse value)
{
  switch (value)
  {
    case CooperativeMatrixUse::MatrixAKHR: return "MatrixAKHR";
    case CooperativeMatrixUse::MatrixBKHR: return "MatrixBKHR";
    case CooperativeMatrixUse::MatrixAccumulatorKHR: return "MatrixAccumulatorKHR";
  }
  return "<unrecognized enum value of CooperativeMatrixUse>";
}

const char *spirv::name_of(InitializationModeQualifier value)
{
  switch (value)
  {
    case InitializationModeQualifier::InitOnDeviceReprogramINTEL: return "InitOnDeviceReprogramINTEL";
    case InitializationModeQualifier::InitOnDeviceResetINTEL: return "InitOnDeviceResetINTEL";
  }
  return "<unrecognized enum value of InitializationModeQualifier>";
}

const char *spirv::name_of(LoadCacheControl value)
{
  switch (value)
  {
    case LoadCacheControl::UncachedINTEL: return "UncachedINTEL";
    case LoadCacheControl::CachedINTEL: return "CachedINTEL";
    case LoadCacheControl::StreamingINTEL: return "StreamingINTEL";
    case LoadCacheControl::InvalidateAfterReadINTEL: return "InvalidateAfterReadINTEL";
    case LoadCacheControl::ConstCachedINTEL: return "ConstCachedINTEL";
  }
  return "<unrecognized enum value of LoadCacheControl>";
}

const char *spirv::name_of(StoreCacheControl value)
{
  switch (value)
  {
    case StoreCacheControl::UncachedINTEL: return "UncachedINTEL";
    case StoreCacheControl::WriteThroughINTEL: return "WriteThroughINTEL";
    case StoreCacheControl::WriteBackINTEL: return "WriteBackINTEL";
    case StoreCacheControl::StreamingINTEL: return "StreamingINTEL";
  }
  return "<unrecognized enum value of StoreCacheControl>";
}

const char *spirv::name_of(NamedMaximumNumberOfRegisters value)
{
  switch (value)
  {
    case NamedMaximumNumberOfRegisters::AutoINTEL: return "AutoINTEL";
  }
  return "<unrecognized enum value of NamedMaximumNumberOfRegisters>";
}
