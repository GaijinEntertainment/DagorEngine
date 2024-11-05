// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include "expanim.h"
#include "expanim2.h"
#include "dagor.h"
#include "resource.h"
// #include "debug.h"

static Interval lim;
static float ort_thr;
static char _usekeys;

static Tab<TimeValue> ktime;

static inline Quat qadd(Quat a, Quat b)
{
  Quat q(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
  q.Normalize();
  return q;
}
double length(Quat &q) { return sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w); }

static inline Quat SLERP(Quat a, Quat b, float t)
{
  float f = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  if (f >= 0.9999)
  {
    return qadd(a * (1 - t), b * t);
  }
  else if (f <= -0.9999)
  {
    return qadd(a * (t - 1), b * t);
  }
  else
  {
    float w = acosf(f);
    if (f >= 0)
    {
      float sinw = sinf(w);
      return qadd(a * (sinf(w * (1 - t)) / sinw), b * (sinf(w * t) / sinw));
    }
    w = PI - w;
    float sinw = sinf(w);
    return qadd(a * (-sinf(w * (1 - t)) / sinw), b * (sinf(w * t) / sinw));
  }
}

static int cmp_time(const void *a, const void *b) { return *(TimeValue *)a - *(TimeValue *)b; }

static inline void sort_ktime() { ktime.Sort(cmp_time); }

static inline void add_ktime(TimeValue t, bool check = true)
{
  if (check)
    for (int i = 0; i < ktime.Count(); ++i)
      if (ktime[i] == t)
        return;
  ktime.Append(1, &t);
}

static inline void calc_cp(PosKey &k0, PosKey &k1, Point3 p13, Point3 p23)
{
  k0.o = k0.p * (-5.0f / 6.0f) + k1.p * (1.0f / 3.0f) + p13 * 3 - p23 * 1.5f;
  k1.i = k1.p * (-5.0f / 6.0f) + k0.p * (1.0f / 3.0f) + p23 * 3 - p13 * 1.5f;
}

static inline void calc_cpi(PosKey &k0, PosKey &k1, Point3 p23)
{
  k1.i = k1.p * (-2.0f / 3.0f) - k0.p * (1.0f / 12.0f) - k0.o * 0.5f + p23 * (9.0f / 4.0f);
}

static inline void calc_cpo(PosKey &k0, PosKey &k1, Point3 p13)
{
  k0.o = k0.p * (-2.0f / 3.0f) - k1.p * (1.0f / 12.0f) - k1.i * 0.5f + p13 * (9.0f / 4.0f);
}

static inline void calc_cp(RotKey &k0, RotKey &k1, Quat p13, Quat p23)
{
  Quat q0 = SLERP(SLERP(k0.p, k1.p, 1.0f / 3.0f), p13, 9.0f / 4.0f);
  Quat q1 = SLERP(SLERP(k1.p, k0.p, 1.0f / 3.0f), p23, 9.0f / 4.0f);
  k0.o = SLERP(q0, q1, -1);
  k1.i = SLERP(q1, q0, -1);
  //      k0.o=SLERP(SLERP(q0,q1,-1),k1.p,1.0f/3.0f);
  //      k1.i=SLERP(SLERP(q1,q0,-1),k0.p,1.0f/3.0f);
}

static inline void calc_cpi(RotKey &k0, RotKey &k1, Quat p23)
{
  Quat q1 = SLERP(SLERP(k1.p, k0.p, 1.0f / 3.0f), p23, 9.0f / 4.0f);
  k1.i = SLERP(k0.o, q1, 1.5f);
}

static inline void calc_cpo(RotKey &k0, RotKey &k1, Quat p13)
{
  Quat q0 = SLERP(SLERP(k0.p, k1.p, 1.0f / 3.0f), p13, 9.0f / 4.0f);
  k0.o = SLERP(k1.i, q0, 1.5f);
}

static inline int is_orthog(Point3 ax, Point3 ay, Point3 az)
{
  if (fabsf(DotProd(ax, ay)) > ort_thr)
    return 0;
  if (fabsf(DotProd(ax, az)) > ort_thr)
    return 0;
  if (fabsf(DotProd(ay, az)) > ort_thr)
    return 0;
  return 1;
}

static void interp_tm(TimeValue t, Point3 &p, Quat &q, Point3 &s, ExpTMAnimCB &cb)
{
  Matrix3 m;
  cb.interp_tm(t, m);
  /*
    debug ( "%s: %d: (%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f)", cb.get_name(), t,
            m.GetRow(0).x, m.GetRow(0).y, m.GetRow(0).z, m.GetRow(1).x, m.GetRow(1).y, m.GetRow(1).z,
            m.GetRow(2).x, m.GetRow(2).y, m.GetRow(2).z, m.GetRow(3).x, m.GetRow(3).y, m.GetRow(3).z );
  */
  Point3 ax = m.GetRow(0), ay = m.GetRow(1), az = m.GetRow(2);
  p = m.GetRow(3);
  float lx = Length(ax), ly = Length(ay), lz = Length(az);
  if (m.Parity())
    lz = -lz;
  s = Point3(lx, ly, lz);
  if (lx != 0)
    m.SetRow(0, ax /= lx);
  if (ly != 0)
    m.SetRow(1, ay /= ly);
  if (lz != 0)
    m.SetRow(2, az /= lz);
  // m.SetRow(3,Point3(0,0,0));
  if (!is_orthog(ax, ay, az))
  {
    cb.non_orthog_tm(t);
    m.Orthogonalize();
  }
  // m.SetIdentFlags(POS_IDENT|SCL_IDENT);
  q = Quat(m);
}

