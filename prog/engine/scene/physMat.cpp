// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <scene/dag_physMat.h>
#include <generic/dag_tab.h>
#include <debug/dag_log.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_localConv.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>
#include <generic/dag_tabUtils.h>
#include <EASTL/bitvector.h>


void AssertPhysMatID_Valid(const char *file, int ln, PhysMat::MatID id)
{
  if (!IsPhysMatID_Valid(id))
    _core_fatal(file, ln, "phys mat id=%d is invalid (matnum=%d)", id, PhysMat::physMatCount());
}

inline int stringIndex(const Tab<SimpleString> &tab, const char *s)
{
  if (!s || !*s)
    return -1;
  for (int i = 0; i < tab.size(); i++)
  {
    if (dd_stricmp(s, tab[i]) == 0)
      return i;
  }

  return -1;
}

namespace PhysMat
{
// fx pool
static Tab<FXDesc> fx(midmem_ptr());

inline FXDesc *getFx(const char *name)
{
  if (!name || !*name)
    return NULL;

  for (int i = 0; i < fx.size(); i++)
  {
    if (dd_stricmp(name, fx[i].name) == 0)
      return &fx[i];
  }

  return NULL;
}

inline FXType getFxType(const char *t)
{
  if (!t || !*t)
    return PMFX_INVALID;

  static const char *fxTypes[] = {"Tires", "Dust", "Sparks", "Splash", "Trash"};
  static const int typeCount = sizeof(fxTypes) / sizeof(const char *);

  for (int i = 0; i < typeCount; i++)
  {
    if (dd_stricmp(t, fxTypes[i]) == 0)
      return (FXType)i;
  }
  return PMFX_INVALID;
}


//************************************************************************
//* interact properties
//************************************************************************
void InteractProps::load(const DataBlock &blk)
{
  bounce_k = blk.getReal("bk", bounce_k);
  frict_k = blk.getReal("fk", frict_k);
  collide = blk.getBool("en", collide);
  rollingFric_k = blk.getReal("rollingFrictionK", rollingFric_k);

  clear_and_shrink(fx);
  const int matFxId = blk.getNameId("matfx");
  for (int i = 0; i < blk.paramCount(); i++)
  {
    if (blk.getParamNameId(i) == matFxId)
    {
      FXDesc *newFx = getFx(blk.getStr(i));
      if (!newFx)
        DAG_FATAL("cannot find matfx='%s'", blk.getStr(i));
      fx.push_back(newFx);
    }
  }
}

void InteractProps::andOp(const InteractProps &a, const InteractProps &b)
{
  bounce_k = a.bounce_k * b.bounce_k;
  frict_k = a.frict_k * b.frict_k;

  // logical and
  for (int i = 0; i < a.fx.size(); i++)
    if (tabutils::find(b.fx, a.fx[i]))
      fx.push_back(a.fx[i]);
}

static Tab<MaterialData> mats(midmem_ptr());
static eastl::bitvector<> interactMatPropsCollidable; // cached per material (not bf_class) 'collide' flag
static Tab<InteractProps> interactProps(midmem_ptr());
static Tab<SimpleString> bfClasses(midmem_ptr());

static Tab<IPhysMatReadyListener *> on_physmat_ready_listeners;

//************************************************************************
//* physmat interface
//************************************************************************
inline InteractID getInteractIdPM(BFClassID bf1, BFClassID bf2)
{
  return (InteractID)(bf1 > bf2 ? (bf1 * bfClasses.size() + bf2) : (bf2 * bfClasses.size() + bf1));
}

// get interact id. return PHYSMAT_INVALID, if failed
InteractID getInteractId(MatID pmid1, MatID pmid2) { return getInteractIdPM(mats[pmid1].bfid, mats[pmid2].bfid); }


// debug functions
void _dump_props(BFClassID bfid1, BFClassID bfid2, const InteractProps &p)
{
  String s(128, "InteractProps(%s<->%s) id=%d bk=%.4f fk=%.4f", (char *)bfClasses[bfid1], (char *)bfClasses[bfid2], p.id, p.bounce_k,
    p.frict_k);
  for (int i = 0; i < p.fx.size(); i++)
  {
    G_ASSERT(p.fx[i]);
    s.aprintf(128, " fx%d='%s'", i, (char *)p.fx[i]->name);
  }

  debug("%s", s.str());
}

void dump_all()
{
  int i;
  debug("-------------PhysMat dump-------------->");
  debug("BfClasses (%d):", bfClasses.size());
  for (i = 0; i < bfClasses.size(); i++)
  {
    debug(" %d: name=%s", i, (char *)bfClasses[i]);
  }
  debug("Materials (%d):", mats.size());
  for (i = 0; i < mats.size(); i++)
  {
    const MaterialData &mat = mats[i];
    debug(" %d: id=%d, bf=%d(%s), name=%s", i, mat.id, mat.bfid, (char *)bfClasses[mat.bfid], (const char *)mat.name);
  }
  debug("FX (%d):", fx.size());
  for (i = 0; i < fx.size(); i++)
  {
    const FXDesc &pfx = fx[i];
    String s;
    pfx.params.dump(s);
    debug(" %d: name=%s type=%d params=[%s]", i, (const char *)pfx.name, pfx.type, (char *)s);
  }
  debug("InteractProps:");
  for (i = 0; i < bfClasses.size(); i++)
    for (int j = i; j < bfClasses.size(); j++)
    {
      _dump_props(i, j, interactProps[getInteractIdPM(i, j)]);
    }
  debug("-------------PhysMat dump ok-----------<");
}

static void loadMaterial(MaterialData &mat, const MaterialData &def, const DataBlock *matBlk,
  register_mat_props_cb_t register_mat_props_cb, void *ud)
{
  // mat bf_class
  const char *bfClass = matBlk->getStr("bf_class", NULL);
  if (bfClass && *bfClass)
  {
    mat.bfid = stringIndex(bfClasses, bfClass);
    if (mat.bfid < 0)
    {
      mat.bfid = bfClasses.size();
      bfClasses.push_back() = bfClass;
    }
  }
  else
  {
    mat.bfid = def.bfid; // default physmat
  }

  if (register_mat_props_cb && mat.id >= 0)
  {
    int propId = register_mat_props_cb(matBlk->getBlockName(), matBlk, ud);
    G_ASSERTF(propId == mat.id, "PropId '%d' doesn't match MatId '%d' for mat '%s'", propId, mat.id, matBlk->getBlockName());
  }

  mat.imp_absorb_k = matBlk->getReal("iak", def.imp_absorb_k);
  mat.imp_weak_k = matBlk->getReal("iwk", def.imp_weak_k);
  mat.r_bounce_k = matBlk->getReal("rbk", def.r_bounce_k);
  mat.mk_dmg = matBlk->getBool("mk_dmg", def.mk_dmg);
  mat.dont_trace = matBlk->getBool("dont_trace", def.dont_trace);
  mat.clippable = matBlk->getBool("clippable", def.clippable);
  mat.autoReset = matBlk->getReal("autoReset", def.autoReset);
  mat.vcm_color = matBlk->getE3dcolor("color", def.vcm_color);
  mat.damage_k = matBlk->getReal("damage_k", def.damage_k);
  mat.disable_control = matBlk->getBool("disable_control", def.disable_control);
  mat.shake_factor = matBlk->getReal("shake_factor", def.shake_factor);
  mat.invisible_clipping = matBlk->getBool("invisible_clipping", def.invisible_clipping);
  mat.phobj_only = matBlk->getBool("phobj_only", def.phobj_only);
  mat.fly_through_clip = matBlk->getBool("fly_through_clip", def.fly_through_clip);
  mat.stick_k = matBlk->getReal("stick_k", def.stick_k);
  mat.lifeTime = matBlk->getReal("life_time", def.lifeTime);
  mat.directocclusion = matBlk->getReal("direct_occlusion", def.directocclusion);
  mat.reverbocclusion = matBlk->getReal("reverb_occlusion", def.reverbocclusion);
  mat.deformableWidth = matBlk->getReal("deformable_width", def.deformableWidth);
  mat.resistanceK = matBlk->getReal("resistanceK", def.resistanceK);
  mat.completelyTransparent = matBlk->getBool("completelyTransparent", def.completelyTransparent);
  mat.lightTransparent = matBlk->getBool("lightTransparent", def.lightTransparent || mat.completelyTransparent);
  mat.noTransparentThickness = matBlk->getReal("noTransparentThickness", def.noTransparentThickness);

  mat.camera_collision = matBlk->getBool("camera_collision", def.camera_collision);
  mat.physics_collision = matBlk->getBool("physics_collision", def.physics_collision);
  mat.bullets_collision = matBlk->getBool("bullets_collision", def.bullets_collision);
  mat.characters_collision = matBlk->getBool("characters_collision", def.characters_collision);
  mat.characters_collision2 = matBlk->getBool("characters_collision2", def.characters_collision2);
  mat.characters_collision3 = matBlk->getBool("characters_collision3", def.characters_collision3);
  mat.soundMaterial = matBlk->getStr("sndMat", def.soundMaterial);
  mat.isSolid = matBlk->getBool("is_solid", def.isSolid);
  mat.tankTracksTexId = matBlk->getInt("tankTracksTexId", def.tankTracksTexId);
  mat.vehicleHeightmapDeformation = matBlk->getPoint2("vehicleHeightmapDeformation", def.vehicleHeightmapDeformation);
  mat.humanHeightmapDeformation = matBlk->getReal("humanHeightmapDeformation", def.humanHeightmapDeformation);
  mat.trailDetailStrength = matBlk->getReal("trailDetailStrength", def.trailDetailStrength);

  mat.physStaticFriction = matBlk->getReal("physStaticFriction", def.physStaticFriction);
  mat.physRestitution = matBlk->getReal("physRestitution", def.physRestitution);
}

// init manager
void init(const char *filename, const DataBlock *loadedBlk, register_mat_props_cb_t register_mat_props_cb, void *ud)
{
  clear_and_shrink(mats);
  clear_and_shrink(interactProps);
  clear_and_shrink(fx);
  clear_and_shrink(bfClasses);
  interactMatPropsCollidable.clear();
  DataBlock phys_blk;
  const DataBlock &blk = loadedBlk ? *loadedBlk : phys_blk;

  MaterialData def;
  def.id = -1;
  def.bfid = 0;
  bfClasses.push_back() = "DefPhysMat";

  def.name = "::default";
  def.imp_absorb_k = 0.05;
  def.imp_weak_k = 0.0;
  def.r_bounce_k = 1;
  def.mk_dmg = true;
  def.dont_trace = false;
  def.clippable = true;
  def.autoReset = -1;
  def.vcm_color = E3DCOLOR(255, 0, 255, 127);
  def.damage_k = 1;
  def.disable_control = false;
  def.shake_factor = 1.0;
  def.invisible_clipping = false;
  def.phobj_only = false;
  def.fly_through_clip = false;
  def.stick_k = 1.0f;
  def.lifeTime = 5.0f;
  def.camera_collision = true;
  def.physics_collision = true;
  def.bullets_collision = true;
  def.characters_collision = true;
  def.characters_collision2 = true;
  def.characters_collision3 = true;
  def.directocclusion = 1.0;
  def.reverbocclusion = 1.0;
  def.soundMaterial = "default";
  def.deformableWidth = 0.f;
  def.resistanceK = 0.f;
  def.completelyTransparent = false;
  def.lightTransparent = false;
  def.noTransparentThickness = 0.f;
  def.isSolid = false;
  def.tankTracksTexId = 0;
  def.vehicleHeightmapDeformation = Point2(0.f, 0.f);
  def.humanHeightmapDeformation = 0.f;
  def.trailDetailStrength = 0.f;
  def.physStaticFriction = 0.9f;
  def.physRestitution = 0.2f;

  if (!loadedBlk && !phys_blk.load(filename))
  {
    logerr("PhysMat: cannot load '%s'!", filename);
    mats.push_back(def);
    return;
  }

  int fatalIfNotInit = 0;
  bool dumpInfo = false;

  // load options
  const DataBlock *optBlk = blk.getBlockByName("PhysMatOptions");
  if (optBlk)
  {
    fatalIfNotInit = optBlk->getInt("fatalWhenNotInit", fatalIfNotInit);
    dumpInfo = optBlk->getBool("dumpResults", dumpInfo);
  }

  // load materials
  const DataBlock *materialsBlk = blk.getBlockByName("PhysMats");
  if (!materialsBlk)
    DAG_FATAL("'PhysMats' section required! (%s)", filename);

  // default params
  const DataBlock *defMatBlk = materialsBlk->getBlockByName("__DefaultParams");
  if (!defMatBlk)
    DAG_FATAL("'__DefaultParams' section required in 'PhysMats'! (%s)", filename);

  loadMaterial(def, def, defMatBlk, register_mat_props_cb, ud);

  for (int i = 0; i < materialsBlk->blockCount(); i++)
  {
    const DataBlock *matBlk = materialsBlk->getBlock(i);
    if (matBlk == defMatBlk)
      continue;

    MaterialData &mat = mats.push_back();
    mat.id = mats.size() - 1;

    // mat name
    mat.name = matBlk->getBlockName();
    if (mat.name.length() == 0)
      DAG_FATAL("Empty physmat names is not allowed! (%s)", filename);

    // material params
    loadMaterial(mat, def, matBlk, register_mat_props_cb, ud);
  }

  // load collision FX
  const DataBlock *fxBlk = blk.getBlockByName("CollisionFX");
  if (fxBlk)
  {
    for (int i = 0; i < fxBlk->blockCount(); i++)
    {
      const DataBlock *colFxBlk = fxBlk->getBlock(i);

      FXDesc &curFx = fx.push_back();

      curFx.type = getFxType(colFxBlk->getBlockName());
      if (curFx.type == PMFX_INVALID)
        DAG_FATAL("Invalid fx type '%s' (%s)", colFxBlk->getBlockName(), filename);

      const char *fxName = colFxBlk->getStr("name", NULL);
      if (!fxName)
        DAG_FATAL("Empty fx names is not allowed! (type '%s') (%s)", colFxBlk->getBlockName(), filename);
      curFx.name = fxName;
      curFx.params.loadFromBlk(*colFxBlk);
    }
  }

  // interact props
  Tab<bool> inited(tmpmem);
  inited.resize(bfClasses.size() * bfClasses.size());
  interactProps.resize(bfClasses.size() * bfClasses.size());

  tabutils::setAll(inited, false);

  // fill props with default values
  const DataBlock *ipropsBlk = blk.getBlockByName("BfClassDefaultList");
  if (ipropsBlk)
  {
    Tab<InteractProps> defProps(tmpmem);
    defProps.resize(bfClasses.size());
    for (int i = 0; i < bfClasses.size(); i++)
    {
      defProps[i].id = -1;
      defProps[i].bounce_k = defProps[i].frict_k = 0;
    }

    for (int i = 0; i < ipropsBlk->blockCount(); i++)
    {
      const DataBlock *propBlk = ipropsBlk->getBlock(i);
      BFClassID bf;
      bf = stringIndex(bfClasses, propBlk->getBlockName());
      if (bf == PHYSMAT_INVALID)
        DAG_FATAL("bf_class '%s' not found while loading defaults for BfClasses!", propBlk->getBlockName());
      defProps[bf].id = 0;
      defProps[bf].load(*propBlk);
    }

    for (int i = 0; i < bfClasses.size(); i++)
    {
      for (int j = 0; j < bfClasses.size(); j++)
      {
        InteractID iid = i * bfClasses.size() + j;

        // logic AND operator
        interactProps[iid].andOp(defProps[i], defProps[j]);
        interactProps[iid].id = getInteractIdPM(i, j);
        if (defProps[i].id >= 0 && defProps[j].id >= 0)
        {
          inited[interactProps[iid].id] = true;
        }
      }
    }
  }

  // fill with user values
  ipropsBlk = blk.getBlockByName("InteractPropsList");
  if (!ipropsBlk)
    DAG_FATAL("'InteractPropsList' section required! (%s)", filename);

  Tab<bool> initedByUser(tmpmem);
  initedByUser.resize(bfClasses.size() * bfClasses.size());
  tabutils::setAll(initedByUser, false);

  for (int i = 0; i < ipropsBlk->blockCount(); i++)
  {
    const DataBlock *prop1Blk = ipropsBlk->getBlock(i);
    BFClassID bf1;
    bf1 = stringIndex(bfClasses, prop1Blk->getBlockName());
    if (bf1 == PHYSMAT_INVALID)
      DAG_FATAL("bf_class '%s' not found while loading interact props!", prop1Blk->getBlockName());

    for (int j = 0; j < prop1Blk->blockCount(); j++)
    {
      const DataBlock *prop2Blk = prop1Blk->getBlock(j);
      BFClassID bf2 = stringIndex(bfClasses, prop2Blk->getBlockName());
      if (bf2 == PHYSMAT_INVALID)
        DAG_FATAL("bf_class '%s' not found while loading interact props for '%s'!", prop2Blk->getBlockName(),
          prop1Blk->getBlockName());

      InteractID iid = getInteractIdPM(bf1, bf2);

      if (initedByUser[iid])
      {
        DAG_FATAL("'%s' and '%s' interact props already set!", (const char *)bfClasses[bf1], (const char *)bfClasses[bf2]);
      }

      inited[iid] = true;
      initedByUser[iid] = true;

      interactProps[iid].load(*prop2Blk);
      interactProps[iid].id = iid;
    }
  }

  for (int i = 0, mcnt = mats.size(); i < mcnt; ++i)
    for (int j = 0; j < mcnt; ++j)
      if (getInteractProps((MatID)i, (MatID)j).collide)
        interactMatPropsCollidable.set(i * mcnt + j, true);

  // debugging
  if (dumpInfo)
    dump_all();

  if (fatalIfNotInit > 0)
  {
    int notInitCount = 0;
    if (fatalIfNotInit == 1)
    {
      debug("Checking all InteractProps for initialization...");
      for (int i = 0; i < bfClasses.size(); i++)
      {
        for (int j = i; j < bfClasses.size(); j++)
        {
          InteractID iid = getInteractIdPM(i, j);
          if (!inited[iid])
          {
            if (!notInitCount)
              debug("-------not inited interact props:------");
            notInitCount++;
            _dump_props(i, j, interactProps[iid]);
          }
        }
      }
    }
    else if (fatalIfNotInit == 2)
    {
      debug("Checking some of bf_classes for initialization...");
      Tab<int> includeTable(tmpmem);
      if (optBlk)
      {
        const DataBlock *includeBlk = optBlk->getBlockByName("ClassesForChecking");
        if (includeBlk)
        {
          for (int i = 0; i < includeBlk->paramCount(); i++)
          {
            if (dd_stricmp(includeBlk->getParamName(i), "bf_class") == 0)
            {
              BFClassID bfid = stringIndex(bfClasses, includeBlk->getStr(i));
              if (bfid != PHYSMAT_INVALID)
                includeTable.push_back(bfid);
            }
          }
        }
      }
      for (int i = 0; i < bfClasses.size(); i++)
      {
        for (int j = i; j < bfClasses.size(); j++)
        {
          if (!tabutils::find(includeTable, i) && !tabutils::find(includeTable, j))
            continue;
          InteractID iid = getInteractIdPM(i, j);
          if (!inited[iid])
          {
            if (!notInitCount)
              debug("-------not inited interact props:------");
            notInitCount++;
            _dump_props(i, j, interactProps[iid]);
          }
        }
      }
    }

    if (notInitCount)
      DAG_FATAL("%d InteractProps are not inited!", notInitCount);
    debug("Checking ok...");
  }
}

void register_physmat_ready_listener(IPhysMatReadyListener *listener) { on_physmat_ready_listeners.push_back(listener); }

void remove_physmat_ready_listener(IPhysMatReadyListener *listener) { erase_item_by_value(on_physmat_ready_listeners, listener); }


void on_physmat_ready()
{
  for (int i = 0; i < on_physmat_ready_listeners.size(); ++i)
    on_physmat_ready_listeners[i]->onPhysMatReady();
}

// release manager
void release()
{
  clear_and_shrink(bfClasses);
  clear_and_shrink(mats);
  clear_and_shrink(interactProps);
  clear_and_shrink(fx);
  decltype(interactMatPropsCollidable)().swap(interactMatPropsCollidable);
  clear_and_shrink(on_physmat_ready_listeners);
}


// return total count of phys materials
int physMatCount() { return mats.size(); }

int bfClassCount() { return bfClasses.size(); }

// get material id. return PHYSMAT_INVALID, if failed
MatID getMaterialId(const char *name)
{
  if (!name || !*name)
    return PHYSMAT_DEFAULT;

  for (int i = 0; i < mats.size(); i++)
  {
    if (dd_stricmp(mats[i].name, name) == 0)
      return (MatID)i;
  }

  return PHYSMAT_DEFAULT;
}

// get matrial by ID
const MaterialData &getMaterial(MatID pmid1)
{
  G_ASSERTF_RETURN(uint32_t(pmid1) < mats.size(), mats[PHYSMAT_DEFAULT], "invalid phys-material: %d, count = %d", pmid1, mats.size());
  return mats[pmid1];
}


// get interact props by ID
const InteractProps &getInteractProps(InteractID eid) { return interactProps[eid]; }


const InteractProps &getInteractProps(MatID pmid1, MatID pmid2) { return getInteractProps(getInteractId(pmid1, pmid2)); }

bool isMaterialsCollide(MatID row, MatID column)
{
  int mcnt = mats.size();
#if _DEBUG_TAB_
  // Use fast assert for better inlining
  G_FAST_ASSERT((unsigned)row < mcnt);
  G_FAST_ASSERT((unsigned)column < mcnt);
#endif
  bool ret = interactMatPropsCollidable.test(row * mcnt + column, false);
#if DAGOR_DBGLEVEL > 1
  G_ASSERT(ret == getInteractProps(row, column).collide);
#endif
  return ret;
}

// return bf-class name
const char *getBFClassName(BFClassID bfid)
{
  if (!tabutils::isCorrectIndex(bfClasses, bfid))
    return NULL;
  return bfClasses[bfid];
}

dag::Span<MaterialData> getMaterials() { return make_span(mats); }

MatID getMaterialIdByPhysBodyMaterial(int material_id)
{
  for (int i = 0; i < mats.size(); i++)
    if (mats[i].physBodyMaterial == material_id)
      return i;
  return -1;
}

void setPhysBodyMaterial(MatID mat_id, int material_id) { mats[mat_id].physBodyMaterial = material_id; }
int getPhysBodyMaterial(MatID mat_id) { return mats[mat_id].physBodyMaterial; }
}; // namespace PhysMat
