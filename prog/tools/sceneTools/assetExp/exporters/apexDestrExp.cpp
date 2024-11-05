// Copyright (C) Gaijin Games KFT.  All rights reserved.

/*
structure of fracturing:
- mimp::MeshSystem is one apex asset
- consists from parts (mMeshes)
- each part consist from submeshes (SubMesh) - triangles with one material


--mimp::MeshSystem
  MiU32           mMeshCount;
  Mesh**            mMeshes;

  --mimp::Mesh
    MiU32       mSubMeshCount;
    SubMesh**     mSubMeshes;
    MiU32       mVertexCount;
    MeshVertex*     mVertices;

    --SubMesh
      const char*     mMaterialName;
      MeshMaterial*   mMaterial;
      MiU32       mTriCount;
      MiU32*        mIndices;

NOTE: each mimp::Mesh fractures separately from others ! (this means if nodes intersects -> their chunks may repelled in the game !)
NOTE: Mesh has 1 vb for all its submeshes, set indices carefully for all submeshes

*.DAG structure:
- consist from nodes (i.e. layers in 3dsMax)
- we can unite several nodes in one mimp::Mesh (create 1 polygon soup from their meshes)
- find all triangles with one material in one mimp::Mesh and create SubMesh

fracture options from DAG:
    apex_destructible:b=yes
    apex_fracture_group:i=0
    apex_num_chunks:i=100
    apex_interior_material:t="broken_walls"
    apex_gradually_destructible:b=no (if set then we want chip chunks one by one... if not set - mesh will fall to pieces instantly)
    apex_cutout_tex_name:t="tex_name" //see String cutout_mask_folder
    apex_cutout_dir:p3 = 0.0, 1.0, 0.0

fracture::FractureDesc - fracture properties for entire apex asset (for MeshSystem)
*/


#include <assets/daBuildExpPluginChain.h>
#include <assets/assetExporter.h>
#include <assets/asset.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <gameRes/dag_stdGameRes.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_btagCompr.h>
#include <3d/dag_materialData.h>
#include <math/dag_mesh.h>
#include <debug/dag_debug.h>
#include <util/dag_texMetaData.h>
#include <startup/dag_startupTex.h>
#include <image/dag_loadImage.h>
#include <image/dag_texPixel.h>
#include <util/dag_string.h>
#include <osApiWrappers/dag_direct.h>
#include <fractureConfig.h>
#include <NxApexUserProgress.h>
#include <NxDestructibleAsset.h>
#include <NxApexSDKCachedData.h>
#include <NxApexScene.h>
#include <PxScene.h>
#include <PxMaterial.h>
#include <PxPhysics.h>
#include <NxApexActor.h>
#include <NxModuleDestructible.h>
#include <NxDestructibleAsset.h>
#include <NxDestructibleActor.h>

#include <fractureApex.h>
#include <fractureUtils.h>
#include <fractureMeshImport.h>
#include <fractureMeshUtils.h>
#include <MeshImport.h>
#include <regExp/regExp.h>
#include "modelExp.h"
#include <libTools/shaderResBuilder/processMat.h>

BEGIN_DABUILD_PLUGIN_NAMESPACE(apex)

static const char *TYPE = "apexDestr";
static float apex_density_multiplier = 10.f;

static fracture::FractureApex *apex;
static String cutout_mask_folder;
static int cutout_max_texture_size = 128;

const bool writeCompressedData = true; // NOTE: don't change it, revert whole commit
const bool writeAssetCache = true;
static bool compressVertexAttributes = true;
const bool writeUserDataBlock = true;
const int user_data_format_version = 1;

static float uv_value_range = 8.f;
static float vertex_value_range = 50.f;

static float verts_dist_delta = 0.01f * 0.01f;
static int check_verts_dist_delta = 1;
static int check_cutout_mesh_alike_plane = 1;

static int debug_write_assets_to_file = 0;
static String debug_output_folder;

static int microgrid_size = 2048;

static bool use_noise = true;
static float default_noise_amplitude = 0.01f;
static float default_noise_frequency = 0.25f;
static int default_noise_grid_size = 5;

static bool preferZstdPacking = false;

class PxFileBufSaveCB : public physx::PxFileBuf
{
  mkbindump::BinDumpSaveCB &cwr;

public:
  PxFileBufSaveCB(mkbindump::BinDumpSaveCB &_cwr) : cwr(_cwr) {}

  virtual OpenMode getOpenMode(void) const { return OPEN_WRITE_ONLY; }

  virtual SeekType isSeekable(void) const { return SEEKABLE_WRITE; }

  virtual PxU32 getFileLength(void) const { return cwr.getSize(); }

  virtual PxU32 seekRead(PxU32 loc) { return cwr.tell(); }
  virtual PxU32 seekWrite(PxU32 loc)
  {
    cwr.seekto(loc);
    return cwr.tell();
  }

  virtual PxU32 read(void *mem, PxU32 len) { return 0; }
  virtual PxU32 peek(void *mem, PxU32 len) { return 0; }

  virtual PxU32 write(const void *mem, PxU32 len)
  {
    cwr.writeRaw(mem, len);
    return len;
  }

  virtual PxU32 tellRead(void) const { return cwr.tell(); }
  virtual PxU32 tellWrite(void) const { return cwr.tell(); }

  virtual void flush(void) {}
};


class ApexDestrExp : public IDagorAssetExporter
{
  static constexpr unsigned ApexDestrGameResClassId = 0x73A10E01u; // ApexDestr
public:
  virtual const char *__stdcall getExporterIdStr() const { return "apexDestr exp"; }

  virtual const char *__stdcall getAssetType() const { return TYPE; }
  virtual unsigned __stdcall getGameResClassId() const { return ApexDestrGameResClassId; }
  virtual unsigned __stdcall getGameResVersion() const { return preferZstdPacking ? 21 : 20; }

  virtual void __stdcall onRegister() {}
  virtual void __stdcall onUnregister() {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override
  {
    files.clear();
    files.push_back() = a.getTargetFilePath();
    if (a.props.getBool("allowProxyMat", false))
      add_proxymat_dep_files(a.getTargetFilePath(), files, a.getMgr());
  }

  virtual bool __stdcall isExportableAsset(DagorAsset &a) { return true; }

  DataBlock appBlkCopy;

  class CB : public Node::NodeEnumCB
  {
  public:
    CB(const char *fn) : fpath(fn) {}
    virtual int node(Node &c)
    {
      if (c.obj && c.obj->isSubOf(OCID_MESHHOLDER) && ((MeshHolderObj *)c.obj)->mesh)
      {
        all_nodes.push_back(&c);

        DataBlock blk;
        dblk::load_text(blk, make_span_const(c.script), dblk::ReadFlag::ROBUST, fpath);
        if (blk.getBool("apex_destructible", false))
          nodes.push_back(&c);
      }
      return 0;
    }

    Tab<Node *> nodes;
    Tab<Node *> all_nodes;
    const char *fpath;
  };


  static IProcessMaterialData *processMat;
  static MaterialData *processMaterial(MaterialData *m) { return processMat ? processMat->processMaterial(m, true) : m; }

  virtual bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log)
  {
#if _TARGET_64BIT
    if (!apex)
      return false;

    fracture::g_global_settings.mMaxChunkSpeed = a.props.getReal("apex_maxChunkSpeed", 300);
    fracture::g_global_settings.mSupportDepth = a.props.getInt("apex_supportDepth", 0);             // 1
    fracture::g_global_settings.mbUseWorldSupport = a.props.getBool("apex_useWorldSupport", false); // true
    fracture::g_global_settings.mbUseFormExtendedStructures = a.props.getBool("apex_useFormExtStruct", true);

    String fpath(a.getTargetFilePath());

    setup_tex_subst(a.props);
    GenericTexSubstProcessMaterialData pm(a.getName(), get_process_mat_blk(a.props, TYPE),
      a.props.getBool("allowProxyMat", false) ? &a.getMgr() : nullptr, &log);
    processMat = &pm; //-V506
    struct OnReturn
    {
      ~OnReturn()
      {
        reset_tex_subst();
        processMat = nullptr;
      }
    } on_return;

    AScene sc;
    if (!load_ascene(fpath, sc, LASF_NOSPLINES | LASF_NOLIGHTS | LASF_MATNAMES, false) || !sc.root)
    {
      log.addMessage(ILogWriter::ERROR, "cannot load destruction geom from %s", fpath);
      return false;
    }
    sc.root->calc_wtm();

    const char *re_excl_mat = a.props.getStr("apex_excludeMatRE", "");
    RegExp re;
    if (*re_excl_mat)
      if (!re.compile(re_excl_mat, "i"))
        re_excl_mat = "";

    CB cb(fpath);
    sc.root->enum_nodes(cb, NULL);

    debug("---------------new apexasset %s (%i nodes)", a.getName(), cb.nodes.size());

    if (!cb.nodes.size())
    {
      cwr.writeInt32e(0);
      debug("no mesh nodes found in  %s", fpath);
      return true;
    }

    return export_as_one_asset(a, cwr, log, cb, re_excl_mat, re, fpath);
#else
    return false;
#endif
  }