static inline Matrix3 make_tm(Point3 p, Quat r, Point3 s)
{
  Matrix3 m(1);
  m.Scale(s);
  Matrix3 qm;
  r.MakeMatrix(qm);
  m = m * qm;
  m.Translate(p);
  return m;
}

static inline BOOL pos_equal(Point3 a, Point3 b, float t)
{
  float d = LengthSquared(a - b);
  if (d <= t * t)
    return 1;
  return 0;
}

static inline BOOL rot_equal(Quat a, Quat b, float t) { return fabsf(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w) >= t; }

static inline BOOL scl_equal(Point3 a, Point3 b, float t)
{
  return fabsf(a.x - b.x) * 2 <= t * (fabsf(a.x) + fabsf(b.x)) && fabsf(a.y - b.y) * 2 <= t * (fabsf(a.y) + fabsf(b.y)) &&
         fabsf(a.z - b.z) * 2 <= t * (fabsf(a.z) + fabsf(b.z));
}

static inline Point3 interp_seg(PosKey a, PosKey b, TimeValue time)
{
  float t = float(time - a.t) / float(b.t - a.t);
  float s = 1 - t;
  return a.p * (s * s * s) + a.o * (3 * s * s * t) + b.i * (3 * s * t * t) + b.p * (t * t * t);
}

static inline Quat interp_seg(RotKey a, RotKey b, TimeValue time)
{
  float t = float(time - a.t) / float(b.t - a.t);
  return SLERP(SLERP(a.p, b.p, t), SLERP(a.o, b.i, t), 2 * (1 - t) * t);
  /*      Quat q1=SLERP(a.o,b.i,t);
          return SLERP(SLERP(SLERP(a.p,a.o,t),q1,t),SLERP(q1,SLERP(b.i,b.p,t),t),t);
  */
}

static inline void make_seg_smooth(PosKey a, PosKey &b, PosKey &c, PosKey d)
{
  float l = Length(c.p - b.p);
  Point3 t = c.p - a.p;
  float tl = Length(t);

  b.o = b.p + t * (l / (tl * 3));
  t = d.p - b.p;
  tl = Length(t);
  c.i = c.p - t * (l / (tl * 3));
}

static inline float quatang(Quat a, Quat b)
{
  float f = fabsf(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w);
  if (f >= 1)
    return 0;
  return acosf(f);
}

static inline void make_seg_smooth(RotKey a, RotKey &b, RotKey &c, RotKey d)
{
  float l = quatang(b.p, c.p);
  Quat cp = SLERP(SLERP(b.p, a.p, -1.0f / 3.0f), SLERP(b.p, c.p, 1.0f / 3.0f), 0.5f);
  float tl = quatang(b.p, cp);
  if (tl != 0)
    cp = SLERP(b.p, cp, l / (tl * 3));
  else
    cp = b.p;
  b.o = SLERP(cp, c.p, -0.5f);
  cp = SLERP(SLERP(c.p, b.p, -1.0f / 3.0f), SLERP(c.p, d.p, 1.0f / 3.0f), 0.5f);
  cp = SLERP(c.p, cp, -1);
  tl = quatang(c.p, cp);
  if (tl != 0)
    cp = SLERP(c.p, cp, l / (tl * 3));
  else
    cp = c.p;
  c.i = SLERP(cp, b.p, -0.5f);
}

static DWORD WINAPI dummyfn(LPVOID a) { return 0; }
static void debug_pos_diff(const char *nodename, const Tab<PosKey> &pos, const Tab<Point3> &temp_pos, float thres);
static void debug_rot_diff(const char *nodename, const Tab<RotKey> &pos, const Tab<Quat> &temp_pos, float thres);

