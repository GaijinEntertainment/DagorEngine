#include <max.h>
#include "expanim.h"
#include "dagor.h"
#include "resource.h"
// #include "debug.h"

static Interval lim;
static float ort_thr;
static char _usekeys;

static Tab<TimeValue> ktime;

static inline Quat qadd(Quat a, Quat b) { return Quat(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }

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

static inline void add_ktime(TimeValue t)
{
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

static bool add_anim_range(Animatable *anim, Interval &rng)
{
  if (!anim)
    return false;
  // DebugPrint("%X sub-anims: %d\n",anim,anim->NumSubs());
  bool ok = false;
  for (int i = 0; i < anim->NumSubs(); ++i)
    if (add_anim_range(anim->SubAnim(i), rng))
      ok = true;
  Interval r;
  int num = anim->NumKeys();
  // DebugPrint("%X keys: %d\n",anim,num);
  if (num == NOT_KEYFRAMEABLE)
  {
    r = anim->GetTimeRange(TIMERANGE_ALL);
  }
  else if (num == 0)
  {
    r.SetEmpty();
  }
  else
  {
    ok = true;
    r.Set(anim->GetKeyTime(0), anim->GetKeyTime(num - 1));
    if (_usekeys)
      for (int i = 1; i < num - 1; ++i)
      {
        TimeValue t = anim->GetKeyTime(i);
        if (t >= lim.Start() && t <= lim.End())
          add_ktime(t);
      }
  }
  r &= lim;
  if (r.Empty())
    return ok;
  if (_usekeys)
  {
    add_ktime(r.Start());
    add_ktime(r.End());
  }
  if (rng.Empty())
    rng.Set(r.Start(), r.End());
  else
  {
    rng += r.Start();
    rng += r.End();
  }
  return ok;
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

struct TMCache
{
  TimeValue t;
  Point3 p;
  Quat r;
  Point3 s;
};

static Tab<TMCache> tmcache;

static void clear_tmcache() { tmcache.ZeroCount(); }

static void interp_tm(TimeValue t, Point3 &p, Quat &q, Point3 &s, ExpTMAnimCB &cb)
{
  for (int i = 0; i < tmcache.Count(); ++i)
    if (tmcache[i].t == t)
    {
      p = tmcache[i].p;
      q = tmcache[i].r;
      s = tmcache[i].s;
      return;
    }
  Matrix3 m;
  cb.interp_tm(t, m);
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
  TMCache ca;
  ca.t = t;
  ca.p = p;
  ca.r = q;
  ca.s = s;
  tmcache.Append(1, &ca, 100);
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

/*
static inline void make_seg_smooth(RotKey a,RotKey &b,RotKey &c,RotKey d) {
        b.o=b.p;
        c.i=c.p;
}
*/

static DWORD WINAPI dummyfn(LPVOID a) { return 0; }

bool get_tm_anim(Tab<PosKey> &pos, Tab<RotKey> &rot, Tab<PosKey> &scl, Tab<TimeValue> &gkeys, Interval limit, Animatable *ctrl,
  ExpTMAnimCB &cb, TimeValue mindt, float pos_thr, float rot_thr, float scl_thr, float ort_eps, char usekeys, char usegkeys,
  char dontchkkeys)
{
  _usekeys = usekeys;
  ort_thr = ort_eps;
  lim = limit;
  ktime.ZeroCount();
  clear_tmcache();

  // DebugPrint("%d..%d\n",lim.Start(),lim.End());
  { // find anim range
    Interval rng;
    rng.SetEmpty();
    if (!add_anim_range(ctrl, rng))
      dontchkkeys = 0;
    /*
                    if(rng.Empty()) {
                            if(lim.Start()!=TIME_NegInfinity) rng.SetInstant(lim.Start());
                            else rng.SetInstant(0);
                    }
    */
    if (!rng.Empty())
      lim &= rng;
  }
  // DebugPrint("%d..%d\n",lim.Start(),lim.End());
  if (usegkeys)
  { // add global keys
    char kb = 0, ka = 0;
    for (int i = 0; i < gkeys.Count(); ++i)
    {
      // DebugPrint("gkey[%d]=%d\n",i,gkeys[i]);
      if (gkeys[i] < lim.Start())
        kb = 1;
      else if (gkeys[i] > lim.End())
        ka = 1;
      else
        add_ktime(gkeys[i]);
    }
    if (kb)
      add_ktime(lim.Start());
    if (ka)
      add_ktime(lim.End());
  }
  if (lim.Start() == TIME_NegInfinity)
    lim.SetInstant(0);
  // DebugPrint("%d..%d\n",lim.Start(),lim.End());
  if (lim.Start() == lim.End())
    add_ktime(lim.Start());
  else if (ktime.Count() < 2)
  {
    add_ktime(lim.Start());
    add_ktime(lim.End());
  }
  sort_ktime();
  // debug("lim: %d %d",lim.Start()/GetTicksPerFrame(),lim.End()/GetTicksPerFrame());

  pos.SetCount(ktime.Count());
  rot.SetCount(ktime.Count());
  scl.SetCount(ktime.Count());

  pos[0].t = rot[0].t = scl[0].t = ktime[0];
  pos[0].f = rot[0].f = scl[0].f = 0;
  interp_tm(ktime[0], pos[0].p, rot[0].p, scl[0].p, cb);

  if (ktime.Count() == 1)
    return true;
  assert(ktime.Count() > 1);

  Interface *ip = GetCOREInterface();
  ip->ProgressStart((TCHAR *)cb.get_name(), FALSE, dummyfn, NULL);
  ip->ProgressUpdate(0, FALSE, _T("initial keys"));

  // set up keys and segments
  int i;
  for (i = 1; i < ktime.Count(); ++i)
  {
    pos[i].t = rot[i].t = scl[i].t = ktime[i];
    pos[i].f = rot[i].f = scl[i].f = 0;
    interp_tm(ktime[i], pos[i].p, rot[i].p, scl[i].p, cb);
    Point3 p0, p1, s0, s1;
    Quat r0, r1;
    TimeValue tt = (ktime[i] - ktime[i - 1]) / 3;
    if (tt <= 0)
      tt = 1;
    interp_tm(ktime[i - 1] + tt, p0, r0, s0, cb);
    interp_tm(ktime[i] - tt, p1, r1, s1, cb);
    calc_cp(pos[i - 1], pos[i], p0, p1);
    calc_cp(rot[i - 1], rot[i], r0, r1);
    calc_cp(scl[i - 1], scl[i], s0, s1);
  }
  pos[0].i = pos[0].p * 2 - pos[0].o;
  rot[0].i = SLERP(rot[0].p, rot[0].o, -1);
  scl[0].i = scl[0].p * 2 - scl[0].o;
  i = ktime.Count() - 1;
  pos[i].o = pos[i].p * 2 - pos[i].i;
  rot[i].o = SLERP(rot[i].p, rot[i].i, -1);
  scl[i].o = scl[i].p * 2 - scl[i].i;

  ip->ProgressUpdate(0, FALSE, _T("about"));

  // check segments
  if (!dontchkkeys)
    for (i = 1; i < ktime.Count(); ++i)
    {
      TSTR str;
      str.printf(_T("%d/%d"), i, ktime.Count() - 1);
      ip->ProgressUpdate(i * 100 / ktime.Count(), FALSE, str);
      if (ip->GetCancel())
        break;
      int poseq = 1, roteq = 1, scleq = 1;
      TimeValue tstep = mindt / 2;
      if (tstep <= 0)
        tstep = 1;
      int cnt = 0, cnti = 0;
      if (ktime.Count() == 2)
        cnti = 1;
      for (TimeValue t = ktime[i - 1] + tstep; t < ktime[i]; t += tstep, cnt += cnti)
      {
        if (cnt >= 30)
        {
          cnt = 0;
          ip->ProgressUpdate((t - ktime[i - 1]) * 100 / (ktime[i] - ktime[i - 1]), FALSE, str);
          if (ip->GetCancel())
            break;
        }
        Point3 p, s;
        Quat r;
        interp_tm(t, p, r, s, cb);
        if (!pos_equal(p, interp_seg(pos[i - 1], pos[i], t), pos_thr))
          poseq = 0;
        if (!rot_equal(r, interp_seg(rot[i - 1], rot[i], t), rot_thr))
          roteq = 0;
        if (!scl_equal(s, interp_seg(scl[i - 1], scl[i], t), scl_thr))
          scleq = 0;
        if (!poseq && !roteq && !scleq)
          break;
      }
      if (ip->GetCancel())
        break;
      if (!poseq || !roteq || !scleq)
      {
        TimeValue dt = (ktime[i] - ktime[i - 1]) / 2;
        if (dt < mindt)
        {
          if (!poseq)
            make_seg_smooth(pos[i > 1 ? i - 2 : 0], pos[i - 1], pos[i], pos[i + 1 < ktime.Count() ? i + 1 : i]);
          if (!roteq)
            make_seg_smooth(rot[i > 1 ? i - 2 : 0], rot[i - 1], rot[i], rot[i + 1 < ktime.Count() ? i + 1 : i]);
          if (!scleq)
            make_seg_smooth(scl[i > 1 ? i - 2 : 0], scl[i - 1], scl[i], scl[i + 1 < ktime.Count() ? i + 1 : i]);
          continue;
        }
        dt -= dt % mindt;
        TimeValue t = ktime[i - 1] + dt;
        Point3 p0, p1, s0, s1;
        Quat r0, r1;
        interp_tm(t, p0, r0, s0, cb);
        ktime.Insert(i, 1, &t);
        PosKey pk;
        pk.t = t;
        pk.f = poseq;
        pk.p = p0;
        pos.Insert(i, 1, &pk);
        RotKey rk;
        rk.t = t;
        rk.f = roteq;
        rk.p = r0;
        rot.Insert(i, 1, &rk);
        PosKey sk;
        sk.t = t;
        sk.f = scleq;
        sk.p = s0;
        scl.Insert(i, 1, &sk);
        TimeValue tt = floorf(dt / 3.0f);
        if (tt <= 0)
          tt = 1;
        interp_tm(ktime[i - 1] + tt, p0, r0, s0, cb);
        interp_tm(t - tt, p1, r1, s1, cb);
        calc_cp(pos[i - 1], pos[i], p0, p1);
        calc_cp(rot[i - 1], rot[i], r0, r1);
        calc_cp(scl[i - 1], scl[i], s0, s1);
        tt = floorf((ktime[i + 1] - t) / 3.0f);
        if (tt <= 0)
          tt = 1;
        interp_tm(t + tt, p0, r0, s0, cb);
        interp_tm(ktime[i + 1] - tt, p1, r1, s1, cb);
        calc_cp(pos[i], pos[i + 1], p0, p1);
        calc_cp(rot[i], rot[i + 1], r0, r1);
        calc_cp(scl[i], scl[i + 1], s0, s1);
        --i;
      }
    }

  if (ip->GetCancel())
  {
    ip->ProgressEnd();
    return false;
  }

  ip->ProgressEnd();

  /*
          for(i=1;i<ktime.Count();++i) {
                  make_seg_smooth(pos[i>1?i-2:0],pos[i-1],pos[i],pos[i+1<ktime.Count()?i+1:i]);
                  make_seg_smooth(rot[i>1?i-2:0],rot[i-1],rot[i],rot[i+1<ktime.Count()?i+1:i]);
                  make_seg_smooth(scl[i>1?i-2:0],scl[i-1],scl[i],scl[i+1<ktime.Count()?i+1:i]);
          }
  */

  // reduce keys
  char ok;
  do
  {
    ok = 0;
    for (i = 1; i < pos.Count() - 1;)
      if (pos[i].f)
      {
        pos.Delete(i, 1);
        ok = 1;
      }
      /*                      else{
                                      Point3 p=interp_seg(pos[i-1],pos[i+1],pos[i].t);
                                      if(pos_equal(p,pos[i].p,pos_thr)) {pos.Delete(i,1);ok=1;}
      */
      else
        ++i;
    //                      }
  } while (ok);
  if (pos.Count() == 2)
  {
    TimeValue dt = pos[1].t - pos[0].t;
    for (i = 1; i <= 5; ++i)
    {
      Point3 p = interp_seg(pos[0], pos[1], dt * i / 5 + pos[0].t);
      if (!pos_equal(p, pos[0].p, pos_thr))
        break;
    }
    if (i > 5)
      pos.Delete(1, 1);
  }

  do
  {
    ok = 0;
    for (i = 1; i < rot.Count() - 1;)
      if (rot[i].f)
      {
        rot.Delete(i, 1);
        ok = 1;
      }
      /*                      else{
                                      Quat p=interp_seg(rot[i-1],rot[i+1],rot[i].t);
                                      if(rot_equal(p,rot[i].p,rot_thr)) {rot.Delete(i,1);ok=1;}
      */
      else
        ++i;
    //                      }
  } while (ok);
  if (rot.Count() == 2)
  {
    TimeValue dt = rot[1].t - rot[0].t;
    for (i = 1; i <= 5; ++i)
    {
      Quat p = interp_seg(rot[0], rot[1], dt * i / 5 + rot[0].t);
      if (!rot_equal(p, rot[0].p, rot_thr))
        break;
    }
    if (i > 5)
      rot.Delete(1, 1);
  }

  do
  {
    ok = 0;
    for (i = 1; i < scl.Count() - 1;)
      if (scl[i].f)
      {
        scl.Delete(i, 1);
        ok = 1;
      }
      /*                      else{
                                      Point3 p=interp_seg(scl[i-1],scl[i+1],scl[i].t);
                                      if(scl_equal(p,scl[i].p,scl_thr)) {scl.Delete(i,1);ok=1;}
      */
      else
        ++i;
    //                      }
  } while (ok);
  if (scl.Count() == 2)
  {
    TimeValue dt = scl[1].t - scl[0].t;
    for (i = 1; i <= 5; ++i)
    {
      Point3 p = interp_seg(scl[0], scl[1], dt * i / 5 + scl[0].t);
      if (!scl_equal(p, scl[0].p, scl_thr))
        break;
    }
    if (i > 5)
      scl.Delete(1, 1);
  }

  clear_tmcache();

  return true;
}
