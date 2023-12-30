#include <render/wind/clusterWind.h>

#include <math/dag_hlsl_floatx.h>
#include <render/wind/rendinst_bend_desc.hlsli>
#include <render/wind/clusterWindRenderer.h>
#include <render/wind/clusterWindParticleDef.hlsli>

#include <perfMon/dag_statDrv.h>

#define PARTICLE_BLAST_TIME (DISK_RADIUS / BOMBS_BLAST_AIR_SPEED)

ClusterWind::ClusterWind(bool need_historical_buffer)
{
  clusterWindRenderer.reset(new ClusterWindRenderer(need_historical_buffer));
  actualClusterWindsCount = 0;
  isInit = false;
  actualDirectionnalClusterWindCount = 0;
  dynamicWindMgr.reset(new DynamicWind);
  mem_set_0(directionnalWindId);
  memset(&clusterWinds, 0, sizeof(clusterWinds));
  memset(&directionnalWindId, 0, sizeof(directionnalWindId));
}

ClusterWind::~ClusterWind()
{
  clusterWindRenderer.reset();
  clear_and_shrink(clusterWindCascades);
  actualClusterWindsCount = 0;
  isInit = false;
  actualDirectionnalClusterWindCount = 0;
  dynamicWindMgr.reset();
}

int ClusterWind::addClusterToList(const Point3 &pos, float r, float power, const Point3 &dir, bool directionnal)
{
  if (actualClusterWindsCount == MAX_CLUSTER)
    return -1;
  ClusterDesc sphereToAdd;
  sphereToAdd.sphere = Point4(pos.x, pos.y, pos.z, r);
  sphereToAdd.power = cvt(power, 0.f, 1.0f, 1.0f, MAX_POWER_FOR_BLAST);
  sphereToAdd.direction = dir;
  sphereToAdd.directionnalId = directionnal ? 1 : -1;
  sphereToAdd.time = 0.0f;
  sphereToAdd.maxTime = BOMBS_BLAST_ANIMATION_TIME;

  if (directionnal)
  {
    directionnalWindId[actualDirectionnalClusterWindCount] = actualClusterWindsCount;
    actualDirectionnalClusterWindCount++;
  }
  clusterWinds[actualClusterWindsCount++] = sphereToAdd;
  return actualClusterWindsCount - 1;
}

void ClusterWind::removeClusterFromList(int id)
{
  for (int i = id; i < actualClusterWindsCount - 1; ++i)
  {
    clusterWinds[i] = clusterWinds[i + 1];
  }
  actualClusterWindsCount--;
}

void ClusterWind::recalculateWholeCascade(const Point3 &pos, int cascadeNo)
{
  for (int i = 0; i < clusterWindCascades[cascadeNo].gridBoxes.size(); ++i)
    clusterWindCascades[cascadeNo].resetId(i);
  for (int i = 0; i < actualClusterWindsCount; ++i)
  {
    ClusterWind::ClusterDesc &cluster = clusterWinds[i];
    Point3 clusterPos = Point3(cluster.sphere.x, cluster.sphere.y, cluster.sphere.z);
    addClusterToGrid(clusterPos, cluster.sphere.x, cluster.power, cluster.direction, cluster.directionnalId, i);
  }
  clusterWindCascades[cascadeNo].position =
    pos + Point3(clusterWindCascades[cascadeNo].size * 0.5f, 0.0f, clusterWindCascades[cascadeNo].size * 0.5f);
  clusterWindCascades[cascadeNo].setOffset(IPoint2(0, 0));

  if (clusterWindRenderer)
  {
    clusterWindRenderer->updateClusterWindGridsBuffer(clusterWindCascades);
    clusterWindRenderer->updateClusterWindGridsDesc(clusterWindCascades);
  }
}