static void optimize_pos_key(Tab<PosKey> &pos, int ktime_count, bool reduce_keys, Interval limit, float pos_thr, char *label)
{
  Tab<Point3> temp_pos;
  int total_pos = 1, removed_pos1 = 0, removed_pos2 = 0;
  int i, j, ok;

  temp_pos.SetCount(ktime_count);
  for (i = 0; i < ktime_count; i++)
    temp_pos[i] = pos[i * 3].p;

  for (i = pos.Count() - 1; i >= 0; i--)
    if (pos[i].f)
      pos.Delete(i, 1);

  if (reduce_keys)
  {
    PosKey nk[2];
    Point3 p13, p23;
    int temp_t;

    do
    {
      ok = 0;

      for (i = pos.Count() - 2; i >= 1; i--)
      {
        // calc virtually new segment
        nk[0].p = pos[i - 1].p;
        nk[0].t = pos[i - 1].t;
        nk[1].p = pos[i + 1].p;
        nk[1].t = pos[i + 1].t;

        temp_t = (pos[i - 1].t * 2 + pos[i + 1].t * 1) / 3;
        if (temp_t <= pos[i].t)
          p13 = interp_seg(pos[i - 1], pos[i], temp_t);
        else
          p13 = interp_seg(pos[i], pos[i + 1], temp_t);

        temp_t = (pos[i - 1].t * 1 + pos[i + 1].t * 2) / 3;
        if (temp_t <= pos[i].t)
          p23 = interp_seg(pos[i - 1], pos[i], temp_t);
        else
          p23 = interp_seg(pos[i], pos[i + 1], temp_t);
        calc_cp(nk[0], nk[1], p13, p23);

        // check i-th point with vnew virtual segment
        Point3 p = interp_seg(nk[0], nk[1], pos[i].t);
        total_pos++;

        if (pos_equal(p, pos[i].p, pos_thr))
        {
          removed_pos1++;
          bool really_remove = true;

          for (j = pos[i - 1].t; j <= pos[i + 1].t; j += GetTicksPerFrame())
          {
            int ki = (j - limit.Start()) / GetTicksPerFrame();
            if (!pos_equal(temp_pos[ki], interp_seg(nk[0], nk[1], j), pos_thr))
            {
              really_remove = false;
              break;
            }
          }

          if (really_remove)
          {
            // replace segment with new (previously computed)
            pos[i - 1].o = nk[0].o;
            pos[i + 1].i = nk[1].i;

            // remove unused i-th point
            removed_pos2++;
            pos.Delete(i, 1);
            ok = 1;
          }
        }
      }
    } while (ok);
  }

  if (pos.Count() == 2)
  {
    TimeValue dt = pos[1].t - pos[0].t;
    int i;
    for (i = 1; i <= 5; ++i)
    {
      Point3 p = interp_seg(pos[0], pos[1], dt * i / 5 + pos[0].t);
      if (!pos_equal(p, pos[0].p, pos_thr))
        break;
    }
    if (i > 5)
      pos.Delete(1, 1);
  }

  // debug ( "---- after full %s optimize ----", label );
  // debug ( "%s: total=%d removed1=%d removed2=%d", label, total_pos, removed_pos1, removed_pos2 );
  // debug_pos_diff ( node[k]->GetName(), pos, temp_pos, pos_thr );
}

static void optimize_rot_key(Tab<RotKey> &rot, int ktime_count, bool reduce_keys, Interval limit, float rot_thr, char *label)
{
  Tab<Quat> temp_rot;
  int total_rot = 1, removed_rot1 = 0, removed_rot2 = 0;
  int i, j, ok;

  temp_rot.SetCount(ktime_count);
  for (i = 0; i < ktime_count; i++)
    temp_rot[i] = rot[i * 3].p;

  for (i = rot.Count() - 1; i >= 0; i--)
    if (rot[i].f)
      rot.Delete(i, 1);

  if (reduce_keys)
  {
    RotKey nk[2];
    Quat p13, p23;
    int temp_t;

    do
    {
      ok = 0;

      for (i = rot.Count() - 2; i >= 1; i--)
      {
        // calc virtually new segment
        nk[0].p = rot[i - 1].p;
        nk[0].t = rot[i - 1].t;
        nk[1].p = rot[i + 1].p;
        nk[1].t = rot[i + 1].t;

        temp_t = (rot[i - 1].t * 2 + rot[i + 1].t * 1) / 3;
        if (temp_t <= rot[i].t)
          p13 = interp_seg(rot[i - 1], rot[i], temp_t);
        else
          p13 = interp_seg(rot[i], rot[i + 1], temp_t);

        temp_t = (rot[i - 1].t * 1 + rot[i + 1].t * 2) / 3;
        if (temp_t <= rot[i].t)
          p23 = interp_seg(rot[i - 1], rot[i], temp_t);
        else
          p23 = interp_seg(rot[i], rot[i + 1], temp_t);

        calc_cp(nk[0], nk[1], p13, p23);

        // check i-th point with vnew virtual segment
        Quat p = interp_seg(nk[0], nk[1], rot[i].t);
        total_rot++;
        if (rot_equal(p, rot[i].p, rot_thr))
        {
          removed_rot1++;
          bool really_remove = true;

          for (j = rot[i - 1].t; j <= rot[i + 1].t; j += GetTicksPerFrame())
          {
            int ki = (j - limit.Start()) / GetTicksPerFrame();
            p = interp_seg(nk[0], nk[1], j);

            if (!rot_equal(temp_rot[ki], p, rot_thr))
            {
              really_remove = false;
              break;
            }
          }

          if (really_remove)
          {
            // replace segment with new (previously computed)
            rot[i - 1].o = nk[0].o;
            rot[i + 1].i = nk[1].i;

            // remove unused i-th point
            removed_rot2++;
            rot.Delete(i, 1);
            ok = 1;
          }
        }
      }
    } while (ok);
  }

  if (rot.Count() == 2)
  {
    TimeValue dt = rot[1].t - rot[0].t;
    int i;
    for (i = 1; i <= 5; ++i)
    {
      Quat p = interp_seg(rot[0], rot[1], dt * i / 5 + rot[0].t);
      if (!rot_equal(p, rot[0].p, rot_thr))
        break;
    }
    if (i > 5)
      rot.Delete(1, 1);
  }

  // debug ( "---- after full %s optimize ----", label );
  // debug ( "%s: total=%d removed1=%d removed2=%d", label, total_rot, removed_rot1, removed_rot2 );
  // debug_rot_diff ( node[k]->GetName(), rot, temp_rot, rot_thr );
}

