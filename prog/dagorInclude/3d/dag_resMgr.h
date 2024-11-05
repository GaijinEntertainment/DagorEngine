//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_d3dResource.h>
#include <drv/3d/dag_resId.h>
#include <osApiWrappers/dag_atomic.h>
#include <util/dag_stdint.h>
#include <startup/dag_globalSettings.h>

//! \file dag_resMgr.h
//! \brief Automatic resource manager for D3D resources.
//! \details
//! Glossary:
//!   - level - feature of texture size, computed as `log2(max(tex.dimensions)) = log2(max(tex.w, tex.h, tex.d))`:
//!     -# level=1 means smallest possing size (2x2x2) and is commonly used to denote smallest quality;
//!     -# level=15 means maximum possible size (up to 32767x32768, but current GPUs don't handle such res);
//!     -# level=2..15 means some non-stub quality.
//!     Variables for holding level usually have suffix Lev or _lev
//!
//!   - QL - quality level (or TQL - texture quality level), do not confuse it with 'level'.
//!     Some predefined quality preset of texture (stub, thumbnail, base, high, ultrahigh, etc.)
//!     Enumerated later with TexQL enum (TQL_*).
//!     Variables for holding quality level usually have suffix QL or _ql.
//!
//!   - LFU - last frame used, ordinary rendered frame counter
//!     (generally from game start) where texture was referenced last time
//!
//!   - levDesc - levels descriptor that maps QL to real levels
//!     (e.g. TQL_thumb -> level=6, TQL_base -> level=9)


//! \brief Quality levels for textures (QL)
enum TexQL : unsigned
{
  TQL_stub = 0xFu, //!< stub (shared 1x1 placeholder)
  TQL_thumb = 0u,  //!< thumbnail quality (upto 64x64, total mem size <= 4K)
  TQL_base,        //!< base quality (low split)
  TQL_high,        //!< full quality (HQ)
  TQL_uhq,         //!< ultra high quality

  TQL__COUNT, //!< enum item count
  TQL__FIRST = 0,
  TQL__LAST = TQL__COUNT - 1
};

//! \cond DETAIL
struct D3dResManagerData
{
  //! D3DRESID and its index validation
  static __forceinline bool isValidIndex(unsigned idx)
  {
    if (idx < unsigned(interlocked_relaxed_load(indexCount)))
      return true;
    return idx < unsigned(interlocked_acquire_load(indexCount));
  }
  static __forceinline bool isValidIDFast(D3DRESID id) { return id != BAD_D3DRESID && isValidIndex(id.index()); }

  //! validates D3DRESID and reports stale generation to logerr (by default)
  static __forceinline bool isValidID(D3DRESID id, void (*report_bad_gen)(D3DRESID) = &report_bad_generation_used)
  {
    if (isValidIDFast(id))
    {
      if (getCurGeneration(id.index()) == id.generation())
        return true;
      if (report_bad_gen)
        report_bad_gen(id);
      return false;
    }
    return false;
  }

  //! returns reference count for D3DRESID resource; negative value means missing (unregistered) resource
  static __forceinline int getRefCount(D3DRESID id)
  {
    unsigned idx = id.index();
    if (isValidIDFast(id) && getCurGeneration(idx) == id.generation())
      return interlocked_acquire_load(refCount[idx]);
    return INVALID_REFCOUNT;
  }

  //! returns registered D3dResource for D3DRESID resource; nullptr value means missing, unregistered or unreferenced resource
  static __forceinline D3dResource *getD3dRes(D3DRESID id)
  {
    return getRefCount(id) > 0 ? interlocked_acquire_load_ptr(asVolatile(d3dRes[id.index()])) : nullptr;
  }
  //! convenience wrapper to get BaseTexture for D3DRESID resource (just reinterpret cast, to type checking!)
  static __forceinline BaseTexture *getBaseTex(D3DRESID id) { return (BaseTexture *)getD3dRes(id); }
  //! returns registered typed texture for D3DRESID resource;
  //! nullptr value means missing, unregistered, unreferenced or non-tex resource
  template <int TYPE>
  static __forceinline BaseTexture *getD3dTex(D3DRESID id)
  {
    D3dResource *r = getD3dRes(id);
    return (r && r->restype() == TYPE) ? (BaseTexture *)r : nullptr;
  }

  //! returns last-frame-used (LFU) for D3DRESID resource; 0 value means missing (unregistered) resource
  static __forceinline unsigned getResLFU(D3DRESID id)
  {
    return isValidID(id, nullptr) ? (unsigned)interlocked_acquire_load(lastFrameUsed[id.index()]) : 0u;
  }