bool ClusterWind::checkAndFillGridBox(const IPoint2 &boxXY, int cascadeNo, int clusterId)
{
  if (boxXY.x < 0 || boxXY.x >= clusterWindCascades[cascadeNo].boxWidthNum || boxXY.y < 0 ||
      boxXY.y >= clusterWindCascades[cascadeNo].boxWidthNum)
    return false;

  int boxId = boxXY.y * clusterWindCascades[cascadeNo].boxWidthNum + boxXY.x;
  for (int i = 0; i < clusterWindCascades[cascadeNo].maxClusterPerBox; ++i)
  {
    int actualClusterId = clusterWindCascades[cascadeNo].getClusterId(boxId, i);
    if (actualClusterId == ClusterWindCascade::NO_INDEX)
    {
      clusterWindCascades[cascadeNo].setClusterIndexToBox(boxId, clusterId, i);
      return true;
    }
    else if (actualClusterId == clusterId)
    {
      return false;
    }
    else
    {
      ClusterDesc newSphere = clusterWinds[clusterId];
      ClusterDesc sphereTocheck = clusterWinds[actualClusterId];
      if (isPriority(newSphere, sphereTocheck, boxId, cascadeNo))
      {
        clusterWindCascades[cascadeNo].setClusterIndexToBox(boxId, clusterId, i);
        return true;
      }
    }
  }
  return false;
}

void ClusterWind::updateAfterPositionChange(ClusterWindCascade::ClusterWindGridUpdateResult gridPositionResult, int cascadeNo)
{
  if (gridPositionResult == ClusterWindCascade::NO_RESULT)
    return;
  Point2 gridPosition =
    Point2(clusterWindCascades[cascadeNo].getActualPosition().x, clusterWindCascades[cascadeNo].getActualPosition().z);
  int boxWidthNum = clusterWindCascades[cascadeNo].boxWidthNum;
  float boxSize = clusterWindCascades[cascadeNo].boxSize;
  Point2 leftTop = Point2(0.0f, 0.0f);
  Point2 rightBottom = Point2(0.0f, 0.0f);
  IPoint2 offset(0, 0);
  // reset other side
  switch (gridPositionResult)
  {
    case ClusterWindCascade::X_POSITIV:
    {
      // reset the negativ_X border
      for (int i = 0; i < clusterWindCascades[cascadeNo].boxWidthNum; ++i)
        clusterWindCascades[cascadeNo].resetId(i * boxWidthNum);
      leftTop = Point2(gridPosition.x + boxWidthNum * boxSize, gridPosition.y);
      rightBottom = Point2(gridPosition.x + (boxWidthNum + 1) * boxSize, gridPosition.y + boxWidthNum * boxSize);
      offset.x = 1;
    }
    break;
    case ClusterWindCascade::X_NEGATIV:
    {
      // reset the positiv_X border
      for (int i = 0; i < clusterWindCascades[cascadeNo].boxWidthNum; ++i)
        clusterWindCascades[cascadeNo].resetId(i * boxWidthNum + (boxWidthNum - 1));
      leftTop = Point2(gridPosition.x - boxSize, gridPosition.y);
      rightBottom = Point2(gridPosition.x, gridPosition.y + boxWidthNum * boxSize);
      offset.x = -1;
    }
    break;
    case ClusterWindCascade::Z_POSITIV:
    {
      // reset the negativ_Y
      for (int i = 0; i < clusterWindCascades[cascadeNo].boxWidthNum; ++i)
        clusterWindCascades[cascadeNo].resetId(i);
      leftTop = Point2(gridPosition.x, gridPosition.y + boxWidthNum * boxSize);
      rightBottom = Point2(gridPosition.x + boxWidthNum * boxSize, gridPosition.y + (boxWidthNum + 1) * boxSize);
      offset.y = 1;
    }
    break;
    case ClusterWindCascade::Z_NEGATIV:
    {
      // reset the positiv_z
      for (int i = 0; i < clusterWindCascades[cascadeNo].boxWidthNum; ++i)
        clusterWindCascades[cascadeNo].resetId(i + (boxWidthNum * (boxWidthNum - 1)));
      leftTop = Point2(gridPosition.x, gridPosition.y - boxSize);
      rightBottom = Point2(gridPosition.x + boxWidthNum * boxSize, gridPosition.y);
      offset.y = -1;
    }
    break;
    default: break;
  }

  // updateOffset
  clusterWindCascades[cascadeNo].updateOffset(offset);

  BBox2 borderBox(leftTop, rightBottom);
  for (int i = 0; i < actualClusterWindsCount; ++i)
  {
    if (clusterWindCascades[cascadeNo].isSphereOnBox(borderBox, Point2(clusterWinds[i].sphere.x, clusterWinds[i].sphere.z),
          clusterWinds[i].sphere.w))
    {
      const Point3 clusterPos = Point3(clusterWinds[i].sphere.x, clusterWinds[i].sphere.y, clusterWinds[i].sphere.z);
      int closestBoxId = clusterWindCascades[cascadeNo].getClosestStaticBoxId(clusterPos);
      IPoint2 boxXY = clusterWindCascades[cascadeNo].getStaticBoxXY(closestBoxId);
      checkAndFillGridBox(boxXY, cascadeNo, i);
      int boxPropagationAmount = abs((float)ceil((clusterWinds[i].sphere.w - clusterWindCascades[cascadeNo].boxSize * 0.5f) /
                                                 (float)clusterWindCascades[cascadeNo].boxSize)); // for now simplified diagonal,
                                                                                                  // treated as V/H
      bool isXAxis = gridPositionResult == ClusterWindCascade::X_POSITIV || gridPositionResult == ClusterWindCascade::X_NEGATIV;
      for (int j = 1; j < boxPropagationAmount + 1; ++j)
      {
        if (isXAxis)
        {
          IPoint2 newBoxXY = boxXY + IPoint2(0, j);
          checkAndFillGridBox(newBoxXY, cascadeNo, i);
          newBoxXY = boxXY + IPoint2(0, -j);
          checkAndFillGridBox(newBoxXY, cascadeNo, i);
        }
        else
        {
          IPoint2 newBoxXY = boxXY + IPoint2(j, 0);
          checkAndFillGridBox(newBoxXY, cascadeNo, i);
          newBoxXY = boxXY + IPoint2(-j, 0);
          checkAndFillGridBox(newBoxXY, cascadeNo, i);
        }
      }
    }
  }

  // render
  if (clusterWindRenderer)
  {
    clusterWindRenderer->updateClusterBuffer(clusterWinds.data(), min(MAX_CLUSTER, actualClusterWindsCount));
    clusterWindRenderer->updateClusterWindGridsBuffer(clusterWindCascades);
    clusterWindRenderer->updateClusterWindGridsDesc(clusterWindCascades);
  }
}

