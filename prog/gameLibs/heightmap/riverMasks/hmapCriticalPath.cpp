// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmapCriticalPath.h"
#include <math/dag_bits.h>
#include <memory/dag_framemem.h>
#include <math.h>
#include <string.h>

static const int8_t dx8[] = {1, 1, 0, -1, -1, -1, 0, 1};
static const int8_t dy8[] = {0, 1, 1, 1, 0, -1, -1, -1};

// --- 2x2 block thinning ---

static int hmap_sdf_thin_2x2(dag::Span<uint32_t> bits, dag::ConstSpan<float> dist, int w)
{
  uint32_t *__restrict bdata = bits.data();
  const float *__restrict ddata = dist.data();
  int total_removed = 0;
  int grid_w = w - 1;

  dag::Vector<uint8_t> in_wl(grid_w * grid_w, 0);
  uint8_t *__restrict iwdata = in_wl.data();

  dag::Vector<int> worklist;
  worklist.reserve(4096);
  for (int y = 0; y < grid_w; y++)
    for (int x = 0; x < grid_w; x++)
    {
      int i00 = y * w + x;
      if (((bdata[i00 >> 5] >> (i00 & 31)) & 1) && ((bdata[(i00 + 1) >> 5] >> ((i00 + 1) & 31)) & 1) &&
          ((bdata[(i00 + w) >> 5] >> ((i00 + w) & 31)) & 1) && ((bdata[(i00 + w + 1) >> 5] >> ((i00 + w + 1) & 31)) & 1))
      {
        int blk = y * grid_w + x;
        worklist.push_back(blk);
        iwdata[blk] = 1;
      }
    }

  int ri = 0;
  while (ri < (int)worklist.size())
  {
    int bidx = worklist[ri++];
    iwdata[bidx] = 0;
    int by = bidx / grid_w;
    int bx = bidx - by * grid_w;
    int i00 = by * w + bx, i10 = i00 + 1, i01 = i00 + w, i11 = i00 + w + 1;

    if (!(((bdata[i00 >> 5] >> (i00 & 31)) & 1) && ((bdata[i10 >> 5] >> (i10 & 31)) & 1) && ((bdata[i01 >> 5] >> (i01 & 31)) & 1) &&
          ((bdata[i11 >> 5] >> (i11 & 31)) & 1)))
      continue;

    int widx = i00;
    float wval = ddata[i00];
    if (ddata[i10] < wval)
    {
      widx = i10;
      wval = ddata[i10];
    }
    if (ddata[i01] < wval)
    {
      widx = i01;
      wval = ddata[i01];
    }
    if (ddata[i11] < wval)
    {
      widx = i11;
    }
    bdata[widx >> 5] &= ~(1u << (widx & 31));
    total_removed++;

    int wy = widx / w, wx = widx - wy * w;
    for (int dy = -1; dy <= 0; dy++)
      for (int dx = -1; dx <= 0; dx++)
      {
        int ny = wy + dy, nx = wx + dx;
        if (nx >= 0 && nx < grid_w && ny >= 0 && ny < grid_w)
        {
          int nblk = ny * grid_w + nx;
          if (!iwdata[nblk])
          {
            iwdata[nblk] = 1;
            worklist.push_back(nblk);
          }
        }
      }
  }
  return total_removed;
}

// --- Smart branch pruning ---

static inline int hmap_sdf_neighbor_count(const uint32_t *ridge_bits, int w, int x, int y)
{
  int ncount = 0;
  for (int n = 0; n < 8; n++)
  {
    int nx = x + dx8[n], ny = y + dy8[n];
    if (nx >= 0 && nx < w && ny >= 0 && ny < w && ((ridge_bits[(ny * w + nx) >> 5] >> ((ny * w + nx) & 31)) & 1))
      ncount++;
  }
  return ncount;
}