bool get_tm_anim_2(Tab<AnimChanPoint3 *> &npos, Tab<AnimChanQuat *> &nrot, Tab<AnimChanPoint3 *> &nscl, const Interval &limit,
  const Tab<INode *> &node, const Tab<ExpTMAnimCB *> &ncb, float pos_thr, float rot_thr, float scl_thr, float ort_eps, int expflags)
{
  int i, k;
  int t0, t1;

  /*
  int total_pos = 0, removed_pos1 = 0, removed_pos2 = 0;
  int total_rot = 0, removed_rot1 = 0, removed_rot2 = 0;
  int total_scl = 0, removed_scl1 = 0, removed_scl2 = 0;
  */

  ort_thr = ort_eps;
  lim = limit;
  ktime.ZeroCount();

  if (lim.Start() == TIME_NegInfinity)
    lim.SetInstant(0);

  if (lim.Start() == lim.End())
    add_ktime(lim.Start());
  else
  {
    for (i = lim.Start(); i < lim.End(); i += GetTicksPerFrame())
      add_ktime(i, false);
    add_ktime(lim.End());
  }
  sort_ktime();
  debug("ktime.Count()=%d", ktime.Count());

  for (k = 0; k < node.Count(); k++)
  {
    Tab<PosKey> &pos = npos[k]->key;
    Tab<PosKey> &scl = nscl[k]->key;
    Tab<RotKey> &rot = nrot[k]->key;
    ExpTMAnimCB &cb = *ncb[k];

    if (ktime.Count() > 1)
    {
      pos.SetCount(ktime.Count() * 3);
      scl.SetCount(ktime.Count() * 3);
      rot.SetCount(ktime.Count() * 3);
    }
    else
    {
      pos.SetCount(ktime.Count());
      scl.SetCount(ktime.Count());
      rot.SetCount(ktime.Count());
    }

    pos[0].t = rot[0].t = scl[0].t = ktime[0];
    pos[0].f = rot[0].f = scl[0].f = 0;
    interp_tm(ktime[0], pos[0].p, rot[0].p, scl[0].p, cb);
  }

  if (ktime.Count() == 1)
    return true;

  assert(ktime.Count() > 1);

  Interface *ip = GetCOREInterface();

  // sample keys
  debug("start sampling");
  ip->ProgressStart(_T("Exporting Anim v2..."), FALSE, dummyfn, NULL);
  t0 = 0;
  for (i = 1; i < ktime.Count() * 3; ++i)
  {
    t1 = timeGetTime();
    if (t1 - t0 > 200)
    {
      ip->ProgressUpdate(i * 100 / (ktime.Count() * 3), FALSE, _T("Sampling"));
      t0 = t1;
    }
    TimeValue t = ktime[i / 3] + (i % 3) * GetTicksPerFrame() / 3;
    int temp_f = (i % 3) == 0 ? 0 : 1;

    for (k = 0; k < node.Count(); k++)
    {
      Tab<PosKey> &pos = npos[k]->key;
      Tab<PosKey> &scl = nscl[k]->key;
      Tab<RotKey> &rot = nrot[k]->key;
      ExpTMAnimCB &cb = *ncb[k];

      pos[i].t = rot[i].t = scl[i].t = t;
      pos[i].f = rot[i].f = scl[i].f = temp_f;
      interp_tm(t, pos[i].p, rot[i].p, scl[i].p, cb);
    }
  }

  // create interpolation data
  debug("start interpolating");
  ip->ProgressUpdate(0, FALSE, _T("Interpolating"));
  t0 = 0;

  for (k = 0; k < node.Count(); k++)
  {
    Tab<PosKey> &pos = npos[k]->key;
    Tab<PosKey> &scl = nscl[k]->key;
    Tab<RotKey> &rot = nrot[k]->key;
    ExpTMAnimCB &cb = *ncb[k];

    t1 = timeGetTime();
    if (t1 - t0 > 200)
    {
      ip->ProgressUpdate(k * 100 / node.Count(), FALSE, _T("Interpolating"));
      t0 = t1;
    }
    for (i = 0; i < ktime.Count() - 1; ++i)
    {
      calc_cp(pos[i * 3], pos[i * 3 + 3], pos[i * 3 + 1].p, pos[i * 3 + 2].p);
      calc_cp(rot[i * 3], rot[i * 3 + 3], rot[i * 3 + 1].p, rot[i * 3 + 2].p);
      calc_cp(scl[i * 3], scl[i * 3 + 3], scl[i * 3 + 1].p, scl[i * 3 + 2].p);
    }

    if (expflags & EXP_LOOPED_ANIM)
    {
      i = ktime.Count() - 1;
      calc_cp(pos[i * 3], pos[0], pos[i * 3 + 1].p, pos[i * 3 + 2].p);
    }
    else
    {
      pos[0].i = pos[0].p * 2 - pos[0].o;
      rot[0].i = SLERP(rot[0].p, rot[0].o, -1);
      scl[0].i = scl[0].p * 2 - scl[0].o;

      i = ktime.Count() - 1;
      pos[i].o = pos[i].p * 2 - pos[i].i;
      rot[i].o = SLERP(rot[i].p, rot[i].i, -1);
      scl[i].o = scl[i].p * 2 - scl[i].i;
    }
  }

  debug("removing unused keys: pos_thr=%.7f  rot_thr=%.7f", pos_thr, rot_thr);
  ip->ProgressUpdate(0, FALSE, _T("Removing keys"));
  t0 = 0;
  for (k = 0; k < node.Count(); k++)
  {
    Tab<PosKey> &pos = npos[k]->key;
    Tab<PosKey> &scl = nscl[k]->key;
    Tab<RotKey> &rot = nrot[k]->key;
    ExpTMAnimCB &cb = *ncb[k];

    t1 = timeGetTime();
    if (t1 - t0 > 200)
    {
      ip->ProgressUpdate(k * 100 / node.Count(), FALSE, _T("Removing keys"));
      t0 = t1;
    }

    // removing unused keys: pos
    optimize_pos_key(pos, ktime.Count(), !(expflags & EXP_DONT_REDUCE_POS), lim, pos_thr, "pos");
    optimize_pos_key(scl, ktime.Count(), !(expflags & EXP_DONT_REDUCE_SCL), lim, scl_thr, "scl");
    optimize_rot_key(rot, ktime.Count(), !(expflags & EXP_DONT_REDUCE_ROT), lim, rot_thr, "rot");

    // report results
    /*
    debug ("node %d: %s", k, cb.get_name ());
    debug ("pos.Count()=%d", pos.Count ());
    debug ("scl.Count()=%d", scl.Count ());
    debug ("rot.Count()=%d", rot.Count ());
    */
  }
  /*
  debug ( "total_pos=%d removed_pos1=%d removed_pos2=%d", total_pos, removed_pos1, removed_pos2 );
  debug ( "total_rot=%d removed_rot1=%d removed_rot2=%d", total_rot, removed_rot1, removed_rot2 );
  debug ( "total_scl=%d removed_scl1=%d removed_scl2=%d", total_scl, removed_scl1, removed_scl2 );
  */
  ip->ProgressEnd();

  return true;
}