void ClusterWind::addClusterToGrid(const Point3 &pos, float r, float power, const Point3 &dir, bool directionnal, int clusterId)
{
  TIME_PROFILE(addClusterToGrid);
  OSSpinlockScopedLock lock(&clusterToGridSpinLock);
  int id = clusterId >= 0 ? clusterId : addClusterToList(pos, r, power, dir, directionnal);
  if (id < 0)
    return;
  int startingCascade = directionnal ? 0 : clusterWindCascades.size() - 1; // no directionnal for externalCascade
  for (int i = startingCascade; i >= 0; --i)
  {
    if (!clusterWindCascades[i].isClusterOnCascade(pos, r))
      continue;
    int closestBoxId = clusterWindCascades[i].getClosestStaticBoxId(pos);
    int clusterWidth = ceil(r / clusterWindCascades[i].boxSize) + 1;
    int cascadeBoxWidthNum = clusterWindCascades[i].boxWidthNum;
    int y = closestBoxId / cascadeBoxWidthNum;
    int x = closestBoxId - y * cascadeBoxWidthNum;

    for (int xx = max(x - clusterWidth, 0); xx < min(x + clusterWidth, cascadeBoxWidthNum); ++xx)
    {
      for (int yy = max(y - clusterWidth, 0); yy < min(y + clusterWidth, cascadeBoxWidthNum); ++yy)
      {
        int boxId = yy * cascadeBoxWidthNum + xx;
        if (clusterWindCascades[i].isClusterOnBox(boxId, Point2(pos.x, pos.z), r))
        {
          IPoint2 boxXY = clusterWindCascades[i].getStaticBoxXY(boxId);
          checkAndFillGridBox(boxXY, i, id);
        }
      }
    }
  }

  // render
  if (clusterWindRenderer)
  {
    clusterWindRenderer->updateClusterBuffer(clusterWinds.data(), min(MAX_CLUSTER, actualClusterWindsCount));
    clusterWindRenderer->updateClusterWindGridsBuffer(clusterWindCascades);
    clusterWindRenderer->updateClusterWindGridsDesc(clusterWindCascades);
  }
}