  //! returns current quality level (TQL_*) for D3DRESID resource
  static __forceinline TexQL getResCurQL(D3DRESID id) { return isValidID(id, nullptr) ? resQS[id.index()].getCurQL() : TQL_stub; }
  //! returns maximum available quality level (TQL_*) for D3DRESID resource
  static __forceinline TexQL getResMaxQL(D3DRESID id) { return isValidID(id, nullptr) ? resQS[id.index()].getMaxQL() : TQL_stub; }

  //! returns current loaded-level for D3DRESID resource (level is log2(tex.dimension), do not mix it with QL!)
  static __forceinline uint8_t getResLoadedLev(D3DRESID id) { return isValidID(id, nullptr) ? resQS[id.index()].getLdLev() : 0; }
  //! returns maximum requested-level for D3DRESID resource (level is log2(tex.dimension), do not mix it with QL!)
  static __forceinline uint8_t getResMaxReqLev(D3DRESID id) { return isValidID(id, nullptr) ? resQS[id.index()].getMaxReqLev() : 0; }

  //! returns generation of D3DRESID
  //! (generation is used to distinct 2 D3DRESIDs that share the same index; only one generation is valid for given index)
  static __forceinline unsigned getCurGeneration(int idx) { return interlocked_acquire_load(generation[idx]); }

  //! marks D3DRESID as used in current frame and records requested-level
  //! when requested-level is greater than loaded-level texture streaming may schedule loading of higher resolution texture
  static __forceinline void markResLFU(D3DRESID id, unsigned req_lev = 15)
  {
    if (isValidID(id, nullptr) /*getRefCount(id) > 0*/)
    {
      unsigned idx = id.index();
      unsigned f = dagor_frame_no();
      if (interlocked_relaxed_load(lastFrameUsed[idx]) < f) // relaxed load here is enough for monotonically increased frame counter
      {
        interlocked_release_store(lastFrameUsed[idx], f);
        updateResReqLev(id, idx, req_lev);
      }
      else if (resQS[idx].getMaxReqLev() < req_lev)
        updateResReqLev(id, idx, req_lev);
    }
  }

  //! return level for specific QL of D3DRESID;  0 means that QL doesn't define level
  static uint8_t getLevDesc(int idx, TexQL ql) { return (levDesc[idx] >> (ql * 4)) & 0xF; }

protected:
  static constexpr int INVALID_REFCOUNT = -1073741824; // 0xC0000000 to be negative and sufficiently far from 0 to both sides

  struct ReqQState
  {
    uint8_t maxReqLev;    //< max requested-level
    uint8_t ldLev__rdLev; //< loaded-level;  level to be loaded (!=0 -> reading in progress)
    uint8_t qLev__maxLev; //< max level for tex quality;  current max level
    uint8_t curQL__maxQL; //< current QL;  max QL

  public:
    uint8_t getMaxReqLev() const { return interlocked_relaxed_load(maxReqLev); } // relaxed is enough
    void setMaxReqLev(uint8_t l) { interlocked_release_store(maxReqLev, l); }

    uint8_t getLdLev() const { return interlocked_acquire_load(ldLev__rdLev) >> 4; }
    uint8_t getRdLev() const { return interlocked_acquire_load(ldLev__rdLev) & 0xF; }
    bool isReading() const { return getRdLev() != 0; }
    void setRdLev(uint8_t l) { return interlocked_release_store(ldLev__rdLev, (getLdLev() << 4) | (l & 0xF)); }
    void setLdLev(uint8_t l) { return interlocked_release_store(ldLev__rdLev, (l << 4) | 0); }

    uint8_t getQLev() const { return interlocked_relaxed_load(qLev__maxLev) >> 4; }
    uint8_t getMaxLev() const { return interlocked_acquire_load(qLev__maxLev) & 0xF; }
    uint8_t clampLev(uint8_t l) const
    {
      uint8_t qlev = getQLev();
      return l < qlev ? l : qlev;
    }
    void setQLev(uint8_t l)
    {
      l &= 0xF;
      interlocked_release_store(qLev__maxLev, l | (l << 4));
    }
    void setMaxLev(uint8_t l)
    {
      uint8_t ql = getQLev();
      interlocked_release_store(qLev__maxLev, (ql << 4) | (l < ql ? l : ql));
    }

