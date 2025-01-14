// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "a2dOptimizer.h"
#include <anim/dag_animChannels.h>
#include <anim/dag_animKeyInterp.h>
#include <generic/dag_tab.h>
#include <debug/dag_debug.h>
#include <vecmath/dag_vecMath.h>

#define DUMP_PRE_OPT  0
#define DUMP_POST_OPT 0

#if DUMP_POST_OPT
float quat_max_div = 0, quat_sum_div = 0;
int quat_num_div = 0;

static Quat interp_key2(const OldAnimKeyQuat &a, const OldAnimKeyQuat &b, real t)
{
  Quat_vec4 res;
  v_st(&res, v_quat_qsquad(t, v_ldu(a.p), v_ldu(a.b0), v_ldu(a.b1), v_ldu(b.p)));
  return normalize(res);
}
#endif

using namespace animopt;

static const int TICKS_PER_FRAME = 4800 / 30;

static inline const Quat &no_conjugate(const Quat &src) { return src; }

/* this one remains here for history; it is not needed since we don't need to convert 3dsMax's Quat to Dagor's Quat
static inline Quat conjugate(const Quat &src)
{
  Quat ret;
  ret.x = -src.x;
  ret.y = -src.y;
  ret.z = -src.z;
  ret.w = src.w;
  return ret;
}
*/