void ClusterWind::initClusterWinds(Tab<ClusterWindCascade::Desc> &desc)
{
  clear_and_shrink(clusterWindCascades);
  clusterWindCascades.resize(desc.size());
  for (int i = 0; i < clusterWindCascades.size(); ++i)
    clusterWindCascades[i].initGrid(desc[i]);

  // fetch desc to gpu
  if (clusterWindRenderer)
  {
    clusterWindRenderer->updateClusterWindGridsDesc(clusterWindCascades);
    clusterWindRenderer->updateClusterWindGridsBuffer(clusterWindCascades);
    clusterWindRenderer->updateClusterBuffer(clusterWinds.data(), min(MAX_CLUSTER, actualClusterWindsCount));
  }

  isInit = true;
}

void ClusterWind::updateClusterWinds(float dt)
{
  TIME_PROFILE(updateClusterWinds);
  bool updateGridsBuffer = false;
  for (int i = 0; i < actualClusterWindsCount; ++i)
  {
    if (clusterWinds[i].directionnalId >= 0)
      continue;
    clusterWinds[i].time += dt;
    if (clusterWinds[i].time >= clusterWinds[i].maxTime)
    {
      updateGridsBuffer = true;
      removeClusterFromList(i);
      for (int j = 0; j < clusterWindCascades.size(); ++j)
        clusterWindCascades[j].removeClusterFromGrids(i);
    }
  }
}
void ClusterWind::loadBendingMultConst(float treeMult, float impostorMult, float grassMult, float treeStaticMult,
  float impostorStaticMult, float grassStaticMult, float treeAnimationMult, float grassAnimationMult)
{
  if (clusterWindRenderer)
    clusterWindRenderer->loadBendingMultConst(treeMult, impostorMult, grassMult, treeStaticMult, impostorStaticMult, grassStaticMult,
      treeAnimationMult, grassAnimationMult);
}

void ClusterWind::updateDirectionnalWinds(const Point3 &pos, float dt)
{
  TIME_PROFILE(updateDirectionnalWinds);
  // moving wind, remove them from grid firs
  // then calculate new pos then integrate it in grid again

  for (int i = actualDirectionnalClusterWindCount - 1; i >= 0; --i)
  {
    removeClusterFromList(directionnalWindId[i]);
    for (int cascadeNo = 0; cascadeNo < clusterWindCascades.size(); ++cascadeNo)
    {
      clusterWindCascades[cascadeNo].removeClusterFromGrids(directionnalWindId[i]);
    }
  }

  dynamicWindMgr->update(pos, dt);
  actualDirectionnalClusterWindCount = 0;

  // recalculate position on grid
  for (int i = 0; i < dynamicWindMgr->waves.size(); ++i)
  {
    WindWave wave = dynamicWindMgr->waves[i];
    if (!wave.alive)
      continue;
    addClusterToGrid(wave.position, wave.radius, wave.strength, wave.direction, true);
  }
}

void ClusterWind::updateRenderer()
{
  TIME_PROFILE(updateRenderer);
  if (clusterWindRenderer)
  {
    clusterWindRenderer->updateClusterBuffer(clusterWinds.data(), min(MAX_CLUSTER, actualClusterWindsCount));
    clusterWindRenderer->updateClusterWindGridsBuffer(clusterWindCascades);
    clusterWindRenderer->updateRenderer();
  }
}

void ClusterWind::flipClusterWindSimArray() { clusterWindRenderer->updateCurrentRendererIndex(); }

ClusterWind::ClusterDesc ClusterWind::getClusterDescAt(int at) { return clusterWinds[at]; }

void ClusterWind::loadDynamicWindDesc(DynamicWind::WindDesc &_desc) { dynamicWindMgr->loadWindDesc(_desc); }

bool ClusterWind::isPriority(const ClusterDesc &value, const ClusterDesc &toCompare, int boxId, int cascadeNo)
{
  if (value.sphere.w < 0.0)
    return false;

  if (value.directionnalId >= 0 && toCompare.directionnalId < 0)
    return false;
  if (value.directionnalId < 0 && toCompare.directionnalId >= 0)
    return true;

  Point3 boxPos = clusterWindCascades[cascadeNo].getBoxPosition(boxId);

  float valuePowerFallof =
    safediv(length(Point3(value.sphere.x, value.sphere.y, value.sphere.z) - boxPos), value.sphere.w) * value.power;
  float toComparePowerFallof =
    (length(Point3(toCompare.sphere.x, toCompare.sphere.y, toCompare.sphere.z) - boxPos) / toCompare.sphere.w) * toCompare.power;
  if (valuePowerFallof > toComparePowerFallof)
    return true;
  if (value.sphere.w == toCompare.sphere.w && value.time <= toCompare.time && value.directionnalId < 0)
    return true;
  return false;
}