    TexQL getCurQL() const { return TexQL(interlocked_acquire_load(curQL__maxQL) >> 4); }
    TexQL getMaxQL() const { return TexQL(interlocked_relaxed_load(curQL__maxQL) & 0xF); }
    bool isEqualCurAndMaxQL() const
    {
      uint8_t cm = interlocked_relaxed_load(curQL__maxQL);
      return (cm >> 4) == (cm & 0xF);
    }
    bool isCurQLGreater(TexQL ql) const
    {
      TexQL cur = getCurQL();
      return cur != TQL_stub && cur > ql;
    }
    void setMaxQL(TexQL max_ql, uint8_t cur_ql) { interlocked_release_store(curQL__maxQL, ((cur_ql & 0xF) << 4) | (max_ql & 0xF)); }
    void setCurQL(TexQL ql) { setMaxQL(getMaxQL(), ql); }
  };

  static int indexCount, maxTotalIndexCount;
  static uint16_t *__restrict generation;
  static int *__restrict refCount;
  static D3dResource *__restrict *__restrict d3dRes;
  static int32_t *__restrict lastFrameUsed;
  static ReqQState *__restrict resQS;
  static uint16_t *__restrict levDesc;

  template <typename T>
  static inline T *volatile &asVolatile(T *__restrict &p)
  {
    return (T *volatile &)p;
  }
  static void report_bad_generation_used(D3DRESID id);
  static void require_tex_load(D3DRESID id);
  static __forceinline void updateResReqLev(D3DRESID id, unsigned idx, unsigned reqLev)
  {
    resQS[idx].setMaxReqLev(reqLev); // eventually consistent
    if (resQS[idx].getRdLev() || resQS[idx].isEqualCurAndMaxQL())
      return;
    reqLev = resQS[idx].clampLev(reqLev);
    if (resQS[idx].getLdLev() < reqLev)
      require_tex_load(id);
  }
};
//! \endcond

//! \brief Registers external D3D resource with the specified \p name as managed and returns the D3DRESID for it.
//! \details Specified resource becomes owned by the manager, no factory is assigned to it.
//! RefCount is set to 1 and when it eventually reaches 0, the resource is automatically destroyed and the name is evicted.
//! Use release_managed_res() or release_managed_res_verified() to del ref and finally destroy.
//! \warning Resource name is case-insensitive and must be unique.
//! \param name the name to register the resource under
//! \param res the resource to register
//! \returns the D3DRESID for the registered resource
D3DRESID register_managed_res(const char *name, D3dResource *res);

//! \brief Checks whether creation/release factory is set for D3DRESID
//! \details E.g. for an ID that was acquired from register_managed_res(), it will return false.
//! \param id the D3DRESID to check
//! \returns true if a factory was specified for the D3DRESID
bool is_managed_res_factory_set(D3DRESID id);

//! \brief Acquires resource object and increments resource reference count.
//! \details If resource was not referenced yet, it is created. If it is unavailable (bad ID), nullptr is returned.
//! Logically paired with release_managed_res().
//! \note When factory fails to create texture, *missing* texture handling may be applied.
//! \param id the managed ID to acquire
//! \returns the acquired resource object or nullptr if the ID is invalid
D3dResource *acquire_managed_res(D3DRESID id);

//! \brief Releases resource object and decrements resource reference count.
//! \details When reference count reaches 0, resource may be released.
//! Logically paired with acquire_managed_res()
//! \param id the managed ID to release
void release_managed_res(D3DRESID id);

//! \brief Releases resource object and decrements resource reference count,
//! but additionally checks that the managed resource is the same as \p check_res.
//! \details When reference count reaches 0, resource may be released.
//! Logically paired with acquire_managed_res()
//! On resource destruction (when name is unregistered) id is set to BAD_D3DRESID
//! \param id the managed ID to release
//! \param check_res the resource to check against
void release_managed_res_verified(D3DRESID &id, D3dResource *check_res);

//! \copydoc acquire_managed_res()
BaseTexture *acquire_managed_tex(D3DRESID id);
//! \copydoc release_managed_res()
static inline void release_managed_tex(D3DRESID id) { release_managed_res(id); }
template <class T>
//! \copydoc release_managed_res_verified()
static inline void release_managed_tex_verified(D3DRESID &id, T &tex)
{
  release_managed_res_verified(id, static_cast<D3dResource *>(tex));
  tex = nullptr;
}

//! \copydoc acquire_managed_res()
Sbuffer *acquire_managed_buf(D3DRESID id);
//! \copydoc release_managed_res()
static inline void release_managed_buf(D3DRESID id) { release_managed_res(id); }
//! \copydoc release_managed_res_verified()
template <class T>
static inline void release_managed_buf_verified(D3DRESID &id, T &buf)
{
  release_managed_res_verified(id, static_cast<D3dResource *>(buf));
  buf = nullptr;
}

