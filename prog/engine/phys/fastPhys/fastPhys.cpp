// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <phys/dag_fastPhys.h>
#include <math/dag_geomTree.h>
#include <ioSys/dag_genIo.h>
#include <math/dag_math3d.h>
#include <math/dag_noise.h>
#include <util/dag_string.h>
#include <math/dag_geomNodeUtils.h>
#include <perfMon/dag_statDrv.h>
#include <debug/dag_debug3d.h>

// #include <debug/dag_debug.h>

#if MEASURE_PERF
#include <perfMon/dag_perfMonStat.h>
#include <perfMon/dag_perfTimer2.h>
#include <startup/dag_globalSettings.h>

static PerformanceTimer2 perf_tm;
static int perf_cnt = 0;
static int perf_frameno_start = 0;
#endif


FastPhysSystem::FastPhysSystem() :
  deltaTime(0),
  windScale(5),
  windTurb(0),
  windPower(0),
  windVel(0, 0, 0),
  windPos(0, 0, 0),
  firstIntegratePoint(0),
  gravityDirection(0, -1, 0)
{
  curWtmOfs = deltaWtmOfs = v_zero();
}


FastPhysSystem::~FastPhysSystem()
{
  for (int i = 0; i < initActions.size(); ++i)
    if (initActions[i])
      delete initActions[i];

  for (int i = 0; i < updateActions.size(); ++i)
    if (updateActions[i])
      delete updateActions[i];
}


void FastPhysSystem::load(IGenLoad &cb)
{
  int id = cb.readInt();
  if (id != FastPhys::FILE_ID)
    DAGOR_THROW(IGenLoad::LoadException("bad file ID", cb.tell()));

  // load points
  clear_and_resize(points, cb.readInt());
  mem_set_0(points);

  for (int i = 0; i < points.size(); ++i)
  {
    cb.readReal(points[i].gravity);
    cb.readReal(points[i].damping);
    cb.readReal(points[i].windK);
  }

  // load actions
  clear_and_resize(initActions, cb.readInt());
  mem_set_0(initActions);
  for (int i = 0; i < initActions.size(); ++i)
    initActions[i] = loadAction(cb);

  clear_and_resize(updateActions, cb.readInt());
  mem_set_0(updateActions);
  for (int i = 0; i < updateActions.size(); ++i)
    updateActions[i] = loadAction(cb);
}


void FastPhysSystem::init(GeomNodeTree &tree)
{
  curWtmOfs = tree.getWtmOfs();
  deltaWtmOfs = v_zero();
  for (int i = 0; i < initActions.size(); ++i)
    initActions[i]->init(*this, tree);

  for (int i = 0; i < initActions.size(); ++i)
    initActions[i]->perform(*this);

  for (int i = 0; i < updateActions.size(); ++i)
    updateActions[i]->init(*this, tree);
}


void FastPhysSystem::update(real dt, vec3f skeleton_wofs)
{
  TIME_PROFILE(fast_phys_update);
#if MEASURE_PERF
  perf_tm.go();
#endif
  deltaTime = dt;

  deltaWtmOfs = v_sub(curWtmOfs, skeleton_wofs);
  curWtmOfs = skeleton_wofs;

  // when curWtmOfs changes we have to rebase all points to avoid discontinuity (we compare .xyz only)
  if (memcmp(&deltaWtmOfs, ZERO_PTR<Point3>(), 3 * sizeof(float)) != 0) //-V512
    for (FastPhysSystem::Point &pt : points)
      pt.pos += as_point3(&deltaWtmOfs);

  Point3 windVelocity = useOwnerTm ? inverse(ownerTm) % windVel : windVel;
  windPos += windVelocity * dt;
  const real MAX_WIND = 1000;
  if (windPos.x > MAX_WIND || windPos.x < -MAX_WIND)
    windPos.x = 0;
  if (windPos.y > MAX_WIND || windPos.y < -MAX_WIND)
    windPos.y = 0;
  if (windPos.z > MAX_WIND || windPos.z < -MAX_WIND)
    windPos.z = 0;

  for (int i = 0; i < updateActions.size(); ++i)
    updateActions[i]->perform(*this);
  for (FastPhysSystem::Point &pt : points)
    pt.acc.zero();
  deltaTime = 0;
#if MEASURE_PERF
  perf_tm.pause();
  perf_cnt++;
#if MEASURE_PERF_FRAMES
  if (::dagor_frame_no() > perf_frameno_start + MEASURE_PERF_FRAMES)
#else
  if (perf_cnt >= MEASURE_PERF)
#endif
  {
    perfmonstat::fastphys_update_cnt = perf_cnt;
    perfmonstat::fastphys_update_time_usec = perf_tm.getTotalUsec();
    perfmonstat::generation_counter++;

    perf_tm.reset();
    perf_cnt = 0;
    perf_frameno_start = ::dagor_frame_no();
  }
#endif
}


