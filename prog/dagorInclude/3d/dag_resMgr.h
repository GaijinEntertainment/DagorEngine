//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//

/**
 * @file 
 * @brief Provides classes and functions for resource management.
 * 
 * ### Glossary: ###
 * \b level is a feature of a texture, computed as <c> log2(max(tex.w, tex.h, tex.d)) </c>
 * - level=1 means smallest possible size (2x2x2) and is commonly used to denote the lowest quality
 * - level=15 means the maximal possible size (up to 32767x32768, but current GPUs don't handle such res)
 * - level=2..15 means some non-stub quality
 * .
 * Level variables usually have suffices \c Lev or \c _lev. \n
 *
 * \b QL stands for 'quality level' or, alternatively TQL - for 'texture quality level' (should not be 
 * confused with 'level'). QL is a predefined quality preset of texture (stub, thumbnail, base, 
 * high, ultrahigh, etc.). Quality levels are defined by \ref TexQL enumeration.\
 * Variables for holding quality level usually have suffix QL or _ql.\n
 *
 * \b LFU stands for 'last frame used' which is just a count of the frame (usually from the game start) 
 * where texture was referenced last time.\n
 *
 * \b levDesc is a level descriptor that maps QL to level (e.g. TQL_thumb -> level=6, TQL_base -> level=9).\n
 */
#pragma once

#include <3d/dag_d3dResource.h>
#include <3d/dag_resId.h>
#include <osApiWrappers/dag_atomic.h>
#include <util/dag_stdint.h>
#include <startup/dag_globalSettings.h>

/**
 * @brief Presets for texture quality levels.
 */
enum TexQL : unsigned
{
  TQL_stub = 0xFu,  /*<< Stub(shared 1x1 placeholder). */
  TQL_thumb = 0u,   /*<< Thumbnail quality(upto 64x64, total mem size <= 4K). */ 
  TQL_base,         /*<< Base quality (low split). */
  TQL_high,         /*<< Full quality (HQ). */
  TQL_uhq,          /*<< Ultra high quality. */

  TQL__COUNT,
  TQL__FIRST = 0,
  TQL__LAST = TQL__COUNT - 1
};

/**
 * @brief Base class that provides static members for managing resources within a resource manager.
 */
struct D3dResManagerData
{
  /**
   * @brief Checks whether the resource index is valid.
   * 
   * @param [in] idx    Resource index.
   * @return            \c true if the index is valid, \c false otherwise.
   */
  static __forceinline bool isValidIndex(unsigned idx)
  {
    if (idx < unsigned(interlocked_relaxed_load(indexCount)))
      return true;
    return idx < unsigned(interlocked_acquire_load(indexCount));
  }

  /**
   * @brief Checks whether the resource handle is valid.
   * 
   * @param [in] id Resource handle.
   * @return        \c true if the handle is valid, \c false otherwise.
   * 
   * This is a fast implementation, which does not verify current generation. Alternatively, use \ref isValidID.
   */
  static __forceinline bool isValidIDFast(D3DRESID id) { return id != BAD_D3DRESID && isValidIndex(id.index()); }

  /**
   * @brief Checks whether the resource handle is valid.
   * 
   * @param [in] id             Resource handle.
   * @param [in] report_bad_gen Callback to report generation invalidity.
   * @return                    \c true if the handle is valid, \c false otherwise.
   * 
   * This implementation also checks generation validity which is potentially slow.
   * Alternatively, use \ref isValidIDFast.
   */
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

  /**
   * @brief Retrieves reference count of the resource.
   * 
   * @param [in] id     A handle to the resource.
   * @return            Valid reference count of the resource or a negative value if the resource is unregistered.
   */
  static __forceinline int getRefCount(D3DRESID id)
  {
    unsigned idx = id.index();
    if (isValidIDFast(id) && getCurGeneration(idx) == id.generation())
      return interlocked_acquire_load(refCount[idx]);
    return INVALID_REFCOUNT;
  }
  
