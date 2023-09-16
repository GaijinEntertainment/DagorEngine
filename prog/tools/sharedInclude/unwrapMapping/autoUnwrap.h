
#ifndef __DAGOR_LTMPMAP_H
#define __DAGOR_LTMPMAP_H
#pragma once


#include <libTools/util/progressInd.h>


class Point3;
class Point2;
struct Face;
struct TFace;


namespace AutoUnwrapMapping
{
enum
{
  SCHEME_DEFAULT = -1,
  SCHEME_DAGOR = 0,
  SCHEME_GRAPHITE = 1,
};

void set_graphite_folder(const char *folder);

//! starts mapping process (reset internal structures)
void start_mapping(int lm_sz = 1024);


//! performs mesh unwrap (dividing into facegroups) and returns object mapping handle
int request_mapping(Face *face, int fnum, Point3 *vert, int vnum, float samplesize, bool perobj, int unwrap_scheme = -1);

//! performs unwrapping of all facegroups (created in preceeding request_mapping())
void prepare_facegroups(IGenericProgressIndicator &pbar, SimpleProgressCB &progr_cb);

//! performs packing of all facegroups (created in preceeding prepare_facegroups()) and
//! returns number of lightmaps (and usage area ratio, if needed)
//! NOTE: when max_lm_num > 0, repack_facegroups() will abort packing when number of
//!       lightmaps is greater than max_lm_num; this is useful for quick-drop for
//!       non-suitable samplesize_scale
//!       if packing was aborted, get_final_mapping() must not be used
int repack_facegroups(float samplesize_scale, float *out_lm_area_usage_ratio, int max_lm_num, IGenericProgressIndicator &pbar,
  SimpleProgressCB &progr_cb);

//! fills output data with final mapping (lmid and TC on lm) for given handle
//! (handle must be previously obtained with request_mapping())
void get_final_mapping(int handle, unsigned short *lmid, int lmid_stride_bytes, TFace *tface, Point2 *&tvert, int &numtv);


//! finishes mapping process (frees all used data)
void finish_mapping();


//! returns current number of lightmaps
int num_lightmaps();

//! returns size of lightmap
void get_lightmap_size(int i, int &w, int &h);
} // namespace AutoUnwrapMapping

#endif
