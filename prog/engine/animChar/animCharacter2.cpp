// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <animChar/dag_animCharacter2.h>
#include <osApiWrappers/dag_spinlock.h>

using namespace AnimV20;

void IAnimCharacter2::setTm(vec4f v_pos, vec3f v_dir, vec3f v_up)
{
  vec3f v_right = v_norm3(v_cross3(v_up, v_dir));
  v_dir = v_cross3(v_right, v_up);
  finalWtm.nwtm[0].col0 = v_neg(v_right);
  finalWtm.nwtm[0].col1 = v_up;
  finalWtm.nwtm[0].col2 = v_neg(v_dir);
  finalWtm.nwtm[0].col3 = v_pos;
  finalWtm.wofs = v_zero();
}
void IAnimCharacter2::setTm(const TMatrix &tm)
{
  as_point4(&finalWtm.nwtm[0].col0).set_xyz0(tm.getcol(0));
  as_point4(&finalWtm.nwtm[0].col1).set_xyz0(tm.getcol(1));
  as_point4(&finalWtm.nwtm[0].col2).set_xyz0(tm.getcol(2));
  as_point4(&finalWtm.nwtm[0].col3).set_xyz1(tm.getcol(3));
  finalWtm.wofs = v_zero();
}
void IAnimCharacter2::setTm(const Point3 &pos, const Point3 &dir, const Point3 &up)
{
  setTm(v_perm_xyzd(v_ldu(&pos.x), V_C_ONE), v_perm_xyzd(v_ldu(&dir.x), v_zero()), v_perm_xyzd(v_ldu(&up.x), v_zero()));
}
void IAnimCharacter2::getTm(TMatrix &tm) const
{
  tm.setcol(0, as_point3(&finalWtm.nwtm[0].col0));
  tm.setcol(1, as_point3(&finalWtm.nwtm[0].col1));
  tm.setcol(2, as_point3(&finalWtm.nwtm[0].col2));
  tm.setcol(3, as_point3(&finalWtm.nwtm[0].col3));
}


static FastNameMap slotNames;
static OSSpinlock slotNamesMapMutex;
template <typename F>
static inline int get_slot_id(const char *slot_name, F getcb)
{
  if (!slot_name || !*slot_name)
    return -1;
  OSSpinlockScopedLock lock(slotNamesMapMutex);
  return getcb(slot_name);
}
int AnimCharV20::getSlotId(const char *slot_name)
{
  return get_slot_id(slot_name, [](const char *sn) { return slotNames.getNameId(sn); });
}
int AnimCharV20::addSlotId(const char *slot_name)
{
  return get_slot_id(slot_name, [](const char *sn) { return slotNames.addNameId(sn); });
}