void ClusterWind::updateGridPosition(const Point3 &pos)
{
  TIME_PROFILE(updateGridPosition);
  for (int i = 0; i < clusterWindCascades.size(); ++i)
  {
    ToroidalQuadRegion result = clusterWindCascades[i].updateGridPosition(pos);

    Point3 positionCluster = clusterWindCascades[i].getActualPosition();
    positionCluster -= Point3(clusterWindCascades[i].size * 0.5f, 0, clusterWindCascades[i].size * 0.5f);
    // just updateOne, even for more region along the same axis.
    // potential of 3 region, but we already update the whole axis for one region
    int texelSize = 1; // once we use texture, use real texelsize
    BBox2 box(point2(result.texelsFrom) * texelSize, point2(result.texelsFrom + result.wd) * texelSize);
    float boxSize = clusterWindCascades[i].boxSize;
    IPoint2 boxNumToUpdate = IPoint2(result.wd.x / (int)boxSize, result.wd.y / (int)boxSize);
    if (boxNumToUpdate.x >= clusterWindCascades[i].boxWidthNum && boxNumToUpdate.y >= clusterWindCascades[i].boxWidthNum)
    {
      recalculateWholeCascade(pos, i);
      return;
    }
    ClusterWindCascade::ClusterWindGridUpdateResult updateResult = ClusterWindCascade::ClusterWindGridUpdateResult::NO_RESULT;
    if ((result.wd.y * texelSize) > ((int)clusterWindCascades[i].boxSize) && result.wd.y > result.wd.x)
    {
      updateResult = (result.texelsFrom.x * texelSize) > positionCluster.x
                       ? ClusterWindCascade::ClusterWindGridUpdateResult::X_POSITIV
                       : ClusterWindCascade::ClusterWindGridUpdateResult::X_NEGATIV;
    }
    else if ((result.wd.x * texelSize) > ((int)clusterWindCascades[i].boxSize))
    {
      updateResult = (result.texelsFrom.y * texelSize) > positionCluster.z
                       ? ClusterWindCascade::ClusterWindGridUpdateResult::Z_POSITIV
                       : ClusterWindCascade::ClusterWindGridUpdateResult::Z_NEGATIV;
    }
    updateAfterPositionChange(updateResult, i);
  }
}

float3 get_cluster_wind_at_pos(void *ctx, const float3 &p) { return ((ClusterWind *)ctx)->getBlastAtPosForParticle(p); }

static bool is_cluster_on_cascade(const BBox2 &box, const float3 &sph)
{
  Point2 pos = Point2(sph.x, sph.y);
  float r = sph.z;
  float dmin = 0;
  if (pos.x < box.getMin().x)
    dmin += sqr(pos.x - box.getMin().x);
  if (pos.x > box.getMax().x)
    dmin += sqr(pos.x - box.getMax().x);

  if (pos.y < box.getMin().y)
    dmin += sqr(pos.y - box.getMin().y);
  if (pos.y > box.getMax().y)
    dmin += sqr(pos.y - box.getMax().y);

  bool value = dmin <= sqr(r);
  return value;
}

static float unpack_value(uint value, float precision, float minValue, float maxValue)
{
  return minValue + (value / precision) * (maxValue - minValue);
}

static float maxHalfPrecision = (BYTE_2 - 1.0) * 0.5;
static float minHalfPrecision = -maxHalfPrecision;