static int hmap_sdf_prune_short_branches(dag::Span<uint32_t> ridge_bits, int w, int min_branch_len, int min_component)
{
  int total_removed = 0;

  dag::Vector<int, framemem_allocator> trace_buf;
  trace_buf.resize_noinit(min_branch_len);

  // Build initial worklist of endpoint pixels (neighbor count == 1)
  dag::Vector<int, framemem_allocator> worklist;
  worklist.reserve(4096);
  for (int y = 0; y < w; y++)
    for (int x = 0; x < w; x++)
    {
      int idx = y * w + x;
      if (((ridge_bits[idx >> 5] >> (idx & 31)) & 1) && hmap_sdf_neighbor_count(ridge_bits.data(), w, x, y) == 1)
        worklist.push_back(idx);
    }

  int ri = 0;
  while (ri < (int)worklist.size())
  {
    int idx = worklist[ri++];
    if (!((ridge_bits[idx >> 5] >> (idx & 31)) & 1))
      continue;
    int x = idx % w, y = idx / w;
    if (hmap_sdf_neighbor_count(ridge_bits.data(), w, x, y) != 1)
      continue;

    int trace_len = 0;
    int cx = x, cy = y, prev_x = -1, prev_y = -1;
    bool reached_junction = false;
    int jx = -1, jy = -1;

    while (trace_len < min_branch_len)
    {
      trace_buf[trace_len++] = cy * w + cx;

      int next_x = -1, next_y = -1, next_count = 0;
      for (int n = 0; n < 8; n++)
      {
        int nx = cx + dx8[n], ny = cy + dy8[n];
        if (nx == prev_x && ny == prev_y)
          continue;
        if (nx >= 0 && nx < w && ny >= 0 && ny < w && ((ridge_bits[(ny * w + nx) >> 5] >> ((ny * w + nx) & 31)) & 1))
        {
          next_x = nx;
          next_y = ny;
          next_count++;
        }
      }

      if (next_count == 0)
        break;
      if (next_count >= 2)
      {
        reached_junction = true;
        jx = cx;
        jy = cy;
        break;
      }

      prev_x = cx;
      prev_y = cy;
      cx = next_x;
      cy = next_y;
    }

    if (reached_junction && trace_len < min_branch_len)
    {
      for (int i = 0; i < trace_len; i++)
      {
        int tidx = trace_buf[i];
        ridge_bits[tidx >> 5] &= ~(1u << (tidx & 31));
      }
      total_removed += trace_len;

      // The junction pixel may now be an endpoint -- enqueue it
      int jidx = jy * w + jx;
      if (((ridge_bits[jidx >> 5] >> (jidx & 31)) & 1) && hmap_sdf_neighbor_count(ridge_bits.data(), w, jx, jy) == 1)
        worklist.push_back(jidx);
    }
  }

  // Remove small connected components using visited bitmask instead of per-pixel labels
  if (min_component > 0)
  {
    int bitmask_words = (w * w + 31) / 32;
    dag::Vector<uint32_t> visited(bitmask_words, 0);
    dag::Vector<int, framemem_allocator> bfs_stack;
    bfs_stack.reserve(4096);
    dag::Vector<int, framemem_allocator> comp_pixels;
    comp_pixels.reserve(4096);

    for (int y = 0; y < w; y++)
      for (int x = 0; x < w; x++)
      {
        int idx = y * w + x;
        if (!((ridge_bits[idx >> 5] >> (idx & 31)) & 1) || ((visited[idx >> 5] >> (idx & 31)) & 1))
          continue;

        // BFS to collect component pixels
        bfs_stack.clear();
        comp_pixels.clear();
        bfs_stack.push_back(idx);
        visited[idx >> 5] |= 1u << (idx & 31);

        while (!bfs_stack.empty())
        {
          int cidx = bfs_stack.back();
          bfs_stack.pop_back();
          comp_pixels.push_back(cidx);
          int cx2 = cidx % w, cy2 = cidx / w;
          for (int n = 0; n < 8; n++)
          {
            int nx = cx2 + dx8[n], ny = cy2 + dy8[n];
            if (nx >= 0 && nx < w && ny >= 0 && ny < w)
            {
              int nidx = ny * w + nx;
              if (((ridge_bits[nidx >> 5] >> (nidx & 31)) & 1) && !((visited[nidx >> 5] >> (nidx & 31)) & 1))
              {
                visited[nidx >> 5] |= 1u << (nidx & 31);
                bfs_stack.push_back(nidx);
              }
            }
          }
        }

        if ((int)comp_pixels.size() < min_component)
        {
          for (int cidx : comp_pixels)
          {
            ridge_bits[cidx >> 5] &= ~(1u << (cidx & 31));
            total_removed++;
          }
        }
      }
  }

  return total_removed;
}

// --- Line-of-sight to contour check ---

