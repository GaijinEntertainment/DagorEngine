// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef __GAIJIN_NSB_LIGHTMAPPEDSCENE_H__
#define __GAIJIN_NSB_LIGHTMAPPEDSCENE_H__
#pragma once


#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/util/dagUuid.h>
#include <libTools/util/progressInd.h>
#include <util/dag_simpleString.h>
#include <math/integer/dag_IPoint2.h>
#include <sceneBuilder/nsb_decl.h>

// forward declarations for external classes
class IGenSave;
class IGenLoad;
class ILogWriter;
class IslLightObject;
class DataBlock;
class ILogWriter;


namespace StaticSceneBuilder
{
extern bool check_degenerates, check_no_smoothing;

class Splitter;

struct IslLightObjList
{
  dag::Span<IslLightObject *> lightsList;
  dag::Span<const char *> lightNamesList;
};


class LightmappedScene : public StaticGeometryMaterialSaveCB, public StaticGeometryMaterialLoadCB
{
public:
  static const int BAD_LMID = 0xFFFF;

  static const int VERSION_NUMBER = 4;

  struct VertNormPair
  {
    int v, n;
  };


  struct GiFace
  {
    uint32_t v[3];
    uint32_t mat;
  };


  struct Object
  {
    SimpleString name;

    int flags;
    int vssDivCount;

    PtrTab<StaticGeometryMaterial> mats;

    Mesh mesh;

    Tab<Point2> lmTc;
    Tab<TFace> lmFaces;
    Tab<uint16_t> lmIds;

    int vlmIndex, vlmSize;
    Tab<TFace> vltmapFaces;

    real lmSampleSize;

    real visRange;
    int transSortPriority;

    int topLodObjId;
    real lodRange;

    real lodNearVisRange, lodFarVisRange;
    int unwrapScheme;

    Object();

    void initMesh(const TMatrix &wtm, int node_flags, const Point3 &normals_dir);

    void save(IGenSave &cb, LightmappedScene &scene);

    void load(IGenLoad &cb, LightmappedScene &scene, int version);
  };


  struct Lightmap
  {
    IPoint2 size;
    IPoint2 offset;


    Lightmap() : size(0, 0), offset(0, 0) {}

    void save(IGenSave &cb);

    void load(IGenLoad &cb);
  };

  DagUUID uuid; // to bind with LTi3 and LTo2

  PtrTab<StaticGeometryTexture> textures, giTextures;
  PtrTab<StaticGeometryMaterial> materials;
  Tab<Object> objects;

  Tab<Lightmap> lightmaps;
  int vltmapSize;
  IPoint2 totalLmSize;

  real lmSampleSizeScale;


  int totalFaces;


  LightmappedScene();


  void clear();

  int getTextureId(StaticGeometryTexture *tex);
  int addTexture(StaticGeometryTexture *tex);

  int getGiTextureId(StaticGeometryTexture *tex);
  int addGiTexture(StaticGeometryTexture *tex);

  int getMaterialId(StaticGeometryMaterial *mat);
  int addMaterial(StaticGeometryMaterial *mat);

  void addObject(Mesh &mesh, PtrTab<StaticGeometryMaterial> &mats, const TMatrix &wtm, const char *name, real lm_sample_size,
    int flags, int unwrap_scheme, const Point3 &normals_dir, int use_vss, int vss_div_count, real visrange, int trans_sort_priority,
    int top_lod_obj_id, real lod_range, real lod_near_range, real lod_far_range);

  void addGeom(dag::ConstSpan<StaticGeometryNode *> geom, ILogWriter *log = NULL);

  bool calcLightmapMapping(int lm_size, int target_lm_num, float pack_fidelity_ratio, IGenericProgressIndicator &pbar,
    float &lm_scale);

  void makeSingleLightmap();

  virtual int getTextureIndex(StaticGeometryTexture *texture);
  virtual StaticGeometryTexture *getTexture(int index);

  void save(IGenSave &cb, IGenericProgressIndicator &pbar);
  bool save(const char *filename, IGenericProgressIndicator &pbar);

  void load(IGenLoad &cb, IGenericProgressIndicator &pbar);
  bool load(const char *filename, IGenericProgressIndicator &pbar);


private:
  bool checkGeometry(const StaticGeometryNode &node, ILogWriter *log);
};

enum
{
  SCENE_FORMAT_LdrTga,
  SCENE_FORMAT_LdrDds,
  SCENE_FORMAT_LdrTgaDds,
  SCENE_FORMAT_SunBump,
  SCENE_FORMAT_AO_DXT1,
};

// routines for scene linkage
bool build_and_save_scene1(LightmappedScene &scene, LightingProvider &lighting, StdTonemapper &tonemapper, Splitter &splitter,
  const char *foldername, int scene_format, unsigned target_code, IGenericProgressIndicator &pbar, ILogWriter &log,
  bool should_make_reference);
bool build_and_save_scene(LightmappedScene &scene, LightingProvider &lighting, StdTonemapper &tonemapper, Splitter &splitter,
  const char *foldername, int scene_format, dag::Span<unsigned> add_target_code, IGenericProgressIndicator &pbar, ILogWriter &log,
  bool should_make_reference);

bool build_scene1(IGenLoad &in_lms1, IGenLoad &in_lto1, IGenLoad *in_ldiro1, const DataBlock &in_tone, const DataBlock &in_split,
  const char *out_scn_folder, int scene_format, unsigned tgt_code, bool can_ignore_task_uuid, int blur_kernel_sz, float blur_sigma,
  bool dont_crop_lightmaps, bool make_reference, ILogWriter &log, IGenericProgressIndicator &pbar);
bool build_scene(IGenLoad &in_lms1, IGenLoad &in_lto1, IGenLoad *in_ldiro1, const DataBlock &in_tone, const DataBlock &in_split,
  const char *out_scn_folder, int scene_format, dag::Span<unsigned> add_tgt_code, bool can_ignore_task_uuid, int blur_kernel_sz,
  float blur_sigma, bool dont_crop_lightmaps, bool make_reference, ILogWriter &log, IGenericProgressIndicator &pbar);

}; // namespace StaticSceneBuilder


#endif