  // data
  Tab<mimp::Mesh *> m_array;
  Tab<mimp::SubMesh *> subm;
  Tab<mimp::MeshVertex> v;
  Tab<mimp::MeshMaterial> mat;
  Tab<String *> materials_data;

  void clear_mesh_export_data()
  {
    clear_all_ptr_items(m_array);
    clear_all_ptr_items(subm);
    v.clear();
    mat.clear();
    clear_all_ptr_items(materials_data);

    clear_duplicate_verts_helper();
  }

  // fracture options
  struct NodeFractureOptions
  {
    int num_chunks;
    bool gradually_destructible;
    float density;
    char interior_material[1024];
    float interior_uv_scale;

    bool writeSolid;
    bool useHighQualityPhysx;
    int destructionQuality;

    bool useCutout;
    char cutoutTexName[256];
    bool useCutoutUserDir;
    Point3 cutoutUserDir;

    bool use_noise;
    float noise_amplitude;
    float noise_frequency;
    int noise_grid_size;
  };
  Tab<NodeFractureOptions> nodes_fracture_options;


#define GET_PROP(TYPE, PROP, DEF) nblk.get##TYPE(PROP, a.props.get##TYPE(PROP, DEF))


  // find materials in all nodes
  char *find_material_in_node(Node &node, SimpleString &interior_material)
  {
    // try find material in all nodes
    int i, j;
    for (i = 0; i < node.mat->subMatCount(); i++)
    {
      // find in list
      Ptr<MaterialData> m = node.mat->getSubMat(i);
      m = processMaterial(m);

      if (m->matName == interior_material)
        return get_material_string(m);
    }
    return NULL;
  }

  char *get_interior_material_data_string(SimpleString &interior_material, Node &node, CB &cb)
  {
    // fast search in our node
    char *res = find_material_in_node(node, interior_material);
    if (res != NULL)
      return res;

    // can't find in our node, try find in all other nodes
    int i;
    for (i = 0; i < cb.all_nodes.size(); i++)
    {
      res = find_material_in_node(*cb.all_nodes[i], interior_material);
      if (res != NULL)
        return res;
    }
    debug("could not find material '%s' in dag_node", interior_material.str());
    return NULL;
  }


  void find_material_data_string(SimpleString &interior_mat_str, Node &node, CB &cb)
  {
    char *material_data_string = get_interior_material_data_string(interior_mat_str, node, cb);
    if (material_data_string != NULL)
      interior_mat_str = material_data_string;
  }


  // fracture groups
  bool load_fracture_options(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log, CB &cb, const char *fpath)
  {
    int nodes_count = cb.nodes.size();
    nodes_fracture_options.clear();
    nodes_fracture_options.resize(cb.nodes.size());

    int i, j;
    bool fracture_asset = false;
    for (i = 0; i < nodes_count; i++)
    {
      Node &node = *cb.nodes[i];
      DataBlock nblk;
      dblk::load_text(nblk, make_span_const(node.script), dblk::ReadFlag::ROBUST, fpath);

      // cutout options
      nodes_fracture_options[i].useCutout = false;
      nodes_fracture_options[i].useCutoutUserDir = false;

      const char *cutoutTexName = GET_PROP(Str, "apex_cutout_tex_name", NULL);
      if (cutoutTexName != NULL)
      {
        // check if file exist
        String fileName = String(0, "%s%s.png", cutout_mask_folder.str(), cutoutTexName);

        if (dd_file_exist(fileName.str()))
        {
          nodes_fracture_options[i].useCutout = true;
          strcpy(&nodes_fracture_options[i].cutoutTexName[0], fileName.str());

          if (nblk.paramExists("apex_cutout_dir"))
          {
            nodes_fracture_options[i].useCutoutUserDir = true;
            nodes_fracture_options[i].cutoutUserDir = nblk.getPoint3("apex_cutout_dir");
          }
        }
        else
          debug("cutoutTex not found '%s'", fileName.str());
      }

      nodes_fracture_options[i].num_chunks = GET_PROP(Int, "apex_num_chunks", 100);
      nodes_fracture_options[i].gradually_destructible = GET_PROP(Bool, "apex_gradually_destructible", false);
      nodes_fracture_options[i].useHighQualityPhysx = GET_PROP(Bool, "apex_good_quality", false);
      nodes_fracture_options[i].destructionQuality = GET_PROP(Int, "apex_destruction_quality", 0); // less is better
      nodes_fracture_options[i].density = GET_PROP(Real, "apex_density", 2000);
      nodes_fracture_options[i].interior_uv_scale = GET_PROP(Real, "apex_interior_uv_scale", 1.f);
      SimpleString mat_name(GET_PROP(Str, "apex_interior_material", "broken_walls"));
      find_material_data_string(mat_name, node, cb);
      strcpy(&nodes_fracture_options[i].interior_material[0], mat_name.str());

      nodes_fracture_options[i].use_noise = GET_PROP(Bool, "apex_use_noise", true);
      nodes_fracture_options[i].noise_amplitude = GET_PROP(Real, "apex_noise_amplitude", -1.f); // NOTE: if negative - use default
                                                                                                // values
      nodes_fracture_options[i].noise_frequency = GET_PROP(Real, "apex_noise_frequency", -1.f);
      nodes_fracture_options[i].noise_grid_size = GET_PROP(Int, "apex_noise_grid_size", -1);

      nodes_fracture_options[i].writeSolid = nodes_fracture_options[i].num_chunks == 1 && !nodes_fracture_options[i].useCutout;

      fracture_asset = fracture_asset || (nodes_fracture_options[i].num_chunks != 1);
    }
    return fracture_asset;
  }


  // set fracture options
  void set_destructible_options(int node_i, fracture::FractureDesc &desc)
  {
    int num_chunks;

    desc.mbUseVoronoi = true;
    desc.mFractureVoronoiDesc.siteCount = nodes_fracture_options[node_i].num_chunks;
    desc.mFractureSliceDesc.maxDepth = 1;
    desc.mRandomSeed = 1;
    desc.mInteriorMaterialName = &nodes_fracture_options[node_i].interior_material[0]; //"broken_walls"
    desc.mFractureMaterialDesc.uvScale = physx::PxVec2(1.f, 1.f) * nodes_fracture_options[node_i].interior_uv_scale;
    desc.mMeshProcessingParameters.islandGeneration = true;        // false default
    desc.mMeshProcessingParameters.removeTJunctions = true;        // false default
    desc.mMeshProcessingParameters.microgridSize = microgrid_size; // 64k default
    desc.mMeshProcessingParameters.meshMode = 1;                   // always closed

    fracture::g_global_settings.mDensity = nodes_fracture_options[node_i].density * apex_density_multiplier;
    fracture::g_global_settings.userData = (nodes_fracture_options[node_i].useHighQualityPhysx ? 1 : 0);

    if (nodes_fracture_options[node_i].gradually_destructible)
    {
      fracture::g_global_settings.mbUseFormExtendedStructures = false;
      fracture::g_global_settings.mbUseWorldSupport = true;
      fracture::g_global_settings.mSupportDepth = 1;
    }
    else
    {
      fracture::g_global_settings.mbUseFormExtendedStructures = false;
      fracture::g_global_settings.mbUseWorldSupport = false;
      fracture::g_global_settings.mSupportDepth = 0;
    }
  }

  // vdecl for node
  void get_mesh_vdecl(Node &node, mimp::Mesh *m)
  {
    int node_i;
    m->mVertexFlags = mimp::MIVF_POSITION | mimp::MIVF_NORMAL;

    MeshHolderObj &mh = *(MeshHolderObj *)node.obj;
    if (!mh.mesh)
      return;

    for (int t = 0; t < 4; t++)
      if (mh.mesh->tface[t].size() == mh.mesh->face.size())
      {
        m->mVertexFlags |= mimp::MIVF_TEXEL1 << t;
      }
  }

  void update_mesh_pointers(mimp::MeshSystem &apex_mesh)
  {
    int prev_subm_count = 0;
    int prev_v_count = 0;
    for (int i = 0; i < apex_mesh.mMeshCount; i++)
    {
      apex_mesh.mMeshes[i]->mSubMeshes = &subm[prev_subm_count];
      apex_mesh.mMeshes[i]->mVertices = &unique_verts[prev_v_count];
      prev_subm_count += apex_mesh.mMeshes[i]->mSubMeshCount;
      prev_v_count += apex_mesh.mMeshes[i]->mVertexCount;
    }
  }

