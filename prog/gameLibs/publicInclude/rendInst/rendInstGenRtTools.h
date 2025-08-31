//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <rendInst/rendInstGen.h>
#include <rendInst/edHugeBitmap2d.h>
#include <shaders/dag_rendInstRes.h>


struct RendInstGenData;
class DataBlock;

namespace rendinst
{

namespace gen::land
{
class AssetData;
}

typedef HierBitMap2d<HierConstSizeBitMap2d<4, ConstSizeBitMap2d<5>>> EditableHugeBitmask;
typedef void (*rigen_gather_pos_t)(Tab<Point4> &dest, Tab<int> &dest_per_inst_data, bool add_per_inst_data, int idx, int pregen_id,
  float x0, float z0, float x1, float z1);
typedef void (*rigen_gather_tm_t)(Tab<TMatrix> &dest, Tab<int> &dest_per_inst_data, bool add_per_inst_data, int idx, int pregen_id,
  float x0, float z0, float x1, float z1);
typedef void (*rigen_calculate_mapping_t)(Tab<int> &indices, int flg, unsigned riPoolBitsMask);
typedef void (*rigen_prepare_pools_t)(bool begin);

bool create_rt_rigen_data(float ofs_x, float ofs_z, float grid2world, int cell_sz, int cell_num_x, int cell_num_z,
  int per_inst_data_dwords, dag::ConstSpan<rendinst::gen::land::AssetData *> land_cls, float dens_map_px, float dens_map_pz);
int register_rt_pregen_ri(RenderableInstanceLodsResource *ri_res, int layer_idx, E3DCOLOR cf, E3DCOLOR ct,
  const char *res_nm = nullptr);
void update_rt_pregen_ri(int pregen_id, RenderableInstanceLodsResource &ri_res);
void set_rt_pregen_gather_cb(rigen_gather_pos_t cb_pos, rigen_gather_tm_t cb_tm, rigen_calculate_mapping_t cb_ind,
  rigen_prepare_pools_t cb_prep);
void release_rt_rigen_data();

void rt_rigen_free_unused_land_classes();

int get_rigen_data_layers();
RendInstGenData *get_rigen_data(int layer_idx);
int get_rigen_layers_cell_div(int layer_idx);

EditableHugeBitMap2d *get_rigen_bit_mask(rendinst::gen::land::AssetData *land_cls);

void set_rigen_sweep_mask(EditableHugeBitmask *bm, float ox, float oz, float scale);
void enable_rigen_mask_generated(bool en = true);

void discard_rigen_rect(int gx0, int gz0, int gx1, int gz1);
void discard_rigen_all();

void notify_ri_moved(int ri_idx, float ox, float oz, float nx, float nz);
void notify_ri_deleted(int ri_idx, float ox, float oz);

void prepare_rt_rigen_data_render(const Point3 &pos, const TMatrix &view_itm, const mat44f &proj_tm);
void generate_rt_rigen_main_cells(const BBox3 &area);
void calculate_box_extension_for_objects_in_grid(bbox3f &out_bbox);

bool compute_rt_rigen_tight_ht_limits(int layer_idx, int cx, int cz, float &out_hmin, float &out_hdelta);

void set_sun_dir_for_global_shadows(const Point3 &sun_dir);
void set_global_shadows_needed(bool need);

void get_layer_idx_and_ri_idx_from_pregen_id(int pregen_id, int &layer_idx, int &ri_idx);

// Tries to find the rendInst matrix by coordinates.
// If there are multiple rendInsts with the same asset type at the same (or very near) location then it returns with failure.
enum class GetRendInstMatrixByRiIdxResult
{
  Success, // out_tm is only set in this case.
  Failure,
  NonReady,
  MultipleInstancesAtThePostion,
};
GetRendInstMatrixByRiIdxResult get_rendinst_matrix_by_ri_idx(int layer_idx, int ri_idx, const Point3 &position, TMatrix &out_tm);

// Aligns/compresses/quantizes the transformation matrix in the same way as it's done by internally in the rendInst system.
// It should return with the same result as riutil::get_rendinst_matrix function but they not always match perfectly.
// This function can be used as a fallback if get_rendinst_matrix_by_ri_idx fails.
enum class QuantizeRendInstMatrixResult
{
  Success, // out_tm is only set in this case.
  Failure,
  NonReady,
};
QuantizeRendInstMatrixResult quantize_rendinst_matrix(int layer_idx, int ri_idx, const TMatrix &in_tm, TMatrix &out_tm);

unsigned precompute_ri_cells_for_stats(DataBlock &out_stats);
} // namespace rendinst
