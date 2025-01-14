// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/shaderResBuilder/meshDataSave.h>
#include <3d/dag_texMgr.h>
#include <generic/dag_tab.h>
#include <generic/dag_DObject.h>
#include <memory/dag_mem.h>
#include <util/dag_stdint.h>
#include <util/dag_globDef.h>
#include <math/dag_color.h>


class ShaderElement;
class ShaderMaterial;
class ShaderMesh;
class GlobalVertexData;
class VertToFaceVertMap;
class Mesh;
struct CompiledShaderChannelId;
struct ShaderChannelId;

class ShaderMeshDataSaveCB;


class IColorConvert
{
public:
  virtual Color4 convert(Color4 from) = 0;
};

class IdentColorConvert : public IColorConvert
{
public:
  virtual Color4 convert(Color4 from) { return from; }

  static IdentColorConvert object;
};

/*********************************
 *
 * class GlobalVertexDataSrc
 *
 *********************************/
class GlobalVertexDataSrc : public DObject
{
public:
  Tab<uint8_t> vData;  // vertex data
  Tab<uint16_t> iData; // index data
  Tab<int> iData32;    // index data 32 bits
  int numv;            // total number of vertices
  int numf;            // total number of faces

  int partCount; // total number of VB/IB ranges
  int baseVertSegCount;

  Tab<CompiledShaderChannelId> vDesc; // compressed channel description
  Tab<int> vRemap, iNew;
  int stride;
  unsigned gvdFlags = 0;
  bool allowVertexMerge = true;

  // ctor/dtor
  GlobalVertexDataSrc();
  ~GlobalVertexDataSrc();

  decl_class_name(GlobalVertexDataSrc);

  // construct from channels
  GlobalVertexDataSrc(const CompiledShaderChannelId *ch, int count);

  // check channels for equality. return true, if channels are checked OK
  bool checkChannels(const CompiledShaderChannelId *ch, int count) const;

  // check for free space into vertex- & indexbuffers.
  // return true, if can add secified count of vertices & indices
  bool checkFreeSpace(int add_numv, int add_numf, bool allow_32_bit) const;

  // check for attach other vertex data
  bool canAttachData(const GlobalVertexDataSrc &other_data, bool allow_32_bit) const;
  bool canAttachData(const GlobalVertexDataSrc &other_data, bool allow_32_bit, int _numv, int _numf) const;

  // attach data
  void attachData(const GlobalVertexDataSrc &other_data);

  // attach one part
  void attachDataPart(const GlobalVertexDataSrc &other_data, int _sv, int _numv, int _si, int _numf, int num_parts);

  void convertToIData32();

  bool is32bit() const { return iData32.size() > 0; }
}; // class GlobalVertexDataSrc
//


/*********************************
 *
 * class ShaderMeshData
 *
 *********************************/
class ShaderMeshData
{
public:
  DAG_DECLARE_NEW(tmpmem)

  static unsigned buildForTargetCode; // set as _MAKE4C('PC') by default
  static bool forceZlibPacking;       // =false by default
  static bool preferZstdPacking;      // =false by default
  static bool allowOodlePacking;      // =false by default
  static bool fastNoPacking;          // =false by default
  static unsigned zstdMaxWindowLog;   // =0 by default (to use zstd defaults for compression level)
  static int zstdCompressionLevel;    // =18 by default

  //*************************************************************
  // desc for single element (vertex group & material)
  //*************************************************************
  struct RElem
  {
    Ptr<ShaderMaterial> mat;             // ptr to material
    Ptr<GlobalVertexDataSrc> vertexData; // ptr to struct with vertex & index data
    int vertexDataId;                    // cached index for fast mesh building after loading
    uint16_t flags, optDone;             // shader element flags; optimize-done mark

    int vdOrderIndex; // index of this element in vertexData (for dynamic connect ranges while rendering)
    int bv;

    int sv;   // start vertex index in VB
    int numv; // number of vertices

    int si;   // start index in IB
    int numf; // number of faces

    float textureScale; // maximum uv change per one unit position change, used only for export

    RElem() : bv(0), sv(-1), numv(0), si(-1), numf(0), flags(0), optDone(0), vertexDataId(-1), vdOrderIndex(0), textureScale(FLT_MAX)
    {}

    // save/load
    void save(mkbindump::BinDumpSaveCB &cwr, ShaderMeshDataSaveCB &mdcb);
    int getStage() const { return flags & SC_STAGE_IDX_MASK; }
  };