static inline Quat qadd(Quat a, Quat b)
{
  Quat q(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
  return normalize(q);
}
static inline Quat SLERP(Quat a, Quat b, float t)
{
  double f = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  if (f >= 0.9999999)
    return qadd(a * (1 - t), b * t);
  else if (f <= -0.9999999)
    return qadd(a * (t - 1), b * t);
  else
  {
    double w = acos(f);
    if (f >= 0)
    {
      double sinw = sin(w);
      return qadd(a * (sin(w * (1 - t)) / sinw), b * (sin(w * t) / sinw));
    }
    w = PI - w;
    double sinw = sin(w);
    return qadd(a * (-sin(w * (1 - t)) / sinw), b * (sin(w * t) / sinw));
  }
}

static inline Point3 interp_key(OldAnimKeyPoint3 a, OldAnimKeyPoint3 /*b*/, real t)
{
  return ((a.k3 * t + a.k2) * t + a.k1) * t + a.p;
}

static inline Quat interp_key(OldAnimKeyQuat a, OldAnimKeyQuat b, real t)
{
  return SLERP(SLERP(a.p, b.p, t), SLERP(a.b0, a.b1, t), 2 * (1 - t) * t);
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
}

static Quat interp_seg2(const RotKey &a, const RotKey &b, TimeValue time)
{
  Quat_vec4 res;
  float t = float(time - a.t) / float(b.t - a.t);
  v_st(&res.x, v_quat_qsquad(t, v_ldu(&a.p.x), v_ldu(&a.o.x), v_ldu(&b.i.x), v_ldu(&b.p.x)));
  return normalize(res);
}


static inline void calc_cp(PosKey &k0, PosKey &k1, Point3 p13, Point3 p23)
{
  k0.o = k0.p * (-5.0f / 6.0f) + k1.p * (1.0f / 3.0f) + p13 * 3 - p23 * 1.5f;
  k1.i = k1.p * (-5.0f / 6.0f) + k0.p * (1.0f / 3.0f) + p23 * 3 - p13 * 1.5f;
}

static inline void calc_cp(RotKey &k0, RotKey &k1, Quat p13, Quat p23)
{
#if 1
  Quat q0 = SLERP(SLERP(k0.p, k1.p, 1.0f / 3.0f), p13, 9.0f / 4.0f);
  Quat q1 = SLERP(SLERP(k1.p, k0.p, 1.0f / 3.0f), p23, 9.0f / 4.0f);
  k0.o = SLERP(q0, q1, -1);
  k1.i = SLERP(q1, q0, -1);
#else
  quat4f q0p = v_ldu(&k0.p.x);
  quat4f q1p = v_ldu(&k1.p.x);
  quat4f q13 = v_ldu(&p13.x);
  quat4f q23 = v_ldu(&p23.x);
  quat4f q0 = v_quat_qslerp(9.0f / 4.0f, v_quat_qslerp(1.0f / 3.0f, q0p, q1p), q13);
  quat4f q1 = v_quat_qslerp(9.0f / 4.0f, v_quat_qslerp(1.0f / 3.0f, q1p, q0p), q23);

  v_stu(&k0.o.x, v_quat_qslerp(-1, q0, q1));
  v_stu(&k1.i.x, v_quat_qslerp(-1, q1, q0));
#endif
}

static inline bool rot_equal(Quat a, Quat b, double t)
{
  return fabs(double(a.x) * b.x + double(a.y) * b.y + double(a.z) * b.z + double(a.w) * b.w) >= t ||
         sqr(double(a.x) - b.x) + sqr(double(a.y) - b.y) + sqr(double(a.z) - b.z) + sqr(double(a.w) - b.w) <=
           max(2.0 - 2.0 * t, 1e-14);
}

static inline bool pos_equal(Point3 a, Point3 b, float t)
{
  return sqr(double(a.x) - b.x) + sqr(double(a.y) - b.y) + sqr(double(a.z) - b.z) <= t * t;
}

static bool key_is_same(const RotKey &key1, const RotKey &key2, double treshold)
{
  return rot_equal(key1.i, key2.i, treshold) && rot_equal(key1.o, key2.o, treshold) && rot_equal(key1.p, key2.p, treshold);
}

static bool key_is_same(const PosKey &key1, const PosKey &key2, float treshold)
{
  return pos_equal(key1.i, key2.i, treshold) && pos_equal(key1.o, key2.o, treshold) && pos_equal(key1.p, key2.p, treshold);
}

static inline Point3 interp_seg(dag::ConstSpan<PosKey> p, int &last_seg, TimeValue time)
{
  if (p[last_seg].t > time)
  {
    if (p[0].t > time)
      return p[0].p;
    else
      last_seg = 0;
  }

  for (; last_seg + 1 < p.size(); last_seg++)
    if (p[last_seg].t <= time && time < p[last_seg + 1].t)
      return interp_seg(p[last_seg], p[last_seg + 1], time);
  return p.back().p;
}
static inline Quat interp_seg(dag::ConstSpan<RotKey> r, int &last_seg, TimeValue time)
{
  if (r[last_seg].t > time)
  {
    if (r[0].t > time)
      return r[0].p;
    else
      last_seg = 0;
  }

  for (; last_seg + 1 < r.size(); last_seg++)
    if (r[last_seg].t <= time && time < r[last_seg + 1].t)
      return interp_seg(r[last_seg], r[last_seg + 1], time);
  return r.back().p;
}

static int find_key(int t, const int *kt, int num, float &frac)
{
  frac = 0;
  if (t < kt[0])
    return 0;
  if (t > kt[num - 1])
    return num - 1;
  for (int i = 0; i + 1 < num; i++)
    if (t >= kt[i] && t < kt[i + 1])
    {
      frac = float(t - kt[i]) / float(kt[i + 1] - kt[i]);
      return i;
    }
  return num - 1;
}

void animopt::convert_to_PosKey(Tab<PosKey> &pos, OldAnimKeyPoint3 *k, const int *kt, int num)
{
  pos.resize(num);
  for (int i = 0; i < num; i++)
  {
    pos[i].p = k[i].p;
    pos[i].t = kt[i];
  }
  pos[0].i = pos[0].p;
  pos[num - 1].o = pos[num - 1].p;

  for (int i = 0; i + 1 < num; i++)
    calc_cp(pos[i], pos[i + 1], interp_key(k[i], k[i + 1], 1.0f / 3.0f), interp_key(k[i], k[i + 1], 2.0f / 3.0f));

#if DUMP_PRE_OPT
  debug("point3 %d key dump (%d..%d)", num, pos[0].t, pos[num - 1].t);
  int last_seg = 0;
  for (int t = pos[0].t, te = pos[num - 1].t; t < te; t += TICKS_PER_FRAME)
  {
    float frac;
    int kp = find_key(t, kt, num, frac);
    Point3 s = interp_key(k[kp], k[kp + 1], frac);
    Point3 d = interp_seg(pos, last_seg, t);
    float diff = length(s - d);
    debug(" %5d: %cdiff=%.3f  " FMT_P3 " -> " FMT_P3 "", t, fabs(diff) > 1e-5 ? '*' : ' ', diff, P3D(s), P3D(d));
  }
#endif
}

void animopt::convert_to_RotKey(Tab<RotKey> &rot, OldAnimKeyQuat *k, const int *kt, int num, int rot_resample_freq)
{
  int t0 = kt[0], t1 = kt[num - 1];
  if (!rot_resample_freq || (rot_resample_freq > 0 && num * 4800 / rot_resample_freq + TICKS_PER_FRAME * 2 >= t1 - t0))
  {
    rot.resize(num);
    for (int i = 0; i < num; i++)
    {
      rot[i].p = k[i].p;
      rot[i].t = kt[i];
    }
    rot[0].i = rot[0].p;
    rot[num - 1].o = rot[num - 1].p;

    for (int i = 0; i + 1 < num; i++)
      calc_cp(rot[i], rot[i + 1], interp_key(k[i], k[i + 1], 1.0f / 3.0f), interp_key(k[i], k[i + 1], 2.0f / 3.0f));
  }
  else if (rot_resample_freq > 0)
  {
    int step = 4800 / rot_resample_freq;
    int rot_num = (t1 - t0) / step + 1;
    if (t0 + rot_num * step < t1)
      rot_num++;

    rot.resize(rot_num);
    rot[0].p = k[0].p;
    rot[0].t = t0;
    rot[rot_num - 1].p = k[num - 1].p;
    rot[rot_num - 1].t = t1;

    int last_key = 0;
    for (int t = t0 + step, kp = 0; t <= t1; t += step)
    {
      float frac;
      kp += find_key(t, kt + kp, num - kp, frac);
      int idx = (t - t0) / step;
      rot[idx].p = interp_key(k[kp], k[kp + 1], frac);
      rot[idx].t = t;
    }
    rot[0].i = rot[0].p;
    rot[rot_num - 1].o = rot[rot_num - 1].p;

    for (int i = 0, kp = 0; i + 1 < rot.size(); i++)
    {
      int dt = rot[i + 1].t - rot[i].t;
      Quat q13, q23;
      float frac;

      kp += find_key(rot[i].t + dt / 3, kt + kp, num - kp, frac);
      q13 = interp_key(k[kp], k[kp + 1], frac);
      kp += find_key(rot[i].t + dt * 2 / 3, kt + kp, num - kp, frac);
      q23 = interp_key(k[kp], k[kp + 1], frac);
      calc_cp(rot[i], rot[i + 1], q13, q23);
    }
  }
  else
    G_ASSERTF(rot_resample_freq >= 0, "rot_resample_freq=%d not supported yet", rot_resample_freq);

#if DUMP_PRE_OPT
  debug("quat   %d(%d) key dump (%d..%d)", num, rot.size(), t0, t1);
  int last_seg = 0;
  for (int t = t0; t <= t1; t += TICKS_PER_FRAME / 2)
  {
    float frac;
    int kp = find_key(t, kt, num, frac);
    Quat s = interp_key(k[kp], k[kp + 1], frac);
    Quat d = interp_seg(rot, last_seg, t);
    float diff = fabs(1.0f - fabsf(s.x * d.x + s.y * d.y + s.z * d.z + s.w * d.w));
    debug(" %5d: %cdiff=%.3f  " FMT_P4 " -> " FMT_P4 "", t, fabs(diff) > 1e-5 ? '*' : ' ', diff, P4D(s), P4D(d));
  }
#endif
}


void animopt::convert_to_OldAnimKeyPoint3(OldAnimKeyPoint3 *k, int *kt, PosKey *s, int num)
{
  for (; num > 1; ++k, ++s, ++kt, --num)
  {
    k->p = s->p;
    k->k1 = (s->o - s->p) * 3;
    k->k2 = (s->p + s[1].i - s->o * 2) * 3;
    k->k3 = (s->o - s[1].i) * 3 + s[1].p - s->p;
    *kt = s->t;
  }
  k->p = s->p;
  k->k1.zero();
  k->k2.zero();
  k->k3.zero();
  *kt = s->t;
}

void animopt::convert_to_OldAnimKeyQuat(OldAnimKeyQuat *k, int *kt, RotKey *s, int num)
{
  char *h = (char *)malloc(num * 2 - 1);
  G_ASSERT(h);
  G_ANALYSIS_ASSUME(h);
  char *x = h + num - 1;
  int i;

  for (i = 0; i < num - 1; ++i)
  {
    kt[i] = s[i].t;
    k[i].p = no_conjugate(s[i].p);
    Quat b0 = no_conjugate(s[i].o), b1 = no_conjugate(s[i + 1].i);
    Quat sip = no_conjugate(s[i].p), sipn = no_conjugate(s[i + 1].p);

    float f = b0.x * b1.x + b0.y * b1.y + b0.z * b1.z + b0.w * b1.w;
    if (f < 0)
    {
      b1 = -b1;
      f = -f;
    }
    k[i].b0 = b0;
    k[i].b1 = b1;
    if (f >= 1)
      k[i].sinbw = k[i].bw = 0;
    else
      k[i].sinbw = sin(k[i].bw = acos(f));

    f = sip.x * sipn.x + sip.y * sipn.y + sip.z * sipn.z + sip.w * sipn.w;
    h[i] = f < 0 ? 1 : 0;
    k[i].pw = f;
  }
  kt[i] = s[i].t;
  k[i].p = no_conjugate(s[i].p);
  k[i].b0 = k[i].p;
  k[i].b1 = k[i].p;
  k[i].pw = 0;

  memset(x, 0, num);

  for (;;)
  {
    for (i = 0; i < num - 1; ++i)
      if (h[i])
        break;
    if (i >= num - 1)
      break;
    for (i = 1; i < num - 1; ++i)
      if (h[i - 1] && h[i])
      {
        h[i - 1] = h[i] = 0;
        x[i] ^= 1;
      }
    if (h[0])
    {
      h[0] = 0;
      x[0] ^= 1;
    }
    for (i = 1; i < num - 1; ++i)
      if (h[i])
      {
        h[i - 1] ^= 1;
        h[i] = 0;
        x[i] ^= 1;
      }
  }

  for (i = 0; i < num; ++i)
    if (x[i])
    {
      k[i].p = -k[i].p;
      k[i].pw = -k[i].pw;
      if (i > 0)
        k[i - 1].pw = -k[i - 1].pw;
    }
  for (i = 0; i < num; ++i)
    if (k[i].pw >= 1)
      k[i].sinpw = k[i].pw = 0;
    else
      k[i].sinpw = sin(k[i].pw = acos(k[i].pw));

  free(h);
}

// if input data have difference less than this number, they will be equal
constexpr float MIN_KEY_EPS = 0.0001f;

static inline int optimize_pos_key(Tab<PosKey> &pos, bool reduce_keys, float pos_thr)
{
  bool onlyOneKey = true;
  for (int i = 1, n = pos.size(); i < n; i++)
  {
    if (!key_is_same(pos[0], pos[i], pos_thr))
    {
      onlyOneKey = false;
      break;
    }
  }
  if (onlyOneKey)
  {
    int removedKeys = pos.size() - 1;
    pos.resize(1);
    // debug("optimized  P/S track: %d->%d", removedKeys + 1, 1);
    return removedKeys;
  }

  Tab<PosKey> temp_pos(pos);
  int total_pos = 1, removed_pos1 = 0, removed_pos2 = 0;
  int i, j, ok;

  if (reduce_keys)
  {
    PosKey nk[2];
    Point3 p13, p23;
    int temp_t;

    do
    {
      ok = 0;

      for (i = pos.size() - 2; i >= 1; i--)
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
          int prev_seg = 0;

          for (j = pos[i - 1].t; j <= pos[i + 1].t; j += TICKS_PER_FRAME)
          {
            Point3 lp = interp_seg(temp_pos, prev_seg, j);
            if (!pos_equal(lp, interp_seg(nk[0], nk[1], j), pos_thr))
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
            erase_items(pos, i, 1); // pos.Delete(i, 1);
            ok = 1;
          }
        }
      }
    } while (ok);
  }

  if (pos.size() == 2)
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
      erase_items(pos, 1, 1);
  }
  // if (pos.size() != temp_pos.size())
  //   debug("optimized P/S track: %d->%d", temp_pos.size(), pos.size());
  return temp_pos.size() - pos.size();
}


