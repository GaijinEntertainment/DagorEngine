//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

/************************************************************************
  shader mesh & mesh data classes
************************************************************************/

#include <3d/dag_texMgr.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_buffers.h>
#include <generic/dag_DObject.h>
#include <generic/dag_patchTab.h>
#include <generic/dag_smallTab.h>
#include <memory/dag_mem.h>

#include <shaders/dag_shaders.h>
#include <util/dag_stdint.h>
#include <util/dag_globDef.h>


class IGenSave;
class IGenLoad;
class VertToFaceVertMap;
class Mesh;

class DynamicRenderableSceneLodsResource;
class RenderableInstanceLodsResource;
template <class RES>
class ShaderResUnitedVdata;
namespace unitedvdata
{
struct BufPool;
}

class ShaderMeshDataSaveCB;

enum
{
  VDATA_D3D_RESET_READY = 0x01, // vdata can survive d3d reset using reload or reset callback, buffers use SBCF_MAYBELOST
  VDATA_NO_IB = 0x08,
  VDATA_NO_VB = 0x10,
  VDATA_NO_IBVB = 0x18,

  VDATA_I16 = 0x20,   // index buffer is 16 bit
  VDATA_I32 = 0x40,   // index buffer is 32 bit
  VDATA_IAUTO = 0x00, // index buffer type will depend on vertexnumber

  VDATA_VB_DYNAMIC = 0x80,
  VDATA_SRC_ONLY = 0x100,
  VDATA_PACKED_IB = 0x200,
  VDATA_BIND_SHADER_RES = 0x400,
  VDATA_LOD_MASK = 0xF000,
};

/*********************************
 *
 * class GlobalVertexData
 *
 *********************************/
class GlobalVertexData
{
public:
  // access atributes
  inline Vbuffer *getVB() const { return vb; };

  inline void *getIBMem() const
  {
    G_ASSERT((cflags & VDATA_NO_IB));
    return ibMem;
  }
  template <typename Index>
  inline const Index *getIBMem(unsigned si, unsigned numf) const
  {
    G_ASSERT_RETURN((cflags & VDATA_NO_IB), nullptr);
    G_ASSERTF_RETURN(ibMem && (si + numf * 3) * sizeof(Index) <= *(unsigned *)ibMem, nullptr, "ibMem=%p (%d bytes) si=%d numf=%d",
      ibMem, ibMem ? *(int *)ibMem : 0, si, numf);
    return reinterpret_cast<const Index *>(1 + (int *)ibMem) + si; //-V769
  }

  inline void *getVBMem() const
  {
    G_ASSERT((cflags & VDATA_NO_VB));
    return vbMem;
  }
  template <typename Vertex>
  inline const Vertex *getVBMem(unsigned base_v, unsigned sv, unsigned numv) const
  {
    G_ASSERT_RETURN((cflags & VDATA_NO_VB), nullptr);
    G_ASSERTF_RETURN(vbMem && (base_v + sv + numv) <= *(int *)vbMem && sizeof(Vertex) == vstride, nullptr,
      "vbMem=%p (%d vertices) base_v=%d sv=%d numv=%d sizeof(Vertex)=%d vstride=%d", vbMem, vbMem ? *(int *)vbMem : 0, base_v, sv,
      numv, sizeof(Vertex), vstride);
    return reinterpret_cast<const Vertex *>(1 + (int *)vbMem) + (base_v + sv); //-V769
  }
  inline Ibuffer *getIB() const
  {
    G_ASSERT(!(cflags & VDATA_NO_IB));
    if (indices->getFlags() & SBCF_BIND_INDEX)
    {
      return indices;
    }
    else
    {
      G_ASSERT(0);
      return NULL;
    }
  }

