#include <phys/dag_physDecl.h>
#include <phys/dag_physics.h>

#include <gamePhys/phys/rendinstPhys.h>

#include <gamePhys/collision/collisionLib.h>

RendInstPhys::RendInstPhys() :
  originalTm(TMatrix::IDENT),
  desc(-1, -1, -1, 0, 0),
  descForDestr(-1, -1, -1, 0, 0),
  ri(NULL),
  riColObj(),
  physModel(NULL),
  physBody(NULL),
  additionalBody(NULL),
  joints(midmem),
  centerOfMassTm(TMatrix::IDENT),
  scale(1.f, 1.f, 1.f),
  lastValidTm(TMatrix::IDENT),
  ttl(0.f),
  maxTtl(0.f),
  maxLifeDist(-1.f),
  distConstrainedPhys(-1.f)
{}

RendInstPhys::RendInstPhys(RendInstPhysType type, const rendinst::RendInstDesc &from_desc, const TMatrix &tm, float bodyMass,
  const Point3 &moment, const Point3 &scal, const Point3 &centerOfGravity, gamephys::DynamicPhysModel::PhysType phys_type,
  const TMatrix &original_tm, float max_life_time, float max_life_dist, rendinst::DestroyedRi *ri) :
  type(type),
  desc(from_desc),
  descForDestr(from_desc),
  ri(ri),
  riColObj(),
  physBody(NULL),
  additionalBody(NULL),
  joints(midmem),
  scale(scal),
  ttl(1.f),
  maxTtl(max_life_time),
  maxLifeDist(max_life_dist),
  distConstrainedPhys(-1.f),
  originalTm(original_tm),
  centerOfMassTm(TMatrix::IDENT),
  lastValidTm(TMatrix::IDENT)
{
  physModel = new gamephys::DynamicPhysModel(tm, bodyMass, moment, centerOfGravity, phys_type);
}

void RendInstPhys::cleanup()
{
  dacoll::destroy_dynamic_collision(riColObj);
  riColObj = CollisionObject();
  del_it(physModel);
  clear_all_ptr_items_and_shrink(joints);
  del_it(physBody);
  del_it(additionalBody);
  treeSound.destroy(lastValidTm != TMatrix::IDENT ? lastValidTm : originalTm);
}

void RendInstTreeSound::init(ri_tree_sound_cb tree_sound_cb, const TMatrix &tm, float tree_height, bool is_bush)
{
  G_ASSERT_RETURN(!inited(), );
  G_ASSERT_RETURN(tree_sound_cb, );
  soundCb = tree_sound_cb;
  wasFalled = false;
  desc.isBush = is_bush;
  desc.treeHeight = tree_height;
  isInited = soundCb(TREE_SOUND_CB_INIT, desc, soundCBData, tm);
}

void RendInstTreeSound::destroy(const TMatrix &tm)
{
  if (inited())
    soundCb(TREE_SOUND_CB_DESTROY, desc, soundCBData, tm);
}

void RendInstTreeSound::setFalled(const TMatrix &tm)
{
  if (inited() && !wasFalled)
  {
    wasFalled = true;
    soundCb(TREE_SOUND_CB_ON_FALLED, desc, soundCBData, tm);
  }
}

void RendInstTreeSound::updateFalling(const TMatrix &tm)
{
  if (inited() && !wasFalled)
    soundCb(TREE_SOUND_CB_FALLING, desc, soundCBData, tm);
}
