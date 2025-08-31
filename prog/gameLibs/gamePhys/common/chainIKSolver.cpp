// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/common/chainIKSolver.h>
#include <math/dag_geomTree.h>
#include <math/dag_mathUtils.h>
#include <ioSys/dag_dataBlock.h>
#include <regExp/regExp.h>
#include <debug/dag_debug3d.h>


using namespace chain_ik;


ParticlePoint::ParticlePoint(const GeomNodeTree *tree, dag::Index16 node_id)
{
  nodeId = node_id;
  tree->getNodeWtmRelScalar(nodeId, initialTm);
  tm = initialTm;
}

void ParticlePoint::updateFromTree(const GeomNodeTree *tree) { tree->getNodeWtmRelScalar(nodeId, tm); }

void ChainIKSolver::loadPoints(const DataBlock *blk, const GeomNodeTree *tree)
{
  const int pointsNum = blk->blockCount();
  G_ASSERTF_RETURN((pointsNum >= 2 && pointsNum <= 4), ,
    "Incorrect points count = %d. IK solver only for 1 to 3 edges! (2 to 4 points)", pointsNum);

  for (int i = 0; i < pointsNum; ++i)
  {
    const DataBlock *particleBlk = blk->getBlock(i);
    const char *particleNameTemplate = particleBlk->getStr("nameTemplate");
    RegExp re;
    re.compile(particleNameTemplate, "");

    for (dag::Index16 nodeId(0), endId(tree->nodeCount()); nodeId != endId; ++nodeId)
    {
      if (!re.test(tree->getNodeName(nodeId)))
        continue;

      if (i == 0)
      {
        Chain chain;
        chain.points.reserve(4);

        chain.points.emplace_back(tree, nodeId);
        chains.push_back(eastl::move(chain));
      }
      else if (pointsNum == 2)
      {
        for (Chain &chain : chains)
        {
          if (chain.points.size() != 2)
            chain.points.emplace_back(tree, nodeId);

          const Point3 initDir = chain.points.front().getOriginalPos() - chain.points.back().getOriginalPos();
          const float k = chain.points.front().initialTm.getcol(0) * initDir;
          const Point3 shoulder = chain.points.back().initialTm.getcol(1) * sqrt(fabs(initDir.lengthSq() - sqr(k)));
          const float shoulderLen = shoulder.length();
          if (shoulderLen > 0.02f)
            chain.shoulder = shoulderLen;
        }
      }
      else
      {
        for (Chain &chain : chains)
        {
          const auto chainTailNodeId = chain.points.back().nodeId;
          for (unsigned j = 0, je = tree->getChildCount(chainTailNodeId); j < je; ++j)
            if (nodeId == tree->getChildNodeIdx(chainTailNodeId, j))
            {
              chain.points.emplace_back(tree, nodeId);
              goto exitPushNodeToChainCycle;
            }
        }
      exitPushNodeToChainCycle:;
      }
    }
  }
}

void ChainIKSolver::loadFromBlk(const DataBlock *blk, const GeomNodeTree *tree, const char *class_name)
{
  G_ASSERT_RETURN(blk, );
  G_UNUSED(class_name);

  chains.clear();
  const bool needSelectInverseSolution = blk->getBool("inverseSolution", false);

  const int pointsId = blk->getNameId("points");
  for (int i = 0; i < blk->blockCount(); ++i)
  {
    const DataBlock *pointsblk = blk->getBlock(i);
    if (pointsblk->getBlockNameId() == pointsId)
      loadPoints(pointsblk, tree);
  }

  const DataBlock *jointsBlk = blk->getBlockByNameEx("joints");
  Joint upperJoint;
  Joint bottomJoint;
  const int upperJointNid = jointsBlk->getNameId("upperJoint");

  for (int i = 0; i < jointsBlk->blockCount(); ++i)
  {
    const DataBlock *jointBlk = jointsBlk->getBlock(i);
    Joint &targetJoint = (jointBlk->getBlockNameId() == upperJointNid) ? upperJoint : bottomJoint;
    targetJoint.angleMinRad = DegToRad(jointBlk->getReal("min", 0.f));
    targetJoint.angleMaxRad = DegToRad(jointBlk->getReal("max", 0.f));
  }

  for (Chain &chain : chains)
  {
    const int numEdges = chain.points.size() - 1;
    chain.edges.resize(numEdges);
    chain.needSelectInverseSolution = needSelectInverseSolution;
    for (int i = 0; i < numEdges; ++i)
    {
      chain.edges[i] = (chain.points[i + 1].getPos() - chain.points[i].getPos()).length();
      chain.points[i].originalVec = chain.points[i + 1].getOriginalPos() - chain.points[i].getOriginalPos();
    }
    chain.upperJoint = upperJoint;
    chain.bottomJoint = bottomJoint;
    const int num = chain.points.size();
    G_ASSERT_LOG(num >= 2 && num <= 4, "check 'chainIK' block, the wrong number of points (%d): %s", num, class_name);
  }
}