  // export logic
  // by default - create 1 asset for 1 dag node
  // but we can group nodes by setting flag apex_fracture_group
  // if for all nodes num_chunks == 1, then we don't need call fracture (nothing to fracture), just export nodes as is
  bool export_as_one_asset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log, CB &cb, const char *re_excl_mat, RegExp &re,
    const char *fpath)
  {
    clear_mesh_export_data();
    load_fracture_options(a, cwr, log, cb, fpath);

    RegExp *re_exclMat = *re_excl_mat ? &re : NULL;
    fracture::FractureDesc desc;

    // clear
    int i;
    int nodes_count = cb.nodes.size();
    mimp::MeshSystem apex_mesh;
    int meshCounter = 0;

    // all nodes with 1 chunk - push to 1 apex asset
    int numNodesWithOneFractureChunks = 0;
    for (i = 0; i < nodes_count; i++)
      if (nodes_fracture_options[i].writeSolid)
        numNodesWithOneFractureChunks++;

    // write num exported nodes & export-options (what additional data we will write)
    int numWritenNodes = numNodesWithOneFractureChunks == 0 ? nodes_count : (nodes_count - numNodesWithOneFractureChunks + 1);
    // ^ +1 here means that all nodes with 1 chunk pushed in 1 special apex asset
    cwr.writeInt16e(numWritenNodes);

    int enabledFeatures = (writeAssetCache ? 1 : 0) | (writeCompressedData ? 2 : 0) | (writeUserDataBlock ? 4 : 0);
    cwr.writeInt16e(enabledFeatures);
    int num_written_assets_counter = 0;

    // export all nodes with 1 fracture chunk (i.e. no fracture)
    if (numNodesWithOneFractureChunks != 0)
    {
      cwr.writeDwString(a.getName());
      apex_mesh.mMeshCount = numNodesWithOneFractureChunks;
      apex_mesh.mMeshes = new mimp::Mesh *[apex_mesh.mMeshCount];
      apex_mesh.mAssetName = "dagorMesh";

      total_unique_verts = 0;
      int special_node_index = -1;

      for (i = 0; i < nodes_count; i++)
      {
        if (!nodes_fracture_options[i].writeSolid)
          continue;

        if (special_node_index == -1)
        {
          special_node_index = i;
          set_destructible_options(i, desc);
        }

        int prev_subm_count = subm.size();
        int prev_v_count = v.size();

        // create multiple meshes
        mimp::Mesh *m = new mimp::Mesh;
        m_array.push_back(m);

        apex_mesh.mMeshes[meshCounter] = m;
        meshCounter++;
        m->mName = "dagormesh";
        get_mesh_vdecl(*cb.nodes[i], m);

        if (!prepare_mesh(a.getName(), apex_mesh, *cb.nodes[i], re_exclMat, m, log))
          return false;

        // fill mesh data
        m->mSubMeshCount = subm.size() - prev_subm_count;
        m->mVertexCount = v.size() - prev_v_count;

        apex_mesh.mAABB.include(m->mAABB.mMin);
        apex_mesh.mAABB.include(m->mAABB.mMax);

        // check if asset meets the requirements
        if (compressVertexAttributes)
        {
          if (apex_mesh.mAABB.mMin[0] < -vertex_value_range || apex_mesh.mAABB.mMin[1] < -vertex_value_range ||
              apex_mesh.mAABB.mMax[0] > vertex_value_range || apex_mesh.mAABB.mMax[1] > vertex_value_range)
            log.addMessage(ILogWriter::ERROR, "asset bounds should be less than %f while using compressed data %s(%s)",
              vertex_value_range, a.getName(), cb.nodes[i]->name);
        }

        remove_duplicate_verts(m, prev_subm_count, prev_v_count);
      }

      apex_mesh.mMaterialCount = mat.size();
      apex_mesh.mMaterials = &mat[0];

      // write
      debug("apex_export_info: %s has %i nodes with 1 fracture chunk", a.getName(), numNodesWithOneFractureChunks);
      update_mesh_pointers(apex_mesh);
      write_apx(apex_mesh, desc, a, cwr, log, re_exclMat, *cb.nodes[special_node_index], nodes_fracture_options[special_node_index],
        false, cb.nodes[special_node_index]->name, num_written_assets_counter);
      num_written_assets_counter++;

      // clear data for other nodes export
      clear_mesh_export_data();
      del_it(apex_mesh.mMeshes);
    }

    // export all other nodes
    total_unique_verts = 0;

    for (i = 0; i < nodes_count; i++)
    {
      if (nodes_fracture_options[i].writeSolid)
        continue;

      // set fracture options
      set_destructible_options(i, desc);

      // for each node - create Mesh
      cwr.writeDwString(cb.nodes[i]->name);

      apex_mesh.mMeshCount = 1;
      apex_mesh.mMeshes = new mimp::Mesh *[apex_mesh.mMeshCount];
      apex_mesh.mAssetName = "dagorMesh";
      meshCounter = 0;
      total_unique_verts = 0;

      int prev_subm_count = subm.size();
      int prev_v_count = v.size();

      // create multiple meshes
      mimp::Mesh *m = new mimp::Mesh;
      m_array.push_back(m);

      apex_mesh.mMeshes[meshCounter] = m;
      meshCounter++;
      m->mName = "dagormesh";
      get_mesh_vdecl(*cb.nodes[i], m);

      if (!prepare_mesh(a.getName(), apex_mesh, *cb.nodes[i], re_exclMat, m, log))
        return false;


      // fill mesh data
      m->mSubMeshCount = subm.size() - prev_subm_count;
      m->mVertexCount = v.size() - prev_v_count;
      apex_mesh.mAABB.include(m->mAABB.mMin);
      apex_mesh.mAABB.include(m->mAABB.mMax);

      // check if asset meets the requirements
      if (compressVertexAttributes)
      {
        if (apex_mesh.mAABB.mMin[0] < -vertex_value_range || apex_mesh.mAABB.mMin[1] < -vertex_value_range ||
            apex_mesh.mAABB.mMax[0] > vertex_value_range || apex_mesh.mAABB.mMax[1] > vertex_value_range)
          log.addMessage(ILogWriter::ERROR, "asset bounds should be less than %f while using compressed data %s(%s)",
            vertex_value_range, a.getName(), cb.nodes[i]->name);
      }

      apex_mesh.mMaterialCount = mat.size();
      apex_mesh.mMaterials = &mat[0];

      remove_duplicate_verts(m, prev_subm_count, prev_v_count);

      // write
      debug("apex_export_info: %s has %i apexDestr nodes, %i subnodes(dips)", a.getName(), cb.nodes.size(), subm.size());
      update_mesh_pointers(apex_mesh);
      write_apx(apex_mesh, desc, a, cwr, log, re_exclMat, *cb.nodes[i], nodes_fracture_options[i], true, cb.nodes[i]->name,
        num_written_assets_counter);
      num_written_assets_counter++;

      // log.addMessage(ILogWriter::WARNING, "'%s' node '%s' done", a.getName(), cb.nodes[i]->name);
      clear_mesh_export_data();
      del_it(apex_mesh.mMeshes);
    }

    return true;
  }


  // write mesh to game pack (or to file)
  bool write_apx(mimp::MeshSystem &src_mesh, fracture::FractureDesc &desc, DagorAsset &a, mkbindump::BinDumpSaveCB &cwr,
    ILogWriter &log, RegExp *re_exclMat, Node &node, NodeFractureOptions &fracture_options, bool fracture_asset = true,
    const char *node_name = "", int node_index = 0)
  {
    fracture::FractureMeshData *m = new fracture::FractureMeshData;
    if (!m->loadMeshSystem(src_mesh, ""))
    {
      log.addMessage(ILogWriter::ERROR, "loadMeshSystem failed");
      delete m;
      return false;
    }

    desc.mSliceParams.resize(desc.mFractureSliceDesc.maxDepth);
    for (PxU32 i = 0; i < desc.mFractureSliceDesc.maxDepth; ++i)
      desc.mSliceParams[i] = desc.mDefaultSliceParams;
    desc.mFractureSliceDesc.sliceParameters = &desc.mSliceParams[0];

    if (desc.mbExportMaterial && (desc.mFractureSliceDesc.useDisplacementMaps || desc.mFractureVoronoiDesc.useDisplacementMaps))
      desc.mbOverrideMaterial = true;
    desc.load(m->mMaterials);


    // build
    physx::apex::NxDestructibleAssetAuthoring *author =
      build_asset(a.getName(), *apex, desc, *m, node, fracture_options, log, fracture_asset);
    delete m;
    freemesh_mem(src_mesh);
    if (!author)
    {
      log.addMessage(ILogWriter::ERROR, "failed to fracture mesh");
      return false;
    }


    // save results
    physx::NxExplicitHierarchicalMesh &mesh = author->getExplicitHierarchicalMesh();
    PX_ASSERT(0 < mesh.submeshCount());

    int compressPosition = compressVertexAttributes ? 1 : 0;
    float compressPositionFactor = compressVertexAttributes ? vertex_value_range : 0;
    int compressNormals = compressVertexAttributes ? 1 : 0;
    int compressUVs = compressVertexAttributes ? 1 : 0;
    float compressUVsFactor = compressVertexAttributes ? uv_value_range : 0;

    physx::NxRenderMeshAssetAuthoring *assetAuthor = fracture::createAssetAuthor(*apex->apexSDK(), mesh, compressPosition,
      compressPositionFactor, compressNormals, compressUVs, compressUVsFactor, 0, desc.mbSmoothNormals);
    PX_ASSERT(assetAuthor);
    if (!assetAuthor)
    {
      apex->apexSDK()->releaseAssetAuthoring(*author);
      log.addMessage(ILogWriter::ERROR, "Unable to create render mesh asset author");
      return false;
    }

    physx::NxRenderMeshAsset *pAsset =
      DYNAMIC_CAST(physx::NxRenderMeshAsset *)(apex->apexSDK()->createAsset(*assetAuthor, "unnamedMesh"));
    PX_ASSERT(pAsset);
    if (!pAsset)
    {
      apex->apexSDK()->releaseAssetAuthoring(*assetAuthor);
      apex->apexSDK()->releaseAssetAuthoring(*author);
      log.addMessage(ILogWriter::ERROR, "Unable to create render mesh asset");
      return false;
    }
    author->setRenderMeshAsset(pAsset);

    fracture::prepareDestructibleAuthorForExport(*author, *assetAuthor, desc.mbCrumbleUsingRuntimeFracture);

    NxParameterized::Serializer *serializer = apex->apexSDK()->createSerializer(NxParameterized::Serializer::NST_BINARY); // NST_XML
                                                                                                                          // NST_BINARY
    PX_ASSERT(serializer);
    if (!serializer)
    {
      apex->apexSDK()->releaseAssetAuthoring(*assetAuthor);
      apex->apexSDK()->releaseAssetAuthoring(*author);
      log.addMessage(ILogWriter::ERROR, "Could not create file serializer");
      return false;
    }

    // create tmp buffer srcCwr which we will pack later with lzma_compress_data()
    mkbindump::BinDumpSaveCB srcCwr(8 << 20, cwr.getTarget(), cwr.WRITE_BE);
    PxFileBufSaveCB fileStream(srcCwr);

    const NxParameterized::Interface *fractureNxParameterized = author->getNxParameterized();
    int res = serializer->serialize(fileStream, &fractureNxParameterized, 1);
    if (res != NxParameterized::Serializer::ERROR_NONE)
      log.addMessage(ILogWriter::ERROR, "Failed to serialize apex data");

    // pack data (from tmp buffer (srcCwr) to output stream)
    cwr.beginBlock();
    cwr.writeInt32e(fileStream.getFileLength()); // write unpacked data size
    MemoryLoadCB mcrd(srcCwr.getMem(), false);
    if (preferZstdPacking)
      zstd_compress_data(cwr.getRawWriter(), mcrd, srcCwr.getSize()); // compress data
    else
      lzma_compress_data(cwr.getRawWriter(), 9, mcrd, srcCwr.getSize()); // compress data
    cwr.endBlock(preferZstdPacking ? btag_compr::ZSTD : btag_compr::UNSPECIFIED);


    // write APX
    if (debug_write_assets_to_file >= 2 || (debug_write_assets_to_file == 1 && fracture_options.useCutout))
    {
      String filename(a.getName());
      filename = debug_output_folder + filename + String(0, "_(%s)", node_name) + String(".apx");
      physx::general_PxIOStream2::PxFileBuf *stream =
        apex->apexSDK()->createStream(filename.str(), physx::general_PxIOStream2::PxFileBuf::OPEN_READ_WRITE_NEW);
      serializer->serialize(*stream, &fractureNxParameterized, 1);
      stream->close();
      stream->release();
    }

    if (writeAssetCache)
    {
      mkbindump::BinDumpSaveCB srcCwrCached(8 << 20, cwr.getTarget(), cwr.WRITE_BE);
      PxFileBufSaveCB fileStreamCached(srcCwrCached);
      String cachedName(120, "%s%i", a.getName(), node_index);
      write_cache(fileStreamCached, author, serializer, cachedName.str(), log);

      cwr.beginBlock();
      cwr.writeInt32e(fileStreamCached.getFileLength()); // write unpacked data size
      MemoryLoadCB mcrd(srcCwrCached.getMem(), false);
      if (preferZstdPacking)
        zstd_compress_data(cwr.getRawWriter(), mcrd, srcCwrCached.getSize());
      else
        lzma_compress_data(cwr.getRawWriter(), 9, mcrd, srcCwrCached.getSize());
      cwr.endBlock(preferZstdPacking ? btag_compr::ZSTD : btag_compr::UNSPECIFIED);
    }

    if (writeUserDataBlock)
    {
      cwr.beginBlock();
      cwr.writeInt32e(user_data_format_version);
      cwr.writeInt32e(fracture_options.destructionQuality);
      cwr.writeInt32e(fracture_options.useCutout ? 1 : 0);
      cwr.endBlock();
    }

    // release
    serializer->release();

    apex->apexSDK()->releaseAssetAuthoring(*author);
    apex->apexSDK()->releaseAssetAuthoring(*assetAuthor);
    return res == NxParameterized::Serializer::ERROR_NONE;
  }


  // test for mesh is watertight
  // count number of triangles for each edge (should be precisely 2 for all edges)
  // bruteforce for now
  struct Edge
  {
    int i1, i2;
    int num_refs;
    void init(int v1_id, int v2_id)
    {
      if (v1_id < v2_id)
      {
        i1 = v1_id;
        i2 = v2_id;
      }
      else
      {
        i2 = v1_id;
        i1 = v2_id;
      }

      num_refs = 1;
    }
  };

  bool check_edge(Edge *edges, int num_edges, int index1, int index2)
  {
    int i1, i2;
    if (index1 < index2)
    {
      i1 = index1;
      i2 = index2;
    }
    else
    {
      i2 = index1;
      i1 = index2;
    }

    int i;
    for (i = 0; i < num_edges; i++)
    {
      if (edges[i].i1 == i1 && edges[i].i2 == i2)
      {
        edges[i].num_refs++;
        return true;
      }
    }

    return false;
  }

  int check_for_mesh_enclosed(Tab<Face> &faces, int numm_tris)
  {
    // each edge must belong precisely to 2 triangles !
    Edge *edges_counter = new Edge[numm_tris * 3]; // max num edges
    int num_edges = 0;

    int i, j, edge_i1, edge_i2;
    for (i = 0; i < numm_tris; i++)
    {
      // stupid pruteforce
      // check if edge exist, if yes - just add ref
      if (!check_edge(edges_counter, num_edges, faces[i].v[0], faces[i].v[1]))
        edges_counter[num_edges++].init(faces[i].v[0], faces[i].v[1]);

      if (!check_edge(edges_counter, num_edges, faces[i].v[1], faces[i].v[2]))
        edges_counter[num_edges++].init(faces[i].v[1], faces[i].v[2]);

      if (!check_edge(edges_counter, num_edges, faces[i].v[2], faces[i].v[0]))
        edges_counter[num_edges++].init(faces[i].v[2], faces[i].v[0]);
    }

    // check counters
    int min_ref = 999999, max_ref = 0, num_incorect_refs = 0;
    for (i = 0; i < num_edges; i++)
    {
      min_ref = min(min_ref, edges_counter[i].num_refs);
      max_ref = max(max_ref, edges_counter[i].num_refs);
      if (edges_counter[i].num_refs != 2)
        num_incorect_refs++;
    }

    delete[] edges_counter;
    edges_counter = NULL;

    return num_incorect_refs;
  }


  bool check_delta_treshold_between_verts(Mesh &mesh)
  {
    // check dist between all verts within triangle
    int i;
    float dist;
    for (i = 0; i < mesh.face.size(); i++)
    {
      dist = (mesh.vert[mesh.face[i].v[0]] - mesh.vert[mesh.face[i].v[1]]).lengthSq();
      if (dist < verts_dist_delta)
        return false;
      dist = (mesh.vert[mesh.face[i].v[1]] - mesh.vert[mesh.face[i].v[2]]).lengthSq();
      if (dist < verts_dist_delta)
        return false;
      dist = (mesh.vert[mesh.face[i].v[2]] - mesh.vert[mesh.face[i].v[0]]).lengthSq();
      if (dist < verts_dist_delta)
        return false;
    }
    return true;
  }


  bool check_cutout_surface_is_plane(Mesh &mesh, int cutout_triangle)
  {
    if (cutout_triangle == -1)
      return true;
// NOTE: this method works just for plane surfaces (and might not work fine in several cases) -> so dont throw errors, just warnings
// check thickness of the mesh
// find backward largest triangle with ~same (inverted) normal
#define GET_FACE_NORMAL(face_i)           \
  p1 = mesh.vert[mesh.face[face_i].v[0]]; \
  p2 = mesh.vert[mesh.face[face_i].v[1]]; \
  p3 = mesh.vert[mesh.face[face_i].v[2]]; \
  nomal = (p2 - p1) % (p3 - p1);

    const float valid_normal = 0.9;
    float len, maxNormalLen = -1.f;
    Point3 p1, p2, p3, nomal, backwardDir;

    GET_FACE_NORMAL(cutout_triangle)
    Point3 dir = -nomal;
    dir.normalize();

    int triIndex = -1;
    for (int i = 0; i < mesh.face.size(); i++)
    {
      GET_FACE_NORMAL(i)
      len = nomal.lengthSq();
      nomal.normalize();

      if (len > maxNormalLen && (nomal * dir > valid_normal))
      {
        maxNormalLen = len;
        backwardDir = nomal;
        triIndex = i;
      }
    }
    if (triIndex == -1)
      return true;

    // find thickness between planes
    float cutoutPlaneD = dir * mesh.vert[mesh.face[cutout_triangle].v[0]];
    float meshThickness = fabs(dir * mesh.vert[mesh.face[triIndex].v[0]] - cutoutPlaneD);

    // loop through all other verts - if dist is much larger than thickness then - error
    float maxThickness = -1.0e9f, thickness;
    for (int i = 0; i < mesh.vert.size(); i++)
    {
      thickness = fabs(dir * mesh.vert[i] - cutoutPlaneD);
      maxThickness = max(maxThickness, thickness);
    }

    if (maxThickness > meshThickness * 1.2f)
      return false;

    return true;
  }

  // optimize verts amount
  Tab<bool> v_exist;                  // false = replaced by one of prev verts
  Tab<int> optimized_verts_indices;   // vocabulary from old vertex buffer to new vertex buffer (in unique_verts[])
  Tab<mimp::MeshVertex> unique_verts; // only unique mesh verts
  int total_unique_verts;

  void clear_duplicate_verts_helper()
  {
    clear_and_shrink(v_exist);
    clear_and_shrink(optimized_verts_indices);
    clear_and_shrink(unique_verts);
    total_unique_verts = 0;
  }

  // for all verts if we find the same -> replace its index
  int find_duplicate_verts(int vert_i, int from, int to, int new_vertex_id)
  {
    int counter = 0;
    for (int j = from; j < to; j++)
    {
      if (v[vert_i] == v[j])
      {
        counter++;
        v_exist[j] = false;
        optimized_verts_indices[j] = new_vertex_id;
      }
    }
    return counter;
  }

  int remove_duplicate_verts(mimp::Mesh *m, int prev_subm_count, int prev_verts_count)
  {
    // algorithm - for take the ith vertex - if there are equal verts with index >i -> 1.mark them 2.replace their index on i
    v_exist.resize(v.size());
    optimized_verts_indices.resize(v.size());
    unique_verts.resize(v.size());

    int numUniqueVerts = 0;

    int i, j;
    for (int i = prev_verts_count; i < v.size(); i++)
      v_exist[i] = true;

    int num_removed_verts = 0;
    for (i = prev_verts_count; i < v.size(); i++)
      if (v_exist[i])
      {
        // if vertex exist - this is unique vertex, try to replace all other verts
        unique_verts[total_unique_verts + numUniqueVerts] = v[i];
        optimized_verts_indices[i] = numUniqueVerts;
        find_duplicate_verts(i, i + 1, v.size(), numUniqueVerts);
        numUniqueVerts++;
      }

    // replace indices
    m->mVertexCount = numUniqueVerts;

    for (i = prev_subm_count; i < subm.size(); i++)
    {
      // recalculate verts indices
      for (j = 0; j < subm[i]->mTriCount * 3; j++)
        subm[i]->mIndices[j] = optimized_verts_indices[prev_verts_count + subm[i]->mIndices[j]];
    }
    total_unique_verts += numUniqueVerts;

    int numSrcVerts = max<int>(v.size() - prev_verts_count, 1);
    debug("duplicate verts removed (%i src verts, %i dst verts, %.2f precents)", numSrcVerts, numUniqueVerts,
      (float)numUniqueVerts * 100.f / (float)numSrcVerts);

    return total_unique_verts;
  }


  //-----------------serialization

  bool write_cache(PxFileBufSaveCB &fileStream, physx::apex::NxDestructibleAssetAuthoring *author,
    NxParameterized::Serializer *serializer, const char *name, ILogWriter &log)
  {
    // init simple physx & apex scene & cache module
    // init cache module
    physx::apex::NxModuleDestructible *destructibleModule = apex->destructibleModule();
    physx::apex::NxApexModuleCachedData *moduleCache =
      apex->apexSDK()->getCachedData().getCacheForModule(destructibleModule->getModuleID());
    if (!moduleCache)
      return false;

    // create physX & apex simple scene
    physx::PxPhysics *physics = apex->physxSDK();
    physx::PxMaterial *material = physics->createMaterial(0.9f, 0.9f, 0.01f);
    physx::PxSceneDesc sceneDesc(physics->getTolerancesScale());
    physx::PxScene *pxscene = physics->createScene(sceneDesc, false);
    if (!pxscene)
      return false;

    physx::apex::NxApexSceneDesc apexSceneDesc;
    apexSceneDesc.scene = pxscene;
    physx::apex::NxApexScene *apexScene = apex->apexSDK()->createScene(apexSceneDesc);
    if (!apexScene)
      return false;


    // get deserialized data
    physx::PxFileBuf *memStream = apex->apexSDK()->createMemoryWriteStream();
    const NxParameterized::Interface *fractureNxParameterized2 = author->getNxParameterized();
    serializer->serialize(*memStream, &fractureNxParameterized2, 1);
    memStream->seekRead(0);
    NxParameterized::Serializer::DeserializedData data;
    serializer->deserialize(*memStream, data);
    NxParameterized::Interface *params = data[0];

    // create asset -> actor -> cache()
    physx::apex::NxDestructibleAsset *asset =
      static_cast<physx::apex::NxDestructibleAsset *>(apex->apexSDK()->createAsset(params, name));
    if (!asset)
    {
      log.addMessage(ILogWriter::ERROR, "caching: could not create apex asset");
      return false;
    }

    NxParameterized::Interface *descParams = asset->getDefaultActorDesc();
    physx::apex::NxApexActor *actor = asset->createApexActor(*descParams, *apexScene);
    if (!actor)
    {
      log.addMessage(ILogWriter::ERROR, "caching: could not create apex actor");
      return false;
    }

    // cache data for this actor
    actor->cacheModuleData();
    moduleCache->getCachedDataForAssetAtScale(*asset, PxVec3(1.0f));
    moduleCache->serializeSingleAsset(*asset, fileStream);

    // output to file
    // String cachedFullFilename = String(120, "D:/cooked_apex_assets/%s.serialized", name);
    // physx::general_PxIOStream2::PxFileBuf* filebuf =
    //   apex->apexSDK()->createStream(cachedFullFilename.str(), physx::general_PxIOStream2::PxFileBuf::OPEN_WRITE_ONLY);
    // moduleCache->serializeSingleAsset(*asset, *filebuf);
    // filebuf->release();

    // release
    actor->release();
    actor = NULL;
    asset->release();
    asset = NULL;
    apex->apexSDK()->releaseScene(apexScene);
    apexScene = NULL;
    pxscene->release();
    pxscene = NULL;
    memStream->close();
    memStream->release();

    return true;
  }


  // prepare triangles, pack to apex structures (vertices, triangles-indices)
  bool prepare_mesh(const char *meshName, mimp::MeshSystem &ms, Node &node, RegExp *re_exclMat, mimp::Mesh *m, ILogWriter &log)
  {
    if (!node.obj || !node.obj->isSubOf(OCID_MESHHOLDER))
      return false;

    MeshHolderObj &mh = *(MeshHolderObj *)node.obj;
    if (!mh.mesh)
      return false;

    int numMat = node.mat->subMatCount();
    mh.mesh->clampMaterials(numMat - 1);
    G_VERIFY(mh.mesh->sort_faces_by_mat());
    if (re_exclMat)
      for (int i = 0, last_mat_face = 0; i < mh.mesh->face.size(); i++)
        if (mh.mesh->face[i].mat != mh.mesh->face[last_mat_face].mat || i + 1 == mh.mesh->face.size())
        {
          if (i + 1 == mh.mesh->face.size())
            i++;
          if (re_exclMat->test(node.mat->list[mh.mesh->face[last_mat_face].mat]->matName))
          {
            mh.mesh->erasefaces(last_mat_face, i - last_mat_face);
            i = last_mat_face;
          }
          last_mat_face = i;
        }

    G_VERIFY(mh.mesh->calc_ngr());
    if (mh.mesh->face.size())
      G_VERIFY(mh.mesh->calc_vertnorms());
    else
      logerr("no faces in mesh");
    mh.mesh->transform(node.wtm);


    // note: mVertexFlags shuold be filled before

    int v_count = v.size();
    int subm_count = subm.size();

    v.resize(v_count + mh.mesh->face.size() * 3);
    mem_set_0(make_span(v).subspan(v_count, mh.mesh->face.size() * 3));
    for (int t = 0; t < 4; t++)
    {
      if (mh.mesh->tface[t].size() == mh.mesh->face.size())
      {
        for (int i = 0; i < mh.mesh->tface[t].size(); i++)
          for (int j = 0; j < 3; j++)
          {
            v[v_count + i * 3 + j].mTexel1[t * 2 + 0] = mh.mesh->tvert[t][mh.mesh->tface[t][i].t[j]].x; //-V557
            v[v_count + i * 3 + j].mTexel1[t * 2 + 1] = mh.mesh->tvert[t][mh.mesh->tface[t][i].t[j]].y; //-V557
          }
      }
      else if (m->mVertexFlags & (mimp::MIVF_TEXEL1 << t))
      {
        // this happens when we try unite DAG meshes with different vdecl in one apex::Mesh which has only 1 vdecl for all submeshes
        // redundancy of data, fill with zeros
        for (int i = 0; i < mh.mesh->face.size() * 3; i++)
        {
          v[v_count + i].mTexel1[t * 2 + 0] = 0.f; //-V557
          v[v_count + i].mTexel1[t * 2 + 1] = 0.f; //-V557
        }
      }
    }


    for (int i = 0, last_mat_face = 0; i < mh.mesh->face.size(); i++)
    {
      for (int j = 0; j < 3; j++)
      {
        v[v_count + i * 3 + j].mPos[0] = mh.mesh->vert[mh.mesh->face[i].v[j]].x;
        v[v_count + i * 3 + j].mPos[1] = mh.mesh->vert[mh.mesh->face[i].v[j]].y;
        v[v_count + i * 3 + j].mPos[2] = mh.mesh->vert[mh.mesh->face[i].v[j]].z;
        m->mAABB.include(v[v_count + i * 3 + j].mPos);
        v[v_count + i * 3 + j].mNormal[0] = mh.mesh->vertnorm[mh.mesh->facengr[i][j]].x;
        v[v_count + i * 3 + j].mNormal[1] = mh.mesh->vertnorm[mh.mesh->facengr[i][j]].y;
        v[v_count + i * 3 + j].mNormal[2] = mh.mesh->vertnorm[mh.mesh->facengr[i][j]].z;
      }

      if (mh.mesh->face[i].mat != mh.mesh->face[last_mat_face].mat || i + 1 == mh.mesh->face.size())
      {
        if (i + 1 == mh.mesh->face.size())
          i++;
        mimp::SubMesh *sm = new mimp::SubMesh;
        subm.push_back(sm);
        sm->mTriCount = i - last_mat_face;
        sm->mIndices = new mimp::MiU32[sm->mTriCount * 3];
        sm->mVertexFlags = m->mVertexFlags;
        for (int j = last_mat_face * 3, idx = 0; j < i * 3; j++, idx++)
        {
          // for all submeshes vertex array is the same
          sm->mIndices[idx] = j;
          sm->mAABB.include(v[v_count + j].mPos);
        }

        // export new material
        mimp::MeshMaterial &mmat = mat.push_back();
        Ptr<MaterialData> new_mat = node.mat->list[mh.mesh->face[last_mat_face].mat];
        new_mat = processMaterial(new_mat);
        mmat.mName = get_material_string(new_mat);
        mmat.mMetaData = "";

        last_mat_face = i;
      }
    }

    int num_error_edges = check_for_mesh_enclosed(mh.mesh->face, mh.mesh->face.size());
    if (num_error_edges != 0)
      log.addMessage(ILogWriter::WARNING, "'%s' node '%s' has %i open edges", meshName, node.name, num_error_edges);

    if (check_verts_dist_delta > 0 && !check_delta_treshold_between_verts(*mh.mesh))
    {
      ILogWriter::MessageType msgType = (check_verts_dist_delta == 1) ? ILogWriter::WARNING : ILogWriter::ERROR;
      log.addMessage(msgType, "'%s' node '%s' has too close verts", meshName, node.name);
    }

    for (int i = subm_count; i < subm.size(); i++)
    {
      subm[i]->mMaterial = &mat[i];
      subm[i]->mMaterialName = mat[i].mName;
    }

    return true;
  }


  // shader name may be replaced according to the list from application.blk
  const char *get_remaped_class_name(const char *className)
  {
    const DataBlock *ri_blk = appBlkCopy.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("rendInst");
    const DataBlock *remap_blk = ri_blk->getBlockByName("remapShaders");
    if (!remap_blk)
      return className;

    for (int i = 0; i < remap_blk->paramCount(); i++)
      if (remap_blk->getParamType(i) == DataBlock::TYPE_STRING)
      {
        const char *paramName = remap_blk->getParamName(i);
        if (strcmp(paramName, className) == 0)
        {
          // debug("remap shader %s on %s", className, remap_blk->getStr(i));
          return remap_blk->getStr(i);
        }
      }

    return className;
  }


  // pack all material data to string
