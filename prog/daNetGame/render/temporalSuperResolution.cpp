// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "temporalSuperResolution.h"
#include "render/antialiasing.h"

#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <render/world/frameGraphHelpers.h>
#include <render/resourceSlot/registerAccess.h>
#include <render/viewVecs.h>

CONSOLE_BOOL_VAL("render", tsr_allow_reactive, true);

namespace var
{
static ShaderVariableInfo contrast_adaptive_sharpening_aa_bias("contrast_adaptive_sharpening_aa_bias", true);
}

bool TemporalSuperResolution::needMotionVectors() const { return preset == Preset::High; }

bool TemporalSuperResolution::supportsReactiveMask() const { return preset == Preset::High && tsr_allow_reactive.get(); }

TemporalSuperResolution::Preset TemporalSuperResolution::parse_preset()
{
  using P = TemporalSuperResolution::Preset;
  return P(clamp(::dgs_get_settings()->getBlockByNameEx("graphics")->getInt("tsrQuality", eastl::to_underlying(P::High)),
    eastl::to_underlying(P::Low), eastl::to_underlying(P::High)));
}

TemporalSuperResolution::~TemporalSuperResolution() { ShaderGlobal::set_float(var::contrast_adaptive_sharpening_aa_bias, 0); }

TemporalSuperResolution::TemporalSuperResolution(const IPoint2 &output_resolution) :
  AntiAliasing(parse_preset() == Preset::Low ? output_resolution : computeInputResolution(output_resolution), output_resolution),
  preset(parse_preset()),
  available(false)
{
  if (!render::antialiasing::try_init_tsr(outputResolution, inputResolution))
  {
    logerr("Temporal Super Resolution initialization failed.");
    return;
  }

  // always run TSR with slight sharpen to compensate fact that non moving pixels are blurred slightly by TSR
  ShaderGlobal::set_float(var::contrast_adaptive_sharpening_aa_bias, 0.1);

  applierNode = dafg::register_node("tsr", DAFG_PP_NODE_SRC, [this, output_resolution](dafg::Registry registry) {
    auto opaqueFinalTargetHndl =
      registry.readTexture("target_for_transparency").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    read_gbuffer(registry, dafg::Stage::PS_OR_CS);
    registry.readTexture("motion_vecs_after_transparency")
      .atStage(dafg::Stage::PS_OR_CS)
      .bindToShaderVar("resolved_motion_vectors")
      .optional();
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("resolved_motion_vectors_samplerstate").optional();
    registry.readTexture("depth_after_transparency").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("depth_gbuf");
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("depth_gbuf_samplerstate");

    auto antialiasedHndl = registry.createTexture2d("frame_after_aa", {TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, output_resolution})
                             .withHistory(dafg::History::ClearZeroOnFirstFrame)
                             .atStage(dafg::Stage::PS_OR_CS)
                             .useAs(dafg::Usage::SHADER_RESOURCE)
                             .handle();

    auto antialiasedHistHndl =
      registry.readTextureHistory("frame_after_aa").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    auto confidenceHndl = registry.createTexture2d("tsr_confidence", {TEXFMT_R8 | TEXCF_UNORDERED, output_resolution})
                            .withHistory(dafg::History::ClearZeroOnFirstFrame)
                            .atStage(dafg::Stage::PS_OR_CS)
                            .useAs(dafg::Usage::SHADER_RESOURCE)
                            .handle();

    auto confidenceHistHndl =
      registry.readTextureHistory("tsr_confidence").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    auto reactiveMaskHndl =
      registry.readTexture("reactive_mask").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).optional().handle();

    auto camera = registry.readBlob<CameraParams>("current_camera");
    CameraViewShvars{camera}.bindViewVecs();

    registry.readBlob<OrderingToken>("motion_vector_access_token");

    return [this, opaqueFinalTargetHndl, antialiasedHndl, antialiasedHistHndl, confidenceHndl, confidenceHistHndl, reactiveMaskHndl] {
      render::antialiasing::ApplyContext ctx;
      ctx.resetHistory = frameCounter == 0;
      ctx.jitterPixelOffset = jitterOffset;
      ctx.inputResolution = inputResolution;
      render::antialiasing::apply_tsr(opaqueFinalTargetHndl.get(), ctx, antialiasedHndl.get(), antialiasedHistHndl.get(),
        confidenceHndl.get(), confidenceHistHndl.get(), reactiveMaskHndl.get());
    };
  });

  available = true;
}

float TemporalSuperResolution::getLodBias() const { return render::antialiasing::get_mip_bias(); }
