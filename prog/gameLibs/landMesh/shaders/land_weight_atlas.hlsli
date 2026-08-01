#ifndef LAND_WEIGHT_ATLAS_INCLUDED
#define LAND_WEIGHT_ATLAS_INCLUDED 1

// Layout of the land detail weight atlas, shared by the packer
// (landMesh/lmeshWeightAtlas.cpp) and the sampling (land_weight_inc.dshl).
//
// A cell blends up to LAND_WEIGHT_DET_NUM landclasses, and its channels are
// dense: channel c is slot c%3 of the cell's page c/3. So a cell needs no
// per-channel table at all, just its two page numbers and how many landclasses
// it blends - one dword, which is all reading the weights costs.
//
// The last blended channel is never read: weights sum to 1, so it is whatever
// the ones before leave, exactly even where the page is quantized. That is why
// a cell blending one landclass stores no page, and one blending four needs a
// single page. It is still written wherever the pages already have room for
// it, so that reading one weight instead of all of them stays a single sample.
//
// The landclass ids follow the pages: indices into the level's landclass list,
// one byte each, so a shader that binds those textures bindlessly needs the
// cell number and nothing else.
#define LAND_WEIGHT_DET_NUM                7 // landclasses blended per cell
#define LAND_WEIGHT_CHANNELS_PER_PAGE      3
#define LAND_WEIGHT_PAGES_PER_CELL         2
#define LAND_WEIGHT_BORDER                 2 // texels of neighbour weights on each page side

#define LAND_WEIGHT_CELL_STRIDE            12 // bytes: the pages dword, then DET_NUM landclass ids
#define LAND_WEIGHT_PAGE_MASK              0x0FFFU
#define LAND_WEIGHT_PAGE1_SHIFT            12
#define LAND_WEIGHT_COUNT_SHIFT            24
#define LAND_WEIGHT_COUNT_MASK             7U // blended landclasses less one: there is always one

#endif
