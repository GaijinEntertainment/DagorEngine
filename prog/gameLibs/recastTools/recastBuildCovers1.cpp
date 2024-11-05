// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <recastTools/recastBuildCovers.h>
#include <recastTools/recastTools.h>

#include "commonLineSampler.h"

namespace recastbuild
{
void traceCovers(const rcHeightfield *m_solid, const rcCompactHeightfield *m_chf, const Edge &edge, const CoversParams &covParams,
  Tab<covers::Cover> &coverList)
{
  Point3 dirToSq = edge.sq - edge.sp;
  float len = length(dirToSq);
  dirToSq *= safeinv(len);

  Point3 normal = dirToSq % Point3(0.f, 1.f, 0.f);
  normal.normalize();
  Point3 depth = normal * covParams.minDepth;

  int nsamples = (int)ceilf(len * safeinv(m_chf->cs));
  Tab<signed char> coverSamplerDirs(tmpmem);
  coverSamplerDirs.reserve(nsamples);
  LineSampler coverSample(edge, nsamples);
  coverSample.checkGround(m_chf, m_chf->cs * 2.f, covParams.agentMaxClimb);

  for (int i = 0; i < (int)coverSample.points.size(); ++i)
  {
    float groundHeight = coverSample.points[i].height;
    coverSamplerDirs.push_back(0);
    coverSample.points[i].height = 0.f;
    if (!coverSample.points[i].haveBit(NAVMESH_CHECKED))
      continue;

    const float u = (float)i / (float)((int)coverSample.points.size() - 1);
    Point3 curPt = lerp(edge.sp, edge.sq, u);
    curPt.y = groundHeight;
    Point3 leftPt = curPt + depth;
    Point3 rightPt = curPt - depth;

    float lHeight = recastcoll::get_line_max_height(m_solid, leftPt, curPt, covParams.shootWindowHeight);
    float rHeight = recastcoll::get_line_max_height(m_solid, rightPt, curPt, covParams.shootWindowHeight);
    float dlHeight = max(lHeight - groundHeight, 0.f);
    float drHeight = max(rHeight - groundHeight, 0.f);

    bool coverDir = dlHeight > drHeight; //? normal : -normal;
    float maxHeight = max(dlHeight, drHeight);
    const float bigCoverTreshold = 0.6f;
    if (maxHeight > (covParams.minHeight + covParams.maxHeight) * 0.5f && min(dlHeight, drHeight) / maxHeight > bigCoverTreshold)
    {
      Point3 traceOffs = Point3(0.f, covParams.minHeight, 0.f);
      Point3 tracePt = curPt + traceOffs;
      Point3 traceL = leftPt + traceOffs;
      Point3 traceR = rightPt + traceOffs;
      bool isL = recastcoll::traceray(m_solid, tracePt, traceL);
      bool isR = recastcoll::traceray(m_solid, tracePt, traceR);
      coverDir = isL && isR ? lengthSq(tracePt - traceL) < lengthSq(tracePt - traceR) : (isL || isR ? isL : coverDir);
    }

    Point3 coverPt = coverDir ? leftPt : rightPt;
    coverPt.y = groundHeight;
    float coverHeight =
      min(recastcoll::get_line_max_height(m_solid, curPt, coverPt, covParams.shootWindowHeight), covParams.maxHeight + groundHeight);
    float coverDHeight = coverHeight - groundHeight;
    if (coverDHeight < covParams.minHeight)
      continue;

    float collTransp = recastcoll::get_line_transparency(m_solid, curPt, coverPt, coverHeight, groundHeight);
    if (collTransp < covParams.minCollTransparent)
      continue;
    coverSamplerDirs[i] = coverDir ? 1 : -1;

    coverSample.points[i].height = coverHeight;
    coverSample.points[i].AddBit(HEIGHT_CHECKED);
  }


  Point3 coverDir = normal;
  { // get cover direction
    int coverDirectionSumm = 0;
    for (auto dir : coverSamplerDirs) // get main caver direction
      coverDirectionSumm += (int)dir;
    if (coverDirectionSumm == 0)
      return;

    bool coverDirB = coverDirectionSumm > 0;
    if (!coverDirB)
      coverDir = -normal;
    signed char coverDirection = coverDirB ? 1 : -1;
    for (int i = 0; i < (int)coverSample.points.size(); ++i)
      if (coverSamplerDirs[i] && coverSamplerDirs[i] != coverDirection)
        coverSample.points[i].RemoveBit(HEIGHT_CHECKED);
  }


  Tab<Edge> coversUpper(tmpmem);
  coversUpper.reserve(5);
  int csWidth = (int)floorf(covParams.minWidth * safeinv(m_chf->cs));
  coverSample.findCoverLines(csWidth, covParams, coversUpper);


  Point3 shootDepth = coverDir * covParams.shootDepth;
  float maxShootDist = max(covParams.minWidth * 2.f, 1.f);
  float offsShootDist = covParams.minWidth * 0.5f;
  int nShootSamples = (int)ceilf(maxShootDist * safeinv(m_chf->cs));

  auto traceShootDir = [&](const Point3 &start, const Point3 &stop, Point3 &shootPos) -> bool {
    for (int i = 0; i < nShootSamples; i++)
    {
      const float u = (float)i / (float)(nShootSamples - 1);
      Point3 curPt = lerp(start, stop, u);

      float height = 0.f;
      if (!recastcoll::get_compact_heightfield_height(m_chf, curPt, m_chf->cs * 2.f, covParams.agentMaxClimb, height))
        return false;
      height = min(height, curPt.y);

      float shootHeight = recastcoll::get_line_max_height(m_solid, curPt, curPt + shootDepth, covParams.shootWindowHeight);
      if (shootHeight < covParams.shootHeight + height && shootHeight > -FLT_MAX)
      {
        curPt.y = height;
        shootPos = curPt;
        return true;
      }
    }
    return false;
  };

  BBox3 traceBox;
  float findCt = 0.f;

  if (coversUpper.size())
  {
    findCt = recastcoll::build_box_on_sphere_cast(m_solid, edge.sp, 15, {10, 5}, traceBox);
    findCt *= recastcoll::build_box_on_sphere_cast(m_solid, edge.sq, 15, {10, 5}, traceBox);
    if (findCt < covParams.openingTreshold)
      traceBox = covParams.maxVisibleBox;
    else
      traceBox.inflate(covParams.boxOffset);
  }

  for (const auto &cover : coversUpper)
  {
    if (lengthSq(Point2::xz(cover.sp - cover.sq)) < sqr(covParams.minWidth))
      continue;

    covers::Cover coverDesc;
    coverDesc.groundLeft = cover.sp;
    coverDesc.groundRight = cover.sq;

    Point3 spPt = Point3::x0z(coverSample.edge.sp);
    Point3 sqPt = Point3::x0z(coverSample.edge.sq);
    Point3 spCov = Point3::x0z(cover.sp);
    Point3 sqCov = Point3::x0z(cover.sq);

    float distSp = length(spPt - spCov);
    coverDesc.groundLeft.y = lerp(coverSample.edge.sp.y, coverSample.edge.sq.y, distSp / (distSp + length(sqPt - spCov)));
    float distSq = length(spPt - sqCov);
    coverDesc.groundRight.y = lerp(coverSample.edge.sp.y, coverSample.edge.sq.y, distSq / (distSq + length(sqPt - sqCov)));

    float groundLeftY = coverDesc.groundLeft.y;
    float groundRightY = coverDesc.groundRight.y;
    if (!recastcoll::get_compact_heightfield_height(m_chf, coverDesc.groundLeft, m_chf->cs * 2.f, 0.5f, groundLeftY))
      continue;
    if (!recastcoll::get_compact_heightfield_height(m_chf, coverDesc.groundRight, m_chf->cs * 2.f, 0.5f, groundRightY))
      continue;

    coverDesc.groundLeft.y = min(groundLeftY, coverDesc.groundLeft.y);
    coverDesc.groundRight.y = min(groundRightY, coverDesc.groundRight.y);

    coverDesc.dir = coverDir;
    coverDesc.hLeft = cover.sp.y - coverDesc.groundLeft.y;
    coverDesc.hRight = cover.sq.y - coverDesc.groundRight.y;

    if (coverDesc.hLeft < covParams.minHeight || coverDesc.hRight < covParams.minHeight)
      continue;

    Point3 shootRightSp = (dirToSq * -offsShootDist) + coverDesc.groundRight;
    Point3 shootRightSq = (dirToSq * maxShootDist) + shootRightSp;

    Point3 shootLeftSp = (dirToSq * offsShootDist) + coverDesc.groundLeft;
    Point3 shootLeftSq = (dirToSq * -maxShootDist) + shootLeftSp;

    coverDesc.hasLeftPos = traceShootDir(shootLeftSp, shootLeftSq, coverDesc.shootLeft);
    coverDesc.hasRightPos = traceShootDir(shootRightSp, shootRightSq, coverDesc.shootRight);

    coverDesc.visibleBox = traceBox;
    coverList.push_back(coverDesc);
  }
}

void build_covers_v1(Tab<covers::Cover> &out_covers, const BBox3 &tile_box, const Tab<Edge> &edges, const CoversParams &covParams,
  const rcCompactHeightfield *chf, const rcHeightfield *solid, RenderEdges *out_render_edges)
{
  G_UNUSED(tile_box);
  G_UNUSED(out_render_edges);

  for (int i = 0; i < (int)edges.size(); ++i)
    traceCovers(solid, chf, edges[i], covParams, out_covers);
}
} // namespace recastbuild