static bool hmap_sdf_has_los_to_contour(const float *__restrict ddata, const uint32_t *__restrict wmdata, int w, float contour_dist,
  int x, int y)
{
  static const float dir_x[] = {1.0f, 0.9239f, 0.7071f, 0.3827f, 0.0f, -0.3827f, -0.7071f, -0.9239f, -1.0f, -0.9239f, -0.7071f,
    -0.3827f, 0.0f, 0.3827f, 0.7071f, 0.9239f};
  static const float dir_y[] = {0.0f, 0.3827f, 0.7071f, 0.9239f, 1.0f, 0.9239f, 0.7071f, 0.3827f, 0.0f, -0.3827f, -0.7071f, -0.9239f,
    -1.0f, -0.9239f, -0.7071f, -0.3827f};

  static constexpr float LOS_RAY_STEP = 0.7f;
  static constexpr float LOS_RAY_RANGE_MUL = 2.0f;
  static constexpr int LOS_DIR_COUNT = 16;

  const float max_ray = contour_dist * LOS_RAY_RANGE_MUL;
  const int max_steps = (int)(max_ray / LOS_RAY_STEP);

  for (int d = 0; d < LOS_DIR_COUNT; d++)
  {
    float fx = (float)x + 0.5f, fy = (float)y + 0.5f;
    float ddx = dir_x[d], ddy = dir_y[d];
    for (int si = 1; si <= max_steps; si++)
    {
      float t = si * LOS_RAY_STEP;
      int px = (int)(fx + ddx * t);
      int py = (int)(fy + ddy * t);
      if ((unsigned)px >= (unsigned)w || (unsigned)py >= (unsigned)w)
        break;
      int pidx = py * w + px;
      if (!((wmdata[pidx >> 5] >> (pidx & 31)) & 1))
        break;
      if (ddata[pidx] >= contour_dist)
        return true;
    }
  }
  return false;
}

// --- Ridge classification helper ---
// Returns true if pixel at (x,y) with distance d and 8 neighbors is a gradient-aligned ridge.
static bool hmap_sdf_classify_ridge(float d, float n_l, float n_r, float n_t, float n_b, float n_tl, float n_br, float n_tr,
  float n_bl, float gx, float gy)
{
  static const float pair_dx[] = {1.0f, 0.0f, 0.707107f, 0.707107f};
  static const float pair_dy[] = {0.0f, 1.0f, 0.707107f, -0.707107f};

  float pa[4] = {n_l, n_t, n_tl, n_tr};
  float pb[4] = {n_r, n_b, n_br, n_bl};
  bool is_ridge = false;
  for (int p = 0; p < 4; p++)
  {
    if (d >= pa[p] && d >= pb[p] && (d > pa[p] || d > pb[p]))
    {
      is_ridge = true;
      break;
    }
  }
  if (!is_ridge)
    return false;

  float glen_sq = gx * gx + gy * gy;
  if (glen_sq < 0.25f)
    return true;

  float glen = sqrtf(glen_sq);
  float ngx = gx / glen, ngy = gy / glen;
  float best_align = 0.0f;
  for (int p = 0; p < 4; p++)
  {
    if (d >= pa[p] && d >= pb[p] && (d > pa[p] || d > pb[p]))
    {
      float dot = ngx * pair_dx[p] + ngy * pair_dy[p];
      if (dot < 0)
        dot = -dot;
      if (dot > best_align)
        best_align = dot;
    }
  }
  return best_align >= 0.5f;
}

// --- Critical path extraction ---