void FastPhysSystem::reset(vec3f skeleton_wofs)
{
  curWtmOfs = skeleton_wofs;
  deltaWtmOfs = v_zero();
  for (int i = 0; i < initActions.size(); ++i)
    initActions[i]->reset(*this);

  for (int i = 0; i < initActions.size(); ++i)
    initActions[i]->perform(*this);

  for (int i = 0; i < updateActions.size(); ++i)
    updateActions[i]->reset(*this);
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class FastPhysIntegrateAction : public FastPhysAction
{
public:
  int firstPoint, endPoint;


  FastPhysIntegrateAction(IGenLoad &cb)
  {
    cb.readInt(firstPoint);
    cb.readInt(endPoint);
  }

  virtual void init(FastPhysSystem &sys, GeomNodeTree &) { sys.firstIntegratePoint = firstPoint; }

  virtual void perform(FastPhysSystem &sys)
  {
    real dt = sys.deltaTime;

    if (!float_nonzero(dt))
    {
      for (int i = firstPoint; i < endPoint; ++i)
      {
        FastPhysSystem::Point &pt = sys.points[i];
        pt.acc = Point3(0, 0, 0);
      }

      return;
    }

    Point3 gravityDirection = sys.gravityDirection;
    Point3 windVelocity = sys.windVel;
    if (sys.useOwnerTm)
    {
      TMatrix itm = inverse(sys.ownerTm);
      gravityDirection = itm % sys.gravityDirection;
      windVelocity = itm % sys.windVel;
    }

    for (int i = firstPoint; i < endPoint; ++i)
    {
      FastPhysSystem::Point &pt = sys.points[i];

      real vk = 1 - dt * pt.damping;
      if (vk < 0)
        vk = 0;

      Point3 acc = pt.acc;
      acc += gravityDirection * pt.gravity;
      float wind = sys.windPower * pt.windK;
      if (float_nonzero(wind))
      {
        Point3 p = (sys.windPos + pt.pos) * sys.windScale;
        acc += (windVelocity + Point3(perlin_noise::noise3(p), perlin_noise::noise3(p + Point3(0, 1, 0)),
                                 perlin_noise::noise3(p + Point3(0, 0, 1))) *
                                 sys.windTurb) *
               wind;
      }

      pt.pos += (pt.vel + acc * (dt * 0.5f)) * dt;
      pt.vel = pt.vel * vk + acc * dt;

      pt.acc = Point3(0, 0, 0);
    }
  }

  virtual void reset(FastPhysSystem &sys)
  {
    for (int i = firstPoint; i < endPoint; ++i)
    {
      FastPhysSystem::Point &pt = sys.points[i];

      pt.vel = Point3(0, 0, 0);
      pt.acc = Point3(0, 0, 0);
    }
  }
};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class FastPhysInitPointAction : public FastPhysAction
{
public:
  int pointIndex;
  Point3_vec4 localPos;
  mat44f *nodeWtm = nullptr;

  String nodeName;


  FastPhysInitPointAction(IGenLoad &cb)
  {
    cb.readInt(pointIndex);
    memset(&localPos, 0, sizeof(localPos));
    cb.read(&localPos, sizeof(Point3));
    cb.readString(nodeName);
  }

  virtual void init(FastPhysSystem &, GeomNodeTree &nodeTree) { nodeWtm = resolve_node_wtm(nodeTree, nodeName); }

  virtual void perform(FastPhysSystem &sys)
  {
    FastPhysSystem::Point &pt = sys.points[pointIndex];

    if (nodeWtm)
    {
      v_st(&pt.pos, v_mat44_mul_vec3p(*nodeWtm, v_ld(&localPos.x))); //== will overwrite pt.vel.x as side effect
      pt.vel = Point3(0, 0, 0);
    }
    else
    {
      pt.pos = localPos;
      pt.vel = Point3(0, 0, 0);
    }
  }

  virtual void reset(FastPhysSystem &) {}
};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class FastPhysMovePointWithNodeAction : public FastPhysAction
{
public:
  static constexpr const float MAX_POINT_VEL_SQ = 74529.f; // sqr(273)
  int pointIndex;
  Point3_vec4 localPos;
  Point3_vec4 lastPos;
  mat44f *nodeWtm = nullptr;

  String nodeName;


  FastPhysMovePointWithNodeAction(IGenLoad &cb)
  {
    cb.readInt(pointIndex);
    memset(&localPos, 0, sizeof(localPos));
    cb.read(&localPos, sizeof(Point3));
    cb.readString(nodeName);
  }

  virtual void init(FastPhysSystem &sys, GeomNodeTree &nodeTree)
  {
    nodeWtm = resolve_node_wtm(nodeTree, nodeName);
    reset(sys);
  }

  virtual void perform(FastPhysSystem &sys)
  {
    FastPhysSystem::Point &pt = sys.points[pointIndex];
    lastPos += as_point3(&sys.deltaWtmOfs);

    Point3 posDelta(0.f, 0.f, 0.f);
    if (nodeWtm)
    {
      v_st(&pt.pos, v_mat44_mul_vec3p(*nodeWtm, v_ld(&localPos.x))); //== will overwrite pt.vel.x as side effect

      if (sys.useOwnerTm)
      {
        Point3_vec4 pointWorldPos;
        getPointWorldPos(sys.ownerTm, pointWorldPos);

        posDelta = -inverse(sys.ownerTm) % (pointWorldPos - lastPos);
        lastPos = pointWorldPos;
      }
      else
      {
        posDelta = pt.pos - lastPos;
        lastPos = pt.pos;
      }
    }
    else
      pt.pos = localPos;

    if (float_nonzero(sys.deltaTime))
    {
      pt.vel = posDelta / sys.deltaTime;
      if (pt.vel.lengthSq() >= MAX_POINT_VEL_SQ)
        pt.vel = ZERO<Point3>(); // assume teleport/non phys movement
    }
    // else
    //   pt.vel.zero();
  }

  virtual void reset(FastPhysSystem &sys)
  {
    FastPhysSystem::Point &pt = sys.points[pointIndex];

    if (nodeWtm)
      v_st(&pt.pos, v_mat44_mul_vec3p(*nodeWtm, v_ld(&localPos.x))); //== will overwrite pt.vel.x as side effect
    else
      pt.pos = localPos;

    pt.vel = Point3(0, 0, 0);
    if (sys.useOwnerTm)
      getPointWorldPos(sys.ownerTm, lastPos);
    else
      lastPos = pt.pos;
  }

  void getNodeWtm(const TMatrix &owner_tm, mat44f &wtm)
  {
    mat44f ownerWtm;
    v_mat44_make_from_43cu_unsafe(ownerWtm, owner_tm.array);
    v_mat44_mul43(wtm, ownerWtm, *nodeWtm);
  }

  void getPointWorldPos(const TMatrix &owner_tm, Point3_vec4 &pos)
  {
    mat44f wtm;
    getNodeWtm(owner_tm, wtm);

    v_st(&pos, v_mat44_mul_vec3p(wtm, v_ld(&localPos.x)));
  }
};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class FastPhysSetBoneLengthAction : public FastPhysAction
{
public:
  int p1Index, p2Index;
  real boneLength;
  real damping;


  FastPhysSetBoneLengthAction(IGenLoad &cb) : boneLength(0.05f)
  {
    cb.readInt(p1Index);
    cb.readInt(p2Index);

    cb.readReal(damping);
  }

  virtual void init(FastPhysSystem &sys, GeomNodeTree &) { boneLength = length(sys.points[p2Index].pos - sys.points[p1Index].pos); }

  virtual void perform(FastPhysSystem &sys)
  {
    FastPhysSystem::Point &pt1 = sys.points[p1Index];
    FastPhysSystem::Point &pt2 = sys.points[p2Index];

    Point3 dir = pt2.pos - pt1.pos;
    real lenSq = lengthSq(dir);

    if (!float_nonzero(lenSq))
      return;

    real len = sqrtf(lenSq);

    pt2.pos = pt1.pos + dir * (boneLength / len);

    // eliminate relative velocity
    Point3 rv = pt2.vel - pt1.vel;

    Point3 dv = dir * ((dir * rv) / lenSq);

    pt2.vel -= dv;

    // apply damping
    Point3 tv = rv - dv;

    real k = damping * sys.deltaTime;
    if (k > 1)
      k = 1;
    pt2.vel -= tv * k;
  }

  virtual void reset(FastPhysSystem &) {}
};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class FastPhysSetBoneLengthWithConeAction : public FastPhysAction
{
public:
  int p1Index, p2Index;
  real boneLength = 0.05f;
  real damping;

  Point3 limitAxis;
  real limitCos, limitSin;

  mat44f *nodeWtm = nullptr;
  const mat44f *nodeParentWtm = nullptr;
  String nodeName;


  FastPhysSetBoneLengthWithConeAction(IGenLoad &cb)
  {
    cb.readInt(p1Index);
    cb.readInt(p2Index);

    cb.readReal(damping);

    cb.read(&limitAxis, sizeof(limitAxis));
    cb.readReal(limitCos);
    limitSin = sqrtf(1 - limitCos * limitCos);

    cb.readString(nodeName);
  }

  virtual void init(FastPhysSystem &sys, GeomNodeTree &nodeTree)
  {
    boneLength = length(sys.points[p2Index].pos - sys.points[p1Index].pos);
    nodeWtm = resolve_node_wtm(nodeTree, nodeName, &nodeParentWtm);
  }

  virtual void perform(FastPhysSystem &sys)
  {
    FastPhysSystem::Point &pt1 = sys.points[p1Index];
    FastPhysSystem::Point &pt2 = sys.points[p2Index];

    Point3 dir = pt2.pos - pt1.pos;
    real lenSq = lengthSq(dir);
    real len = sqrtf(lenSq);

    if (!float_nonzero(len))
      return;

    if (nodeWtm && nodeParentWtm)
    {
      Point3_vec4 axis;
      float proj;
      v_st(&axis, v_norm3(v_mat44_mul_vec3v(*nodeParentWtm, v_ldu(&limitAxis.x))));
      proj = (axis * dir) / len;

      if (proj < limitCos)
      {
        // apply cone limit
        Point3 sideDir = normalize(dir - axis * (len * proj));
        dir = axis * limitCos + sideDir * limitSin;
        len = 1;
        lenSq = 1;
      }
    }

    pt2.pos = pt1.pos + dir * (boneLength / len);

    // eliminate relative velocity
    Point3 rv = pt2.vel - pt1.vel;

    Point3 dv = dir * ((dir * rv) / lenSq);

    pt2.vel -= dv;

    // apply damping
    Point3 tv = rv - dv;

    real k = damping * sys.deltaTime;
    if (k > 1)
      k = 1;
    pt2.vel -= tv * k;
  }

  virtual void reset(FastPhysSystem &) {}
};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class FastPhysMinMaxLengthAction : public FastPhysAction
{
public:
  int p1Index, p2Index;
  real minLenRatio, maxLenRatio;

  real boneLength;
  real minLenSq, maxLenSq;


  FastPhysMinMaxLengthAction(IGenLoad &cb) : boneLength(0.05f)
  {
    cb.readInt(p1Index);
    cb.readInt(p2Index);

    cb.readReal(minLenRatio);
    cb.readReal(maxLenRatio);
  }

  virtual void init(FastPhysSystem &sys, GeomNodeTree &)
  {
    boneLength = length(sys.points[p2Index].pos - sys.points[p1Index].pos);

    real boneLenSq = boneLength * boneLength;
    minLenSq = boneLenSq * minLenRatio * minLenRatio;
    maxLenSq = boneLenSq * maxLenRatio * maxLenRatio;
  }

  virtual void perform(FastPhysSystem &sys)
  {
    FastPhysSystem::Point &pt1 = sys.points[p1Index];
    FastPhysSystem::Point &pt2 = sys.points[p2Index];

    Point3 dir = pt2.pos - pt1.pos;
    real lenSq = lengthSq(dir);

    if (minLenSq <= lenSq && lenSq <= maxLenSq)
      return;

    real len = sqrtf(lenSq);
    if (!float_nonzero(len))
      return;

    real rv = ((pt2.vel - pt1.vel) * dir) / len;

    if (lenSq < minLenSq)
    {
      Point3 dp = dir * (0.5f * (boneLength * minLenRatio - len) / len);
      pt2.pos += dp;
      pt1.pos -= dp;

      if (rv < 0)
      {
        Point3 dv = dir * (0.5f * rv / len);
        pt2.vel -= dv;
        pt1.vel += dv;
      }
    }
    else
    {
      Point3 dp = dir * (0.5f * (boneLength * maxLenRatio - len) / len);
      pt2.pos += dp;
      pt1.pos -= dp;

      if (rv > 0)
      {
        Point3 dv = dir * (0.5f * rv / len);
        pt2.vel -= dv;
        pt1.vel += dv;
      }
    }
  }

  virtual void reset(FastPhysSystem &) {}
};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class FastPhysSimpleBoneCtrlAction : public FastPhysAction
{
public:
  int p1Index, p2Index;
  mat44f *nodeWtm = nullptr;
  mat44f *nodeTm = nullptr;
  const mat44f *nodeParentWtm = nullptr;
  vec3f localBoneAxis;
  mat33f orgTm;

  String nodeName;


  FastPhysSimpleBoneCtrlAction(IGenLoad &cb)
  {
    localBoneAxis = orgTm.col0 = orgTm.col1 = orgTm.col2 = v_zero();

    cb.readInt(p1Index);
    cb.readInt(p2Index);
    cb.read(&localBoneAxis, sizeof(float) * 3);
    cb.read(&orgTm.col0, sizeof(float) * 3);
    cb.read(&orgTm.col1, sizeof(float) * 3);
    cb.read(&orgTm.col2, sizeof(float) * 3);
    cb.readString(nodeName);
  }

  virtual void init(FastPhysSystem &, GeomNodeTree &nodeTree)
  {
    nodeWtm = resolve_node_wtm(nodeTree, nodeName, &nodeParentWtm, &nodeTm);
    if (!nodeWtm)
      return;

    orgTm.col0 = nodeTm->col0;
    orgTm.col1 = nodeTm->col1;
    orgTm.col2 = nodeTm->col2;
  }

  virtual void perform(FastPhysSystem &sys)
  {
    if (!nodeWtm)
      return;

    FastPhysSystem::Point &pt1 = sys.points[p1Index];
    FastPhysSystem::Point &pt2 = sys.points[p2Index];

    vec3f pt1_pos = v_ld(&pt1.pos.x);
    vec3f dir = v_sub(v_ld(&pt2.pos.x), pt1_pos);

    mat33f tm, mq, m3;
    if (nodeParentWtm)
    {
      m3.col0 = nodeParentWtm->col0;
      m3.col1 = nodeParentWtm->col1;
      m3.col2 = nodeParentWtm->col2;
      v_mat33_mul(tm, m3, orgTm);
    }
    else
      tm = orgTm;

    vec4f ax = v_mat33_mul_vec3(tm, localBoneAxis);
    // test 'ax' as v_quat_from_arc(zero) will give NaN
    if (v_test_vec_x_gt(v_length3_sq_x(dir), V_C_EPS_VAL) && v_test_vec_x_gt(v_length3_sq_x(ax), V_C_EPS_VAL))
    {
      quat4f q = v_quat_from_arc(ax, dir);
      v_mat33_from_quat(mq, q);
      v_mat33_mul(m3, mq, tm);
      nodeWtm->set33(m3, pt1_pos);
    }
    else
      nodeWtm->col3 = pt1_pos;

    if (nodeParentWtm)
    {
      /*
      mat33f iwtm;
      tm.col0 = node->parent->wtm.col0;
      tm.col1 = node->parent->wtm.col1;
      tm.col2 = node->parent->wtm.col2;
      v_mat33_inverse(iwtm, tm);
      v_mat33_mul(tm, iwtm, m3);
      nodeTm->set33(tm);
      /*/
      mat44f iwtm;
      v_mat44_inverse43(iwtm, *nodeParentWtm);
      v_mat44_mul43(iwtm, iwtm, *nodeWtm);
      nodeTm->col0 = iwtm.col0;
      nodeTm->col1 = iwtm.col1;
      nodeTm->col2 = iwtm.col2;
      if (p1Index >= sys.firstIntegratePoint)
        nodeTm->col3 = iwtm.col3;
      //*/
    }
    else
      nodeTm->set33(m3);
  }

  virtual void reset(FastPhysSystem &) {}
};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

TMatrix makeTMFromUpAndSide(Point3_vec4 &local_dir, Point3_vec4 &local_side_dir, Point3_vec4 local_pos)
{
  TMatrix localTM;
  Point3_vec4 tmp = cross(local_side_dir, local_dir);
  localTM.setcol(0, tmp);
  localTM.setcol(1, local_dir);
  localTM.setcol(2, local_side_dir);
  localTM.setcol(3, local_pos);
  return localTM;
}

TMatrix makeTMFromUp(Point3_vec4 &local_dir, Point3_vec4 local_pos)
{
  Point3_vec4 axis = Point3_vec4(0, 1, 0);
  Point3_vec4 localSideDir = cross(local_dir, cross(axis, local_dir));
  return makeTMFromUpAndSide(local_dir, localSideDir, local_pos);
}

TMatrix makeNodeWTM(mat44f *node_wtm, const vec3f *node_wofs)
{
  vec4f ofs = v_add(node_wtm->col3, *node_wofs);
  TMatrix wtm;
  wtm.setcol(0, as_point3(&(node_wtm->col0)));
  wtm.setcol(1, as_point3(&(node_wtm->col1)));
  wtm.setcol(2, as_point3(&(node_wtm->col2)));
  wtm.setcol(3, as_point3(&ofs));
  return wtm;
}


class FastPhysClipSphericalAction : public FastPhysAction
{
public:
  mat44f *nodeWtm = nullptr;
  const vec3f *nodeWofs = nullptr;
  Point3_vec4 localPos, localDir;
  real angle; //  in radians, angle = 90 - alpha / 2 (alpha is cone angle)
  real radius, radiusSq, lineCos, lineSin, lineCosSq;
  SmallTab<int, MidmemAlloc> ptInd;
  SmallTab<FastPhys::ClippedLine, MidmemAlloc> lines;

  String nodeName;


  FastPhysClipSphericalAction(IGenLoad &cb)
  {

    cb.readString(nodeName);

    memset(&localPos, 0, sizeof(localPos));
    memset(&localDir, 0, sizeof(localDir));
    cb.read(&localPos, sizeof(Point3));
    cb.read(&localDir, sizeof(Point3));

    cb.readReal(radius);
    cb.readReal(angle);

    cb.readTab(ptInd);

    cb.readTab(lines);

    // compute derived values
    radiusSq = radius * radius;
    lineCos = cosf(angle);
    lineSin = sinf(angle);
    lineCosSq = lineCos * lineCos;
  }


  virtual void init(FastPhysSystem &, GeomNodeTree &nodeTree)
  {
    nodeWtm = resolve_node_wtm(nodeTree, nodeName);
    nodeWofs = nodeTree.getWtmOfsPersistentPtr();
  }


  inline real clipPoint(const Point3 &pos, Point3 &normal, const Point3 &center, const Point3 &dir)
  {
    Point3 dp = pos - center;

    real y = dp * dir;
    if (y >= radius)
      return MAX_REAL;

    real lsq = lengthSq(dp);

    if (y < 0 || y * y < lineCosSq * lsq)
    {
      // side
      Point3 sdp = dp - dir * y;
      real slen = length(sdp);

      if (float_nonzero(slen))
        normal = dir * lineCos + sdp * (lineSin / slen);
      else
        normal = dir;

      real d = normal * dp - radius;
      if (d >= 0)
        return d;

      return d;
    }
    else
    {
      // sphere
      if (lsq >= radiusSq)
        return MAX_REAL;

      real len = sqrtf(lsq);

      if (float_nonzero(len))
        normal = dp / len;
      else
        normal = dir;

      return len - radius;
    }
  }


  virtual void perform(FastPhysSystem &sys)
  {
    if (!nodeWtm)
      return;

    Point3_vec4 center;
    Point3_vec4 dir;

    v_st(&center.x, v_mat44_mul_vec3p(*nodeWtm, v_ld(&localPos.x)));
    v_st(&dir.x, v_norm4(v_mat44_mul_vec3v(*nodeWtm, v_ld(&localDir.x))));

    // clip points
    for (int i = 0; i < ptInd.size(); ++i)
    {
      FastPhysSystem::Point &pt = sys.points[ptInd[i]];

      Point3 normal;
      real d = clipPoint(pt.pos, normal, center, dir);
      if (d >= 0)
        continue;

      pt.pos -= normal * d;

      real nv = pt.vel * normal;
      if (nv < 0)
        pt.vel -= normal * nv;
    }

    // clip lines
    for (int i = 0; i < lines.size(); ++i)
    {
      FastPhysSystem::Point &pt1 = sys.points[lines[i].p1Index];
      FastPhysSystem::Point &pt2 = sys.points[lines[i].p2Index];
      int segs = lines[i].numSegs;

      Point3 lineVec = pt2.pos - pt1.pos;

      Point3 normal(0, 0, 0);
      real minD = MAX_REAL, minT = 0.f;

      for (int j = 0; j <= segs; ++j)
      {
        real t = real(j) / segs;
        Point3 pos = pt1.pos + lineVec * t;

        Point3 norm;

        real d = clipPoint(pos, norm, center, dir);
        if (d >= 0)
          continue;

        if (d < minD)
        {
          minD = d;
          normal = norm;
          minT = t;
        }
      }

      if (minD >= 0)
        continue;

      pt1.pos -= normal * (minD * (1 - minT));
      pt2.pos -= normal * (minD * minT);

      real nv = (pt1.vel * (1 - minT) + pt2.vel * minT) * normal;
      if (nv < 0)
      {
        pt1.vel -= normal * (nv * (1 - minT));
        pt2.vel -= normal * (nv * minT);
      }
    }
  }

  virtual void reset(FastPhysSystem &) {}

  virtual void debugRender()
  {
    E3DCOLOR color(255, 255, 50);
    TMatrix localTm = makeTMFromUp(localDir, localPos);
    TMatrix wtm = nodeWtm && nodeWofs ? makeNodeWTM(nodeWtm, nodeWofs) * localTm : localTm;
    set_cached_debug_lines_wtm(wtm);
    float angleCone = (90 - RadToDeg(angle)) * 2;
    FastPhysRender::draw_sphere(radius, angleCone, 3.0, color, true);
  }
};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class FastPhysClipCylindricalAction : public FastPhysAction
{
public:
  mat44f *nodeWtm = nullptr;
  const vec3f *nodeWofs = nullptr;
  Point3_vec4 localPos, localDir, localSideDir;
  real angle; //  in radians, angle = 90 - alpha / 2 (alpha is cone angle)
  real radius, radiusSq, lineCos, lineSin, lineCosSq;
  SmallTab<int, MidmemAlloc> ptInd;
  SmallTab<FastPhys::ClippedLine, MidmemAlloc> lines;

  String nodeName;


  FastPhysClipCylindricalAction(IGenLoad &cb)
  {
    cb.readString(nodeName);

    memset(&localPos, 0, sizeof(localPos));
    memset(&localDir, 0, sizeof(localDir));
    memset(&localSideDir, 0, sizeof(localSideDir));
    cb.read(&localPos, sizeof(Point3));
    cb.read(&localDir, sizeof(Point3));
    cb.read(&localSideDir, sizeof(Point3));

    cb.readReal(radius);
    cb.readReal(angle);

    cb.readTab(ptInd);

    cb.readTab(lines);

    // compute derived values
    radiusSq = radius * radius;
    lineCos = cosf(angle);
    lineSin = sinf(angle);
    lineCosSq = lineCos * lineCos;
  }


  virtual void init(FastPhysSystem &, GeomNodeTree &nodeTree)
  {
    nodeWtm = resolve_node_wtm(nodeTree, nodeName);
    nodeWofs = nodeTree.getWtmOfsPersistentPtr();
  }


  inline real clipPoint(const Point3 &pos, Point3 &normal, const Point3 &center, const Point3 &dir, const Point3 &side_dir)
  {
    Point3 dp = pos - center;

    real y = dp * dir;
    if (y >= radius)
      return MAX_REAL;

    real x = dp * side_dir;

    real lsq = x * x + y * y;

    if (y < 0 || y * y < lineCosSq * lsq)
    {
      // side
      if (x > 0)
        normal = dir * lineCos + side_dir * lineSin;
      else if (x < 0)
        normal = dir * lineCos - side_dir * lineSin;
      else
        normal = dir;

      real d = normal * dp - radius;

      return d;
    }
    else
    {
      // sphere
      if (lsq >= radiusSq)
        return MAX_REAL;

      real len = sqrtf(lsq);

      if (float_nonzero(len))
        normal = dir * (y / len) + side_dir * (x / len);
      else
        normal = dir;

      return len - radius;
    }
  }


  virtual void perform(FastPhysSystem &sys)
  {
    if (!nodeWtm)
      return;

    Point3_vec4 center;
    Point3_vec4 dir;
    Point3_vec4 sideDir;

    v_st(&center.x, v_mat44_mul_vec3p(*nodeWtm, v_ld(&localPos.x)));
    v_st(&dir.x, v_norm4(v_mat44_mul_vec3v(*nodeWtm, v_ld(&localDir.x))));
    v_st(&sideDir.x, v_norm4(v_mat44_mul_vec3v(*nodeWtm, v_ld(&localSideDir.x))));

    // clip points
    for (int i = 0; i < ptInd.size(); ++i)
    {
      FastPhysSystem::Point &pt = sys.points[ptInd[i]];

      Point3 normal;
      real d = clipPoint(pt.pos, normal, center, dir, sideDir);
      if (d >= 0)
        continue;

      pt.pos -= normal * d;

      real nv = pt.vel * normal;
      if (nv < 0)
        pt.vel -= normal * nv;
    }

    // clip lines
    for (int i = 0; i < lines.size(); ++i)
    {
      FastPhysSystem::Point &pt1 = sys.points[lines[i].p1Index];
      FastPhysSystem::Point &pt2 = sys.points[lines[i].p2Index];
      int segs = lines[i].numSegs;

      Point3 lineVec = pt2.pos - pt1.pos;

      Point3 normal(0, 0, 0);
      real minD = MAX_REAL, minT = 0.f;

      for (int j = 0; j <= segs; ++j)
      {
        real t = real(j) / segs;
        Point3 pos = pt1.pos + lineVec * t;

        Point3 norm;

        real d = clipPoint(pos, norm, center, dir, sideDir);
        if (d >= 0)
          continue;

        if (d < minD)
        {
          minD = d;
          normal = norm;
          minT = t;
        }
      }

      if (minD >= 0)
        continue;

      pt1.pos -= normal * (minD * (1 - minT));
      pt2.pos -= normal * (minD * minT);

      real nv = (pt1.vel * (1 - minT) + pt2.vel * minT) * normal;
      if (nv < 0)
      {
        pt1.vel -= normal * (nv * (1 - minT));
        pt2.vel -= normal * (nv * minT);
      }
    }
  }

  virtual void reset(FastPhysSystem &) {}

  virtual void debugRender()
  {
    E3DCOLOR color(255, 255, 50);
    TMatrix localTm = makeTMFromUpAndSide(localDir, localSideDir, localPos);
    TMatrix wtm = nodeWtm && nodeWofs ? makeNodeWTM(nodeWtm, nodeWofs) * localTm : localTm;
    set_cached_debug_lines_wtm(wtm);
    float angleCone = (90 - RadToDeg(angle)) * 2;
    FastPhysRender::draw_cylinder(radius, angleCone, 0.5, color, true);
  }
};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class FastPhysAngularSpringAction : public FastPhysAction
{
public:
  int p1Index, p2Index;
  mat44f *nodeWtm = nullptr;
  const mat44f *nodeParentWtm = nullptr;
  // Point3 localBoneAxis;
  vec3f orgAxis;

  String nodeName;

  real springK;


  FastPhysAngularSpringAction(IGenLoad &cb)
  {
    orgAxis = v_zero();
    cb.readInt(p1Index);
    cb.readInt(p2Index);
    cb.seekrel(sizeof(Point3)); // cb.read(&localBoneAxis, sizeof(localBoneAxis));
    cb.read(&orgAxis, 3 * sizeof(float));
    cb.readString(nodeName);
    cb.readReal(springK);
  }

  virtual void init(FastPhysSystem &, GeomNodeTree &nodeTree)
  {
    nodeWtm = resolve_node_wtm(nodeTree, nodeName, &nodeParentWtm);
    if (!nodeWtm)
      return;

    // orgAxis=normalize(*nodeTm%localBoneAxis);
  }

  virtual void perform(FastPhysSystem &sys)
  {
    if (!nodeWtm || !float_nonzero(sys.deltaTime))
      return;

    FastPhysSystem::Point &pt1 = sys.points[p1Index];
    FastPhysSystem::Point &pt2 = sys.points[p2Index];
    Point3_vec4 acc;

    vec3f dir = v_norm3(v_sub(v_ld(&pt2.pos.x), v_ld(&pt1.pos.x)));

    vec3f targetDir = orgAxis;
    if (nodeParentWtm)
      targetDir = v_norm3(v_mat44_mul_vec3v(*nodeParentWtm, targetDir));

    vec3f torque = v_mul(v_cross3(targetDir, dir), v_splats(springK));
    v_st(&acc, v_cross3(torque, dir));
    pt1.acc += acc;
    pt2.acc -= acc;
  }

  virtual void reset(FastPhysSystem &) {}
};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class FastPhysLookAtBoneCtrlAction : public FastPhysAction
{
public:
  int p1Index, p2Index;
  mat44f *nodeWtm = nullptr;
  mat44f *nodeTm = nullptr;
  const mat44f *nodeParentWtm = nullptr;
  mat33f orgTm;

  String nodeName;

  FastPhysLookAtBoneCtrlAction(IGenLoad &cb)
  {
    vec3f localBoneAxis;

    cb.readInt(p1Index);
    cb.readInt(p2Index);
    cb.read(&localBoneAxis, sizeof(float) * 3);
    cb.read(&orgTm.col0, sizeof(float) * 3);
    cb.read(&orgTm.col1, sizeof(float) * 3);
    cb.read(&orgTm.col2, sizeof(float) * 3);
    cb.readString(nodeName);
  }

  virtual void init(FastPhysSystem &, GeomNodeTree &nodeTree)
  {
    nodeWtm = resolve_node_wtm(nodeTree, nodeName, &nodeParentWtm, &nodeTm);
    if (!nodeWtm)
      return;

    orgTm.col0 = nodeTm->col0;
    orgTm.col1 = nodeTm->col1;
    orgTm.col2 = nodeTm->col2;
  }

  virtual void perform(FastPhysSystem &sys)
  {
    if (!nodeWtm)
      return;

    FastPhysSystem::Point &pt1 = sys.points[p1Index];
    FastPhysSystem::Point &pt2 = sys.points[p2Index];

    mat33f tm, m3;
    if (nodeParentWtm)
    {
      if (p1Index < sys.firstIntegratePoint)
      {
        tm.col0 = nodeWtm->col0;
        tm.col1 = nodeWtm->col1;
        tm.col2 = nodeWtm->col2;
      }
      else
      {
        m3.col0 = nodeParentWtm->col0;
        m3.col1 = nodeParentWtm->col1;
        m3.col2 = nodeParentWtm->col2;
        v_mat33_mul(tm, m3, orgTm);
      }
    }
    else
      tm = orgTm;

    vec3f pt1_pos = v_ld(&pt1.pos.x);
    vec3f dir = v_sub(v_ld(&pt2.pos.x), pt1_pos);

    mat44f lookAt;
    lookAt.col0 = v_norm3(dir);
    lookAt.col2 = v_norm3(v_cross3(orgTm.col1, lookAt.col0));
    lookAt.col1 = v_cross3(lookAt.col0, lookAt.col2);
    lookAt.col3 = pt1_pos;
    *nodeWtm = lookAt;

    if (nodeParentWtm)
    {
      mat44f iwtm;
      v_mat44_inverse43(iwtm, *nodeParentWtm);
      v_mat44_mul43(iwtm, iwtm, *nodeWtm);
      nodeTm->col0 = iwtm.col0;
      nodeTm->col1 = iwtm.col1;
      nodeTm->col2 = iwtm.col2;
      if (p1Index >= sys.firstIntegratePoint)
        nodeTm->col3 = iwtm.col3;
    }
    else
      nodeTm->set33(m3);
  }

  virtual void reset(FastPhysSystem &) {}
};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class FastPhysHingeConstraintAction : public FastPhysAction
{
public:
  int p1Index, p2Index;
  mat44f *nodeWtm = nullptr;
  const mat44f *nodeParentWtm = nullptr;
  Point3 hingeAxis;
  Point3 referenceDir;
  real boneLength;
  real damping;
  real limitCos, limitSin;

  String nodeName;

  FastPhysHingeConstraintAction(IGenLoad &cb) : boneLength(0.05f)
  {
    cb.readInt(p1Index);
    cb.readInt(p2Index);
    cb.readReal(damping);
    cb.read(&hingeAxis, sizeof(hingeAxis));
    cb.readReal(limitCos);
    limitSin = sqrtf(1.0f - limitCos * limitCos);
    cb.readString(nodeName);
  }

  virtual void init(FastPhysSystem &sys, GeomNodeTree &nodeTree)
  {
    boneLength = length(sys.points[p2Index].pos - sys.points[p1Index].pos);
    referenceDir = normalize(sys.points[p2Index].pos - sys.points[p1Index].pos);
    nodeWtm = resolve_node_wtm(nodeTree, nodeName, &nodeParentWtm);
  }

  virtual void perform(FastPhysSystem &sys)
  {
    FastPhysSystem::Point &pt1 = sys.points[p1Index];
    FastPhysSystem::Point &pt2 = sys.points[p2Index];

    Point3 dir = pt2.pos - pt1.pos;
    real lenSq = lengthSq(dir);
    real len = sqrtf(lenSq);
    if (!float_nonzero(len))
      return;

    Point3_vec4 axis = hingeAxis;
    Point3_vec4 ref = referenceDir;
    if (nodeWtm && nodeParentWtm)
      v_st(&axis, v_norm3(v_mat44_mul_vec3v(*nodeParentWtm, v_ldu(&hingeAxis.x))));

    // enforce hinge
    Point3 nd = dir / len;
    Point3 perpd = normalize(nd - axis * (axis * nd));

    // clamp rotation around hinge
    if (limitCos < 1.0f)
    {
      real d = clamp(ref * perpd, -1.0f, 1.0f);
      if (d < limitCos)
      {
        real sign = cross(ref, perpd) * axis >= 0.0f ? 1.0f : -1.0f;
        Point3 perpRef = normalize(cross(axis, ref));
        perpd = normalize(ref * limitCos + perpRef * (sign * limitSin));
      }
    }

    // apply constraints
    pt2.pos = pt1.pos + perpd * boneLength;

    Point3 rv = pt2.vel - pt1.vel;
    // recompute direction after correction
    dir = pt2.pos - pt1.pos;
    lenSq = lengthSq(dir);
    len = sqrtf(lenSq);
    Point3 n2 = dir * (1.0f / len);
    Point3 dv = n2 * (n2 * rv);
    pt2.vel -= dv;

    // apply damping
    Point3 tv = rv - dv;
    real k = damping * sys.deltaTime;
    if (k > 1)
      k = 1;
    pt2.vel -= tv * k;
  }

  virtual void reset(FastPhysSystem &sys) {}

  virtual void debugRender()
  {
    if (!nodeWtm || !nodeParentWtm)
      return;

    E3DCOLOR axisColor(0, 255, 0);
    E3DCOLOR refDirColor(255, 255, 0);
    E3DCOLOR limitColor(255, 0, 0);

    Point3 pos = as_point3(&nodeWtm->col3);
    Point3 refDir = referenceDir;
    Point3_vec4 worldAxis;
    v_st(&worldAxis, v_norm3(v_mat44_mul_vec3v(*nodeParentWtm, v_ldu(&hingeAxis.x))));

    float length = boneLength;
    draw_cached_debug_line(pos, pos + worldAxis * length, axisColor);
    draw_cached_debug_line(pos, pos + refDir * length, refDirColor);

    if (limitCos < 1.0f)
    {
      Point3 perpDir = normalize(cross(worldAxis, refDir));
      Point3 limitDir1 = normalize(refDir * limitCos + perpDir * limitSin);
      Point3 limitDir2 = normalize(refDir * limitCos - perpDir * limitSin);
      draw_cached_debug_line(pos, pos + limitDir1 * length, limitColor);
      draw_cached_debug_line(pos, pos + limitDir2 * length, limitColor);
    }
  }
};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

FastPhysAction *FastPhysSystem::loadAction(IGenLoad &cb)
{
  int id = cb.readInt();

  switch (id)
  {
    case FastPhys::AID_INTEGRATE: return new FastPhysIntegrateAction(cb);

    case FastPhys::AID_INIT_POINT: return new FastPhysInitPointAction(cb);

    case FastPhys::AID_MOVE_POINT_WITH_NODE: return new FastPhysMovePointWithNodeAction(cb);

    case FastPhys::AID_SET_BONE_LENGTH: return new FastPhysSetBoneLengthAction(cb);

    case FastPhys::AID_SET_BONE_LENGTH_WITH_CONE: return new FastPhysSetBoneLengthWithConeAction(cb);

    case FastPhys::AID_MIN_MAX_LENGTH: return new FastPhysMinMaxLengthAction(cb);

    case FastPhys::AID_SIMPLE_BONE_CTRL: return new FastPhysSimpleBoneCtrlAction(cb);

    case FastPhys::AID_CLIP_SPHERICAL: return new FastPhysClipSphericalAction(cb);

    case FastPhys::AID_CLIP_CYLINDRICAL: return new FastPhysClipCylindricalAction(cb);

    case FastPhys::AID_ANGULAR_SPRING: return new FastPhysAngularSpringAction(cb);

    case FastPhys::AID_LOOK_AT_BONE_CTRL: return new FastPhysLookAtBoneCtrlAction(cb);

    case FastPhys::AID_HINGE_CONSTRAINT: return new FastPhysHingeConstraintAction(cb);
  }

  // DEBUG_CTX("unknown action %d", id);
  DAGOR_THROW(IGenLoad::LoadException("invalid action", cb.tell()));
  return NULL;
}