  int getStride() const { return vstride; }
  unsigned getFlags() const { return cflags; }
  unsigned testFlags(unsigned f) const { return cflags & f; }
  unsigned getVbSize() const { return vCnt * vstride; }
  unsigned getIbElemSz() const { return testFlags(VDATA_I32) ? 4 : 2; }
  unsigned getIbSize() const { return iCnt * getIbElemSz(); }
  unsigned getIbPackedSize() const { return iPackedSz; }
  unsigned getLodIndex() const { return (cflags & VDATA_LOD_MASK) >> __bsf(VDATA_LOD_MASK); }

  bool isRenderable(bool &has_indices) const
  {
    has_indices = !!indices;
    return !!vb;
  }

  // explicit constructor
  void initGvd(const char *name, unsigned vNum, unsigned vStride, unsigned idxPacked, unsigned idxSize, unsigned flags, IGenLoad *crd,
    Tab<uint8_t> &tmp_decoder_stor);

  void initGvdMem(int vertNum, int vertStride, int idxSize, unsigned flags, const void *vb_data, const void *ib_data);

  // explicit destructor
  void free();

  // set all params to driver
  void setToDriver() const;

  // check for null vertex buffer if it needed
  bool isEmpty() const
  {
    if (cflags & VDATA_NO_IBVB)
      return false;
    if (vCnt != 0 && vb == nullptr)
      return true;
    return false;
  }

  void unpackToBuffers(IGenLoad &zcrd, bool update_ib_vb_only, Tab<uint8_t> &buf_stor);
  void unpackToSharedBuffer(IGenLoad &zcrd, Vbuffer *shared_vb, Ibuffer *shared_ib, int &vb_byte_pos, int &ib_byte_pos,
    Tab<uint8_t> &buf_stor);
  int getVbIdx() const { return vbIdx; }

  void copyDescFrom(const GlobalVertexData &src)
  {
    vbMem = ibMem = nullptr;
    vbIdx = 0;

    vstride = src.vstride;
    cflags = src.cflags;
    vCnt = src.vCnt;
    iCnt = src.iCnt;
    vOfs = src.vOfs;
    iOfs = src.iOfs;
    iPackedSz = src.iPackedSz;
  }

private:
  union
  {
    Vbuffer *vb;
    void *vbMem;
  };
  union
  {
    Ibuffer *indices;
    void *ibMem;
  };
  uint16_t vstride;
  uint16_t cflags; // create flags
  uint32_t vCnt : 28, vbIdx : 4, iCnt, vOfs, iOfs;
  uint32_t iPackedSz = 0;

  // ctor/dtor
  GlobalVertexData() : vb(NULL), indices(NULL), vstride(0), cflags(0), vCnt(0), vbIdx(0), iCnt(0), vOfs(0), iOfs(0) {}
  ~GlobalVertexData() { free(); }

  friend struct unitedvdata::BufPool;
  template <typename T, typename A, bool, typename C>
  friend class dag::Vector;
  template <class F, bool I>
  friend typename eastl::disable_if<I, void>::type dag::small_vector_default_fill_n(F first, size_t n);
  template <class F>
  friend void eastl::destruct_impl(F, F, eastl::false_type);
  template <class F>
  friend void eastl::destruct(F *);
  friend class ShaderResUnitedVdata<RenderableInstanceLodsResource>;
  friend class ShaderResUnitedVdata<DynamicRenderableSceneLodsResource>;
}; // class GlobalVertexData
//


class ShaderMatVdata : public DObject, public Sbuffer::IReloadData
{
public:
  decl_class_name(ShaderMatVdata);

  static ShaderMatVdata *create(int tex_num, int mat_num, int vdata_num, int mvhdr_sz, unsigned model_type = 0);

  static ShaderMatVdata *make_tmp_copy(ShaderMatVdata *smv, int apply_skip_first_lods_cnt = -1);
  static void update_vdata_from_tmp_copy(ShaderMatVdata *dest, ShaderMatVdata *src);

