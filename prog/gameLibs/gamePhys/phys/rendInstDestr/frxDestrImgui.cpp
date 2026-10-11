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
  rendinst::RendInstBufferData buffer;
  FrxDestrProps riFrxDestr;
  frx::ShatterImpactProfile impact;
  Point3 center = Point3::ZERO;
  frx::ShatterMaterialProfileImGuiState profileState;
  dag::Vector<FrxPreviewPiece> pieces;
  int seed = 1;
  float spread = 1.2f;
  bool selected = false;
  bool hasOwnProps = false;
  bool armed = false;
  bool dbgDrawn = false;
} frx_preview;

static void rebuild_frx_preview()
{
  FRAMEMEM_REGION;
  frx_preview.pieces.clear();
  if (frx_preview.dbgDrawn)
    clear_buffered_debug_lines();

  FrxDestrProps frxDestr = frx_preview.riFrxDestr;
  frxDestr.shatterProfile = frx_preview.profileState.profile;
  int seed = frx_preview.seed;
  frx::DestrContext ctx;
  ctx.dbgDraw.drawCutEdgeGraph = frx_preview.dbgDrawn = frx_preview.spread < DBG_DRAW_MAX_SPREAD;
  frx::DestrSystem sys;
  if (!frx_destroy_ri(ctx, sys, rendinst::getRIGenExtraRes(frx_preview.buffer.desc.pool), frx_preview.buffer, frxDestr,
        frx_preview.impact, seed))
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

  ImGui::Text("%s%s: %d pieces", rendinst::getRIGenExtraName(frx_preview.buffer.desc.pool),
    frx_preview.hasOwnProps ? "" : " (no frxDestr, defaults)", int(frx_preview.pieces.size()));
  if (ImGui::Button("Restore rendinst"))
  {
    rendinst::restoreRendInst(frx_preview.buffer, true);
    frx_preview_clear();
    return;
  }
  ImGui::DragFloat("spread", &frx_preview.spread, 0.01f, 1.f, 2.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
  bool changed = (frx_preview.spread < DBG_DRAW_MAX_SPREAD) != frx_preview.dbgDrawn;
  changed |= ImGui::InputInt("seed", &frx_preview.seed);
  ImGui::SameLine();
  if (ImGui::Button("Reroll"))
  {
    frx_preview.seed = grnd() % 10000;
    changed = true;
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
    rebuild_frx_preview();
}

REGISTER_IMGUI_WINDOW("Render", "Fracture Preview", frx_preview_window);

} // namespace rendinstdestr

void rendinstdestr::frx_preview_select(const rendinst::RendInstBufferData &buffer, const Point3 &impact_pos)
{
  G_ASSERT_RETURN(buffer.desc.isRiExtra(), );
  if (frx_preview.selected)
    rendinst::restoreRendInst(frx_preview.buffer, true);
  const RiDestrProps *props = get_ri_destr_props(buffer.desc);
  frx_preview.hasOwnProps = props && props->frxDestr;
  frx_preview.riFrxDestr = frx_preview.hasOwnProps ? *props->frxDestr : FrxDestrProps{};
  frx_preview.selected = true;
  frx_preview.buffer = buffer;
  frx_preview.impact = make_shatter_impact_profile(impact_pos);
  v_stu_p3(&frx_preview.center.x, v_mat44_mul_vec3p(buffer.tm, v_bbox3_center(rendinst::riex_get_lbb(buffer.desc.pool))));
  frx_preview.profileState = {};
  frx_preview.profileState.setProfile(frx_preview.riFrxDestr.shatterProfile);
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