int hmap_sdf_extract_critical_path(dag::ConstSpan<float> dist, dag::ConstSpan<uint32_t> water_mask, int w, float contour_dist,
  int min_branch_len, int min_comp_size, dag::Vector<uint32_t> &out_path)
{

  int bitmask_words = (w * w + 31) / 32;
  out_path.assign(bitmask_words, 0);

  dag::Vector<uint32_t> ridge_bits(bitmask_words, 0);

  // Step 1: extract raw ridge pixels + fused contour boundary detection.
  // Contour boundary pixels (d >= contour_dist with a neighbor < contour_dist)
  // are written directly to out_path, avoiding a separate combine pass.
  {
    const float *__restrict ddata = dist.data();
    const uint32_t *__restrict wmdata = water_mask.data();
    uint32_t *__restrict rbdata = ridge_bits.data();
    uint32_t *__restrict opdata = out_path.data();

    // Interior loop: no bounds checking on neighbor reads
    for (int y = 1; y < w - 1; y++)
    {
      int row = y * w;
      for (int x = 1; x < w - 1; x++)
      {
        int idx = row + x;
        if (!((wmdata[idx >> 5] >> (idx & 31)) & 1))
          continue;
        float d = ddata[idx];

        if (d >= contour_dist)
        {
          // Contour boundary: water pixel at contour_dist with a neighbor below contour_dist
          float n_l = ddata[idx - 1], n_r = ddata[idx + 1];
          float n_t = ddata[idx - w], n_b = ddata[idx + w];
          if (n_l < contour_dist || n_r < contour_dist || n_t < contour_dist || n_b < contour_dist ||
              ddata[idx - w - 1] < contour_dist || ddata[idx + w + 1] < contour_dist || ddata[idx - w + 1] < contour_dist ||
              ddata[idx + w - 1] < contour_dist)
            opdata[idx >> 5] |= 1u << (idx & 31);
          continue;
        }

        if (d <= 1.0f)
          continue;

        float n_l = ddata[idx - 1], n_r = ddata[idx + 1];
        float n_t = ddata[idx - w], n_b = ddata[idx + w];
        float n_tl = ddata[idx - w - 1], n_br = ddata[idx + w + 1];
        float n_tr = ddata[idx - w + 1], n_bl = ddata[idx + w - 1];

        if (hmap_sdf_classify_ridge(d, n_l, n_r, n_t, n_b, n_tl, n_br, n_tr, n_bl, n_r - n_l, n_b - n_t))
          rbdata[idx >> 5] |= 1u << (idx & 31);
      }
    }

    // Border pixels: four explicit edge loops with mirror addressing
    auto mirror = [w](int v) { return v < 0 ? -v : (v >= w ? 2 * (w - 1) - v : v); };
    auto border_pixel = [&](int x, int y) {
      int idx = y * w + x;
      if (!((wmdata[idx >> 5] >> (idx & 31)) & 1))
        return;
      float d = ddata[idx];

      if (d >= contour_dist)
      {
        for (int n = 0; n < 8; n++)
        {
          int nx = mirror(x + dx8[n]), ny = mirror(y + dy8[n]);
          if (ddata[ny * w + nx] < contour_dist)
          {
            opdata[idx >> 5] |= 1u << (idx & 31);
            break;
          }
        }
        return;
      }

      if (d <= 1.0f)
        return;

      int mx_l = mirror(x - 1), mx_r = mirror(x + 1);
      int my_t = mirror(y - 1), my_b = mirror(y + 1);
      float n_l = ddata[y * w + mx_l], n_r = ddata[y * w + mx_r];
      float n_t = ddata[my_t * w + x], n_b = ddata[my_b * w + x];
      float n_tl = ddata[my_t * w + mx_l], n_br = ddata[my_b * w + mx_r];
      float n_tr = ddata[my_t * w + mx_r], n_bl = ddata[my_b * w + mx_l];

      if (hmap_sdf_classify_ridge(d, n_l, n_r, n_t, n_b, n_tl, n_br, n_tr, n_bl, n_r - n_l, n_b - n_t))
        rbdata[idx >> 5] |= 1u << (idx & 31);
    };
    for (int x = 0; x < w; x++)
      border_pixel(x, 0); // top row
    for (int x = 0; x < w; x++)
      border_pixel(x, w - 1); // bottom row
    for (int y = 1; y < w - 1; y++)
      border_pixel(0, y); // left column (excluding corners)
    for (int y = 1; y < w - 1; y++)
      border_pixel(w - 1, y); // right column (excluding corners)
  }

  // Step 2: thin 2x2 blocks to get 1-px wide lines
  hmap_sdf_thin_2x2(make_span(ridge_bits), dist, w);

  // Step 3: smart branch pruning + small component removal
  if (min_branch_len > 0)
    hmap_sdf_prune_short_branches(make_span(ridge_bits), w, min_branch_len, min_comp_size);

  // Step 4: filter ridge pixels by LOS to contour.
  {
    const float *__restrict ddata = dist.data();
    const uint32_t *__restrict wmdata = water_mask.data();
    uint32_t *__restrict rbdata = ridge_bits.data();
    for (int wi = 0; wi < bitmask_words; wi++)
    {
      uint32_t word = rbdata[wi];
      if (!word)
        continue;
      while (word)
      {
        int bit = __bsf(word);
        word &= word - 1;
        int idx = wi * 32 + bit;
        if (idx >= w * w)
          break;
        int x = idx % w, y = idx / w;
        if (hmap_sdf_has_los_to_contour(ddata, wmdata, w, contour_dist, x, y))
          rbdata[wi] &= ~(1u << bit);
      }
    }
  }

  // Combine: OR filtered ridges into out_path (contour already written in Step 1)
  int count = 0;
  {
    const uint32_t *__restrict rbdata = ridge_bits.data();
    uint32_t *__restrict opdata = out_path.data();
    for (int wi = 0; wi < bitmask_words; wi++)
    {
      opdata[wi] |= rbdata[wi];
      count += __popcount(opdata[wi]);
    }
  }

  return count;
}