  /**
   * @brief Retrieves a pointer to the resource by its handle.
   * 
   * @param [in] id A handle to the resource.
   * @return        A pointer to the handled resource or NULL if the handle is invalid or the resource in unregistered or unreferenced.
   */
  static __forceinline D3dResource *getD3dRes(D3DRESID id)
  {
    return getRefCount(id) > 0 ? interlocked_acquire_load_ptr(asVolatile(d3dRes[id.index()])) : nullptr;
  }

  /**
   * @brief Retrieves a pointer to the handled texture resource.
   * 
   * @param [in] id A handle to the resource.
   * @return        A pointer to the handled texture or NULL on error.
   * 
   * This function is a wrapper over \ref getD3dRes which casts its result to \ref BaseTexture. 
   * NULL is returned if \ref getD3dRes fails or if the casting fails.
   */
  static __forceinline BaseTexture *getBaseTex(D3DRESID id) { return (BaseTexture *)getD3dRes(id); }

  /**
   * @brief Retrieves a pointer to the handled texture.
   * 
   * @tparam TYPE Assumed texture type. It must be a value from \c RES3D_ enumeration.
   * 
   * @param [in] id Resource id.
   * @return        A pointer to the handled texture.
   * 
   * This function is a wrapper over \ref degD3dRes which compares the handled resource type with \b TYPE.
   * In case the types match, it returns a resource pointer cast to \ref BaseTexture
   * NULL is returned if \ref getD3dRes fails or if the types mismatch.
   */
  template <int TYPE>
  static __forceinline BaseTexture *getD3dTex(D3DRESID id)
  {
    D3dResource *r = getD3dRes(id);
    return (r && r->restype() == TYPE) ? (BaseTexture *)r : nullptr;
  }

  /**
   * @brief Returns the last-frame-used (LFU) for the resource.
   * 
   * @param [in] id Resource handle.
   * @return        Resource LFU or 0 if \b id is invalid.
   */
  static __forceinline unsigned getResLFU(D3DRESID id)
  {
    return isValidID(id, nullptr) ? (unsigned)interlocked_acquire_load(lastFrameUsed[id.index()]) : 0u;
  }

  /**
   * @brief Returns the current quality level for the resource.
   * 
   * @param [in] id Resource handle.
   * @return        Resource current QL or 0 if \b id is invalid.
   */
  static __forceinline TexQL getResCurQL(D3DRESID id) { return isValidID(id, nullptr) ? resQS[id.index()].getCurQL() : TQL_stub; }

  /**
   * @brief Returns the maximal available quality level for the resource.
   * 
   * @param [in] id Resource handle.
   * @return        Resource maximal QL or 0 if \b id is invalid.
   */
  static __forceinline TexQL getResMaxQL(D3DRESID id) { return isValidID(id, nullptr) ? resQS[id.index()].getMaxQL() : TQL_stub; }

  /**
   * @brief Retrieves the current loaded level of the resource.
   * 
   * @param [in] id Resource handle.
   * @return        Current loaded level of the resource or 0 if \b id is invalid.
   * 
   * See dag_resMgr.h for definition of 'level'.
   */
  static __forceinline uint8_t getResLoadedLev(D3DRESID id) { return isValidID(id, nullptr) ? resQS[id.index()].getLdLev() : 0; }
  
  /**
   * @brief Retrieves maximal requested level of the resource.
   *
   * @param [in] id Resource handle.
   * @return        Maximal requested level of the resource or 0 if \b id is invalid.
   *
   * See dag_resMgr.h for definition of 'level'.
   */
  static __forceinline uint8_t getResMaxReqLev(D3DRESID id) { return isValidID(id, nullptr) ? resQS[id.index()].getMaxReqLev() : 0; }

  /**
   * @brief Retrieves the current generation of the resource.
   *
   * @param [in] idx    Resource index. Use \ref D3DRESID::index to extract resource index from the handle.
   * @return            Current generation of the resource or 0 if \b idx is invalid.
   *
   * Generation is used to differentiate between 2 resources with identical ids.
   * Only a single generation is valid for a given id.
   */
  static __forceinline unsigned getCurGeneration(int idx) { return interlocked_acquire_load(generation[idx]); }
  