static int optimize_rot_key(Tab<RotKey> &rot, bool reduce_keys, float rot_thr)
{
  bool onlyOneKey = true;
  for (int i = 1, n = rot.size(); i < n; i++)
  {
    if (!key_is_same(rot[0], rot[i], rot_thr))
    {
      onlyOneKey = false;
      break;
    }
  }
  if (onlyOneKey)
  {
    int removedKeys = rot.size() - 1;
    rot.resize(1);
    // debug("optimized  R track: %d->%d", removedKeys + 1, 1);
    return removedKeys;
  }
  Tab<RotKey> temp_rot(rot);
  int total_rot = 1, removed_rot1 = 0, removed_rot2 = 0;
  int i, j, ok;

  if (reduce_keys)
  {
    RotKey nk[2];
    Quat p13, p23;
    int temp_t;

    do
    {
      ok = 0;

      for (i = rot.size() - 2; i >= 1; i--)
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

        // check i-th point with new virtual segment
        Quat p = interp_seg(nk[0], nk[1], rot[i].t);
        total_rot++;
        if (rot_equal(p, rot[i].p, rot_thr))
        {
          removed_rot1++;
          bool really_remove = true;
          int prev_seg = 0;

          for (j = rot[i - 1].t; j <= rot[i + 1].t; j += TICKS_PER_FRAME)
          {
            Quat lp = interp_seg(temp_rot, prev_seg, j);
            p = interp_seg2(nk[0], nk[1], j);

            if (!rot_equal(lp, p, rot_thr))
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
            erase_items(rot, i, 1);
            ok = 1;
          }
        }
      }
    } while (ok);
  }

  if (rot.size() == 2)
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
      erase_items(rot, 1, 1);
  }
  // if (rot.size() != temp_rot.size())
  //   debug("optimized  R track: %d->%d", temp_rot.size(), rot.size());
  return temp_rot.size() - rot.size();
}