static ClusterDescUncompressed uncompress_cluster_desc(ClusterDescGpu clusterDesc)
{
  ClusterDescUncompressed clusterUncompressed;
  clusterUncompressed.sphere.x =
    unpack_value(((uint)clusterDesc.sphere.x & 0xFFFF0000) >> SHIFT_2_BYTES, BYTE_2, minHalfPrecision, maxHalfPrecision);
  clusterUncompressed.sphere.y = unpack_value((uint)clusterDesc.sphere.x & 0x0000FFFF, BYTE_2, minHalfPrecision, maxHalfPrecision);
  clusterUncompressed.sphere.z =
    unpack_value(((uint)clusterDesc.sphere.y & 0xFFFF0000) >> SHIFT_2_BYTES, BYTE_2, minHalfPrecision, maxHalfPrecision);
  clusterUncompressed.sphere.w = unpack_value((uint)clusterDesc.sphere.y & 0x0000FFFF, BYTE_2, minHalfPrecision, maxHalfPrecision);
  clusterUncompressed.angle = unpack_value(((uint)clusterDesc.time & 0xFFFF0000) >> SHIFT_2_BYTES, BYTE_2, 0.0, 2.0 * PI);
  clusterUncompressed.time =
    unpack_value(((uint)clusterDesc.time & 0x0000FF00) >> SHIFT_1_BYTES, BYTE_1, 0.0, 1.0) * BOMBS_BLAST_ANIMATION_TIME;
  clusterUncompressed.directionnal = unpack_value((uint)clusterDesc.time & 0x000000FF, BYTE_1, -1, 1);
  clusterUncompressed.power = unpack_value((uint)clusterDesc.power & 0x0000FFFF, BYTE_2, 0.0, MAX_POWER_FOR_BLAST);
  return clusterUncompressed;
}

float3 ClusterWind::getBlastAtPosForParticle(const float3 &pos)
{
  float3 result(0.0f, 0.0f, 0.0f);
  for (int i = 0; i < clusterWindRenderer->getClusterCascadeDescNum(); ++i)
  {
    const ClusterCascadeDescGpu cascade = clusterWindRenderer->getClusterCascadeDescForCpuSim(i);
    float2 cascadePos;
    cascadePos.x = unpack_value(((uint)cascade.pos & 0xFFFF0000) >> SHIFT_2_BYTES, BYTE_2, minHalfPrecision, maxHalfPrecision);
    cascadePos.y = unpack_value((uint)cascade.pos & 0x0000FFFF, BYTE_2, minHalfPrecision, maxHalfPrecision);
    BBox2 box = BBox2(Point2(cascadePos.x, cascadePos.y), (float)cascade.gridSize);

    if (!is_cluster_on_cascade(box, float3(pos.x, pos.z, 1.0f)))
      continue;

    float2 gridToCluster = float2(cascadePos.x, cascadePos.y) - float2(pos.x, pos.z);
    float boxSize = (float)cascade.gridSize / cascade.boxWidthNum;
    int x = abs(gridToCluster.x / boxSize);
    x = x < 0 ? 0 : x;
    x = x >= cascade.boxWidthNum ? cascade.boxWidthNum - 1 : x;
    int y = abs(gridToCluster.y / boxSize);
    y = y < 0 ? 0 : y;
    y = y >= cascade.boxWidthNum ? cascade.boxWidthNum - 1 : y;
    int boxId = y * cascade.boxWidthNum + x;
    if (boxId > MAX_BOX_PER_GRID)
      continue;
    if (i > 0)
      for (int j = 0; j < i; ++j)
        boxId += sqr(clusterWindRenderer->getClusterCascadeDescForCpuSim(j).boxWidthNum);
    if (boxId < 0)
      continue;
    for (int j = 0; j < cascade.clusterPerBox; ++j)
    {
      uint4 boxIndexes = uint4(MAX_BOX_PER_GRID, MAX_BOX_PER_GRID, MAX_BOX_PER_GRID, MAX_BOX_PER_GRID);
      if (!clusterWindRenderer->getClusterGridsIdForCpuSim(boxId, boxIndexes))
        continue;
      unsigned int boxIndex = boxIndexes[boxId % 4];
      int clusterIndex = (boxIndex & (0x000000FF << (8 * j))) >> (8 * j);
      if (clusterIndex > MAX_CLUSTER)
        break;
      ClusterDescGpu clusterDescGpu;
      if (!clusterWindRenderer->getClusterDescForCpuSim(clusterIndex, clusterDescGpu))
        continue;
      ClusterDescUncompressed clusterDesc = uncompress_cluster_desc(clusterDescGpu);
      if (clusterDesc.directionnal >= 0)
        continue;

      float3 relPos = pos - float3(clusterDesc.sphere.x, clusterDesc.sphere.y, clusterDesc.sphere.z);
      float lenSq = lengthSq(relPos);
      float relRad = clusterDesc.time * BOMBS_BLAST_AIR_SPEED;
      if (lenSq >= sqr(relRad) || relRad > (clusterDesc.sphere.w + DISK_RADIUS))
        continue;

      float len = sqrtf(lenSq);
      float bandPower = sqr(1.f - len / relRad);

      float3 vec = len > FLT_MIN ? relPos / len : float3(0, 0, 0);
      result += vec * bandPower * clusterDesc.power * particlesForceMul;
    }
    return result;
  }
  return result;
}