  /** 
   * @brief Marks the resource as used, i.e. updates its LFU with the current frame count and records the requested level. 
   * 
   * @param [in] id         Resource handle.
   * @param [in] req_lev    Requested level of the resource.
   * 
   * When the requested level value is greater than the loaded level value, texture streaming may schedule higher resolution texture load.
   */
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

  /**
   * @brief Translates quality level (QL) for a given resource into a level value.
   * 
   * @param [in] idx    Resource index. Use \ref D3DRESID::index to extract resource index from the handle.
   * @param [in] ql     Quality level to translate from.
   * @return            Level value (positive) or 0 if \b ql does not define level.
   */
  static uint8_t getLevDesc(int idx, TexQL ql) { return (levDesc[idx] >> (ql * 4)) & 0xF; }

protected:
  /**
   * @brief Indicates invalid reference count value for a resource.
   */
  static constexpr int INVALID_REFCOUNT = -1073741824; // 0xC0000000 to be negative and sufficiently far from 0 to both sides

  /**
   * @brief Stores quality state of the resource.
   * 
   * @todo There supposedly is a misspelling in the struct name, and it was supposed to be named 'ResQState.'
   */
  struct ReqQState
  {
    /**
     * @brief Determines maximal requested resource level.
     */
    uint8_t maxReqLev;

    /**
     * @brief   Stores the loaded level of the resource (left 4 bits) and 
     *          level of the resource to be loaded (or 0 if no reading is being performed at the moment; right 4 bits).
     */
    uint8_t ldLev__rdLev;

    /**
     * @brief   Stores maximal level of the texture quality (left 4 bits) and
     *          current maximal level of the resource (right 4 bits).
     */
    uint8_t qLev__maxLev; //< max level for tex quality;  current max level
    
    /**
     * @brief Stores current QL of the resource (left 4 bits) and maximal (right 4 bits) QL of the resource.
     */
    uint8_t curQL__maxQL; 

  public:

    /**
     * @brief   Retrieves maximal requested level of the resource.
     * 
     * @return  Maximal requested level of the resource.
     */
    uint8_t getMaxReqLev() const { return interlocked_relaxed_load(maxReqLev); } // relaxed is enough

    /**
     * @brief Sets maximal requested level of the resource.
     * 
     * @param [in] Level value to assign.
     */
    void setMaxReqLev(uint8_t l) { interlocked_release_store(maxReqLev, l); }

    /**
     * @brief Retrieves the loaded level of the resource.
     *
     * @return Loaded level of the resource.
     */
    uint8_t getLdLev() const { return interlocked_acquire_load(ldLev__rdLev) >> 4; }

    /**
     * @brief Retrieves the currently read (to be loaded) level of the resource.
     *
     * @return Read level of the resource.
     */
    uint8_t getRdLev() const { return interlocked_acquire_load(ldLev__rdLev) & 0xF; }

    /**
     * @brief Checks whether the resource is being read at the moment.
     * 
     * @return  \c true if the resource is being read at the moment, \c false otherwise.
     */
    bool isReading() const { return getRdLev() != 0; }

    /**
     * @brief Sets the currently read (to be loaded) level of the resource.
     *
     * @param [in] Level value to assign.
     */
    void setRdLev(uint8_t l) { return interlocked_release_store(ldLev__rdLev, (getLdLev() << 4) | (l & 0xF)); }

    /**
     * @brief Sets the loaded level of the resource.
     *
     * @param [in] Level value to assign.
     */
    void setLdLev(uint8_t l) { return interlocked_release_store(ldLev__rdLev, (l << 4) | 0); }

    /**
     * @brief Retrieves the maximal level of the texture quality.
     *
     * @return Maximal level of the texture quality.
     */
    uint8_t getQLev() const { return interlocked_relaxed_load(qLev__maxLev) >> 4; }