Point3 getAngularVelocity(const Quat &q0, const Quat &q1, float dt)
{
  Quat dq = q1 * Conjugate(q0);
  dq.Normalize();

  double _1_w2 = 1 - dq.w * dq.w;
  if (dq.w < 0)
    dq = -dq;
  /*debug ( "q0=%.5f,%.5f,%.5f,%.5f\nq1=%.5f,%.5f,%.5f,%.5f\ndq=%.5f,%.5f,%.5f,%.5f\n1-w*w=%.9f",
          q0.x, q0.y, q0.z, q0.w,
          q1.x, q1.y, q1.z, q1.w,
          dq.x, dq.y, dq.z, dq.w,
          _1_w2 );*/

  if (_1_w2 < 1e-7)
    return Point3(0, 0, 0);

  Point3 wvel = Point3(dq.x, dq.y, dq.z);
  wvel *= (2 * acos(dq.w) / sqrt(_1_w2)) / dt;
  // debug ( "wvel=%.5f, %.5f, %.5f  dt=%.3f", wvel.x, wvel.y, wvel.z, dt );
  return wvel;
}

bool get_node_vel(Tab<AnimKeyPoint3> &lin_vel, Tab<AnimKeyPoint3> &ang_vel, Tab<int> &lin_vel_t, Tab<int> &ang_vel_t,
  const Interval &limit, ExpTMAnimCB &cb, float pos_thr, float rot_thr, int expflags)
{
  int i;
  int t0, t1;

  lim = limit;
  ktime.ZeroCount();

  if (lim.Start() == TIME_NegInfinity)
    lim.SetInstant(0);

  if (lim.Start() == lim.End())
    add_ktime(lim.Start());
  else
  {
    for (i = lim.Start(); i < lim.End(); i += GetTicksPerFrame())
      add_ktime(i, false);
    add_ktime(lim.End());
  }
  sort_ktime();
  debug("nodevel: ktime.Count()=%d", ktime.Count());

  Tab<PosKey> pos;
  Tab<RotKey> rot;
  Point3 scl;

  // create temporary structures
  if (ktime.Count() > 1)
  {
    pos.SetCount(ktime.Count() * 3);
    rot.SetCount(ktime.Count() * 3);
  }
  else
  {
    pos.SetCount(ktime.Count());
    rot.SetCount(ktime.Count());
  }

  pos[0].t = rot[0].t = ktime[0];
  pos[0].f = rot[0].f = 0;
  interp_tm(ktime[0], pos[0].p, rot[0].p, scl, cb);

  assert(ktime.Count() >= 1);

  Interface *ip = GetCOREInterface();
  ip->ProgressStart(_T("Exporting Anim v2..."), FALSE, dummyfn, NULL);

  if (ktime.Count() > 1)
  {
    // sample keys
    debug("start sampling origin pos/rot");

    t0 = 0;
    for (i = 1; i < ktime.Count() * 3; ++i)
    {
      t1 = timeGetTime();
      if (t1 - t0 > 200)
      {
        ip->ProgressUpdate(i * 100 / (ktime.Count() * 3), FALSE, _T("Sampling node pos/rot"));
        t0 = t1;
      }
      TimeValue t = ktime[i / 3] + (i % 3) * GetTicksPerFrame() / 3;
      int temp_f = (i % 3) == 0 ? 0 : 1;

      pos[i].t = rot[i].t = t;
      pos[i].f = rot[i].f = temp_f;
      interp_tm(t, pos[i].p, rot[i].p, scl, cb);
    }

    // transform origin movement to origin-relative coord system
    Point3 or_pos = Point3(0, 0, 0), d_pos;
    Point3 l_pos = pos[0].p;
    Matrix3 or_rot;

    for (i = 0; i < ktime.Count() * 3 - 1; ++i)
    {
      d_pos = pos[i + 1].p - l_pos;
      or_rot = make_tm(Point3(0, 0, 0), rot[i].p, Point3(1, 1, 1));
      pos[i].p = or_pos;
      or_pos += Inverse(or_rot) * d_pos;
      //==debug ( "p=(%.3f,%.3f,%.3f) -> (%.3f,%.3f,%.3f)", l_pos.x, l_pos.y, l_pos.z, pos[i].p.x, pos[i].p.y, pos[i].p.z );

      l_pos = pos[i + 1].p;
    }

    // create interpolation data
    debug("start interpolating");
    ip->ProgressUpdate(0, FALSE, _T("Processing node rot/pos"));

    for (i = 0; i < ktime.Count() - 1; ++i)
    {
      calc_cp(pos[i * 3], pos[i * 3 + 3], pos[i * 3 + 1].p, pos[i * 3 + 2].p);
      calc_cp(rot[i * 3], rot[i * 3 + 3], rot[i * 3 + 1].p, rot[i * 3 + 2].p);
    }

    if (expflags & EXP_LOOPED_ANIM)
    {
      i = ktime.Count() - 1;
      calc_cp(pos[i * 3], pos[0], pos[i * 3 + 1].p, pos[i * 3 + 2].p);
    }
    else
    {
      pos[0].i = pos[0].p * 2 - pos[0].o;
      rot[0].i = SLERP(rot[0].p, rot[0].o, -1);

      i = ktime.Count() - 1;
      pos[i].o = pos[i].p * 2 - pos[i].i;
      rot[i].o = SLERP(rot[i].p, rot[i].i, -1);
    }

    debug("removing unused keys: pos_thr=%.7f rot_eps=%.7f", pos_thr, rot_thr);
    ip->ProgressUpdate(30, FALSE, _T("Processing node rot/pos"));

    // removing unused keys: pos
    optimize_pos_key(pos, ktime.Count(), true, lim, pos_thr, "origin pos");
    optimize_rot_key(rot, ktime.Count(), true, lim, rot_thr, "origin rot");
  }
  debug("calculating velocities");
  ip->ProgressUpdate(70, FALSE, _T("Processing node rot/pos"));

  debug("pos.Count()=%d rot.Count()=%d", pos.Count(), rot.Count());

  // calculate linear velocity
  lin_vel.SetCount(pos.Count());
  lin_vel_t.SetCount(pos.Count());

  // float sum = 0;
  for (i = pos.Count() - 1; i >= 0; i--)
  {
    AnimKeyPoint3 k;
    int dt;

    lin_vel_t[i] = pos[i].t;
    // debug ( "%5d: t=%5d  pos[i].p=%.3f,%.3f,%.3f", i, pos[i].t, pos[i].p.x, pos[i].p.y, pos[i].p.z );

    if (i + 1 >= pos.Count())
    {
      if (!(expflags & EXP_LOOPED_ANIM))
      {
        memset(&lin_vel[i], 0, sizeof(AnimKeyPoint3));
        continue;
      }

      k.p = pos[i].p;
      k.k1 = (pos[i].o - pos[i].p) * 3;
      k.k2 = (pos[i].p + pos[0].i - pos[i].o * 2) * 3;
      k.k3 = (pos[i].o - pos[0].i) * 3 + pos[0].p - pos[i].p;
      dt = pos[0].t - pos[i].t + (lim.End() - lim.Start() + 1);
      if (dt < 1)
        dt = 1;
    }
    else
    {
      k.p = pos[i].p;
      k.k1 = (pos[i].o - pos[i].p) * 3;
      k.k2 = (pos[i].p + pos[i + 1].i - pos[i].o * 2) * 3;
      k.k3 = (pos[i].o - pos[i + 1].i) * 3 + pos[i + 1].p - pos[i].p;
      dt = pos[i + 1].t - pos[i].t;
    }

    lin_vel[i].p = k.k1 / dt;
    lin_vel[i].k1 = 2 * k.k2 / dt; // dt;
    lin_vel[i].k2 = 3 * k.k3 / dt; // dt/dt;
    lin_vel[i].k3 = Point3(0, 0, 0);

    //==debug ( "%3d: t=%5d  lin_vel[i].p=%.7f,%.7f,%.7f  dt=%d", i, lin_vel_t[i], lin_vel[i].p.x, lin_vel[i].p.y, lin_vel[i].p.z, dt
    //);

    /*
    debug ( "%3d: t=%5d  lin_vel[i].p=%.7f,%.7f,%.7f  p=(%.5f,%.5f,%.5f) k1=(%.5f,%.5f,%.5f) k2=(%.5f,%.5f,%.5f) k3=(%.5f,%.5f,%.5f),
    dt=%d", i, lin_vel_t[i], lin_vel[i].p.x, lin_vel[i].p.y, lin_vel[i].p.z, k.p.x, k.p.y, k.p.z, k.k1.x, k.k1.y, k.k1.z, k.k2.x,
    k.k2.y, k.k2.z, k.k3.x, k.k3.y, k.k3.z, dt );

    float step_x = 0.02;
    for ( float px = 1.0; px >= 0.0; px -= step_x ) {
      float vel_z = (lin_vel[i].k2.z*px+lin_vel[i].k1.z)*px+lin_vel[i].p.z;
      sum += (vel_z * dt) * step_x;
      debug ( "p.z=%.8f  vel=%.8f  sum=%.8f", ((k.k3.z*px+k.k2.z)*px+k.k1.z)*px+k.p.z, vel_z, sum );
    }
    debug ( "approx. len: %.7f", sum );
    */
  }

  // calculate angular velocity
  ang_vel.SetCount(rot.Count());
  ang_vel_t.SetCount(rot.Count());

  for (i = rot.Count() - 1; i >= 0; i--)
    rot[i].p.Normalize();

  for (i = rot.Count() - 1; i >= 0; i--)
  {
    AnimKeyPoint3 k;
    Quat drot;

    ang_vel_t[i] = rot[i].t;
    // debug ( "segment i=%d, rot.count=%d", i, rot.Count());

    if (i + 1 >= rot.Count())
    {
      memset(&ang_vel[i], 0, sizeof(AnimKeyPoint3));
      debug("fr %4d: vel = 0", rot[i].t / 160);
      continue;
    }

    PosKey nk[2];
    Point3 p13, p23;
    int temp_t;

    nk[0].t = rot[i].t;
    nk[1].t = rot[i + 1].t;

    const int deriv_dt = 48;

    if (i > 0)
    {
      nk[0].p = getAngularVelocity(interp_seg(rot[i - 1], rot[i], rot[i].t - deriv_dt / 2),
        interp_seg(rot[i], rot[i + 1], rot[i].t + deriv_dt / 2), deriv_dt);
      /*debug ( "nk0: (%d:%d, %d:%d, %d), (%d:%d, %d:%d, %d), %d",
              i-1, rot[i-1].t, i, rot[i].t, rot[i].t-deriv_dt/2,
              i, rot[i].t, i+1, rot[i+1].t, rot[i].t+deriv_dt/2, deriv_dt );*/
    }
    else
    { // extrapolate then
      nk[0].p = getAngularVelocity(interp_seg(rot[i], rot[i + 1], rot[i].t - deriv_dt / 2),
        interp_seg(rot[i], rot[i + 1], rot[i].t + deriv_dt / 2), deriv_dt);
      /*debug ( "nk0: (%d:%d, %d:%d, %d), (%d:%d, %d:%d, %d), %d",
              i, rot[i].t, i+1, rot[i+1].t, rot[i].t-deriv_dt/2,
              i, rot[i].t, i+1, rot[i+1].t, rot[i].t+deriv_dt/2, deriv_dt );*/
    }

    if (i + 2 < rot.Count())
    {
      nk[1].p = getAngularVelocity(interp_seg(rot[i], rot[i + 1], rot[i + 1].t - deriv_dt / 2),
        interp_seg(rot[i + 1], rot[i + 2], rot[i + 1].t + deriv_dt / 2), deriv_dt);
      /*debug ( "nk1: (%d:%d, %d:%d, %d), (%d:%d, %d:%d, %d), %d",
              i, rot[i].t, i+1, rot[i+1].t, rot[i+1].t-deriv_dt/2,
              i+1, rot[i+1].t, i+2, rot[i+2].t, rot[i+1].t+deriv_dt/2, deriv_dt );*/
    }
    else
    { // extrapolate then
      nk[1].p = getAngularVelocity(interp_seg(rot[i], rot[i + 1], rot[i + 1].t - deriv_dt / 2),
        interp_seg(rot[i], rot[i + 1], rot[i + 1].t + deriv_dt / 2), deriv_dt);
      /*debug ( "nk1: (%d:%d, %d:%d, %d), (%d:%d, %d:%d, %d), %d",
              i, rot[i].t, i+1, rot[i+1].t, rot[i+1].t-deriv_dt/2,
              i, rot[i].t, i+1, rot[i+1].t, rot[i+1].t+deriv_dt/2, deriv_dt );*/
    }

    temp_t = (rot[i].t * 2 + rot[i + 1].t * 1) / 3;
    p13 = getAngularVelocity(interp_seg(rot[i], rot[i + 1], temp_t - deriv_dt / 2),
      interp_seg(rot[i], rot[i + 1], temp_t + deriv_dt / 2), deriv_dt);
    temp_t = (rot[i].t * 1 + rot[i + 1].t * 2) / 3;
    p23 = getAngularVelocity(interp_seg(rot[i], rot[i + 1], temp_t - deriv_dt / 2),
      interp_seg(rot[i], rot[i + 1], temp_t + deriv_dt / 2), deriv_dt);
    debug("fr %4d: dt=%5d  t=%5d  w.y=%.8f; t=%5d  w.y=%.8f; t=%5d  w.y=%.8f; t=%5d  w.y=%.8f", nk[0].t / 160, nk[1].t - nk[0].t,
      nk[0].t, nk[0].p.y, (rot[i].t * 2 + rot[i + 1].t * 1) / 3, p13.y, (rot[i].t * 1 + rot[i + 1].t * 2) / 3, p23.y, nk[1].t,
      nk[1].p.y);
    calc_cp(nk[0], nk[1], p13, p23);

    ang_vel[i].p = nk[0].p;
    ang_vel[i].k1 = (nk[0].o - nk[0].p) * 3;
    ang_vel[i].k2 = (nk[0].p + nk[1].i - nk[0].o * 2) * 3;
    ang_vel[i].k3 = (nk[0].o - nk[1].i) * 3 + nk[1].p - nk[0].p;

    /*
        debug ( "%3d: time=%5d  ang_vel[i].p=%.7f,%.7f,%.7f ang_vel[i].k1=%.7f,%.7f,%.7f ang_vel[i].k2=%.7f,%.7f,%.7f
       ang_vel[i].k3=%.7f,%.7f,%.7f", i, ang_vel_t[i], ang_vel[i].p.x, ang_vel[i].p.y, ang_vel[i].p.z, ang_vel[i].k1.x,
       ang_vel[i].k1.y, ang_vel[i].k1.z, ang_vel[i].k2.x, ang_vel[i].k2.y, ang_vel[i].k2.z, ang_vel[i].k3.x, ang_vel[i].k3.y,
       ang_vel[i].k3.z );
    */
  }

  /*
    float sumrot = 0;
    for ( i = 0; i < rot.Count()-1; i ++ ) {
      const int dt = ang_vel_t[i+1]-ang_vel_t[i];
      float step_x = 48.0 / dt;
      for ( float px = 0.0; px < 1.0; px += step_x ) {
        float vel_y = ((ang_vel[i].k3.y*px+ang_vel[i].k2.y)*px+ang_vel[i].k1.y)*px+ang_vel[i].p.y;
        vel_y *= dt;
        sumrot += vel_y * step_x;
        debug ( "fr %d: vel=%.8f  sum=%.8f", int(ang_vel_t[i]+dt*px)/160, vel_y, sumrot*180/PI );
      }
    }
    debug ( "approx. rot: %.7f", sumrot );
  */

  ip->ProgressEnd();

  return true;
}

