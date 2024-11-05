// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_color.h>
#include <math/dag_bounds3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_mesh.h>

#include <generic/dag_ptrTab.h>

#include <util/dag_simpleString.h>

#include <ioSys/dag_dataBlock.h>


// forward declarations for external classes
class IGenSave;
class IGenLoad;
class MaterialData;
class String;


void split_two_sided(Mesh &m, const PtrTab<class StaticGeometryMaterial> &mats);


class StaticGeometryTexture : public DObject
{
public:
  SimpleString fileName;

  StaticGeometryTexture(const char *fn) : fileName(fn) {}
};


class StaticGeometryMaterialSaveCB
{
public:
  virtual int getTextureIndex(StaticGeometryTexture *texture) = 0;
};


class StaticGeometryMaterialLoadCB
{
public:
  virtual StaticGeometryTexture *getTexture(int index) = 0;
};


class StaticGeometryMaterial : public DObject
{
public:
  static const int NUM_TEXTURES = 16;

  SimpleString name, className, scriptText;

  Color4 amb, diff, spec, emis;
  real power;

  Ptr<StaticGeometryTexture> textures[NUM_TEXTURES];

  Color4 trans;
  int atest;
  real cosPower;

  int flags; // see tools/matFlags.h


  StaticGeometryMaterial() : flags(0), atest(0), cosPower(1)
  {
    amb = diff = Color4(1, 1, 1, 1);
    trans = spec = emis = Color4(0, 0, 0, 0);
    power = 0;
  }

  StaticGeometryMaterial(const StaticGeometryMaterial &from) :
    name(from.name),
    className(from.className),
    scriptText(from.scriptText),
    amb(from.amb),
    diff(from.diff),
    spec(from.spec),
    emis(from.emis),
    power(from.power),
    trans(from.trans),
    atest(from.atest),
    cosPower(from.cosPower),
    flags(from.flags)
  {
    for (int i = 0; i < NUM_TEXTURES; ++i)
      textures[i] = from.textures[i];
  }

  // without name comparison
  bool equal(const StaticGeometryMaterial &mat) const
  {
    bool isEqual = className == mat.className && scriptText == mat.scriptText && amb == mat.amb && diff == mat.diff &&
                   spec == mat.spec && emis == mat.emis && power == mat.power && trans == mat.trans && atest == mat.atest &&
                   cosPower == mat.cosPower && flags == mat.flags;

    if (isEqual)
    {
      for (int i = 0; i < NUM_TEXTURES; ++i)
        if (textures[i] != mat.textures[i])
        {
          if (textures[i] && mat.textures[i] && !stricmp(textures[i]->fileName, mat.textures[i]->fileName))
            continue;

          return false;
        }

      return true;
    }

    return false;
  }

  bool operator==(const StaticGeometryMaterial &mat) const
  {
    if (equal(mat))
      return name == mat.name;

    return false;
  }

  bool operator!=(const StaticGeometryMaterial &mat) const { return !operator==(mat); }

  void save(IGenSave &cb, StaticGeometryMaterialSaveCB &texcb);

  void load(IGenLoad &cb, StaticGeometryMaterialLoadCB &texcb);
};


class StaticGeometryMesh : public DObject
{
public:
  Mesh mesh;
  PtrTab<StaticGeometryMaterial> mats;
  void *user_data;


  StaticGeometryMesh(IMemAlloc *mem = tmpmem) : mats(mem), user_data(NULL) {}
};


class StaticGeometryNode
{
public:
  Ptr<StaticGeometryMesh> mesh;
  TMatrix wtm;
  BSphere3 boundSphere;
  BBox3 boundBox;

  SimpleString name;

  DataBlock script;
  SimpleString nonBlkScript;

  real visRange;
  int transSortPriority;

  SimpleString topLodName;
  real lodRange;

  SimpleString linkedResource;

  real lodNearVisRange, lodFarVisRange;
  int unwrapScheme;

  enum
  {
    FLG_RENDERABLE = 1 << 0,
    FLG_COLLIDABLE = 1 << 1,
    FLG_CASTSHADOWS = 1 << 2,
    FLG_CASTSHADOWS_ON_SELF = 1 << 3,
    FLG_FORCE_LOCAL_NORMALS = 1 << 4,
    FLG_FORCE_WORLD_NORMALS = 1 << 5,
    FLG_FADE = 1 << 6,
    FLG_FADENULL = 1 << 7,
    FLG_BILLBOARD_MESH = 1 << 8,
    FLG_FORCE_SPHERICAL_NORMALS = 1 << 9,
    FLG_OCCLUDER = 1 << 10,
    FLG_AUTOMATIC_VISRANGE = 1 << 11,
    FLG_BACK_FACE_DYNAMIC_LIGHTING = 1 << 12,
    FLG_NO_RECOMPUTE_NORMALS = 1 << 13,
    FLG_DO_NOT_MIX_LODS = 1 << 14,
    FLG_NON_TOP_LOD = 1 << 15,

    FLG_OPERATIVE_HIDE = 1 << 30,
  };

  enum Lighting
  {
    LIGHT_DEFAULT,
    LIGHT_NONE,
    LIGHT_LIGHTMAP,
    LIGHT_VLTMAP,
  };

  Lighting lighting;
  int vss; // 0=disable, 1=force, -1=as master
  real ltMul;
  int vltMul;

  int flags;

  Point3 normalsDir;


  StaticGeometryNode() :
    flags(0),
    normalsDir(0, 1, 0),
    lodRange(100),
    lodNearVisRange(-1),
    lodFarVisRange(-1),
    lighting(LIGHT_DEFAULT),
    ltMul(1.0),
    vltMul(1)
  {
    vss = -1;
    visRange = 1000.0;
    transSortPriority = 0;
    wtm.identity();
    unwrapScheme = -1;
  }

  bool haveBillboardMat() const;
  bool checkBillboard();

  void calcBoundSphere();
  void calcBoundBox();

  // set material's lighting to node lighting
  void setMaterialLighting(StaticGeometryMaterial &mat) const;

  // set/get lighting in CFG script
  static void setScriptLighting(String &script, const char *lighting);
  static void getScriptLighting(const char *script, String &lighting);

  static const char *lightingToStr(Lighting light);
  static Lighting strToLighting(const char *light);

  inline int isNonTopLod() { return flags & FLG_NON_TOP_LOD; }
};