    /**
     * @brief Retrieves the current maximal level of the resource.
     *
     * @return Current maximal level of the resource.
     */
    uint8_t getMaxLev() const { return interlocked_acquire_load(qLev__maxLev) & 0xF; }

    /**
     * @brief Clamps a value so that it does not exceed the maximal level of the texture quality.
     * 
     * @param [in] l    Level value to clamp.
     * @return          Clamped level value.
     */
    uint8_t clampLev(uint8_t l) const
    {
      uint8_t qlev = getQLev();
      return l < qlev ? l : qlev;
    }
    
    /**
     * @brief Sets the maximal level of the texture quality.
     *
     * @param [in] Level value to assign.
     */
    void setQLev(uint8_t l)
    {
      l &= 0xF;
      interlocked_release_store(qLev__maxLev, l | (l << 4));
    }

    /**
     * @brief Sets the current maximal level of the resource.
     *
     * @param [in] Level value to assign.
     */
    void setMaxLev(uint8_t l)
    {
      uint8_t ql = getQLev();
      interlocked_release_store(qLev__maxLev, (ql << 4) | (l < ql ? l : ql));
    }

    /**
     * @brief Retrieves the  current QL of the resource.
     * 
     * @return Current quality level of the resource.
     */
    TexQL getCurQL() const { return TexQL(interlocked_acquire_load(curQL__maxQL) >> 4); }

    /**
     * @brief Retrieves the maximal QL of the resource.
     *
     * @return Maximal quality level of the resource.
     */
    TexQL getMaxQL() const { return TexQL(interlocked_relaxed_load(curQL__maxQL) & 0xF); }

    /**
     * @brief Checks if current and maximal quality levels of the resource are equal.
     * 
     * @return \c true if the current and maximal quality level are equal, \c false otherwise.
     */
    bool isEqualCurAndMaxQL() const
    {
      uint8_t cm = interlocked_relaxed_load(curQL__maxQL);
      return (cm >> 4) == (cm & 0xF);
    }

    /**
     * @brief Checks if current quality level of the resource is greater than a given value.
     * 
     * @param [in] ql   Quality level to compare with.
     * @return          \c true if the resource's current quality level is greater than \b ql.
     */
    bool isCurQLGreater(TexQL ql) const
    {
      TexQL cur = getCurQL();
      return cur != TQL_stub && cur > ql;
    }

    /**
     * @brief Sets maximal and current QLs of the resource.
     * 
     * @param [in] max_ql   Maximal QL value to assign.
     * @param [in] cur_ql   Current QL value to assign.
     */
    void setMaxQL(TexQL max_ql, uint8_t cur_ql) { interlocked_release_store(curQL__maxQL, ((cur_ql & 0xF) << 4) | (max_ql & 0xF)); }

    /**
     * @brief Sets current QL of the resource.
     *
     * @param [in] ql   Current QL value to assign.
     */
    void setCurQL(TexQL ql) { setMaxQL(getMaxQL(), ql); }
  };

  /**
   * @brief Determines the number of resource handles currently maintained by the manager
   */
  static int indexCount;

  /**
   * @brief Determins the maximal number of indices maintained (i.e. resources handled) by the manager.
   */
  static int maxTotalIndexCount;

  /**
   * @brief Buffer that stores reources' generations.
   */
  static uint16_t *__restrict generation;

  /**
   * @brief Buffer that stores resources' reference counters.
   */
  static int *__restrict refCount;

  /**
   * @brief Buffer that stores pointers to resources.
   */
  static D3dResource *__restrict *__restrict d3dRes;

  /**
   * @brief Buffer that stores resources' LFUs.
   */
  static int32_t *__restrict lastFrameUsed;

  /**
   * @brief Buffer that stores resources' quality states.
   */
  static ReqQState *__restrict resQS;
  
  /**
   * @brief Buffer that stores resources' level descriptors.
   */
  static uint16_t *__restrict levDesc;