#define COLOR4_TO_POINT4(color) (Point4(color.r, color.g, color.b, color.a))
  char *get_material_string(MaterialData *m)
  {
    // write all write all material info instead just material name
    // write: name, shader, shader script, textures, etc.)
    DataBlock blk;
    blk.setStr("matName", m->matName);
    blk.setStr("className", get_remaped_class_name(m->className));
    blk.setStr("matScript", m->matScript);
    blk.setInt("flags", m->flags);
    blk.setPoint4("mat_diff", COLOR4_TO_POINT4(m->mat.diff));
    blk.setPoint4("mat_amb", COLOR4_TO_POINT4(m->mat.amb));
    blk.setPoint4("mat_spec", COLOR4_TO_POINT4(m->mat.spec));
    blk.setPoint4("mat_emis", COLOR4_TO_POINT4(m->mat.emis));
    blk.setReal("mat_power", m->mat.power);

    char tex_short_name[DAGOR_MAX_PATH];
    for (int k = 0; k < MAXMATTEXNUM; k++)
    {
      const char *tex_name = get_managed_texture_name(m->mtex[k]);
      if (tex_name != NULL)
      {
        // compile short name from file full path
        strcpy(tex_short_name, tex_name);
        dd_simplify_fname_c(tex_short_name);

        char *short_name = strrchr(tex_short_name, '/');
        if (short_name == NULL)
          short_name = &tex_short_name[0];
        else
          short_name++;

        char *endof_name = strchr(short_name, '*');
        if (!endof_name)
          endof_name = strchr(short_name, '.');
        if (endof_name != NULL)
          *endof_name = '\0';

        // tex name 't'+id
        char ti_name[4];
        sprintf(&ti_name[0], "%s%i", "t", k);

        // set name
        blk.setStr(ti_name, short_name);
      }
    }

    DynamicMemGeneralSaveCB cwr(tmpmem, 0, 4 << 10);
    blk.saveToTextStream(cwr);
    String *new_str = new String(0, "%.*s", cwr.size(), cwr.data());
    new_str->replace("\r\n", ";");

    materials_data.push_back(new_str);

    return new_str->str();
  }


  // free mem
  void freemesh_mem(mimp::MeshSystem &ms)
  {
    for (int j = 0; j < ms.mMeshCount; j++)
    {
      for (int i = 0; i < ms.mMeshes[j]->mSubMeshCount; i++)
      {
        delete[] ms.mMeshes[j]->mSubMeshes[i]->mIndices;
        del_it(ms.mMeshes[j]->mSubMeshes[i]);
      }
      erase_item_by_value(m_array, ms.mMeshes[j]);
      del_it(ms.mMeshes[j]);
    }
    ms.mMeshCount = 0;

    clear_mesh_export_data();
  }


  // cutout
  // cutout texture might be maped along not 0 mapChannel -> this depends on shader (read option from application.blk)
  int get_cutout_shader_map_channel(const char *className)
  {
    const DataBlock *cutoutUvChannelBlk =
      appBlkCopy.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("apex")->getBlockByName("cutoutUvChannel");
    if (!cutoutUvChannelBlk)
      return 0;

    for (int i = 0; i < cutoutUvChannelBlk->paramCount(); i++)
      if (cutoutUvChannelBlk->getParamType(i) == DataBlock::TYPE_INT)
      {
        const char *paramName = cutoutUvChannelBlk->getParamName(i);
        if (strcmp(paramName, className) == 0)
          return cutoutUvChannelBlk->getInt(i);
      }

    return 0;
  }

  // get cutout dir along which project texture on mesh
  Point3 get_cutout_mapping(const char *meshName, Node &node, Point3 userDefinedCutoutDir, bool use_existing_dir, int &tri_index,
    int &uvChannel, ILogWriter &log)
  {
    // simple - find largest (non normalized) normal len (save ins normal, triangle index & uvChannel according to shader type)
    Point3 cutoutDir = use_existing_dir ? userDefinedCutoutDir : Point3(0, 1, 0);
    tri_index = -1;
    uvChannel = 0;

    if (!node.obj || !node.obj->isSubOf(OCID_MESHHOLDER))
      return cutoutDir;

    MeshHolderObj &mh = *(MeshHolderObj *)node.obj;
    if (!mh.mesh || mh.mesh->face.size() == 0)
      return cutoutDir;

    float len, maxLen = -1.f;
    Point3 p1, p2, p3, nomal;
    for (int i = 0; i < mh.mesh->face.size(); i++)
    {
      p1 = mh.mesh->vert[mh.mesh->face[i].v[0]];
      p2 = mh.mesh->vert[mh.mesh->face[i].v[1]];
      p3 = mh.mesh->vert[mh.mesh->face[i].v[2]];

      nomal = (p2 - p1) % (p3 - p1);
      len = nomal.lengthSq();

      if (use_existing_dir)
        len *= fabs(nomal * userDefinedCutoutDir); // weight according to userDefinedCutoutDir

      if (len > maxLen)
      {
        maxLen = len;
        if (!use_existing_dir)
          cutoutDir = nomal;
        tri_index = i;
      }
    }

    cutoutDir.normalize();
    debug("selected cutout dir: %f %f %f", cutoutDir.x, cutoutDir.y, cutoutDir.z);

    // change direction if it is negative
    if ((fabs(cutoutDir.x) > fabs(cutoutDir.y) && fabs(cutoutDir.x) > fabs(cutoutDir.z) && cutoutDir.x < 0.f) ||
        (fabs(cutoutDir.y) > fabs(cutoutDir.x) && fabs(cutoutDir.y) > fabs(cutoutDir.z) && cutoutDir.y < 0.f) ||
        (fabs(cutoutDir.z) > fabs(cutoutDir.x) && fabs(cutoutDir.z) > fabs(cutoutDir.y) && cutoutDir.z < 0.f))
      cutoutDir = -cutoutDir;

    // get correct uv channel for this material.shader
    int matId = mh.mesh->face[tri_index].mat;
    Ptr<MaterialData> m = node.mat->getSubMat(matId);
    m = processMaterial(m);
    uvChannel = get_cutout_shader_map_channel(m->className);

    if (check_cutout_mesh_alike_plane > 0 && !check_cutout_surface_is_plane(*mh.mesh, tri_index))
    {
      ILogWriter::MessageType msgType = (check_cutout_mesh_alike_plane == 1) ? ILogWriter::WARNING : ILogWriter::ERROR;
      log.addMessage(msgType, "'%s' node '%s' in not a plane - use just plane-like surfaces for cutout", meshName, node.name);
    }

    return cutoutDir;
  }


  // find cutout matrix
  // we need to find texture projection direction & tiling
  // we find most suitable triangle which gieves us its notmal (texture projection direction & vertices-alows us calculate projection
  // tiling)
  void get_triangle_cutout_mapping(const char *meshName, NxDestructibleAssetAuthoring *author, Node &node, physx::PxMat33 &theMapping,
    physx::PxVec3 &userDefinedDirection, NodeFractureOptions &fracture_options, ILogWriter &log)
  {
    int triIndex = -1;
    int uvChannel = 0;
    Point3 cutoutDir = Point3(0.f, 1.f, 0.f);
    if (fracture_options.useCutoutUserDir)
      cutoutDir = get_cutout_mapping(meshName, node, fracture_options.cutoutUserDir, true, triIndex, uvChannel, log);
    else
      cutoutDir = get_cutout_mapping(meshName, node, cutoutDir, false, triIndex, uvChannel, log);
    userDefinedDirection = physx::PxVec3(cutoutDir.x, cutoutDir.y, cutoutDir.z);

    if (triIndex == -1)
    {
      // NOTE: uv from 1st channel will be taken into account
      author->calculateCutoutUVMapping(theMapping, userDefinedDirection);
    }
    else
    {
      MeshHolderObj &mh = *(MeshHolderObj *)node.obj;
      physx::apex::NxExplicitRenderTriangle triangle;
      for (int j = 0; j < 3; j++)
      {
        Point3 &p1 = mh.mesh->vert[mh.mesh->face[triIndex].v[j]];
        Point2 &uv1 = mh.mesh->tvert[uvChannel][mh.mesh->tface[uvChannel][triIndex].t[j]];
        triangle.vertices[j].position = physx::PxVec3(p1.x, p1.y, p1.z);
        triangle.vertices[j].uv[0] = physx::apex::NxVertexUV(uv1.x, uv1.y);
      }
      author->calculateCutoutUVMapping(theMapping, triangle);
    }
  }

  // if mesh is too thin - this may cause serious visual bugs
  // mostly cutout surfaces are thin -> so make this only for cutout for now
  unsigned get_suitable_micro_grid_size(Node &node, physx::PxVec3 &dir)
  {
    if (!node.obj || !node.obj->isSubOf(OCID_MESHHOLDER))
      return 100;

    MeshHolderObj &mh = *(MeshHolderObj *)node.obj;
    if (!mh.mesh || mh.mesh->face.size() == 0)
      return 100;

    Point3 cutout_dir = Point3(dir.x, dir.y, dir.z);
    float minD = 9999999;
    float maxD = -9999999;
    float d;
    for (int i = 0; i < mh.mesh->vert.size(); i++)
    {
      d = mh.mesh->vert[i] * cutout_dir;
      minD = min(minD, d);
      maxD = max(maxD, d);
    }

    float meshThickness = maxD - minD;
    const float suitableThickness = 0.1f;
    const float minRequiredThickness = 0.02f;
    float thicknessKoef = clamp((meshThickness - minRequiredThickness) / (suitableThickness - minRequiredThickness), 0.f, 1.f);
    unsigned requiredMicrogridSize = lerp(256, 100, thicknessKoef);

    debug("meshThickness %f requiredGridSize %i", meshThickness, requiredMicrogridSize);

    return requiredMicrogridSize;
  }

  // fracture mesh
  class Listener : public physx::apex::IProgressListener
  {
  public:
    Listener() {}
    virtual ~Listener() {}
    virtual void setProgress(int, const char *) {}
  };

  bool fracture_cutout(const char *meshName, NxDestructibleAssetAuthoring *author, fracture::FractureDesc &desc,
    physx::NxCollisionDesc &collisionDesc, Node &node, NodeFractureOptions &fracture_options, ILogWriter &log)
  {
    TexImage32 *img = ::load_image(&fracture_options.cutoutTexName[0], tmpmem);
    if (img == NULL)
    {
      debug("cutout tex does not exist: use voronoi instead");
      return false;
    }

    if (img->w > cutout_max_texture_size || img->h > cutout_max_texture_size)
      log.addMessage(ILogWriter::ERROR, "too large cutout texture: %s", &fracture_options.cutoutTexName[0]);

    // apex uses only rgb texture
    TexPixel32 *pixels = img->getPixels();
    unsigned char *pixelBuffer = new unsigned char[img->w * img->h * 3];
    int i, j;
    for (i = 0; i < img->w; i++)
      for (j = 0; j < img->h; j++)
      {
        // to do: check texture format
        pixelBuffer[(i + j * img->w) * 3 + 0] = pixels[i + j * img->w].r;
        pixelBuffer[(i + j * img->w) * 3 + 1] = pixels[i + j * img->w].r;
        pixelBuffer[(i + j * img->w) * 3 + 2] = pixels[i + j * img->w].r;
      }

    NxFractureCutoutDesc cutoutDesc;
    cutoutDesc.directions = NxFractureCutoutDesc::UserDefined;
    physx::PxMat33 theMapping;
    get_triangle_cutout_mapping(meshName, author, node, theMapping, cutoutDesc.userDefinedDirection, fracture_options, log);
    desc.mMeshProcessingParameters.microgridSize = get_suitable_micro_grid_size(node, cutoutDesc.userDefinedDirection);
    desc.mMeshProcessingParameters.meshMode = 1; // always closed mesh
    cutoutDesc.userUVMapping = theMapping;
    cutoutDesc.tileFractureMap = true;
    cutoutDesc.userDefinedCutoutParameters.depth = 0.f;
    cutoutDesc.cutoutSizeX = img->w;
    cutoutDesc.cutoutSizeY = img->h;

    physx::PxF32 snapThreshold = 4.f;
    bool periodic = false;
    author->buildCutoutSet(pixelBuffer, img->w, img->h, snapThreshold, periodic);

    Listener listener;
    return author->createChippedMesh(desc.mMeshProcessingParameters, cutoutDesc, author->getCutoutSet(), desc.mFractureSliceDesc,
      desc.mFractureVoronoiDesc, collisionDesc, desc.mRandomSeed, listener, NULL);
  }


  bool fracture_voronoi(NxDestructibleAssetAuthoring *author, fracture::FractureDesc &desc, physx::NxCollisionDesc &collisionDesc,
    NodeFractureOptions &fracture_options)
  {
    Listener listener;
    dag::Vector<PxVec3> voronoiSites(desc.mFractureVoronoiDesc.siteCount);
    dag::Vector<PxU32> siteChunkIndices(desc.mFractureVoronoiDesc.siteCount);
    author->createVoronoiSitesInsideMesh(&voronoiSites[0], &siteChunkIndices[0], desc.mFractureVoronoiDesc.siteCount,
      &desc.mRandomSeed, &desc.mMeshProcessingParameters.microgridSize, desc.mMeshProcessingParameters.meshMode, listener);

    desc.mFractureVoronoiDesc.sites = &voronoiSites[0];
    desc.mFractureVoronoiDesc.chunkIndices = &siteChunkIndices[0];
    NxNoiseParameters noise;
    noise.setToDefault();

    if (use_noise && fracture_options.use_noise)
    {
      noise.amplitude = fracture_options.noise_amplitude < 0.f ? default_noise_amplitude : fracture_options.noise_amplitude;
      noise.frequency = fracture_options.noise_frequency < 0.f ? default_noise_frequency : fracture_options.noise_frequency;
      noise.gridSize = fracture_options.noise_grid_size < 0 ? default_noise_grid_size : fracture_options.noise_grid_size;
    }
    desc.mFractureVoronoiDesc.faceNoise = noise;

    return author->createVoronoiSplitMesh(desc.mMeshProcessingParameters, desc.mFractureVoronoiDesc, collisionDesc,
      desc.mbExportCoreMesh, desc.mbApplyCoreMeshMaterialToNeighborChunks, desc.mRandomSeed, listener, NULL);
  }

  bool fracture_slices(NxDestructibleAssetAuthoring *author, fracture::FractureDesc &desc, physx::NxCollisionDesc &collisionDesc)
  {
    Listener listener;
    return author->createHierarchicallySplitMesh(desc.mMeshProcessingParameters, desc.mFractureSliceDesc, collisionDesc,
      desc.mbExportCoreMesh, desc.mbApplyCoreMeshMaterialToNeighborChunks, desc.mRandomSeed, listener, NULL);
  }


  bool fracture(const char *meshName, fracture::FractureApex &apex, NxDestructibleAssetAuthoring *author, fracture::FractureDesc &desc,
    physx::NxCollisionDesc &collisionDesc, Node &node, NodeFractureOptions &fracture_options, ILogWriter &log)
  {
    prepareDestructibleAuthorForFracture(desc, *author);

    bool bSuccess = false;

    if (fracture_options.useCutout)
    {
      bSuccess = fracture_cutout(meshName, author, desc, collisionDesc, node, fracture_options, log);
      if (!bSuccess)
      {
        desc.mbUseVoronoi = true;
        bSuccess = fracture_voronoi(author, desc, collisionDesc, fracture_options);
      }
    }
    else
    {
      if (desc.mbUseVoronoi)
        bSuccess = fracture_voronoi(author, desc, collisionDesc, fracture_options);
      else
        bSuccess = fracture_slices(author, desc, collisionDesc);
    }

    if (!bSuccess)
    {
      logerr("Unable to create hierarchically split mesh");
      apex.apexSDK()->releaseAssetAuthoring(*author);
      return false;
    }
    return true;
  }

  // create apex asset
  NxDestructibleAssetAuthoring *build_asset(const char *meshName, fracture::FractureApex &apex, fracture::FractureDesc &desc,
    fracture::FractureMeshData &data, Node &node, NodeFractureOptions &fracture_options, ILogWriter &log, bool fracture_asset = true)
  {
    NxDestructibleAssetAuthoring *author =
      DYNAMIC_CAST(NxDestructibleAssetAuthoring *)(apex.apexSDK()->createAssetAuthoring(NX_DESTRUCTIBLE_AUTHORING_TYPE_NAME));

    if (!author)
    {
      logerr("Unable to create destructible author");
      return NULL;
    }

    debug("apex_build_asset: mTriangles %i mSubmeshData %i mPartitions %i", data.mTriangles.size(), data.mSubmeshData.size(),
      data.mPartitions.size());

    if (!author->setRootMesh(data.mTriangles.size() ? &data.mTriangles[0] : nullptr, (PxU32)data.mTriangles.size(),
          data.mSubmeshData.size() ? &data.mSubmeshData[0] : nullptr, (PxU32)data.mSubmeshData.size(),
          data.mPartitions.size() ? &data.mPartitions[0] : nullptr, (PxU32)data.mPartitions.size()))
    {
      logerr("Error: Unable to set destructible author's root mesh");
      apex.apexSDK()->releaseAssetAuthoring(*author);
      return NULL;
    }

    author->getExplicitHierarchicalMesh().calculateMeshBSP(0x11223344, NULL, &desc.mMeshProcessingParameters.microgridSize,
      desc.mMeshProcessingParameters.meshMode);

    // Now using per-depth volume descs.  For now we'll just copy the volume desc to each depth.
    physx::NxCollisionVolumeDesc volumeDescs[5] = {
      desc.mVolumeDesc, desc.mVolumeDesc, desc.mVolumeDesc, desc.mVolumeDesc, desc.mVolumeDesc};
    physx::NxCollisionDesc collisionDesc;
    collisionDesc.mDepthCount = 2;
    collisionDesc.mVolumeDescs = volumeDescs;

    if (fracture_asset)
    {
      if (!fracture(meshName, apex, author, desc, collisionDesc, node, fracture_options, log))
        return NULL;
    }

    author->getExplicitHierarchicalMesh().buildCollisionGeometryForRootChunkParts(collisionDesc);
    return author;
  }
};