  inline int getMaterialCount() const { return mat.size(); }
  inline ShaderMaterial *getMaterial(int idx) const { return mat[idx]; }
  inline int getGlobVDataCount() const { return vdata.size(); }
  inline GlobalVertexData *getGlobVData(int idx) const { return const_cast<GlobalVertexData *>(&vdata[idx]); }
  inline void setGlobVData(dag::Span<GlobalVertexData> vd) { vdata = vd; }

  void loadTexStr(IGenLoad &crd, bool sym_tex, const char *base_path = NULL);
  void loadTexIdx(IGenLoad &crd, dag::ConstSpan<TEXTUREID> texMap);
  void getTexIdx(const ShaderMatVdata &other_smvd);

  void makeTexAndMat(const DataBlock &texBlk, const DataBlock &matBlk);
  void loadMatVdata(const char *name, IGenLoad &crd, unsigned flags);

  void preloadTex();
  void finalizeMatRefs();

  void unpackBuffersTo(dag::Span<Sbuffer *> buf, int *buf_byte_ofs, dag::Span<int> start_end_stride, Tab<uint8_t> &buf_stor);
  void clearVdataSrc();
  bool isReloadable() const { return matVdataSrcRef.fname != nullptr; }

  bool areLodsSplit() const { return lodsAreSplit; }
  void setLodsAreSplit() { lodsAreSplit = 1; }
  void applyFirstLodsSkippedCount(int skip_first_lods_cnt)
  {
    if (skip_first_lods_cnt > 0 && areLodsSplit())
      for (int i = 0; i < vdataFullCount; i++)
        if ((vdata.data() + i)->getLodIndex() < skip_first_lods_cnt)
        {
          vdata.set(vdata.data(), i);
          for (; i < vdataFullCount; i++)
            (vdata.data() + i)->free();
          return;
        }
    vdata.set(vdata.data(), vdataFullCount);
  }

  static void closeTlsReloadCrd();

  static int cmp_src_file_and_ofs(const ShaderMatVdata &a, const ShaderMatVdata &b)
  {
    return a.matVdataSrcRef.fname == b.matVdataSrcRef.fname ? int(a.matVdataSrcRef.fileOfs - b.matVdataSrcRef.fileOfs)
                                                            : int(a.matVdataSrcRef.fname - b.matVdataSrcRef.fname);
  }

  // Sbuffer::IReloadData
  void reloadD3dRes(Sbuffer *) override;
  void destroySelf() override {}

  struct ModelLoadStats
  {
    unsigned reloadTimeMsec = 0;
    unsigned reloadDataSizeKb = 0;
    unsigned reloadDataCount = 0;
  };
  static const ModelLoadStats &get_model_load_stats(unsigned model_type) { return modelLoadStats[model_type <= 2 ? model_type : 0]; }

protected:
  static ModelLoadStats modelLoadStats[1 + 2];

protected:
  ShaderMatVdata(int tex_num, int mat_num, int vdata_num, int mvhdr_sz, unsigned model_type);
  ~ShaderMatVdata();

  dag::Span<TEXTUREID> tex;
  dag::Span<ShaderMaterial *> mat;
  dag::Span<GlobalVertexData> vdata;
  struct MatVdataSrc // Note: vector-like class that does `tryAlloc` on `resize`
  {
    using value_type = uint8_t;

    MatVdataSrc() = default;
    MatVdataSrc(const MatVdataSrc &) = delete;
    MatVdataSrc &operator=(MatVdataSrc &&o)
    {
      eastl::swap(ptr, o.ptr);
      eastl::swap(count, o.count);
      return *this;
    }
    ~MatVdataSrc()
    {
      if (ptr)
        delete[] ptr;
    }
    bool empty() const { return !count; }
    uint32_t size() const { return count; }
    bool resize(uint32_t sz)
    {
      delete[] ptr;
      ptr = sz ? new (std::nothrow) uint8_t[sz] : nullptr; // Note: tryAlloc
      count = ptr ? sz : 0;
      return !!ptr;
    }
    uint8_t *data() { return ptr; }
    void clear()
    {
      delete[] ptr;
      ptr = nullptr;
      count = 0;
    }

