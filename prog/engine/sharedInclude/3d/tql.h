// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_resId.h>
#include <3d/ddsxTex.h>
#include <generic/dag_tab.h>
#include <math/dag_mathBase.h>
#include <startup/dag_globalSettings.h>

class BaseTexture;
class RegExp;
enum TexQL : unsigned;

namespace tql
{
static constexpr int IMPLICIT_STUBS = 4;

extern Tab<BaseTexture *> texStub;

extern bool streaming_enabled;
extern int mem_quota_kb;
extern int mem_quota_limit_kb;
extern bool lq_not_more_than_split_bq;
extern bool lq_not_more_than_split_bq_difftex;

extern void (*on_tex_created)(BaseTexture *t);
extern void (*on_tex_released)(BaseTexture *t);
extern void (*on_buf_changed)(bool add, int delta_sz_kb);
extern void (*unload_on_drv_shutdown)();
extern void (*on_frame_finished)();
extern unsigned (*get_tex_lfu)(TEXTUREID texId);
extern TexQL (*get_tex_cur_ql)(TEXTUREID texId);
extern bool (*check_texture_id_valid)(TEXTUREID texId);
extern const char *(*get_tex_info)(TEXTUREID texId, bool verbose, String &tmp_stor);
extern void (*dump_tex_state)(TEXTUREID idx);

extern void get_tex_streaming_stats(int &_mem_used_discardable_kb, int &_mem_used_persistent_kb, int &_mem_used_discardable_kb_max,
  int &_mem_used_persistent_kb_max, int &_mem_used_sum_kb_max, int &_tex_used_discardable_cnt, int &_tex_used_persistent_cnt,
  int &_max_mem_used_overdraft_kb);
} // namespace tql

namespace tql
{
extern void initTexStubs();
extern void termTexStubs();
/// @brief Returns either a new texture with same properties & contents as the provided one
/// but new width/height/depth/mips, or the original texture. If some data becomes redundant after
/// a resize (mips), it is discarded. If some data (mips) is missing, it is left uninitialized.
/// @warning May return the original texture if the resize operation is a no-op, but if it returns a
/// new texture, the caller is responsible for freeing it.
/// @param t Texture to resize.
/// @param w New width.
/// @param h New height.
/// @param d New depth.
/// @param l New mip count.
/// @param tex_ld_lev New level of detail.
[[nodiscard]] extern BaseTexture *makeResizedTmpTexResCopy(BaseTexture *t, unsigned w, unsigned h, unsigned d, unsigned l,
  unsigned tex_ld_lev);

inline bool updateSkipLev(const ddsx::Header &hdr, int &levels, int &skip_levels, int w, int h, int d)
{
  int lev = hdr.levels;
  if (hdr.flags & hdr.FLG_VOLTEX)
  {
    while ((hdr.w >> skip_levels) > w || (hdr.h >> skip_levels) > h || (hdr.depth >> skip_levels) > d)
      skip_levels++, lev--;
  }
  else
  {
    while ((hdr.w >> skip_levels) > w || (hdr.h >> skip_levels) > h)
      skip_levels++, lev--;
  }
  if (levels > lev)
    levels = lev;
  return levels > 0;
}

inline int getEffStubIdx(int stub_idx, unsigned tex_type)
{
  if (stub_idx > 0 && stub_idx + IMPLICIT_STUBS - 1 < texStub.size())
    return stub_idx + IMPLICIT_STUBS - 1;
  return stub_idx >= 0 && tex_type < IMPLICIT_STUBS && tex_type < texStub.size() ? tex_type : -1;
}

inline int sizeInKb(int sz) { return 4 * ((sz + 4095) / 4096); }

inline bool isTexStub(BaseTexture *t) { return find_value_idx(texStub, t) >= 0; }
} // namespace tql

#define DECLARE_TQL_TID_AND_STUB()                                                            \
  uint32_t tidXored = D3DRESID::INVALID_ID ^ 0xBAADF00Du;                                     \
  int32_t stubTexIdx = -1;                                                                    \
  void setTID(TEXTUREID tid) override final                                                   \
  {                                                                                           \
    tidXored = uint32_t(tid) ^ 0xBAADF00Du;                                                   \
  }                                                                                           \
  TEXTUREID getTID() const override final                                                     \
  {                                                                                           \
    return TEXTUREID(tidXored ^ 0xBAADF00Du);                                                 \
  }                                                                                           \
  bool isStub() const                                                                         \
  {                                                                                           \
    return stubTexIdx >= 0 && isTexResEqual(tql::texStub[stubTexIdx]);                        \
  }                                                                                           \
  inline auto getStubTex()                                                                    \
  {                                                                                           \
    return stubTexIdx >= 0 ? static_cast<decltype(this)>(tql::texStub[stubTexIdx]) : nullptr; \
  }

#define TEXQL_ON_ALLOC(BT)   tql::on_tex_created(BT)
#define TEXQL_ON_RELEASE(BT) tql::on_tex_released(BT)

#define TEXQL_ON_BUF_ALLOC(B)                               \
  do                                                        \
  {                                                         \
    tql::on_buf_changed(true, tql::sizeInKb(B->ressize())); \
  } while (0)
#define TEXQL_ON_BUF_RELEASE(B) tql::on_buf_changed(false, -tql::sizeInKb(B->ressize()))

#define TEXQL_ON_BUF_ALLOC_SZ(sz)                 \
  do                                              \
  {                                               \
    tql::on_buf_changed(true, tql::sizeInKb(sz)); \
  } while (0)
#define TEXQL_ON_BUF_RELEASE_SZ(sz) tql::on_buf_changed(false, -tql::sizeInKb(sz))

#define TEXQL_SHUTDOWN_TEX() tql::unload_on_drv_shutdown()