static void debug_rot_diff(const char *nodename, const Tab<RotKey> &pos, const Tab<Quat> &temp_pos, float thres)
{
  int i, j;

  debug("node <%s> rot points (thres=%.5f):", nodename, thres);
  for (i = 0; i < ktime.Count() - 1; ++i)
  {
    Quat p = Quat(-10000.0f, -10000.0f, -10000.0f, -10000.0f), dp;
    if (ktime[i] <= pos[0].t)
      p = pos[0].p;
    else if (ktime[i] >= pos[pos.Count() - 1].t)
      p = pos[pos.Count() - 1].p;
    else
      for (j = 0; j < pos.Count() - 1; j++)
        if (pos[j].t <= ktime[i] && ktime[i] < pos[j + 1].t)
        {
          p = interp_seg(pos[j], pos[j + 1], ktime[i]);
          break;
        }

    dp = p - temp_pos[i];
    if (!rot_equal(p, temp_pos[i], thres))
      debug("!!! %5d: dp=(%8.5f, %8.5f, %8.5f, %8.5f) p=(%9.5f, %9.5f, %9.5f, %9.5f)", ktime[i], dp.x, dp.y, dp.z, dp.w, p.x, p.y, p.z,
        p.w);
    // else
    //   debug ( "    %5d:  p=(%9.5f, %9.5f, %9.5f, %9.5f)", ktime[i], p.x, p.y, p.z, p.w );
  }
}
static void debug_pos_diff(const char *nodename, const Tab<PosKey> &pos, const Tab<Point3> &temp_pos, float thres)
{
  return;

  int i, j;

  debug("node <%s> pos points (thres=%.5f):", nodename, thres);
  for (i = 0; i < ktime.Count() - 1; ++i)
  {
    Point3 p = Point3(-10000, -10000, -10000), dp;
    if (ktime[i] <= pos[0].t)
      p = pos[0].p;
    else if (ktime[i] >= pos[pos.Count() - 1].t)
      p = pos[pos.Count() - 1].p;
    else
      for (j = 0; j < pos.Count() - 1; j++)
        if (pos[j].t <= ktime[i] && ktime[i] < pos[j + 1].t)
        {
          p = interp_seg(pos[j], pos[j + 1], ktime[i]);
          break;
        }

    dp = p - temp_pos[i];
    if (dp.x * dp.x + dp.y * dp.y + dp.z * dp.z > thres * thres)
      debug("!!! %5d: dp=(%8.5f, %8.5f, %8.5f) p=(%9.5f, %9.5f, %9.5f)", ktime[i], dp.x, dp.y, dp.z, p.x, p.y, p.z);
    else
      debug("    %5d:  p=(%9.5f, %9.5f, %9.5f)", ktime[i], p.x, p.y, p.z);
  }
}