  private:
    uint8_t *ptr = nullptr;
    uint32_t count = 0;
  } matVdataSrc;
  int matVdataHdrSz;
  unsigned lodsAreSplit : 1, modelType : 2, _resv : 21, vdataFullCount : 8;
  struct VdataSrcRef
  {
    const char *fname;
    unsigned fileOfs;
    unsigned dataSz : 26, comprType : 3, packTag : 3;
#if !_TARGET_64BIT
    unsigned _pad = 0;
#endif
    VdataSrcRef() : fname(nullptr), fileOfs(0), dataSz(0), comprType(0), packTag(0) {}
  } matVdataSrcRef;

  struct MatVdataHdr;
  struct TexStrHdr;
};


/*********************************
 *
 * class ShaderMesh
 *
 *********************************/
class ShaderMesh
{
public:
  enum
  {
    STG_opaque = 0,
    STG_atest,
    STG_imm_decal,
    STG_decal,
    STG_trans,
    STG_distortion,
    STG_COUNT,
  };

  //*************************************************************
  // desc for single element (vertex group & shader)
  //*************************************************************
  struct RElem
  {
    PATCHABLE_DATA64(Ptr<ShaderElement>, e);          // ptr to shader element
    PATCHABLE_DATA64(Ptr<ShaderMaterial>, mat);       // ptr to material
    PATCHABLE_DATA64(GlobalVertexData *, vertexData); // ptr to struct with VB, IB

    int vdOrderIndex; // index of this element in vertexData (for dynamic connect ranges while rendering)

    int sv;   // start vertex index in VB
    int numv; // number of vertices

    int si;         // start index in IB
    int numf;       // number of faces
    int baseVertex; // baseVertex in ib

    int drawIndTriList() const { return d3d::drawind(PRIM_TRILIST, si, numf, baseVertex); }
    int drawIndTriList(uint32_t num_inst, uint32_t start_inst = 0) const
    {
      return d3d::drawind_instanced(PRIM_TRILIST, si, numf, baseVertex, num_inst, start_inst);
    }
    void renderWithElem(const ShaderElement &elem) const { elem.render(sv, numv, si, numf, baseVertex); }
    void render() const { renderWithElem(*e); }

    uint8_t getPrimitive() const { return si != RELEM_NO_INDEX_BUFFER ? PRIM_TRILIST : PRIM_POINTLIST; }

    RElem() = delete;
    RElem(const RElem &) = delete;
  };

public:
  DAG_DECLARE_NEW(midmem)

  // build simple single-material mesh with it own vertex/index buffers
  static ShaderMesh *createSimple(Mesh &m, ShaderMaterial *mat, const char *info_str = NULL);

  // create copy of existing ShaderMesh
  static ShaderMesh *createCopy(const ShaderMesh &sm);

  // create Shader mesh by load dump from stream
  static ShaderMesh *load(IGenLoad &crd, int sz, ShaderMatVdata &smvd, bool acqire_tex_refs = true);
  static ShaderMesh *loadMem(const void *p, int sz, ShaderMatVdata &smvd, bool acqire_tex_refs = true);


  // ctor/dtor
  ShaderMesh() { memset(stageEndElemIdx, 0, sizeof(stageEndElemIdx)); }
  ~ShaderMesh() { clearData(); }

  // patch mesh data after loading from dump
  void patchData(void *base, ShaderMatVdata &smvd);

  // explicit destructor
  void clearData();

  // rebase and clone data (useful for data copies)
  void rebaseAndClone(void *new_base, const void *old_base);

  // rendering
  void render() const { render(getElems(STG_opaque, STG_atest)); }
  void render_trans() const { render(getElems(STG_trans)); }
  void render_distortion() const { render(getElems(STG_distortion)); }