//! \brief Gets the name for a managed resource by ID, or nullptr if ID is invalid.
//! \param id the D3DRESID to get the name for
//! \returns the name of the managed resource with ID \p id or nullptr
const char *get_managed_res_name(D3DRESID id);

//! \brief Gets managed resource ID by its name, or BAD_D3DRESID if name is invalid.
//! \param res_name the name of the resource to get the ID for
//! \returns the managed resource ID that corresponds to \p res_name or BAD_D3DRESID if the name is invalid
D3DRESID get_managed_res_id(const char *res_name);


//! \brief Gets the reference count for a managed resource by ID
//! \param id the ID to get the reference count for
//! \returns the reference count for the D3DRESID resource or a
//!   negative value if the resource is missing
inline int get_managed_res_refcount(D3DRESID id) { return D3dResManagerData::getRefCount(id); }

//! \brief Gets the last-frame-used (LFU) counter for a managed ID
//! \param id the ID to get the LFU counter for
//! \returns the last-frame-used counter for the D3DRESID resource or 0 if the resource is missing/unregistered
inline unsigned get_managed_res_lfu(D3DRESID id) { return D3dResManagerData::getResLFU(id); }

//! \brief Gets the current quality level (TQL_*) for a managed ID
//! \param id the ID to get the quality level for
//! \returns the current quality level for the D3DRESID resource or TQL_stub if the resource is missing/unregistered
inline TexQL get_managed_res_cur_tql(D3DRESID id) { return D3dResManagerData::getResCurQL(id); }

//! \brief Gets the maximum available quality level (TQL_*) for a managed ID
//! \param id the D3DRESID to get the quality level for
//! \returns the current quality level for the D3DRESID resource or TQL_stub if the resource is missing/unregistered
inline TexQL get_managed_res_max_tql(D3DRESID id) { return D3dResManagerData::getResMaxQL(id); }

//! \brief Gets the maximum requested level for a managed ID, i.e. log2(tex.dimension). Do not confuse it with QL!
//! \param id the D3DRESID to get the loaded level for
//! \returns the current loaded level for \p id or 0 if the resource is missing/unregistered
inline unsigned get_managed_res_maxreq_lev(D3DRESID id) { return D3dResManagerData::getResMaxReqLev(id); }

//! \brief Gets the current loaded level for a managed ID, i.e. log2(tex.dimension). Do not confuse it with QL!
//! \param id the D3DRESID to get the loaded level for
//! \returns the current loaded level for \p id or 0 if the resource is missing/unregistered
inline unsigned get_managed_res_loaded_lev(D3DRESID id) { return D3dResManagerData::getResLoadedLev(id); }

//! \brief Enables or disables multithreading support for resource manager and preallocates internal structures
//! to handle upto \p max_res_entry_count resource entries without reallocations.
//! \details enable_res_mgr_mt(false, -1) will disable multithreading support and shrink entries array to used size
//! \param enable whether to enable multithreading support
//! \param max_res_entry_count the maximum number of resource entries to preallocate or -1 to shrink to fit
void enable_res_mgr_mt(bool enable, int max_res_entry_count);


//! \brief Utility for iterating all managed resources.
//! \details Managed resources form a linked list, so for an arbitrary ID, this function returns the "next" ID
//! in the list. Hence one is supposed to call this function in a loop until it returns BAD_D3DRESID.
//! \param after_rid ID of the texture to get the next texture for. Use BAD_D3DRESID to start iteration.
//! \param min_ref_count Minimum reference count filter
//! \returns Some ID that follows \p after_rid and has refCount >= \p min_ref_count OR BAD_RESID if the iteration is done.
D3DRESID iterate_all_managed_d3dres(D3DRESID after_rid, int min_ref_count);

//! \name Helper wrappers
//! \brief See iterate_all_managed_d3dres(). Use as follows:
//! \code for (D3DRESID id = first_managed_d3dres(); id != BAD_D3DRESID; id = next_managed_d3dres(id)) \endcode
//!@{
//! \brief Get the first managed resource ID for iteration
//! \param min_rc Minimum reference count filter
//! \returns First managed resource ID with refCount >= \p min_rc
inline D3DRESID first_managed_d3dres(int min_rc = 0) { return iterate_all_managed_d3dres(BAD_D3DRESID, min_rc); }

//! \brief Get the next managed resource ID for iteration
//! \param prev_id ID of the previous resource
//! \param min_rc Minimum reference count filter
//! \returns Next managed resource ID after \p prev_id with refCount >= \p min_rc
inline D3DRESID next_managed_d3dres(D3DRESID prev_id, int min_rc = 0) { return iterate_all_managed_d3dres(prev_id, min_rc); }
//!@}
