#include <anim/dag_simpleNodeAnim.h>
#include <anim/dag_animKeyInterp.h>
#include <math/dag_quatInterp.h>
#include <debug/dag_debug.h>
#include <limits.h>

bool SimpleNodeAnim::init(AnimV20::AnimData *a, const char *node_name)
{
  anim = a;
  return setTargetNode(node_name);
}

bool SimpleNodeAnim::setTargetNode(const char *node_name)
{
  if (!anim.get())
    return false;

  pos = anim->getPoint3Anim(AnimV20::CHTYPE_POSITION, node_name);
  rot = anim->getQuatAnim(AnimV20::CHTYPE_ROTATION, node_name);
  scl = anim->getPoint3Anim(AnimV20::CHTYPE_SCALE, node_name);

  return isValid();
}

void SimpleNodeAnim::calcTimeMinMax(int &t_min, int &t_max)
{
  if (!pos || !rot || !scl)
  {
    t_min = INT_MAX;
    t_max = INT_MIN;
    return;
  }

  t_min = pos->keyTimeFirst();
  if (t_min > rot->keyTimeFirst())
    t_min = rot->keyTimeFirst();
  if (t_min > scl->keyTimeFirst())
    t_min = scl->keyTimeFirst();

  t_max = pos->keyTimeLast();
  if (t_max < rot->keyTimeLast())
    t_max = rot->keyTimeLast();
  if (t_max < scl->keyTimeLast())
    t_max = scl->keyTimeLast();
}

void SimpleNodeAnim::calcAnimTm(TMatrix &tm, int t, int d_keys_no_blend /* = -1*/)
{
  float tpos = 0, tscl = 0, trot = 0;
  vec3f p, s;
  quat4f r;
  int dkeys = 0;
  AnimV20::AnimKeyPoint3 *kpos = pos ? pos->findKeyEx(t, &tpos, dkeys) : NULL;
  AnimV20::AnimKeyQuat *krot = rot ? rot->findKey(t, &trot) : NULL;
  AnimV20::AnimKeyPoint3 *kscl = scl ? scl->findKey(t, &tscl) : NULL;

  if (dkeys <= d_keys_no_blend)
  {
    tpos = 0;
    tscl = 0;
    trot = 0;
  }

  if (kpos)
    p = (tpos != 0.f) ? AnimV20Math::interp_key(kpos[0], v_splat4(&tpos)) : kpos->p;
  else
    p = v_zero();

  if (krot)
    r = (trot != 0.f) ? AnimV20Math::interp_key(krot[0], krot[1], trot) : krot->p;
  else
    r = v_zero();

  if (kscl)
    s = (tscl != 0.f) ? AnimV20Math::interp_key(kscl[0], v_splat4(&tscl)) : kscl->p;
  else
    s = V_C_ONE;

  tm = AnimV20Math::makeTM((Point3 &)p, (Quat &)r, (Point3 &)s);
}