  Tab<RElem> elems;
  uint16_t stageEndElemIdx[SC_STAGE_IDX_MASK + 1];

  // ctor/dtor
  ShaderMeshData();
  ~ShaderMeshData();

  // build meshdata
  bool build(class Mesh &m, ShaderMaterial **mats, int nummats, IColorConvert &color_convert, bool allow_32_bit = false,
    int node_id = 128);

  // optimize data for cache
  void optimizeForCache(bool opt_overdraw_too = true);

  // save
  int save(mkbindump::BinDumpSaveCB &cwr, ShaderMeshDataSaveCB &mdcb, bool new_orig);

  // direct creation of ShaderMesh
  ShaderMesh *makeShaderMesh(const PtrTab<GlobalVertexDataSrc> vdata, dag::Span<GlobalVertexData> builtVdata);

  // change texture by texture ID
  bool replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new);

  // replace one matrial with other. no channel checking performed
  void replaceMaterial(ShaderMaterial *exists_mat, ShaderMaterial *new_mat);

  // add all used vertex data to CB
  void enumVertexData(ShaderMeshDataSaveCB &mdcb) const;

  // WARN! following functions assumes that no shared global vdatas used
  // (each meshdata MUST use their own global vdatas)
  // you MUST use this functions BEFORE connect global vdatas
  // check for attach other mesh data
  bool canAttachData(const ShaderMeshData &other_data, bool allow_32_bit) const;

  // attach data. mix elements from current & other mesh datas.
  void attachData(const ShaderMeshData &other_data, bool allow_32_bit);

  static void reset_channel_cvt_errors();
  static int get_channel_cvt_errors();
  static int get_channel_cvt_critical_errors();

  RElem *addElem(int stage)
  {
    int idx = insert_items(elems, stageEndElemIdx[stage], 1);
    for (int i = stage; i < SC_STAGE_IDX_MASK + 1; i++)
      stageEndElemIdx[i]++;
    return &elems[idx];
  }
  RElem *addElem(int stage, const RElem &re)
  {
    int idx = insert_item_at(elems, stageEndElemIdx[stage], re);
    for (int i = stage; i < SC_STAGE_IDX_MASK + 1; i++)
      stageEndElemIdx[i]++;
    return &elems[idx];
  }
  void delElem(int idx)
  {
    for (int i = 0; i < SC_STAGE_IDX_MASK + 1; i++)
      if (stageEndElemIdx[i] > idx)
        stageEndElemIdx[i]--;
    erase_items(elems, idx, 1);
  }
  dag::Span<RElem> getElems(int start_stage, int end_stage) const
  {
    G_ASSERT_RETURN(start_stage >= 0 && start_stage <= SC_STAGE_IDX_MASK, {});
    G_ASSERT_RETURN(end_stage >= 0 && end_stage <= SC_STAGE_IDX_MASK, {});
    G_ASSERT_RETURN(start_stage <= end_stage, {});
    uint16_t start = start_stage > 0 ? stageEndElemIdx[start_stage - 1] : 0;
    return make_span(const_cast<ShaderMeshData *>(this)->elems).subspan(start, stageEndElemIdx[end_stage] - start);
  }
  dag::Span<RElem> getElems(int stage) const { return getElems(stage, stage); }

  unsigned countFaces() const
  {
    unsigned faces = 0;
    for (const auto &e : elems)
      faces += e.numf;
    return faces;
  }

private:
  void optimizeForCache(RElem &re, bool opt_overdraw_too);

  // check for attach elements. return true, if can attach at least 1 element
  bool canAttachDataElem(const Tab<RElem> &elem, const Tab<RElem> &other_elem, bool allow_32_bit) const;


  // add elems from other data
  void AddRElems(Tab<RElem> &elem, const Tab<RElem> &other_elem, bool allow_32_bit);
};
DAG_DECLARE_RELOCATABLE(ShaderMeshData);


bool can_combine_elems(const ShaderMeshData::RElem &left, const ShaderMeshData::RElem &right, bool allow_32_bit = false,
  int additional_verices_num = 0, int additional_faces_num = 0);

void create_vertex_color_data(Mesh &m, int usage, int usage_index);

void add_per_vertex_domain_uv(Mesh &m, int usage, int usage_index_0, int usage_index_1);
