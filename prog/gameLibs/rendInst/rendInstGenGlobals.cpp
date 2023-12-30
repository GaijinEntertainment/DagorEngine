#include "riGen/genObjUtil.h"
#include "riGen/riGenData.h"


rendinst::gen::WorldHugeBitmask rendinst::gen::lcmapExcl;
rendinst::gen::WorldEditableHugeBitmask rendinst::gen::destrExcl;

float rendinst::gen::SingleEntityPool::ox = 0;
float rendinst::gen::SingleEntityPool::oy = 0;
float rendinst::gen::SingleEntityPool::oz = 0;
float rendinst::gen::SingleEntityPool::cell_xz_sz = 1;
float rendinst::gen::SingleEntityPool::cell_y_sz = 8192.0;
bbox3f rendinst::gen::SingleEntityPool::bbox;
bbox3f *rendinst::gen::SingleEntityPool::per_pool_local_bb = nullptr;
dag::ConstSpan<const TMatrix *> rendinst::gen::SingleEntityPool::sweep_boxes_itm;
dag::ConstSpan<E3DCOLOR> rendinst::gen::SingleEntityPool::ri_col_pair;
int rendinst::gen::SingleEntityPool::cur_cell_id = 0;
int rendinst::gen::SingleEntityPool::cur_ri_extra_ord = 0;
int rendinst::gen::SingleEntityPool::per_inst_data_dwords = 0;

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
rendinst::ri_register_collision_cb rendinst::regCollCb = nullptr;
rendinst::ri_unregister_collision_cb rendinst::unregCollCb = nullptr;
int rendinst::ri_game_render_mode = -1;

bool rendinst::enable_apex = false;

void (*RendInstGenData::riGenPrepareAddPregenCB)(RendInstGenData::CellRtData &crt, int layer_idx, int per_inst_data_dwords, float ox,
  float oy, float oz, float cell_xz_sz, float cell_y_sz) = nullptr;
RendInstGenData::CellRtData *(*RendInstGenData::riGenValidateGeneratedCell)(RendInstGenData *rgl, RendInstGenData::CellRtData *crt,
  int idx, int cx, int cz) = nullptr;
void (*rendinst::do_delayed_ri_extra_destruction)() = nullptr;
void (*rendinst::sweep_rendinst_cb)(const RendInstDesc &) = nullptr;
