// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frxDestr.h"
#include "riDestrInternal.h"

#if RI_DESTR_DEBUG_ENABLED

#include <EASTL/unique_ptr.h>
#include <dag/dag_vector.h>
#include <debug/dag_debug3d.h>
#include <generic/dag_span.h>
#include <gui/dag_imgui.h>
#include <imgui/imgui.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include <math/dag_TMatrix.h>
#include <math/random/dag_random.h>
#include <memory/dag_framemem.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstDebris.h>
#include <rendInst/rendInstExtra.h>
#include <vecmath/dag_vecMath.h>

#include <daFracture/core/destrMesh.h>
#include <daFracture/core/shatterMaterialImgui.h>
#include <daFracture/render/renderList.h>
#include <daFracture/render/renderMesh.h>

#include "riDestrProps.h"


namespace rendinstdestr
{

static constexpr float DBG_DRAW_MAX_SPREAD = 1.01f;

struct FrxPreviewPiece
{
  eastl::unique_ptr<frx::RenderMesh> mesh;
  TMatrix tm;
};

static struct FrxPreview
{
  FrxDestrInput input;
  FrxDestrProps riFrxDestr;
  FrxDestrProps editedFrxDestr;
  Point3 center = Point3::ZERO;
  frx::ShatterMaterialProfileImGuiState profileState;
  dag::Vector<FrxPreviewPiece> pieces;

  int seed = 1;
  float spread = 1.2f;
  float drawHeightScale = 1.f;
  bool drawCuts = false;
  bool selected = false;
  bool hasOwnProps = false;
  bool armed = false;
  bool dbgDrawn = false;