Point3 ClusterWind::getBlastAtPosForParticle(const Point3 &pos) { return (Point3)getBlastAtPosForParticle((float3)pos); }

Point3 ClusterWind::getBlastAtPosForAntenna(const Point3 &pos)
{
  Point3 result(0.0f, 0.0f, 0.0f);
  for (int i = 0; i < clusterWindCascades.size(); ++i)
  {
    if (clusterWindCascades[i].isClusterOnCascade(pos, 1.0))
    {
      int boxId = clusterWindCascades[i].getClosestStaticBoxId(pos);
      float weight = 0.0f;
      for (int j = 0; j < clusterWindCascades[i].maxClusterPerBox; ++j)
      {
        int clusterId = clusterWindCascades[i].getClusterId(boxId, j);
        if (clusterId == ClusterWindCascade::NO_INDEX)
          break;
        Point3 clusterToParticle =
          pos - Point3(clusterWinds[clusterId].sphere.x, clusterWinds[clusterId].sphere.y, clusterWinds[clusterId].sphere.z);
        if (lengthSq(clusterToParticle) > sqr(clusterWinds[clusterId].sphere.w))
          continue;
        float clusterToParticleLength = length(clusterToParticle);
        float clusterToParticleLengthInv = clusterToParticleLength > FLT_MIN ? 1.f / clusterToParticleLength : 0;
        if (clusterWinds[clusterId].directionnalId < 0)
        {
          float toParticleAirSpeedTime = clusterToParticleLength * (1 / BOMBS_BLAST_AIR_SPEED);
          float blastDuration = clusterWinds[clusterId].time - toParticleAirSpeedTime;
          if (blastDuration > 0.0 && blastDuration < PARTICLE_BLAST_TIME)
          {
            float powerFallof = (1 - (clusterToParticleLength / clusterWinds[clusterId].sphere.w)) * clusterWinds[clusterId].power;
            float relativBlastTime = blastDuration / PARTICLE_BLAST_TIME;
            float power = sin(relativBlastTime * PI);
            weight += power; //[0,1]
            power *= powerFallof;
            result += normalize(clusterToParticle) * power;
          }
        }
        else
        {
          float powerFallof = max(
            (1 - (clusterToParticleLength / clusterWinds[clusterId].sphere.w)) * (clusterWinds[clusterId].power / MAX_POWER_FOR_BLAST),
            0.f);
          Point2 p0 =
            Point2(clusterWinds[clusterId].sphere.x, clusterWinds[clusterId].sphere.z) +
            Point2(clusterWinds[clusterId].direction.x, clusterWinds[clusterId].direction.z) * clusterWinds[clusterId].sphere.w;
          Point2 p1 =
            Point2(clusterWinds[clusterId].sphere.x, clusterWinds[clusterId].sphere.z) -
            Point2(clusterWinds[clusterId].direction.x, clusterWinds[clusterId].direction.z) * clusterWinds[clusterId].sphere.w;
          Point2 ldir = p1 - p0;
          Point2 toProjetcPoint = Point2(pos.x, pos.z) - p0;
          float projectedLength = dot(toProjetcPoint, ldir) / sqr(clusterWinds[clusterId].sphere.w * 2.0);
          float power = sin(projectedLength * PI);
          weight += power; //[0,1]
          power *= powerFallof;
          result += -Point3(ldir.x, 0.0f, ldir.y) * clusterToParticleLengthInv * power;
        }
      }
      return result / max(weight, 1.0f);
    }
  }
  return result;
}

Tab<WindSpawner> ClusterWind::getDynamicWindGenerator() { return dynamicWindMgr->spawner; }

void ClusterWind::initDynamicWind(int beaufortScale, const Point3 &direction) { dynamicWindMgr->init(beaufortScale, direction); }

void ClusterWind::enableDynamicWind(bool isEnabled) { dynamicWindMgr->enableDynamicWind(isEnabled); }
