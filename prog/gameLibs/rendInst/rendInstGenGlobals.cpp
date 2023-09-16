#include "riGen/genObjUtil.h"
#include "riGen/riGenData.h"


rendinstgen::WorldHugeBitmask rendinstgen::lcmapExcl;
rendinstgen::WorldEditableHugeBitmask rendinstgen::destrExcl;

float rendinstgen::SingleEntityPool::ox = 0;
float rendinstgen::SingleEntityPool::oy = 0;
float rendinstgen::SingleEntityPool::oz = 0;
float rendinstgen::SingleEntityPool::cell_xz_sz = 1;
float rendinstgen::SingleEntityPool::cell_y_sz = 8192.0;
bbox3f rendinstgen::SingleEntityPool::bbox;
bbox3f *rendinstgen::SingleEntityPool::per_pool_local_bb = NULL;
dag::ConstSpan<const TMatrix *> rendinstgen::SingleEntityPool::sweep_boxes_itm;
dag::ConstSpan<E3DCOLOR> rendinstgen::SingleEntityPool::ri_col_pair;
int rendinstgen::SingleEntityPool::cur_cell_id = 0;
int rendinstgen::SingleEntityPool::cur_ri_extra_ord = 0;
int rendinstgen::SingleEntityPool::per_inst_data_dwords = 0;

#if _TARGET_PC && !_TARGET_STATIC_LIB // Tools (De3X, AV2)
namespace rendinst
{
bool forceRiExtra = false;
}
#endif

StaticTab<RendInstGenData *, 16> rendinst::rgLayer;
StaticTab<rendinst::RiGenDataAttr, 16> rendinst::rgAttr;
unsigned rendinst::rgPrimaryLayers = 0;
unsigned rendinst::rgRenderMaskO = 0, rendinst::rgRenderMaskDS = 0, rendinst::rgRenderMaskCMS = 0;

bool RendInstGenData::isLoading = false;
bool RendInstGenData::renderResRequired = true;
bool RendInstGenData::useDestrExclForPregenAdd = true;
bool RendInstGenData::maskGeneratedEnabled = true;
rendinst::ri_register_collision_cb rendinst::regCollCb = NULL;
rendinst::ri_unregister_collision_cb rendinst::unregCollCb = NULL;
int rendinst::ri_game_render_mode = -1;

bool rendinst::enable_apex = false;

void (*RendInstGenData::riGenPrepareAddPregenCB)(RendInstGenData::CellRtData &crt, int layer_idx, int per_inst_data_dwords, float ox,
  float oy, float oz, float cell_xz_sz, float cell_y_sz) = NULL;
RendInstGenData::CellRtData *(*RendInstGenData::riGenValidateGeneratedCell)(RendInstGenData *rgl, RendInstGenData::CellRtData *crt,
  int idx, int cx, int cz) = NULL;