IProcessMaterialData *ApexDestrExp::processMat = nullptr;


// plugin
class ApexDestrExporterPlugin : public IDaBuildPlugin
{
public:
  virtual bool __stdcall init(const DataBlock &appblk)
  {
#if _TARGET_64BIT
    const char *appDir = appblk.getStr("appDir", ".");
    const char *assetsDir = appblk.getBlockByNameEx("assets")->getStr("base", ".");
    const DataBlock *apexOptionsBlk = appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("apex");
    const char *apexMaskDir = apexOptionsBlk->getStr("mask_dir", ".");
    apex_density_multiplier = apexOptionsBlk->getReal("density_multiplier", 10.f);
    cutout_mask_folder = String(260, "%s%s%s", appDir, assetsDir, apexMaskDir);
    cutout_max_texture_size = apexOptionsBlk->getInt("cutout_max_texture_size", 128);
    verts_dist_delta = apexOptionsBlk->getReal("verts_dist_delta", 0.01f);
    verts_dist_delta *= verts_dist_delta; // sq - just not to calc sqrt during dist calculations
    check_verts_dist_delta = apexOptionsBlk->getInt("check_verts_dist_delta", 1);
    check_cutout_mesh_alike_plane = apexOptionsBlk->getInt("check_cutout_mesh_alike_plane", 1);

    debug_write_assets_to_file = apexOptionsBlk->getInt("debug_write_assets_to_file", 0);
    compressVertexAttributes = debug_write_assets_to_file == 0; // NOTE: apex-asset-viewer don't understand compressed vertex data, we
                                                                // might disable it for debugging purposes (when we write assets to
                                                                // *.apx files)
    if (!compressVertexAttributes)
      debug("APEX WARNING: disabled compressVertexAttributes !");
    debug_output_folder = String(apexOptionsBlk->getStr("debug_output_folder", "D:/apex_assets/"));

    microgrid_size = apexOptionsBlk->getInt("microgrid_size", 2048);

    use_noise = apexOptionsBlk->getBool("use_noise", true);
    default_noise_amplitude = apexOptionsBlk->getReal("noise_amplitude", 0.01f);
    default_noise_frequency = apexOptionsBlk->getReal("noise_frequency", 0.25f);
    default_noise_grid_size = apexOptionsBlk->getInt("noise_grid_size", 5);

    uv_value_range = apexOptionsBlk->getReal("uv_value_range", 8.f);
    vertex_value_range = apexOptionsBlk->getReal("vertex_value_range", 50.f);


    ::register_png_tex_load_factory();
    ::register_avif_tex_load_factory();
    if (bool zstd = appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBool("preferZSTD", false))
    {
      preferZstdPacking = zstd; //-V547
      debug("apexExp prefers ZSTD");
    }

    apex = new fracture::FractureApex;
    if (apex->initialize())
      debug("init apex: %p", &apex);
    else
      del_it(apex);
    exp.appBlkCopy.setFrom(&appblk);
    return apex != NULL;
#else
    apex = NULL;
    debug("32bit version of ApexDestrExporterPlugin is disabled as it requires > 2gb memory, run 64bit version");
    return true;
#endif
  }
  virtual void __stdcall destroy()
  {
#if _TARGET_64BIT
    del_it(apex);
#endif
    delete this;
  }

  virtual int __stdcall getExpCount() { return 1; }
  virtual const char *__stdcall getExpType(int idx) { return TYPE; }
  virtual IDagorAssetExporter *__stdcall getExp(int idx) { return &exp; }

  virtual int __stdcall getRefProvCount() { return 0; }
  virtual const char *__stdcall getRefProvType(int idx) { return NULL; }
  virtual IDagorAssetRefProvider *__stdcall getRefProv(int idx) { return NULL; }

protected:
  ApexDestrExp exp;
};

String validate_texture_types(const char *tex_name, const char *class_name, int slot, DagorAsset &a) { return {}; }

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) ApexDestrExporterPlugin; }
END_DABUILD_PLUGIN_NAMESPACE(apex)
REGISTER_DABUILD_PLUGIN(apex, nullptr)
