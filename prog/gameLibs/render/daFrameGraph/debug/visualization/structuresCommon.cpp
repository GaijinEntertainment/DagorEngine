// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "structuresCommon.h"

namespace dafg::visualization
{

ImU32 color_by_hash(size_t hash)
{
  constexpr size_t HUE_COUNT = 256;

  // Hash some more to get different colors for close numbers
  hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
  hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
  hash = (hash >> 16) ^ hash;
  float hue = static_cast<float>((6607 * hash) % HUE_COUNT) / static_cast<float>(HUE_COUNT - 1);

  ImVec4 color(0, 0, 0, 1);
  ImGui::ColorConvertHSVtoRGB(hue, 1.f, 1.f, color.x, color.y, color.z);
  return ImGui::ColorConvertFloat4ToU32(color);
};

ImU32 apply_alpha_coeff(ImU32 color, float k)
{
  return ImGui::ColorConvertFloat4ToU32(ImGui::ColorConvertU32ToFloat4(color) * ImVec4{1.f, 1.f, 1.f, k});
}

bool rect_is_visible(const ImRect &rect, const ImRect &view_rect) { return view_rect.Overlaps(rect); }

bool spline_is_visible(const ImCubicBezierPoints &spline, const ImRect &view_rect)
{
  return !(
    spline.P0.x < view_rect.Min.x && spline.P1.x < view_rect.Min.x && spline.P2.x < view_rect.Min.x && spline.P3.x < view_rect.Min.x ||
    spline.P0.x > view_rect.Max.x && spline.P1.x > view_rect.Max.x && spline.P2.x > view_rect.Max.x && spline.P3.x > view_rect.Max.x ||
    spline.P0.y < view_rect.Min.y && spline.P1.y < view_rect.Min.y && spline.P2.y < view_rect.Min.y && spline.P3.y < view_rect.Min.y ||
    spline.P0.y > view_rect.Max.y && spline.P1.y > view_rect.Max.y && spline.P2.y > view_rect.Max.y && spline.P3.y > view_rect.Max.y);
}

void add_triangle_filled_multicolor(ImDrawList *draw_list, const ImVec2 &p0, const ImVec2 &p1, const ImVec2 &p2, ImU32 c0, ImU32 c1,
  ImU32 c2)
{
  if (((c0 | c1 | c2) & IM_COL32_A_MASK) == 0)
    return;

  const ImVec2 uv = draw_list->_Data->TexUvWhitePixel;
  draw_list->PrimReserve(3, 3);

  draw_list->PrimWriteIdx((ImDrawIdx)(draw_list->_VtxCurrentIdx));
  draw_list->PrimWriteIdx((ImDrawIdx)(draw_list->_VtxCurrentIdx + 1));
  draw_list->PrimWriteIdx((ImDrawIdx)(draw_list->_VtxCurrentIdx + 2));

  draw_list->PrimWriteVtx(p0, uv, c0);
  draw_list->PrimWriteVtx(p1, uv, c1);
  draw_list->PrimWriteVtx(p2, uv, c2);
}


const char *side_effect_name(const SideEffects effect)
{
  switch (effect)
  {
    case SideEffects::None: return "None";
    case SideEffects::Internal: return "Internal";
    case SideEffects::External: return "External";
    default: return "";
  }
}

const char *bind_type_name(const BindingType type)
{
  switch (type)
  {
    case BindingType::ShaderVar: return "SV";
    case BindingType::ViewMatrix: return "VM";
    case BindingType::ProjMatrix: return "PM";
    case BindingType::Invalid: return "IN";
    default: return "";
  }
}

const char *vrs_combiner_name(const VariableRateShadingCombiner comb)
{
  switch (comb)
  {
    case VariableRateShadingCombiner::VRS_PASSTHROUGH: return "PASSTHROUGH";
    case VariableRateShadingCombiner::VRS_OVERRIDE: return "OVERRIDE";
    case VariableRateShadingCombiner::VRS_MIN: return "MIN";
    case VariableRateShadingCombiner::VRS_MAX: return "MAX";
    case VariableRateShadingCombiner::VRS_SUM: return "SUM";
    default: return "";
  }
}

const char *res_type_name(const ResourceType type)
{
  switch (type)
  {
    case ResourceType::Invalid: return "Invalid";
    case ResourceType::Texture: return "Tex";
    case ResourceType::Buffer: return "Buf";
    case ResourceType::Blob: return "Blob";
    default: return "";
  }
}

const char *res_clear_flag_name(const ResourceClearFlags flags)
{
  switch (flags)
  {
    case ResourceClearFlags::RESOURCE_CLEAR_DEPTH: return "Depth";
    case ResourceClearFlags::RESOURCE_CLEAR_STENCIL: return "Stencil";
    case ResourceClearFlags::RESOURCE_CLEAR_ALL_CONTENT: return "All";
    default: return "";
  }
}

const char *compare_func_name(const uint8_t func)
{
  switch (func)
  {
    case CMPF::CMPF_NEVER: return "NEVER";
    case CMPF::CMPF_LESS: return "<";
    case CMPF::CMPF_EQUAL: return "=";
    case CMPF::CMPF_LESSEQUAL: return "<=";
    case CMPF::CMPF_GREATER: return ">";
    case CMPF::CMPF_NOTEQUAL: return "!=";
    case CMPF::CMPF_GREATEREQUAL: return ">=";
    case CMPF::CMPF_ALWAYS: return "ALWAYS";
    default: return "";
  }
}

const char *stencil_op_name(const uint8_t op)
{
  switch (op)
  {
    case STNCLOP_KEEP: return "KEEP";
    case STNCLOP_ZERO: return "ZERO";
    case STNCLOP_REPLACE: return "REPLACE";
    case STNCLOP_INCRSAT: return "INCRSAT";
    case STNCLOP_DECRSAT: return "DECRSAT";
    case STNCLOP_INVERT: return "INVERT";
    case STNCLOP_INCR: return "INCREASE";
    case STNCLOP_DECR: return "DECREASE";
    default: return "";
  }
}

const char *blend_op_name(const uint8_t op)
{
  switch (op)
  {
    case BLENDOP::BLENDOP_ADD: return "ADD";
    case BLENDOP::BLENDOP_SUBTRACT: return "SUB";
    case BLENDOP::BLENDOP_REVSUBTRACT: return "REV SUB";
    case BLENDOP::BLENDOP_MIN: return "MIN";
    case BLENDOP::BLENDOP_MAX: return "MAX";
    default: return "";
  }
}

const char *blend_factor_name(const uint8_t factor)
{
  switch (factor)
  {
    case BLEND_FACTOR::BLEND_ZERO: return "ZERO";
    case BLEND_FACTOR::BLEND_ONE: return "ONE";
    case BLEND_FACTOR::BLEND_SRCCOLOR: return "SRC COLOR";
    case BLEND_FACTOR::BLEND_INVSRCCOLOR: return "INV SRC COLOR";
    case BLEND_FACTOR::BLEND_SRCALPHA: return "SRC ALPHA";
    case BLEND_FACTOR::BLEND_INVSRCALPHA: return "INV SRC ALPHA";
    case BLEND_FACTOR::BLEND_DESTALPHA: return "DST ALPHA";
    case BLEND_FACTOR::BLEND_INVDESTALPHA: return "INV DST ALPHA";
    case BLEND_FACTOR::BLEND_DESTCOLOR: return "DST COLOR";
    case BLEND_FACTOR::BLEND_INVDESTCOLOR: return "INV DST COLOR";
    case BLEND_FACTOR::BLEND_SRCALPHASAT: return "SRC ALPHA SAT";
    case BLEND_FACTOR::BLEND_BOTHINVSRCALPHA: return "BOTH INV SRC ALPHA";
    case BLEND_FACTOR::BLEND_BLENDFACTOR: return "BLEND FACTOR";
    case BLEND_FACTOR::BLEND_INVBLENDFACTOR: return "INV BLEND FACTOR";

    case EXT_BLEND_FACTOR::EXT_BLEND_SRC1COLOR: return "SRC 1 COLOR";
    case EXT_BLEND_FACTOR::EXT_BLEND_INVSRC1COLOR: return "INV SRC 1 COLOR";
    case EXT_BLEND_FACTOR::EXT_BLEND_SRC1ALPHA: return "SRC 1 ALPHA";
    case EXT_BLEND_FACTOR::EXT_BLEND_INVSRC1ALPHA: return "INV SRC 1 ALPHA";

    default: return "";
  }
}

const char *texture_type_name(const D3DResourceType type)
{
  switch (type)
  {
    case D3DResourceType::TEX: return "2D";
    case D3DResourceType::CUBETEX: return "Cube";
    case D3DResourceType::VOLTEX: return "Volume";
    case D3DResourceType::ARRTEX: return "2D Array";
    case D3DResourceType::CUBEARRTEX: return "Cube Array";
    default: return "";
  }
}

const char *clear_stage_name(const intermediate::ClearStage stage)
{
  switch (stage)
  {
    case intermediate::ClearStage::None: return "None";
    case intermediate::ClearStage::Activation: return "Activation";
    case intermediate::ClearStage::RenderPass: return "Render Pass";
    default: return "";
  }
}

const char *activation_action_name(const ResourceActivationAction action)
{
  switch (action)
  {
    case ResourceActivationAction::REWRITE_AS_COPY_DESTINATION: return "Rewrite as copy destination";
    case ResourceActivationAction::REWRITE_AS_UAV: return "REWRITE as UAV";
    case ResourceActivationAction::REWRITE_AS_RTV_DSV: return "REWRITE as RTV | DSV";
    case ResourceActivationAction::CLEAR_F_AS_UAV: return "CLEAR as FLOAT UAV";
    case ResourceActivationAction::CLEAR_I_AS_UAV: return "CLEAR as INT UAV";
    case ResourceActivationAction::CLEAR_AS_RTV_DSV: return "CLEAR as RTV | DSV";
    case ResourceActivationAction::DISCARD_AS_UAV: return "Discard as UAV";
    case ResourceActivationAction::DISCARD_AS_RTV_DSV: return "Discard as RTV | DSV";
    default: return "";
  }
}


const char *override_state_descr(const shaders::OverrideState &state)
{
  static String res;
  res.clear();

  using Bits = shaders::OverrideState::StateBits;

  res.aprintf(0, "Color write mask: 0x%X\n", state.colorWr);

  if (state.isOn(Bits::Z_TEST_DISABLE | Bits::Z_WRITE_DISABLE | Bits::Z_WRITE_ENABLE | Bits::Z_BOUNDS_ENABLED | Bits::Z_CLAMP_ENABLED |
                 Bits::Z_FUNC | Bits::Z_BIAS))
  {
    res.append("Z overrides:\n");
    if (state.isOn(Bits::Z_TEST_DISABLE))
      res.append("  test disabled\n");
    if (state.isOn(Bits::Z_WRITE_DISABLE))
      res.append("  write disabled\n");
    if (state.isOn(Bits::Z_WRITE_ENABLE))
      res.append("  write enabled\n");
    if (state.isOn(Bits::Z_BOUNDS_ENABLED))
      res.append("  bounds enabled\n");
    if (state.isOn(Bits::Z_CLAMP_ENABLED))
      res.append("  clamp enabled\n");
    if (state.isOn(Bits::Z_FUNC))
      res.aprintf(0, "  function: %s\n", compare_func_name(state.zFunc));
    if (state.isOn(Bits::Z_BIAS))
      res.aprintf(0, "  bias: %f\n  slope: %f\n", state.zBias, state.slopeZBias);
  }

  if (state.isOn(Bits::STENCIL))
    res.aprintf(0,
      "Stencil satate override:\n"
      "  func: %s\n"
      "  fail: 0x%X\n"
      "  zFail: 0x%X\n"
      "  pass: 0x%X\n"
      "  readMask: 0x%X\n"
      "  writeMask: 0x%X\n",
      compare_func_name(state.stencil.func), stencil_op_name(state.stencil.fail), stencil_op_name(state.stencil.zFail),
      stencil_op_name(state.stencil.pass), state.stencil.readMask, state.stencil.writeMask);

  if (state.isOn(Bits::BLEND_OP | Bits::BLEND_OP_A | Bits::BLEND_SRC_DEST | Bits::BLEND_SRC_DEST_A))
  {
    res.append("Blend overrides:\n");
    if (state.isOn(Bits::BLEND_OP))
      res.aprintf(0, "  operation: %s\n", blend_op_name(state.blendOp));
    if (state.isOn(Bits::BLEND_OP_A))
      res.aprintf(0, "  operation A: %s\n", blend_op_name(state.blendOpA));
    if (state.isOn(Bits::BLEND_SRC_DEST))
      res.aprintf(0, "  factors SRC/DST:\n    %d\n    %d\n", blend_factor_name(state.sblend), blend_factor_name(state.dblend));
    if (state.isOn(Bits::BLEND_SRC_DEST_A))
      res.aprintf(0, "  factors A SRC/DST:\n    %d\n    %d\n", blend_factor_name(state.sblenda), blend_factor_name(state.dblenda));
  }

  if (state.isOn(Bits::CULL_NONE | Bits::FLIP_CULL))
  {
    res.append("Culling overrides:\n");
    if (state.isOn(Bits::CULL_NONE))
      res.append("  disabled\n");
    if (state.isOn(Bits::FLIP_CULL))
      res.append("  flip cull\n");
  }

  if (state.isOn(Bits::FORCED_SAMPLE_COUNT))
    res.aprintf(0, "Forced sample count: %d\n", state.forcedSampleCount);

  if (state.isOn(Bits::CONSERVATIVE))
    res.append("Conservative rasterization\n");

  if (state.isOn(Bits::SCISSOR_ENABLED))
    res.append("Scissor test\n");

  if (state.isOn(Bits::ALPHA_TO_COVERAGE))
    res.append("Alpha-to-coverage\n");

  return res.c_str();
}

const char *state_reqs_descr(const NodeStateRequirements &reqs)
{
  static String res;
  res.clear();

  res.aprintf(0, "Support wireframe: %s\n", reqs.supportsWireframe ? "Y" : "N");

  if (!(reqs.vrsState.rateX == 1 && reqs.vrsState.rateY == 1 &&
        reqs.vrsState.vertexCombiner == VariableRateShadingCombiner::VRS_PASSTHROUGH &&
        reqs.vrsState.pixelCombiner == VariableRateShadingCombiner::VRS_PASSTHROUGH))
    res.aprintf(0, "\nVRS state:\n  rates X/Y: %d %d\n  vtx combiner: %s\n  pix combiner: %s\n", reqs.vrsState.rateX,
      reqs.vrsState.rateY, vrs_combiner_name(reqs.vrsState.vertexCombiner), vrs_combiner_name(reqs.vrsState.pixelCombiner));


  if (reqs.pipelineStateOverride)
    res.aprintf(0, "\n%s", override_state_descr(*reqs.pipelineStateOverride));

  return res.c_str();
}

const char *multiplexing_descr(const multiplexing::Mode mode)
{
  static String res;
  res.clear();

  if (mode == multiplexing::Mode::None)
  {
    res.append("Exec once\n");
  }
  else
  {
    if (eastl::to_underlying(mode & multiplexing::Mode::SuperSampling))
      res.append("Super Sampling\n");
    if (eastl::to_underlying(mode & multiplexing::Mode::SubSampling))
      res.append("Sub Sampling\n");
    if (eastl::to_underlying(mode & multiplexing::Mode::Viewport))
      res.append("Viewport\n");
    if (eastl::to_underlying(mode & multiplexing::Mode::CameraInCamera))
      res.append("Camera In Camera\n");
  }

  return res.c_str();
}

const char *clear_value_descr(const ResourceClearValue &value)
{
  static String res;
  res.clear();

  res.aprintf(0, "%4s: %12u %12u %12u %12u\n", "Uint", value.asUint[0], value.asUint[1], value.asUint[2], value.asUint[3]);
  res.aprintf(0, "%4s: %12d %12d %12d %12d\n", "Int", value.asInt[0], value.asInt[1], value.asInt[2], value.asInt[3]);
  res.aprintf(0, "%4s: %6f %6f %6f %6f\n", "Flt", value.asFloat[0], value.asFloat[1], value.asFloat[2], value.asFloat[3]);
  res.aprintf(0, "%4s: %6f %4X\n", "D/S", value.asDepth, value.asStencil);

  return res.c_str();
}

const char *auto_res_data_descr(const AutoResTypeData &auto_res, const float mul)
{
  static String res;
  res.clear();

  eastl::visit(
    [&](const auto &res_values) {
      using T = eastl::decay_t<decltype(res_values)>;
      if constexpr (std::is_same_v<T, ResolutionValues<IPoint2>>)
      {
        const ResolutionValues<IPoint2> &vals = res_values;
        res.aprintf(0, "%c: %dx%d\n", 'S', vals.staticResolution.x, vals.staticResolution.y);
        res.aprintf(0, "%c: %dx%d\n", 'D', vals.dynamicResolution.x, vals.dynamicResolution.y);
        res.aprintf(0, "%c: %dx%d\n", 'L', vals.lastAppliedDynamicResolution.x, vals.lastAppliedDynamicResolution.y);
        res.aprintf(0, "x%3f, cd:%3d\n", mul, auto_res.dynamicResolutionCountdown);
      }
      else if constexpr (std::is_same_v<T, ResolutionValues<IPoint3>>)
      {
        const ResolutionValues<IPoint3> &vals = res_values;
        res.aprintf(0, "%c: %dx%dx%d\n", 'S', vals.staticResolution.x, vals.staticResolution.y, vals.staticResolution.z);
        res.aprintf(0, "%c: %dx%dx%d\n", 'D', vals.dynamicResolution.x, vals.dynamicResolution.y, vals.dynamicResolution.z);
        res.aprintf(0, "%c: %dx%dx%d\n", 'L', vals.lastAppliedDynamicResolution.x, vals.lastAppliedDynamicResolution.y,
          vals.lastAppliedDynamicResolution.z);
        res.aprintf(0, "x%3f, cd:%3d\n", mul, auto_res.dynamicResolutionCountdown);
      }
    },
    auto_res.values);

  return res.c_str();
}

const char *texture_flags_descr(const int flags)
{
  static String res;
  res.clear();

  const int createFlags = flags & ~TEXFMT_MASK & ~TEXCF_TYPEMASK;
  if (createFlags == TEXCF_READONLY)
  {
    res.append("READONLY\n");
  }
  else
  {
#define VAR(x)                 \
  if (createFlags & TEXCF_##x) \
    res.append(#x "\n");

    VAR(UNORDERED)
    VAR(VARIABLE_RATE)
    VAR(UPDATE_DESTINATION)
    VAR(SYSTEXCOPY)
    VAR(DYNAMIC)
    VAR(READABLE)

    VAR(WRITEONLY)
    VAR(LOADONCE)
    VAR(SYSMEM)

    VAR(SAMPLECOUNT_2)
    VAR(SAMPLECOUNT_4)
    VAR(SAMPLECOUNT_8)

    VAR(CPU_CACHED_MEMORY)
    VAR(LINEAR_LAYOUT)
    VAR(ESRAM_ONLY)
    VAR(TC_COMPATIBLE)
    VAR(RT_COMPRESSED)

    VAR(SRGBWRITE)
    VAR(SRGBREAD)
    VAR(GENERATEMIPS)
    VAR(CLEAR_ON_CREATE)
    VAR(TILED_RESOURCE)
    VAR(TRANSIENT)

#undef VAR
  }


  return res.c_str();
}

const char *buffer_flags_descr(const int flags)
{
  static String res;
  res.clear();

#define VAR(x)          \
  if (flags & SBCF_##x) \
    res.append("  " #x "\n");

  res.append("Creation flags:\n");

  VAR(USAGE_SHADER_BINDING_TABLE)
  VAR(USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE)
  VAR(USAGE_STREAM_OUTPUT_COUNTER)
  VAR(USAGE_STREAM_OUTPUT)

  VAR(DYNAMIC)
  VAR(OPACITY_MICRO_MAP_TRIANGLE_SOURCE_DATA)
  VAR(ZEROMEM)
  VAR(INDEX32)
  VAR(FRAMEMEM)
  VAR(USAGE_READ_BACK)
  VAR(ALIGN16)

  VAR(CPU_ACCESS_WRITE)
  VAR(CPU_ACCESS_READ)

  VAR(BIND_VERTEX)
  VAR(BIND_INDEX)
  VAR(BIND_CONSTANT)
  VAR(BIND_SHADER_RES)
  VAR(BIND_UNORDERED)

  VAR(MISC_DRAWINDIRECT)
  VAR(MISC_ALLOW_RAW)
  VAR(MISC_STRUCTURED)
  VAR(MISC_ESRAM_ONLY)

  res.append("\nFlag tests:\n");

  VAR(CB_PERSISTENT)
  VAR(CB_ONE_FRAME)

  VAR(UA_SR_BYTE_ADDRESS)
  VAR(UA_SR_STRUCTURED)
  VAR(UA_BYTE_ADDRESS)
  VAR(UA_STRUCTURED)
  VAR(UA_BYTE_ADDRESS_READBACK)
  VAR(UA_STRUCTURED_READBACK)
  VAR(UA_INDIRECT)

  VAR(INDIRECT)
  VAR(STAGING_BUFFER)

#undef VAR

  return res.c_str();
}


static ImU32 negate_color(const ImU32 color)
{
  return ImGui::ColorConvertFloat4ToU32(
    ImVec4{1.f, 1.f, 1.f, 1.f} - ImGui::ColorConvertU32ToFloat4(color) * ImVec4{1.f, 1.f, 1.f, 0.f});
}

void format_pretty(ImDrawList *draw_list, const int format)
{
  constexpr ImU32 ALPHA = IM_COL32_WHITE;
  constexpr ImU32 RED = IM_COL32(192, 16, 16, 255);      // rgb(192, 16, 16)
  constexpr ImU32 GREEN = IM_COL32(16, 192, 16, 255);    // rgb(16, 192, 16)
  constexpr ImU32 BLUE = IM_COL32(16, 16, 192, 255);     // rgb(16, 16, 192)
  constexpr ImU32 DEP_STL = IM_COL32(16, 192, 192, 255); // rgb(16, 192, 192)
  constexpr ImU32 EXO = IM_COL32(192, 16, 192, 255);     // rgb(192, 16, 192)
  constexpr ImU32 SUFFIX = IM_COL32(192, 192, 16, 255);  // rgb(192, 192, 16)

  ImVec2 cursorPos = ImGui::GetCursorScreenPos();

  const auto drawColorBlock = [draw_list, &cursorPos](const char *text, const ImU32 block_color = IM_COL32_BLACK_TRANS) {
    constexpr ImVec2 COLOR_BLOCK_PADDING = ImVec2{2.f, 2.f};

    const ImVec2 blockSize = ImGui::CalcTextSize(text) + 2.f * COLOR_BLOCK_PADDING;

    draw_list->AddRectFilled(cursorPos, cursorPos + blockSize, block_color);
    ImGui::SetCursorScreenPos(cursorPos + COLOR_BLOCK_PADDING);
    ImGui::PushStyleColor(ImGuiCol_Text, negate_color(block_color));
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();

    cursorPos += ImVec2{blockSize.x, 0.f};
  };

  drawColorBlock(get_tex_format_name(format));

  if (format == TEXFMT_A8R8G8B8)
  {
    drawColorBlock("8", ALPHA);
    drawColorBlock("8", RED);
    drawColorBlock("8", GREEN);
    drawColorBlock("8", BLUE);
  }
  else if (format == TEXFMT_A2R10G10B10)
  {
    drawColorBlock("2", ALPHA);
    drawColorBlock("10", RED);
    drawColorBlock("10", GREEN);
    drawColorBlock("10", BLUE);
  }
  else if (format == TEXFMT_A2B10G10R10)
  {
    drawColorBlock("2", ALPHA);
    drawColorBlock("10", BLUE);
    drawColorBlock("10", GREEN);
    drawColorBlock("10", RED);
  }
  else if (format == TEXFMT_A16B16G16R16)
  {
    drawColorBlock("16", ALPHA);
    drawColorBlock("16", BLUE);
    drawColorBlock("16", GREEN);
    drawColorBlock("16", RED);
  }
  else if (format == TEXFMT_A16B16G16R16F)
  {
    drawColorBlock("16", ALPHA);
    drawColorBlock("16", BLUE);
    drawColorBlock("16", GREEN);
    drawColorBlock("16", RED);
    drawColorBlock("F", SUFFIX);
  }
  else if (format == TEXFMT_A32B32G32R32F)
  {
    drawColorBlock("32", ALPHA);
    drawColorBlock("32", BLUE);
    drawColorBlock("32", GREEN);
    drawColorBlock("32", RED);
    drawColorBlock("F", SUFFIX);
  }
  else if (format == TEXFMT_G16R16)
  {
    drawColorBlock("16", GREEN);
    drawColorBlock("16", RED);
  }
  else if (format == TEXFMT_G16R16F)
  {
    drawColorBlock("16", GREEN);
    drawColorBlock("16", RED);
    drawColorBlock("F", SUFFIX);
  }
  else if (format == TEXFMT_G32R32F)
  {
    drawColorBlock("32", GREEN);
    drawColorBlock("32", RED);
    drawColorBlock("F", SUFFIX);
  }
  else if (format == TEXFMT_R16F)
  {
    drawColorBlock("16", RED);
    drawColorBlock("F", SUFFIX);
  }
  else if (format == TEXFMT_R32F)
  {
    drawColorBlock("32", RED);
    drawColorBlock("F", SUFFIX);
  }
  else if (format == TEXFMT_DXT1)
  {
    drawColorBlock("COMPRESS", SUFFIX);
  }
  else if (format == TEXFMT_DXT3)
  {
    drawColorBlock("COMPRESS", SUFFIX);
  }
  else if (format == TEXFMT_DXT5)
  {
    drawColorBlock("COMPRESS", SUFFIX);
  }
  else if (format == TEXFMT_R32G32UI)
  {
    drawColorBlock("32", RED);
    drawColorBlock("32", GREEN);
    drawColorBlock("UI", SUFFIX);
  }
  else if (format == TEXFMT_L16)
  {
    drawColorBlock("16", RED);
  }
  else if (format == TEXFMT_A8)
  {
    drawColorBlock("8", ALPHA);
  }
  else if (format == TEXFMT_R8)
  {
    drawColorBlock("8", RED);
  }
  else if (format == TEXFMT_A1R5G5B5)
  {
    drawColorBlock("1", ALPHA);
    drawColorBlock("5", RED);
    drawColorBlock("5", GREEN);
    drawColorBlock("5", BLUE);
  }
  else if (format == TEXFMT_A4R4G4B4)
  {
    drawColorBlock("4", ALPHA);
    drawColorBlock("4", RED);
    drawColorBlock("4", GREEN);
    drawColorBlock("4", BLUE);
  }
  else if (format == TEXFMT_R5G6B5)
  {
    drawColorBlock("5", RED);
    drawColorBlock("6", GREEN);
    drawColorBlock("5", BLUE);
  }
  else if (format == TEXFMT_A8B8G8R8)
  {
    drawColorBlock("8", ALPHA);
    drawColorBlock("8", BLUE);
    drawColorBlock("8", GREEN);
    drawColorBlock("8", RED);
  }
  else if (format == TEXFMT_A16B16G16R16S)
  {
    drawColorBlock("16", ALPHA);
    drawColorBlock("16", BLUE);
    drawColorBlock("16", GREEN);
    drawColorBlock("16", RED);
    drawColorBlock("S", SUFFIX);
  }
  else if (format == TEXFMT_A16B16G16R16UI)
  {
    drawColorBlock("16", ALPHA);
    drawColorBlock("16", BLUE);
    drawColorBlock("16", GREEN);
    drawColorBlock("16", RED);
    drawColorBlock("UI", SUFFIX);
  }
  else if (format == TEXFMT_A32B32G32R32UI)
  {
    drawColorBlock("32", ALPHA);
    drawColorBlock("32", BLUE);
    drawColorBlock("32", GREEN);
    drawColorBlock("32", RED);
    drawColorBlock("UI", SUFFIX);
  }
  else if (format == TEXFMT_ATI1N)
  {
    drawColorBlock("COMPRESS", SUFFIX);
  }
  else if (format == TEXFMT_ATI2N)
  {
    drawColorBlock("COMPRESS", SUFFIX);
  }
  else if (format == TEXFMT_R8G8B8A8)
  {
    drawColorBlock("8", RED);
    drawColorBlock("8", GREEN);
    drawColorBlock("8", BLUE);
    drawColorBlock("8", ALPHA);
  }
  else if (format == TEXFMT_R32UI)
  {
    drawColorBlock("32", RED);
    drawColorBlock("UI", SUFFIX);
  }
  else if (format == TEXFMT_R32SI)
  {
    drawColorBlock("32", RED);
    drawColorBlock("SI", SUFFIX);
  }
  else if (format == TEXFMT_R11G11B10F)
  {
    drawColorBlock("11", RED);
    drawColorBlock("11", GREEN);
    drawColorBlock("10", BLUE);
    drawColorBlock("F", SUFFIX);
  }
  else if (format == TEXFMT_R9G9B9E5)
  {
    drawColorBlock("9", RED);
    drawColorBlock("9", GREEN);
    drawColorBlock("9", BLUE);
    drawColorBlock("5", EXO);
  }
  else if (format == TEXFMT_R8G8)
  {
    drawColorBlock("8", RED);
    drawColorBlock("8", GREEN);
  }
  else if (format == TEXFMT_R8G8S)
  {
    drawColorBlock("8", RED);
    drawColorBlock("8", GREEN);
    drawColorBlock("S", SUFFIX);
  }
  else if (format == TEXFMT_BC6H)
  {
    drawColorBlock("COMPRESS", SUFFIX);
  }
  else if (format == TEXFMT_BC7)
  {
    drawColorBlock("COMPRESS", SUFFIX);
  }
  else if (format == TEXFMT_R8UI)
  {
    drawColorBlock("8", RED);
    drawColorBlock("UI", SUFFIX);
  }
  else if (format == TEXFMT_R16UI)
  {
    drawColorBlock("16", RED);
    drawColorBlock("UI", SUFFIX);
  }
  else if (format == TEXFMT_DEPTH24)
  {
    drawColorBlock("D", DEP_STL);
    drawColorBlock("24", RED);
  }
  else if (format == TEXFMT_DEPTH16)
  {
    drawColorBlock("D", DEP_STL);
    drawColorBlock("16", RED);
  }
  else if (format == TEXFMT_DEPTH32)
  {
    drawColorBlock("D", DEP_STL);
    drawColorBlock("32", RED);
  }
  else if (format == TEXFMT_DEPTH32_S8)
  {
    drawColorBlock("D", DEP_STL);
    drawColorBlock("32", RED);
    drawColorBlock("S", DEP_STL);
    drawColorBlock("8", RED);
  }
  else if (format == TEXFMT_ASTC4)
  {
    drawColorBlock("COMPRESS", SUFFIX);
  }
  else if (format == TEXFMT_ASTC8)
  {
    drawColorBlock("COMPRESS", SUFFIX);
  }
  else if (format == TEXFMT_ASTC12)
  {
    drawColorBlock("COMPRESS", SUFFIX);
  }
  else if (format == TEXFMT_ETC2_RG)
  {
    drawColorBlock("COMPRESS", SUFFIX);
  }
  else if (format == TEXFMT_ETC2_RGBA)
  {
    drawColorBlock("COMPRESS", SUFFIX);
  }
#if _TARGET_C2




#endif
}


static NodeNameId requestedFocusNode = NodeNameId::Invalid;
void request_user_node_focus(const NodeNameId id) { requestedFocusNode = id; }
bool check_user_node_focus_request() { return requestedFocusNode != NodeNameId::Invalid; }
NodeNameId get_user_node_focus_request() { return eastl::exchange(requestedFocusNode, NodeNameId::Invalid); }

static ResNameId requestedFocusResource = ResNameId::Invalid;
void request_user_resource_focus(const ResNameId id) { requestedFocusResource = id; }
bool check_user_resource_focus_request() { return requestedFocusResource != ResNameId::Invalid; }
ResNameId get_user_resource_focus_request() { return eastl::exchange(requestedFocusResource, ResNameId::Invalid); }

} // namespace dafg::visualization