static Point2 rotate_vec(const Point2 &vec, float cosa, bool clockwise = true)
{
  const float sina = safe_sqrt(1.f - sqr(cosa)) * (clockwise ? 1.f : -1.f);
  return Point2(vec.x * cosa - vec.y * sina, vec.y * cosa + vec.x * sina);
}


static float get_cos_in_triangle(float near_edge_a, float near_edge_b, float opposite_edge)
{
  return clamp((sqr(near_edge_a) + sqr(near_edge_b) - sqr(opposite_edge)) / (2.f * near_edge_b * near_edge_a), -1.f, 1.f);
}


static float get_edge_in_triangle(float near_edge_a, float near_edge_b, float angle_rad)
{
  return safe_sqrt(sqr(near_edge_a) + sqr(near_edge_b) - 2.f * near_edge_b * near_edge_a * cosf(angle_rad));
}


static bool get_circles_intersection(const Point2 &p1, float r1, const Point2 &p2, float r2, Point2 &result_in_out)
{
  const Point2 p2Translated = p2 - p1;
  // line equation
  const float A = -2.f * p2Translated.x;
  const float B = -2.f * p2Translated.y;
  const float C = sqr(p2Translated.x) + sqr(p2Translated.y) + sqr(r1) - sqr(r2);

  // intersection of line (A, B, C) and circle ((0, 0), r1)
  const float sqrSum = sqr(A) + sqr(B);
  const float sqrSumInv = safeinv(sqrSum);
  const float nearestDist = safediv(fabs(C), safe_sqrt(sqrSum));

  if (nearestDist > r1)
    return false;

  const Point2 nearestPt(-A * C * sqrSumInv, -B * C * sqrSumInv);
  const float dSqr = sqr(r1) - sqr(C) * sqrSumInv;
  const float mult = safe_sqrt(dSqr * sqrSumInv);

  const Point2 dirOffset = Point2(-B, A) * mult;
  const Point2 result1 = nearestPt + dirOffset;
  const Point2 result2 = nearestPt - dirOffset;

  const float deltaSq1 = (result1 - result_in_out).lengthSq();
  const float deltaSq2 = (result2 - result_in_out).lengthSq();

  result_in_out = (deltaSq1 < deltaSq2) ? result1 : result2;

  return true;
}


static Point2 get_point_in_triangle(float near_edge_a, float near_edge_b, float opposite_edge, const Point2 &dir, const Point2 &vertex,
  bool is_clockwise)
{
  const float cosAngle = get_cos_in_triangle(near_edge_a, opposite_edge, near_edge_b); //-V764
  Point2 vec = dir * near_edge_a;
  vec = rotate_vec(vec, cosAngle, is_clockwise) + vertex;
  return vec;
}

void rotate_at_point(TMatrix &tm, const Point3 &at)
{
  tm.setcol(0, -normalize(tm.getcol(3) - at));
  tm.setcol(2, normalize(tm.getcol(1) % tm.getcol(0)));
  tm.setcol(1, tm.getcol(0) % tm.getcol(2));
}