static int optimize_Point3Chan(ChannelData &ch, float eps)
{
  int opt_cnt = 0;
  for (int i = 0; i < ch.nodeNum; ++i)
  {
    if (ch.nodeAnim[i].keyNum <= 2)
      continue;

    Tab<PosKey> pos(tmpmem);
    convert_to_PosKey(pos, (OldAnimKeyPoint3 *)ch.nodeAnim[i].key, ch.nodeAnim[i].keyTime, ch.nodeAnim[i].keyNum);

#if DUMP_POST_OPT
    Tab<OldAnimKeyPoint3> prevk(tmpmem);
    Tab<int> prevt(tmpmem);
    OldAnimKeyPoint3 *new_k = (OldAnimKeyPoint3 *)ch.nodeAnim[i].key;
    prevk = make_span_const(new_k, ch.nodeAnim[i].keyNum);
    prevt = make_span_const(ch.nodeAnim[i].keyTime, ch.nodeAnim[i].keyNum);
#endif

    opt_cnt += optimize_pos_key(pos, true, eps);
    if (ch.nodeAnim[i].keyNum == pos.size())
      continue;

    ch.nodeAnim[i].keyNum = pos.size();
    convert_to_OldAnimKeyPoint3((OldAnimKeyPoint3 *)ch.nodeAnim[i].key, ch.nodeAnim[i].keyTime, pos.data(), pos.size());

#if DUMP_POST_OPT
    debug("point3 optimized keys %d->%d (%d..%d) (%d..%d)", prevt.size(), ch.nodeAnim[i].keyNum, prevt[0], prevt.back(),
      ch.nodeAnim[i].keyTime[0], ch.nodeAnim[i].keyTime[ch.nodeAnim[i].keyNum - 1]);
    for (int t = prevt[0], te = prevt.back(); t <= te; t += TICKS_PER_FRAME)
    {
      float fracn, fracp;
      int kpn = find_key(t, ch.nodeAnim[i].keyTime, ch.nodeAnim[i].keyNum, fracn);
      int kpp = find_key(t, prevt.data(), prevt.size(), fracp);

      Point3 pn = interp_key(new_k[kpn], new_k[kpn + 1], fracn);
      Point3 pp = interp_key(prevk[kpp], prevk[kpp + 1], fracp);

      float diff = length(pn - pp);
      debug("%c%5d: %cdiff=%.3f  " FMT_P3 " <- " FMT_P3 "", t == ch.nodeAnim[i].keyTime[kpn] ? '+' : ' ', t,
        fabs(diff) > eps ? '*' : ' ', diff, P3D(pn), P3D(pp));
    }
#endif
  }
  return opt_cnt;
}