  /**
   * @brief Casts the value to volatile type.
   * 
   * @tparam     T  
   * 
   * @param [in] p  A pointer to the value to cast.
   * @return        A casted pointer.
   */
  template <typename T>
  static inline T *volatile &asVolatile(T *__restrict &p)
  {
    return (T *volatile &)p;
  }

  /**
   * @brief Reports an error of addressing the resource with wrong generation.
   * 
   * @param [in] id A handle to the resource that caused the error.
   */
  static void report_bad_generation_used(D3DRESID id);

  /**
   * @brief Requests the texture to be loaded on usage.
   * 
   * @param [in] id A handle to the texture to load.
   */
  static void require_tex_load(D3DRESID id);

  /**
   * @brief Updates maximal requested level of the resource and requests texture load on usage if necessary.
   * 
   * @param [in] id     A handle to the resource to update.
   * @param [in] idx    Index of the resource to update.
   * @param [in] reqLev Value to update with.
   */
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

/**
 * @brief Grants the manager ownerwhip of the resource.
 * 
 * @param [in] name Name to register the resource under.
 * @param [in] res  A pointer to the resource to register.
 * @return          A handle to the registered resource.
 * 
 * The registered resource becomes owned by the resource manager, and no factory is assigned to it.
 * The manager maintains a reference count for the resource with the initial value 1. 
 * When the counter reaches 0, the resource is destroyed and its name is also unregistered.
 * 
 * @note Use \ref release_managed_res() or \ref release_managed_res_verified() to delete references and finally destroy the resource.
 */
D3DRESID register_managed_res(const char *name, D3dResource *res);

/**
 * @brief Checks whether a creation/release factory is set for the resource. 
 * 
 * @param [in] id   A handle to the resource to check.
 * @return          \c true if a factory is set, \c false otherwise.
 * 
 * Particularly, if the resource was registered with \ref register_managed_res, the check returns \c false.
 */
bool is_managed_res_factory_set(D3DRESID id);

/**
 * @brief Returns a pointer to the managed resource.
 * 
 * @param [in] id   A handle to the resource to acquire.
 * @return          A pointer to the resource or NULL on error (wrong ID).
 * 
 * The function supports reference count, i.e. increments the resource reference count, or
 * if the resource is not referenced yet, creates it. If a factory fails to create the resource
 * missing texture handling may be applied.
 * @note This function usage should be coupled with \ref release_managed_res or \ref release_managed_res_verified.
 */
D3dResource *acquire_managed_res(D3DRESID id);

/**
 * @brief Releases the resource.
 * 
 * @param [in] id   A handle to the resource to release.
 * 
 * Calling this function effectively decrements resource reference counter and, 
 * if it reaches 0, the resource is genuinely released.
 * @note This function usage should be coupled with \ref acquire_managed_res.
 */
void release_managed_res(D3DRESID id);

/**
 * @brief Releases the resource with verification.
 * 
 * @param [in, out] id          A handle to the resource to release. It is reset on release.
 * @param [in]      check_res   A pointer to the resource to check the released resource against.
 *  
 * @note This function usage should be coupled with \ref acquire_managed_res.
 * 
 * Calling this function effectively decrements resource reference counter and, 
 * if it reaches 0, the resource is genuinely released.
 * In adition to releasing the resource the function checks the released resource against \b check_res. 
 * An error is thrown on mismatch.
 */
void release_managed_res_verified(D3DRESID &id, D3dResource *check_res);

// helpers for textures (derived from D3dResource)

/**
 * @brief Returns a pointer to the managed texture.
 *
 * @param [in] id   A handle to the texture resource to acquire.
 * @return          A pointer to the texture or NULL on error (wrong ID or the acquired resource is not a texture).
 *
 * The function supports reference counting, i.e. increments the texture reference count, or
 * if the texture is not referenced yet, creates it. If a factory fails to create the texture
 * missing texture handling may be applied.
 * @note This function usage should be coupled with \ref release_managed_tex or \ref release_managed_tex_verified.
 */
BaseTexture *acquire_managed_tex(D3DRESID id);

/**
 * @brief Releases the texture.
 *
 * @param [in] id   A handle to the texture to release.
 *
 * Calling this function effectively decrements resource reference counter and, 
 * if it reaches 0, the texture resource is genuinely released.
 * @note This function usage should be coupled with \ref acquire_managed_tex.
 */
static inline void release_managed_tex(D3DRESID id) { release_managed_res(id); }

/**
 * @brief Releases the texture with verification.
 *
 * @param [in, out] id  A handle to the texture to release. It is reset on release.
 * @param [in, out] tex A pointer to the texture to check the released texture against. It is set to NULL.
 *
 * Calling this function effectively decrements resource reference counter and,
 * if it reaches 0, the texture  resource is genuinely released.
 * @note This function usage should be coupled with \ref acquire_managed_tex.
 * In adition to releasing the texture the function checks the released texture resource against \b check_res.
 * An error is thrown on mismatch.
 */
template <class T>
static inline void release_managed_tex_verified(D3DRESID &id, T &tex)
{
  release_managed_res_verified(id, static_cast<D3dResource *>(tex));
  tex = nullptr;
}

// helpers for buffers (derived from D3dResource)

/**
 * @brief Returns a pointer to the managed shader buffer.
 *
 * @param [in] id   A handle to the buffer resource to acquire.
 * @return          A pointer to the buffer or NULL on error (i.e. on wrong ID or on resource not being a shader buffer).
 *
 * The function supports reference counting, i.e. increments the buffer reference count, or
 * if the buffer is not referenced yet, creates it.
 * 
 * @note This function usage should be coupled with \ref release_managed_buf or \ref release_managed_buf_verified.
 */
Sbuffer *acquire_managed_buf(D3DRESID id);

/**
 * @brief Releases the shader buffer.
 *
 * @param [in] id   A handle to the buffer to release.
 *
 * Calling this function effectively decrements resource reference counter and,
 * if it reaches 0, the buffer resource is genuinely released.
 * 
 * @note This function usage should be coupled with \ref acquire_managed_buf.
 */
static inline void release_managed_buf(D3DRESID id) { release_managed_res(id); }

/**
 * @brief Releases the shader buffer with verification.
 *
 * @tparam          T       Buffer type.
 *
 * @param [in, out] id      A handle to the buffer to release. It is reset on release.
 * @param [in, out] buf     A pointer to the buffer to check the released buffer against. It is set to NULL.
 *
 * @note This function usage should be coupled with \ref acquire_managed_buf.
 *
 * Calling this function effectively decrements resource reference counter and,
 * if it reaches 0, the buffer  resource is genuinely released.
 * In adition to releasing the buffer the function checks the released buffer resource against \b buf.
 * An error is thrown on mismatch.
 */
template <class T>
static inline void release_managed_buf_verified(D3DRESID &id, T &buf)
{
  release_managed_res_verified(id, static_cast<D3dResource *>(buf));
  buf = nullptr;
}

/**
 * @brief Retrieves the managed resource name.
 * 
 * @param [in] id   A handle to the resource.
 * @return          A string storing the name of the resource or NULL, if \b id is invalid.
 */
const char *get_managed_res_name(D3DRESID id);

/**
 * @brief Retrieves a handle to the resource by its name.
 * 
 * @param [in] res_name The name of the resource.
 * @return              A handle to the resource or \c BAD_D3DRESID if \b res_name is invalid.
 */
D3DRESID get_managed_res_id(const char *res_name);

/**
 * @brief Retrieves the reference count of the managed resource.
 * 
 * @param [in] id   A handle to the resource.
 * @return          Resource reference count.
 */
inline int get_managed_res_refcount(D3DRESID id) { return D3dResManagerData::getRefCount(id); }

/**
 * @brief Retrieves the LFU value of the managed resource.
 * 
 * @param [in] id   A handle to the resource.
 * @return          Resource LFU value
 */
inline unsigned get_managed_res_lfu(D3DRESID id) { return D3dResManagerData::getResLFU(id); }

/**
 * @brief Retrieves the current TQL value of the managed resource.
 * 
 * @param [in] id   A handle to the resource.
 * @return          Resource current TQL value.
 */
inline TexQL get_managed_res_cur_tql(D3DRESID id) { return D3dResManagerData::getResCurQL(id); }

/**
 * @brief Retrieves the maximal available TQL value of the managed resource.
 * 
 * @param [in] id   A handle to the resource.
 * @return          Resource maximal available TQL value.
 */
inline TexQL get_managed_res_max_tql(D3DRESID id) { return D3dResManagerData::getResMaxQL(id); }

/**
 * @brief Retrieves the maximal requested level value of the managed resource.
 * 
 * @param [in] id   A handle to the resource.
 * @return          Resource current loaded level value.
 */
inline unsigned get_managed_res_maxreq_lev(D3DRESID id) { return D3dResManagerData::getResMaxReqLev(id); }
//! returns maximum requested-level for D3DRESID resource
/**
 * @brief Retrieves the loaded level value of the managed resource.
 * 
 * @param [in] id   A handle to th resource.
 * @return          Resource maximal requested level value.
 */
inline unsigned get_managed_res_loaded_lev(D3DRESID id) { return D3dResManagerData::getResLoadedLev(id); }

/**
 * @brief Turns the resource manager multithreading support on or off 
 * and preallocates enough memory to handle a specified number of resources without without reallocations.
 * 
 * @param [in] enable               Indicates whether multithreading should be enabled for the resource manager.
 * @param [in] max_res_entry_count  Maximal number of resources for the manager to handle.
 * 
 * @note Calling \c enable_res_mgr_mt(false, -1) will disable multithreading support and shrink the entries array to the used size.
 */
void enable_res_mgr_mt(bool enable, int max_res_entry_count);

/**
 * @brief Retrieves a handle to the next managed resource.
 * 
 * @param [in] after_rid        A handle to the resource preceding the resource to retrieve.
 * @param [in] min_ref_count    Determines which resources are to iterate through. All resources with a lesser reference count value will be ignored. 
 * @return                      A handle to the resource following \b after_id with the reference count value greater or equal to \b min_ref_count or \c BAD_TEXTUREID if there is no fitting resource.
 */
D3DRESID iterate_all_managed_d3dres(D3DRESID after_rid, int min_ref_count);

/**
 * @brief Retrieves a handle to the first managed resource.
 * 
 * @param [in] min_rc   Determines which resources are to look for. All resources with a lesser reference count value will be ignored.
 * @return              A handle to the first managed resource with the reference count value greater or equal to \b min_rc or \c BAD_TEXTUREID if there is no fitting resource.
 * 
 * Convenient for for-loops. See \ref next_managed_d3dres.
 * Example use case: <tt>for (D3DRESID id = first_managed_d3dres(); id != BAD_D3DRESID; id = next_managed_d3dres(id)) <tt>.
 */
inline D3DRESID first_managed_d3dres(int min_rc = 0) { return iterate_all_managed_d3dres(BAD_D3DRESID, min_rc); }

/**
 * @brief Retrieves a handle to the next managed resource.
 *
 * @param [in] prev_id  A handle to the resource preceding the resource to retrieve.
 * @param [in] min_rc   Minimal refernce count value for resources to iterate through. All resources with a lesser reference count value will be ignored.
 * @return              A handle to the resource following \b prev_id with the reference count value greater or equal to \b min_rc or \c BAD_TEXTUREID if there is no fitting resource.
 * 
 * Convenient for for-loops. See \ref first_managed_d3dres.
 * Example use case: <tt>for (D3DRESID id = first_managed_d3dres(); id != BAD_D3DRESID; id = next_managed_d3dres(id)) </tt>.
 */
inline D3DRESID next_managed_d3dres(D3DRESID prev_id, int min_rc = 0) { return iterate_all_managed_d3dres(prev_id, min_rc); }