void Chain::solve()
{
  const int num = points.size();

  G_ASSERT_RETURN((num >= 2 && num <= 4), );

  const Point3 &pStart = points.front().getPos();
  const Point3 &pFinal = points.back().getPos();

  if (num == 2)
  {
    if (shoulder > 0.f)
    {
      TMatrix dirTm{TMatrix::IDENT};
      dirTm.setcol(3, pStart);
      rotate_at_point(dirTm, pFinal);

      const Point3 offset = dirTm.getcol(1) * shoulder;
      if (needRotateFirst)
        rotate_at_point(points.front().tm, pFinal + offset);
      rotate_at_point(points.back().tm, pStart - offset);
    }
    else
    {
      if (needRotateFirst)
        rotate_at_point(points.front().tm, pFinal);
      rotate_at_point(points.back().tm, pStart);
    }
    return;
  }

  // make local tm for plane by start point, end point and x-axis of start node
  tmToLocal2DSpace.setcol(3, pStart);
  tmToLocal2DSpace.setcol(0, points.front().initialTm.getcol(0));
  tmToLocal2DSpace.setcol(2, normalize(tmToLocal2DSpace.getcol(0) % (pFinal - pStart)));
  tmToLocal2DSpace.setcol(1, tmToLocal2DSpace.getcol(0) % tmToLocal2DSpace.getcol(2));

  const Point2 endEffectorLocal = Point2::xy(pFinal * inverse(tmToLocal2DSpace));

  eastl::vector<Point2> localPoints;
  localPoints.resize(num);
  localPoints.front() = ZERO<Point2>();
  localPoints.back() = endEffectorLocal;

  // calculate angles and local positions
  if (num == 3)
  {
    const float length = endEffectorLocal.length();
    const Point2 dir = normalize(endEffectorLocal);
    const float l1 = edges.front();
    const float l2 = edges.back();

    localPoints[1] = get_point_in_triangle(l1, length, l2, dir, ZERO<Point2>(), false);
  }
  else
  {
    calculate3EdgesSolution(endEffectorLocal, localPoints);
  }

  // calculate transformed positions
  for (int i = 1; i < num - 1; ++i)
  {
    points[i].tm.setcol(3, Point3::xy0(localPoints[i]) * tmToLocal2DSpace);
  }

  // calculate node rotations
  for (int i = 0; i < num - 1; ++i)
  {
    const Point3 currentVec = points[i + 1].getPos() - points[i].getPos();
    const TMatrix rotTm = makeTM(quat_rotation_arc(points[i].originalVec, currentVec));
    const Point3 savedPos = points[i].getPos();
    points[i].tm = rotTm * points[i].initialTm;
    points[i].tm.setcol(3, savedPos);
  }

  const Point3 savedPos = points.back().getPos();
  points.back().tm = points.back().tm * points.back().initialTm;
  points.back().tm.setcol(3, savedPos);
}