static int optimize_QuatChan(ChannelData &ch, double eps, int rot_resample_freq)
{
  int opt_cnt = 0;
  for (int i = 0; i < ch.nodeNum; ++i)
  {
    if (ch.nodeAnim[i].keyNum <= 1)
      continue;

    Tab<RotKey> rot(tmpmem);
    convert_to_RotKey(rot, (OldAnimKeyQuat *)ch.nodeAnim[i].key, ch.nodeAnim[i].keyTime, ch.nodeAnim[i].keyNum, rot_resample_freq);
    opt_cnt -= rot.size() - ch.nodeAnim[i].keyNum;

#if DUMP_POST_OPT
    OldAnimKeyQuat *new_k = (OldAnimKeyQuat *)ch.nodeAnim[i].key;
    Tab<OldAnimKeyQuat> prevk(tmpmem);
    Tab<int> prevt(tmpmem);
    prevk = make_span_const(new_k, ch.nodeAnim[i].keyNum);
    prevt = make_span_const(ch.nodeAnim[i].keyTime, ch.nodeAnim[i].keyNum);
#endif

    opt_cnt += optimize_rot_key(rot, true, eps);
    if (!rot_resample_freq && ch.nodeAnim[i].keyNum == rot.size())
      continue;

    if (ch.nodeAnim[i].keyNum < rot.size())
    {
      ch.nodeAnim[i].key = memalloc(rot.size() * sizeof(OldAnimKeyQuat), tmpmem);
      ch.nodeAnim[i].keyTime = (int *)memalloc(rot.size() * sizeof(int), tmpmem);
    }
    ch.nodeAnim[i].keyNum = rot.size();
    convert_to_OldAnimKeyQuat((OldAnimKeyQuat *)ch.nodeAnim[i].key, ch.nodeAnim[i].keyTime, rot.data(), rot.size());

#if DUMP_POST_OPT
    new_k = (OldAnimKeyQuat *)ch.nodeAnim[i].key;
    debug("quat   optimized keys %d->%d (%d..%d) (%d..%d) (eps=%.9f)", prevt.size(), ch.nodeAnim[i].keyNum, prevt[0], prevt.back(),
      ch.nodeAnim[i].keyTime[0], ch.nodeAnim[i].keyTime[ch.nodeAnim[i].keyNum - 1], eps);
    for (int t = prevt[0], te = prevt.back(); t <= te; t += TICKS_PER_FRAME)
    {
      float fracn, fracp;
      int kpn = find_key(t, ch.nodeAnim[i].keyTime, ch.nodeAnim[i].keyNum, fracn);
      int kpp = find_key(t, prevt.data(), prevt.size(), fracp);
      Quat d = interp_key(prevk[kpp], prevk[kpp + 1], fracp);
      Quat s = interp_key2(new_k[kpn], new_k[kpn + 1], fracn);
      double diff = fabs(s.x * d.x + s.y * d.y + s.z * d.z + s.w * d.w);
      debug("%c%5d: %cdiff=%.8f  " FMT_P4 " <- " FMT_P4 "", t == ch.nodeAnim[i].keyTime[kpn] ? '+' : ' ', t, diff < eps ? '*' : ' ',
        diff > 1 ? 0 : fabs(1.0 - diff), P4D(s), P4D(d));
      if (diff < eps)
      {
        inplace_max(quat_max_div, 1 - diff);
        quat_sum_div += 1 - diff;
        quat_num_div++;
      }
    }
#endif
  }
  return opt_cnt;
}


