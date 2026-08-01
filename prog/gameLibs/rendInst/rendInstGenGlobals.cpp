// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "riGen/genObjUtil.h"
#include "riGen/riGenData.h"


rendinst::gen::WorldHugeBitmask rendinst::gen::lcmapExcl;
rendinst::gen::WorldEditableHugeBitmask rendinst::gen::destrExcl;

namespace rendinst
{
#if _TARGET_PC_TOOLS_BUILD
bool forceRiExtra = false;
bool enableRiExtra = true;
#endif
bool persistentRiExtraInstances = true;
bool allowOptimizeCollResOnLoad = true;
} // namespace rendinst

// TODO: eliminate these globals, e.g. by extracting them into a struct and passing it explicitly.
StaticTab<RendInstGenData *, rendinst::MAX_RG_LAYERS> rendinst::rgLayer;
StaticTab<rendinst::RiGenDataAttr, rendinst::MAX_RG_LAYERS> rendinst::rgAttr;
unsigned rendinst::rgPrimaryLayers = 0;
unsigned rendinst::rgRenderMaskO = 0, rendinst::rgRenderMaskDS = 0, rendinst::rgRenderMaskCMS = 0;

bool RendInstGenData::isLoading = false;
bool RendInstGenData::renderResRequired = true;
bool RendInstGenData::useDestrExclForPregenAdd = true;
bool RendInstGenData::maskGeneratedEnabled = true;
rendinst::ri_register_collision_cb rendinst::regCollCb = nullptr;
rendinst::ri_unregister_collision_cb rendinst::unregCollCb = nullptr;
int rendinst::ri_game_render_mode = -1;

bool rendinst::enable_apex = false;

void (*RendInstGenData::riGenPrepareAddPregenCB)(RendInstGenData::CellRtData &crt, int layer_idx, int per_inst_data_dwords, float ox,
  float oy, float oz, float cell_xz_sz, float cell_y_sz, bbox3f &cell_bbox) = nullptr;
RendInstGenData::CellRtData *(*RendInstGenData::riGenValidateGeneratedCell)(RendInstGenData *rgl, RendInstGenData::CellRtData *crt,
  int idx, int cx, int cz) = nullptr;
void (*rendinst::do_delayed_ri_extra_destruction)() = nullptr;
void (*rendinst::sweep_rendinst_cb)(const RendInstDesc &) = nullptr;
void (*rendinst::shader_material_validation_cb)(ShaderMaterial *mat, const char *res_name) = nullptr;