void Chain::calculate3EdgesSolution(const Point2 &end_effector_local, eastl::vector<Point2> &local_points_out)
{
  const float length = end_effector_local.length();
  const Point2 dir = normalize(end_effector_local);

  const float l1 = edges.front();
  const float l2 = edges.at(1);
  const float l3 = edges.back();

  const float ratioL2 = 0.5f; // second edge is divided by end_effector_local vec by 1:1 ratio
  const float ratio = (l1 + l2 * 0.5) / (l1 + l2 + l3);

  // 1st triangle
  const float a1 = l1;
  const float b1 = l2 * ratioL2;
  const float c1 = length * ratio;
  Point2 pt1 = get_point_in_triangle(a1, b1, c1, dir, ZERO<Point2>(), !needSelectInverseSolution);

  // 2nd triangle
  const float a2 = l3;
  const float b2 = l2 * (1.f - ratioL2);
  const float c2 = length * (1.f - ratio);
  Point2 pt2 = get_point_in_triangle(a2, b2, c2, -dir, end_effector_local, !needSelectInverseSolution);

  // check limits
  if (upperJoint.needCheckRange())
  {
    float upperJointAngle = safe_acos(get_cos_in_triangle(a1, b1, c1));

    if (upperJoint.trimAngle(upperJointAngle))
    {
      // upper joint is clamped, need correct pt2 and pt1 position
      const float c1New = get_edge_in_triangle(l1, l2, upperJointAngle);
      Point2 p2Solution = pt2;
      if (get_circles_intersection(ZERO<Point2>(), c1New, end_effector_local, l3, p2Solution))
      {
        pt2 = p2Solution;
        pt1 = get_point_in_triangle(l1, l2, pt2.length(), normalize(pt2), ZERO<Point2>(), !needSelectInverseSolution);
      }
      else
      {
        debug("IK Error! There is no solution for this case, try increase upper joint max limit! %f",
          RadToDeg(upperJoint.angleMaxRad));
      }
    }
  }

  if (bottomJoint.needCheckRange())
  {
    float bottomJointAngle = safe_acos(get_cos_in_triangle(l2, l3, (end_effector_local - pt1).length()));

    if (bottomJoint.trimAngle(bottomJointAngle))
    {
      // bottom joint is clamped, need correct pt1 and pt2 position
      const float c2New = get_edge_in_triangle(l3, l2, bottomJointAngle);
      Point2 p1Solution = pt1;

      if (get_circles_intersection(ZERO<Point2>(), l1, end_effector_local, c2New, p1Solution))
      {
        pt1 = p1Solution;
        const Point2 p3p1Vec = pt1 - end_effector_local;
        pt2 = get_point_in_triangle(l3, l2, p3p1Vec.length(), normalize(p3p1Vec), end_effector_local, !needSelectInverseSolution);
      }
      else
      {
        debug("IK Error! There is no solution for this case, try increase bottom joint max limit! %f",
          RadToDeg(bottomJoint.angleMaxRad));
      }

      if (upperJoint.needCheckRange())
      {
        const float upperJointAngle = safe_acos(get_cos_in_triangle(l1, l2, pt2.length()));
        if (upperJoint.isInRange(upperJointAngle))
        {
          debug("IK Error! There is no solution for this case!");
        }
      }
    }
  }

  local_points_out[1] = pt1;
  local_points_out[2] = pt2;
}

void ChainIKSolver::solve()
{
  for (Chain &chain : chains)
    chain.solve();
}

void ChainIKSolver::updateGeomNodeTree(GeomNodeTree *tree, const TMatrix &render_space_tm /*= TMatrix::IDENT*/) const
{
  for (const Chain &chain : chains)
  {
    for (int i = 0; i < chain.points.size(); ++i)
    {
      const ParticlePoint &p = chain.points[i];
      TMatrix finalTm = render_space_tm * p.tm;
      tree->setNodeWtmRelScalar(p.nodeId, finalTm);
      tree->markNodeTmInvalid(p.nodeId);
    }
  }
}

#if DAGOR_DBGLEVEL > 0

void debug_draw_smooth_sector(const Point3 &pt, const Point3 &dir, const Point3 &normal, float angle_rad, float radius,
  const E3DCOLOR &color)
{
  const float N = 6.f;
  const float deltaAngle = angle_rad / N;

  const Point3 scaledDir = dir * radius;

  for (int i = 0; i < N; ++i)
  {
    const float angleOffset = i * deltaAngle;
    const Quat quat0(normal, angleOffset);
    const Quat quat1(normal, angleOffset + deltaAngle * 0.5f);
    const Quat quat2(normal, angleOffset + deltaAngle);

    const Point3 pt1 = pt + quat0 * scaledDir;
    const Point3 pt2 = pt + quat1 * scaledDir;
    const Point3 pt3 = pt + quat2 * scaledDir;

    const Point3 ptArray[4] = {pt, pt1, pt3, pt2};
    draw_cached_debug_quad(ptArray, color);
  }
}