void optimize_keys(dag::Span<ChannelData> chan, float pos_eps, float rot_eps, float scale_eps, int rot_resample_freq)
{
  if (pos_eps == 0)
    pos_eps = 1e-4;
  if (rot_eps == 0)
    rot_eps = 1e-5;
  if (scale_eps == 0)
    scale_eps = 1e-6;

#if DUMP_POST_OPT
  quat_max_div = quat_sum_div = 0;
  quat_num_div = 0;
#endif

  int pos_opt_cnt = 0, rot_opt_cnt = 0, scl_opt_cnt = 0;
  double rot_eps_d = (rot_eps > 0) ? cos(rot_eps * DEG_TO_RAD) : -3;

  for (int channelId = 0; channelId < chan.size(); channelId++)
  {
    ChannelData &ch = chan[channelId];

    switch (ch.dataType)
    {
      case AnimV20::DATATYPE_POINT3X:
        if (ch.channelType == AnimV20::CHTYPE_POSITION && pos_eps > 0)
          pos_opt_cnt += optimize_Point3Chan(ch, pos_eps);
        else if (ch.channelType == AnimV20::CHTYPE_SCALE && scale_eps > 0)
          scl_opt_cnt += optimize_Point3Chan(ch, scale_eps);
        break;

      case AnimV20::DATATYPE_QUAT:
        if (rot_eps > 0)
          rot_opt_cnt += optimize_QuatChan(ch, rot_eps_d, rot_resample_freq);
        break;
    }
  }
  if (pos_opt_cnt + rot_opt_cnt + scl_opt_cnt > 0)
    debug("  removed %d poskeys (%dK), %d rotkeys (%dK), %d sclkeys (%dK); %dK saved total", pos_opt_cnt, (pos_opt_cnt * 68) >> 10,
      rot_opt_cnt, (rot_opt_cnt * 52) >> 10, scl_opt_cnt, (scl_opt_cnt * 68) >> 10,
      (pos_opt_cnt * 68 + rot_opt_cnt * 52 + scl_opt_cnt * 68) >> 10);

#if DUMP_POST_OPT
  if (quat_num_div)
    debug("max_div=%.8f mean_div=%.8f num=%d", quat_max_div, quat_sum_div / quat_num_div, quat_num_div);
#endif
}

