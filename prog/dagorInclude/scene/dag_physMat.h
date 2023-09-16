//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_e3dColor.h>
#include <util/dag_globDef.h>
#include <util/dag_simplePBlock.h>
#include <util/dag_simpleString.h>
#include <scene/dag_physMatId.h>


namespace PhysMat
{
typedef int BFClassID;
//************************************************************************
//* physmat description
//************************************************************************
struct MaterialData
{
  MatID id;          // material id
  BFClassID bfid;    // id for bf-class of this material
  SimpleString name; // material name

  real imp_absorb_k, //< Impulse absorbtion koef, 0..1 (0=imp not applied, 1=full impulse applied)
    imp_weak_k,      //< Impulse weakening koef, 0..1 (0=imp diminishes, 1=full impulse penetration)
    r_bounce_k,      //< Ricochet bounce koef, 0..1 (bounce koef for bullets)
    shake_factor;
  bool mk_dmg,             //< makes damage to colliding phys objs
    dont_trace,            //< uses RT_FLAG_DONT_TRACE flag when added to raytracer
    clippable;             //< materal should clip (used for capsules in physobj)
  real autoReset;          //< autoreset car to track in autoReset seconds (if <0 - no autoreset)
  bool disable_control;    //
  bool invisible_clipping; //< clipping withot corresponding scene mesh
  bool phobj_only;         //< no collisions with static world
  E3DCOLOR vcm_color;      //< color for clip mesh with visclipmesh feature
  float damage_k;          //< defines damage applied to other phobjects
  float deformableWidth;
  float resistanceK;

  bool fly_through_clip;
  real stick_k;  /// Probability stick in other object
                 //  real weak_stick_k;
  real lifeTime; /// Life time collision object

  int physBodyMaterial;

  // new fundamental properties for static/dynamic scene materials
  bool camera_collision;
  bool physics_collision;
  bool bullets_collision;
  bool characters_collision;
  bool characters_collision2;
  bool characters_collision3;

  // sound occlusion factors of material
  float directocclusion;
  float reverbocclusion;
  SimpleString soundMaterial;
  bool isSolid;
  int tankTracksTexId;

  inline MaterialData() { physBodyMaterial = 0; } //-V730
};
//************************************************************************
//* FX types
//************************************************************************
enum FXType
{
  PMFX_INVALID = -1, // invalid FX
  PMFX_TIRES,        // tiretrack fx
  PMFX_DUST,         // dust fx
  PMFX_SPARKS,       // sparks fx
  PMFX_SPLASH,       // water splash fx
  PMFX_TRASH,        // trash fx
};

struct FXDesc
{
  SimpleString name;
  FXType type;
  SimplePBlock params;
};

typedef int (*register_mat_props_cb_t)(const char *name, const DataBlock *blk, void *ud);

//************************************************************************
//* interact properties
//************************************************************************
struct InteractProps
{
  InteractID id = PMFX_INVALID;
  real bounce_k = 0;        //< Bounce koef, 0..1
  real frict_k = 0;         //< Friction koef, 0..1
  real rollingFric_k = 1.f; //< Rolling friction koef
  bool collide = true;
  Tab<FXDesc *> fx;

private:
  friend void init(const char *, const DataBlock *loadedBlk, register_mat_props_cb_t, void *);
  void load(const DataBlock &blk);
  void andOp(const InteractProps &a, const InteractProps &b);
};

class IPhysMatReadyListener
{
public:
  virtual void onPhysMatReady() = 0;
};

//************************************************************************
//* physmat interface
//************************************************************************
// init manager
void init(const char *filename, const DataBlock *loadedBlk = NULL, register_mat_props_cb_t reg_cb = NULL, void *cb_data = NULL);

void register_physmat_ready_listener(IPhysMatReadyListener *listener);
void remove_physmat_ready_listener(IPhysMatReadyListener *listener);
void on_physmat_ready();

// release manager
void release();

// return total count of bf classes
int bfClassCount();

// get matrial by ID. return default, if not found
const MaterialData &getMaterial(MatID pmid1);
inline const MaterialData &getMaterial(const char *name) { return getMaterial(getMaterialId(name)); };

// get interact props by ID. return default vs default
const InteractProps &getInteractProps(InteractID eid);
const InteractProps &getInteractProps(MatID pmid1, MatID pmid2);

bool isMaterialsCollide(MatID row, MatID column); // same as 'getInteractProps(pmid1, pmid2).collide' but faster. Invalid materials are
                                                  // not allowed

// return bf-class name
const char *getBFClassName(BFClassID bfid);

dag::Span<MaterialData> getMaterials();

// debug functions
void _dump_props(BFClassID bfid1, BFClassID bfid2, const InteractProps &p);
void dump_all();

MatID getMaterialIdByPhysBodyMaterial(int material_id);
void setPhysBodyMaterial(MatID mat_id, int material_id);
int getPhysBodyMaterial(MatID mat_id);
}; // namespace PhysMat
DAG_DECLARE_RELOCATABLE(PhysMat::FXDesc);
DAG_DECLARE_RELOCATABLE(PhysMat::InteractProps);