  // render with current shader
  void renderRawImmediate(bool trans) const;

  // render with specific shader
  void renderWithShader(const ShaderElement &shader_element, bool trans) const;

  void gatherUsedTex(TextureIdSet &tex_id_list) const;
  void gatherUsedMat(Tab<ShaderMaterial *> &mat_list) const;

  // change texture by texture ID
  bool replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new);

  // get number of faces
  int calcTotalFaces() const;

  static void duplicateMaterial(TEXTUREID tex_id, dag::Span<RElem> elem, Tab<ShaderMaterial *> &old_mat,
    Tab<ShaderMaterial *> &new_mat);
  static void duplicateMat(ShaderMaterial *prev_m, dag::Span<RElem> elem, Tab<ShaderMaterial *> &old_mat,
    Tab<ShaderMaterial *> &new_mat);
  void duplicateMaterial(TEXTUREID tex_id, Tab<ShaderMaterial *> &old_mat, Tab<ShaderMaterial *> &new_mat)
  {
    duplicateMaterial(tex_id, make_span(elems), old_mat, new_mat);
  }
  void duplicateMat(ShaderMaterial *prev_m, Tab<ShaderMaterial *> &old_mat, Tab<ShaderMaterial *> &new_mat)
  {
    duplicateMat(prev_m, make_span(elems), old_mat, new_mat);
  }

  bool getVbInfo(RElem &elem, int usage, int usage_index, unsigned int &stride, unsigned int &offset, int &type, int &mod) const;

  void acquireTexRefs();
  void releaseTexRefs();

  void updateShaderElems();

  dag::ConstSpan<RElem> getAllElems() const { return elems; }
  uint32_t getElemsCount(uint32_t start_stage, uint32_t end_stage) const
  {
    G_ASSERT(start_stage <= end_stage && end_stage < STG_COUNT);
    return stageEndElemIdx[end_stage] - (start_stage == 0 ? 0 : stageEndElemIdx[start_stage - 1]);
  }
  dag::Span<RElem> getElems(uint32_t start_stage, uint32_t end_stage) const
  {
    G_STATIC_ASSERT((uint32_t)STG_COUNT <= (uint32_t)SC_STAGE_IDX_MASK);
    G_ASSERT_RETURN(start_stage <= end_stage && end_stage < (uint32_t)STG_COUNT, {});
    uint16_t start = start_stage == 0 ? 0 : stageEndElemIdx[start_stage - 1];
    return make_span(const_cast<ShaderMesh *>(this)->elems).subspan(start, stageEndElemIdx[end_stage] - start);
  }
  dag::Span<RElem> getElems(int stage) const { return getElems(stage, stage); }
  dag::ConstSpan<uint16_t> getElemsIdx() const { return dag::ConstSpan<uint16_t>(stageEndElemIdx, countof(stageEndElemIdx)); }

public:
  static void set_material_pass(int mat_pass, int whole_pass_id);


private:
  PatchableTab<RElem> elems;
  uint16_t stageEndElemIdx[SC_STAGE_IDX_MASK + 1];

  int _deprecatedMaxMatPass = 1; // deprecated
  int _resv = 0;

  // render items
  void render(dag::Span<RElem> elem_array) const;

#if DAGOR_DBGLEVEL > 0
  static bool dbgRenderStarted;
#endif
  friend class ShaderMeshData;
}; // class ShaderMesh
//

template <class T>
inline ShaderMaterial *replace_shader_mat(T &mesh, ShaderMaterial *prev_m, Tab<ShaderMaterial *> &old_mat,
  Tab<ShaderMaterial *> &new_mat)
{
  mesh.duplicateMat(prev_m, old_mat, new_mat);
  int idx = find_value_idx(old_mat, prev_m);
  return idx < 0 ? NULL : new_mat[idx];
}

//-V:getAllElems:758
//-V:getElems:758