  bool measuredTimeValid = false;
  int measuredTimeUs = 0;
  int measuredTimeRunCnt = 0;
} frx_preview;

static void rebuild_frx_preview()
{
  FRAMEMEM_REGION;
  frx_preview.pieces.clear();
  if (frx_preview.dbgDrawn)
    clear_buffered_debug_lines();

  frx_preview.editedFrxDestr = frx_preview.riFrxDestr;
  frx_preview.editedFrxDestr.shatterProfile = frx_preview.profileState.profile;
  int seed = frx_preview.seed;
  frx::DestrContext ctx;
  ctx.dbgDraw.drawCutEdgeGraph = frx_preview.dbgDrawn = frx_preview.spread < DBG_DRAW_MAX_SPREAD || frx_preview.drawCuts;
  ctx.dbgDraw.heightScale = frx_preview.drawHeightScale;
  frx::DestrSystem sys;
  if (!frx_destroy_ri(ctx, sys, frx_preview.input, seed))
    return;

  dag::Vector<frx::MeshUploadRequest> uploads;
  uploads.reserve(sys.pieces.size());
  for (auto &[id, piece] : sys.pieces)
  {
    FrxPreviewPiece &previewPiece = frx_preview.pieces.push_back();
    previewPiece.mesh = eastl::make_unique<frx::RenderMesh>();
    previewPiece.tm = piece.tm;
    uploads.push_back({&piece, previewPiece.mesh.get()});
  }
  frx::upload_mesh_batch(ctx, make_span(uploads));
}

static void copy_shatter_blk_to_clipboard(const frx::ShatterMaterialProfile *base)
{
  DataBlock blk(framemem_ptr());
  frx_preview.profileState.profile.saveToBlk(*blk.addNewBlock("shatter"), base);
  DynamicMemGeneralSaveCB dump(framemem_ptr());
  blk.saveToTextStream(dump);
  dump.write("\0", 1);
  ImGui::SetClipboardText((const char *)dump.data());
}

static void frx_preview_window()
{
  ImGui::Checkbox("Capture next destroyed rendinst", &frx_preview.armed);
  if (!frx_preview.selected)
    return;

  ImGui::Text("%s%s: %d pieces", rendinst::getRIGenExtraName(frx_preview.input.buffer.desc.pool),
    frx_preview.hasOwnProps ? "" : " (no frxDestr, defaults)", int(frx_preview.pieces.size()));
  if (ImGui::Button("Restore rendinst"))
  {
    restore_rendinst(frx_preview.input.buffer);
    frx_preview_clear();
    return;
  }
  ImGui::DragFloat("spread", &frx_preview.spread, 0.01f, 1.f, 2.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
  ImGui::Checkbox("Draw cuts", &frx_preview.drawCuts);
  bool redraw = (frx_preview.spread < DBG_DRAW_MAX_SPREAD || frx_preview.drawCuts) != frx_preview.dbgDrawn;
  redraw |=
    ImGui::DragFloat("draw height scale", &frx_preview.drawHeightScale, 0.01f, 0.f, 1.f, "%.2f", ImGuiSliderFlags_AlwaysClamp) &&
    frx_preview.dbgDrawn;
  bool changed = ImGui::InputInt("seed", &frx_preview.seed);
  ImGui::SameLine();
  if (ImGui::Button("Reroll"))
  {
    frx_preview.seed = grnd() % 10000;
    changed = true;
  }
  if (ImGui::Button("Measure"))
  {
    const uint64_t timeStart = profile_ref_ticks();
    frx_preview.measuredTimeRunCnt = 0;
    do
    {
      FRAMEMEM_REGION;
      frx::DestrContext ctx;
      frx::DestrSystem sys;
      int seed = frx_preview.seed;
      frx_destroy_ri(ctx, sys, frx_preview.input, seed);
      frx_preview.measuredTimeRunCnt++;
    } while ((frx_preview.measuredTimeUs = profile_time_usec(timeStart)) < 500000 && frx_preview.measuredTimeRunCnt < 50);
    frx_preview.measuredTimeValid = true;
  }
  if (frx_preview.measuredTimeRunCnt > 0)
  {
    const float runSec = float(frx_preview.measuredTimeUs) / float(frx_preview.measuredTimeRunCnt) * 1e-6f;
    float runFrames = runSec * ImGui::GetIO().Framerate;
    if (runFrames > 2.f)
      runFrames = floorf(runFrames * 2.f + 0.5f) * 0.5f;
    const ImVec4 color = !frx_preview.measuredTimeValid       ? ImVec4(0.5f, 0.5f, 0.5f, 1.f)
                         : runFrames < 3.f && runSec < 0.075f ? ImVec4(0.f, 1.f, 0.f, 1.f)
                         : runFrames < 8.f && runSec < 0.25f  ? ImVec4(1.f, 1.f, 0.f, 1.f)
                                                              : ImVec4(1.f, 0.f, 0.f, 1.f);
    ImGui::SameLine();
    ImGui::TextColored(color, "Avg time %.3fms ~ %.1f frames (%i runs)%s", runSec * 1e3f, runFrames, frx_preview.measuredTimeRunCnt,
      frx_preview.measuredTimeValid ? "" : " [STALE]");
  }
  if (ImGui::Button("Reset profile"))
  {
    frx_preview.profileState = {};
    frx_preview.profileState.setProfile(frx_preview.riFrxDestr.shatterProfile);
    changed = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Copy full blk"))
    copy_shatter_blk_to_clipboard(nullptr);
  ImGui::SameLine();
  if (ImGui::Button("Copy non-default"))
  {
    const frx::ShatterMaterialProfile defaults{};
    copy_shatter_blk_to_clipboard(&defaults);
  }
  ImGui::Separator();
  changed |= frx::imgui_draw_shatter_material_profile(frx_preview.profileState);
  if (changed)
    frx_preview.measuredTimeValid = false;
  if (changed || redraw)
    rebuild_frx_preview();
}

REGISTER_IMGUI_WINDOW("Render", "Fracture Preview", frx_preview_window);

} // namespace rendinstdestr

void rendinstdestr::frx_preview_select(const rendinst::RendInstBufferData &buffer, const Point3 &impact_pos)
{
  G_ASSERT_RETURN(buffer.desc.isRiExtra(), );
  if (frx_preview.selected)
    restore_rendinst(frx_preview.input.buffer);
  const RiDestrProps *props = get_ri_destr_props(buffer.desc);
  frx_preview.hasOwnProps = props && props->frxDestr;
  frx_preview.riFrxDestr = frx_preview.hasOwnProps ? *props->frxDestr : FrxDestrProps{};
  frx_preview.selected = true;
  frx_prepare_destr_input(buffer, impact_pos, frx_preview.input);
  frx_preview.input.frxProps = &frx_preview.editedFrxDestr;
  v_stu_p3(&frx_preview.center.x, v_mat44_mul_vec3p(buffer.tm, v_bbox3_center(rendinst::riex_get_lbb(buffer.desc.pool))));
  frx_preview.profileState = {};
  frx_preview.profileState.setProfile(frx_preview.riFrxDestr.shatterProfile);
  frx_preview.measuredTimeRunCnt = 0;
  rebuild_frx_preview();
}

bool rendinstdestr::frx_preview_intercept_destruction(const rendinst::RendInstBufferData &buffer,
  rendinst::riex_handle_t destroyed_riex_handle, const Point3 &impact_pos)
{
  if (!frx_preview.armed || !buffer.desc.isRiExtra())
    return false;
  frx_preview.armed = false;
  rendinst::delRIGenExtra(destroyed_riex_handle);
  frx_preview_select(buffer, impact_pos);
  return true;
}

void rendinstdestr::frx_preview_clear()
{
  if (frx_preview.dbgDrawn)
    clear_buffered_debug_lines();
  frx_preview.dbgDrawn = false;
  frx_preview.selected = false;
  frx_preview.armed = false;
  frx_preview.pieces.clear();
  frx_preview.input.meshes.clear();
}

void rendinstdestr::frx_preview_add_meshes(frx::MeshRenderList &list)
{
  for (const FrxPreviewPiece &piece : frx_preview.pieces)
  {
    TMatrix tm = piece.tm;
    tm.setcol(3, frx_preview.center + (tm.getcol(3) - frx_preview.center) * frx_preview.spread);
    list.add(*piece.mesh, tm, tm);
  }
}

#else

void rendinstdestr::frx_preview_select(const rendinst::RendInstBufferData &, const Point3 &) {}
bool rendinstdestr::frx_preview_intercept_destruction(const rendinst::RendInstBufferData &, rendinst::riex_handle_t, const Point3 &)
{
  return false;
}
void rendinstdestr::frx_preview_clear() {}
void rendinstdestr::frx_preview_add_meshes(frx::MeshRenderList &) {}

#endif