void ChainIKSolver::renderDebug(const TMatrix &render_space_tm) const
{
  begin_draw_cached_debug_lines(false, false);

  const float sectorMinRadius = 0.6f;
  const float sectorMaxRadius = 0.42f;
  const E3DCOLOR minColor(255.f, 0.f, 0.f, 200.f);
  const E3DCOLOR maxColor(0.f, 0.f, 255.f, 200.f);
  const E3DCOLOR lineColor(255.f, 0.f, 0.f, 255.f);
  const E3DCOLOR pointColor(255.f, 255.f, 0.f, 255.f);

  for (const Chain &chain : chains)
  {
    Point3 surfaceNormal = ZERO<Point3>();
    for (int i = 0; i < (int)chain.points.size() - 1; ++i)
    {
      const Point3 pt1 = (render_space_tm * chain.points[i].tm).getcol(3);
      const Point3 pt2 = (render_space_tm * chain.points[i + 1].tm).getcol(3);
      draw_cached_debug_line(pt1, pt2, lineColor);
      draw_cached_debug_sphere(pt1, 0.05f, pointColor);

      if (chain.shoulder > 0.f)
        draw_cached_debug_sphere(pt1 - chain.shoulder * chain.points[i].tm.getcol(1), 0.02f, maxColor);

      if (i == 1)
      {
        // upper joint
        const Point3 pt0 = (render_space_tm * chain.points[0].tm).getcol(3);
        const Point3 dir = pt0 - pt1;
        surfaceNormal = normalize(dir % (pt2 - pt1));
        if (chain.upperJoint.needCheckRange())
        {
          debug_draw_smooth_sector(pt1, normalize(dir), surfaceNormal, chain.upperJoint.angleMinRad, sectorMinRadius, minColor);
          debug_draw_smooth_sector(pt1, normalize(dir), surfaceNormal, chain.upperJoint.angleMaxRad, sectorMaxRadius, maxColor);
        }
      }
      else if (i == 2)
      {
        // bottom joint
        const Point3 pt0 = (render_space_tm * chain.points[1].tm).getcol(3);
        const Point3 dir = pt0 - pt1;
        if (chain.bottomJoint.needCheckRange())
        {
          debug_draw_smooth_sector(pt1, normalize(dir), surfaceNormal, -chain.bottomJoint.angleMinRad, sectorMinRadius, minColor);
          debug_draw_smooth_sector(pt1, normalize(dir), surfaceNormal, -chain.bottomJoint.angleMaxRad, sectorMaxRadius, maxColor);
        }
      }
    }

    const Point3 controlPoint = (render_space_tm * chain.points.back().tm).getcol(3);
    draw_cached_debug_sphere(controlPoint, 0.05f, E3DCOLOR_MAKE(255, 255, 0, 255));

    const TMatrix localTMDebug = render_space_tm * chain.tmToLocal2DSpace;
    const Point3 pos = localTMDebug.getcol(3);
    draw_cached_debug_sphere(pos, 0.2f, E3DCOLOR_MAKE(255, 255, 255, 255));
    draw_cached_debug_line(pos, pos + localTMDebug.getcol(0) * 0.8f, E3DCOLOR_MAKE(255, 0, 0, 255));
    draw_cached_debug_line(pos, pos + localTMDebug.getcol(1) * 0.8f, E3DCOLOR_MAKE(0, 255, 0, 255));
    draw_cached_debug_line(pos, pos + localTMDebug.getcol(2) * 0.8f, E3DCOLOR_MAKE(0, 0, 255, 255));
  }

  end_draw_cached_debug_lines();
}

#endif


void Chain::initWithEdges(const eastl::vector<float> &edges_lengths, const TMatrix &tm_start, const Point3 &pt_finish)
{
  edges = edges_lengths;
  points.resize(edges.size() + 1);

  Point3 chainDirection = (pt_finish - tm_start.getcol(3));
  const float vecLength = chainDirection.length();
  chainDirection *= safeinv(vecLength);

  for (int i = 0; i < points.size(); ++i)
  {
    points[i].tm = tm_start;
    points[i].tm.setcol(3, tm_start.getcol(3) + chainDirection * vecLength * i / edges.size());
    points[i].initialTm = points[i].tm;
    if (i < edges.size())
      points[i].originalVec = chainDirection * edges[i];
  }
}