static inline vec4f interp_point3(int ui, ChannelData::Anim &a, int t)
{
  OldAnimKeyPoint3 *key = reinterpret_cast<OldAnimKeyPoint3 *>(a.key);
  return AnimV20Math::interp_key(key[ui - 1], v_splats(float(t - a.keyTime[ui - 1]) / (a.keyTime[ui] - a.keyTime[ui - 1])));
}

vec4f interp_point3(ChannelData::Anim &a, int t, int &hint)
{
  int keyNum = a.keyNum;

  if (DAGOR_LIKELY(hint + 1 < keyNum && a.keyTime[hint] <= t && t <= a.keyTime[hint + 1]))
    return interp_point3(hint + 1, a, t);
  hint++;
  if (DAGOR_LIKELY(hint + 1 < keyNum && a.keyTime[hint] <= t && t <= a.keyTime[hint + 1]))
    return interp_point3(hint + 1, a, t);

  OldAnimKeyPoint3 *key = reinterpret_cast<OldAnimKeyPoint3 *>(a.key);
  auto ui = eastl::upper_bound(a.keyTime, a.keyTime + keyNum, t) - a.keyTime;
  if (DAGOR_UNLIKELY(ui == 0))
    return v_ld(&key[0].p.x);
  hint = ui - 1;
  if (DAGOR_UNLIKELY(ui == keyNum))
    return v_ld(&key[keyNum - 1].p.x);

  return interp_point3(ui, a, t);
}

static inline vec4f interp_quat(int ui, ChannelData::Anim &a, int t)
{
  OldAnimKeyQuat *key = reinterpret_cast<OldAnimKeyQuat *>(a.key);
  return AnimV20Math::interp_key(key[ui - 1], key[ui], float(t - a.keyTime[ui - 1]) / (a.keyTime[ui] - a.keyTime[ui - 1]));
}

vec4f interp_quat(ChannelData::Anim &a, int t, int &hint)
{
  int keyNum = a.keyNum;

  if (DAGOR_LIKELY(hint + 1 < keyNum && a.keyTime[hint] <= t && t <= a.keyTime[hint + 1]))
    return interp_quat(hint + 1, a, t);
  hint++;
  if (DAGOR_LIKELY(hint + 1 < keyNum && a.keyTime[hint] <= t && t <= a.keyTime[hint + 1]))
    return interp_quat(hint + 1, a, t);

  OldAnimKeyQuat *key = reinterpret_cast<OldAnimKeyQuat *>(a.key);
  auto ui = eastl::upper_bound(a.keyTime, a.keyTime + keyNum, t) - a.keyTime;
  if (DAGOR_UNLIKELY(ui == 0))
    return v_ld(&key[0].p.x);
  if (DAGOR_UNLIKELY(ui == keyNum))
    return v_ld(&key[keyNum - 1].p.x);

  return interp_quat(ui, a, t);
}
