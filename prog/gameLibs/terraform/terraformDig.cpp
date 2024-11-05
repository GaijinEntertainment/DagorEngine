// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <terraform/terraformDig.h>
#include <terraform/terraform.h>
#include <math/dag_math3d.h>
#include <math/dag_mathUtils.h>
#include <math/dag_color.h>
#include <math/random/dag_random.h>
#include <render/debug3dSolid.h>
#include <startup/dag_inpDevClsDrv.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <memory/dag_framemem.h>
#include <time.h>
#include <perfMon/dag_statDrv.h>

static const float ALT_EPS = 0.001f;
static const int PATCH_BLUR_RADIUS = 5;

#if DAGOR_DBGLEVEL > 0
#define DIG_TRACK 0
#endif

struct TerraformDigCtx
{
  Point2 pos;
  Point2 dir;
  float width;
  float dAlt;

  float diffAlt;
  IBBox2 worldBox;
  float primLen;
  float primH;
  uint16_t primId;
  bool changed;

  bool isPrimValid() const { return primId > 0; }

#if DIG_TRACK
  Tab<Point2> track;
  Point2 trackDir;
#endif
};


static bool is_keyboard_btn_down(int key)
{
  if (global_cls_drv_kbd && global_cls_drv_kbd->getDeviceCount())
  {
    const HumanInput::KeyboardRawState &state = global_cls_drv_kbd->getDevice(0)->getRawState();
    return state.isKeyDown(key);
  }
  return false;
}


TerraformDig::TerraformDig(Terraform &in_tform, const Desc &in_desc) :
  tform(in_tform), desc(in_desc), BLUR_RADIUS(max(in_tform.getPrimSpreadRadius() + 2, PATCH_BLUR_RADIUS * 2))
{}

TerraformDig::~TerraformDig()
{
  G_ASSERT(digContexts.empty());
  for (TerraformDigCtx *digCtx : digContexts)
    del_it(digCtx);
}

TerraformDigCtx *TerraformDig::startDigging(const Point2 &pos, const Point2 &dir, float width, float d_alt)
{
  TerraformDigCtx *digCtx = new TerraformDigCtx();
  digCtx->pos = pos;
  digCtx->dir = dir;
  normalizeDef(digCtx->dir, Point2(1, 0));
  digCtx->width = width;
  digCtx->diffAlt = 0;
  digCtx->worldBox = IBBox2();
  digCtx->primLen = 0;
  digCtx->primH = 0;
#if DIG_TRACK
  digCtx->trackDir = Point2(1, 0);
#endif
  digCtx->primId = 0;
  digCtx->dAlt = max(d_alt, tform.getMinDAlt());
  digCtx->changed = false;

  digContexts.push_back(digCtx);
  return digCtx;
}

void TerraformDig::stopDigging(TerraformDigCtx *dig_ctx)
{
  G_ASSERT_RETURN(dig_ctx, );
  storePrim(dig_ctx);
  erase_item_by_value(digContexts, dig_ctx);
  del_it(dig_ctx);
}

void TerraformDig::advanceDigging(TerraformDigCtx *dig_ctx, const Point2 &pos, const Point2 &dir)
{
  G_ASSERT_RETURN(dig_ctx, );
  TerraformDigCtx &digCtx = *dig_ctx;

  Point2 trackDir = pos - digCtx.pos;
  float trackLenSq = lengthSq(trackDir);
  if (trackLenSq < ALT_EPS)
    return;

  float trackLen = sqrtf(trackLenSq);
  trackDir /= trackLen;
  Point2 cellVec(tform.getWorldSize().x / tform.getResolution().x, tform.getWorldSize().y / tform.getResolution().y);
  float cellLen = cellVec.length();
  Point2 up = Point2(-digCtx.dir.y, digCtx.dir.x);

  if (trackLen > cellLen)
  {
    if (trackDir * digCtx.dir < 0.0f && digCtx.isPrimValid())
    {
      storePrim(dig_ctx);
      return;
    }
    Point2 prevPos = digCtx.pos;
    digCtx.pos = pos;
    digCtx.dir = dir;
    normalizeDef(digCtx.dir, Point2(1, 0));

    if (trackDir * digCtx.dir > 0)
    {
#if DIG_TRACK
      if (digCtx.track.empty())
      {
        digCtx.track.push_back(prevPos);
        digCtx.track.push_back(pos);
        digCtx.trackDir = trackDir;
      }
      Point2 curTrackDir = pos - digCtx.track[digCtx.track.size() - 2];
      if (fabsf(curTrackDir * Point2(-digCtx.trackDir.y, digCtx.trackDir.x)) < 0.1f)
      {
        digCtx.track.back() = pos;
      }
      else
      {
        digCtx.track.push_back(pos);
        digCtx.trackDir = trackDir;
      }
#endif

      Point2 corners[4] = {Point2(-cellLen, -digCtx.width * 0.5f), Point2(trackLen, -digCtx.width * 0.5f),
        Point2(trackLen, digCtx.width * 0.5f), Point2(-cellLen, digCtx.width * 0.5f)};
      IPoint2 points[4];
      for (int pointNo = 0; pointNo < countof(points); ++pointNo)
      {
        points[pointNo] = tform.getCoordFromPos(prevPos + corners[pointNo].x * digCtx.dir + corners[pointNo].y * up);
        Pcd pcd = tform.makePcdFromCoord(points[pointNo]);
        if (pcd.patchNo >= tform.getPatchNum())
        {
          storePrim(dig_ctx);
          return;
        }
      }

      struct Context
      {
        Context(TerraformDig &in_tdig, Terraform &in_tform, TerraformDigCtx &in_dig_ctx) :
          tdig(in_tdig), tform(in_tform), digCtx(in_dig_ctx)
        {}

        TerraformDig &tdig;
        Terraform &tform;
        TerraformDigCtx &digCtx;
        float digAlt = 0;
        bool digEnabled = false;
      } ctx(*this, tform, digCtx);

      ctx.digEnabled = (digCtx.primLen < digCtx.width * desc.heapAspect || digCtx.diffAlt < tform.getDesc().getMaxLevel());

      struct DigCell
      {
        DigCell(const Context &in_ctx) : ctx(in_ctx) { maxDigAlt = ctx.tform.getDesc().getMinLevel(); }

        __forceinline void operator()(int x, int y) const
        {
          Pcd pcd = ctx.tform.makePcdFromCoord(IPoint2(x, y));
          float curAlt = ctx.tform.getPatchAltUnpacked(pcd);
          float digAlt = max(ctx.tform.getDesc().getMinDigLevel(), curAlt - ctx.digCtx.dAlt);
          maxDigAlt = max(maxDigAlt, curAlt - (ctx.digEnabled ? ctx.digCtx.dAlt : ctx.tform.getMinDAlt()));
          sumDigAlt += digAlt;
          ++numDigAlt;
        }

        const Context &ctx;
        mutable float sumDigAlt = 0;
        mutable int numDigAlt = 0;
        mutable float maxDigAlt = 0;
      } digCell(ctx);
      IPoint2 digCoords[2] = {tform.getCoordFromPos(digCtx.pos + cellLen * 2 * digCtx.dir + digCtx.width * 0.5 * up),
        tform.getCoordFromPos(digCtx.pos + cellLen * 2 * digCtx.dir - digCtx.width * 0.5 * up)};
      rastr_line(digCoords[0].x, digCoords[0].y, digCoords[1].x, digCoords[1].y, digCell);
      ctx.digAlt = ctx.digEnabled ? (digCell.sumDigAlt / max(digCell.numDigAlt, 1) + digCell.maxDigAlt) * 0.5f
                                  : max(digCell.maxDigAlt, -ctx.tform.getMinDAlt());

      struct ConsumeCell
      {
        ConsumeCell(Context &in_ctx) : ctx(in_ctx) {}

        __forceinline void operator()(int x, int y) const
        {
          Pcd pcd = ctx.tform.makePcdFromCoord(IPoint2(x, y));
          float alt = ctx.tform.getPatchAltUnpacked(pcd);
          float diffAlt = max(alt - ctx.digAlt, 0.0f);
          float newAlt = alt - diffAlt;
          if (diffAlt > 0)
            ctx.tdig.doStorePatchAlt(pcd, ctx.tform.packAlt(newAlt));
          if (ctx.digEnabled)
            ctx.digCtx.diffAlt += diffAlt * ctx.tdig.desc.consumeWeight;
        }
        Context &ctx;
      } consumeCell(ctx);
      rastr_tri(points[0], points[1], points[2], consumeCell);
      rastr_tri(points[0], points[2], points[3], consumeCell);
    }
  }

  float chunk =
    max(safediv(tform.getWorldSize().x, tform.getResolution().x), safediv(tform.getWorldSize().y, tform.getResolution().y));
  float densAlt = digCtx.diffAlt * chunk * chunk / max(digCtx.width, 1.0f);
  if (densAlt > ALT_EPS)
  {
    digCtx.primH = min(sqrt(densAlt), desc.maxPrimHeight);
    digCtx.primLen = densAlt / max(digCtx.primH, ALT_EPS);
    Point2 digCorners[4] = {Point2(0, -digCtx.width * 0.5f), Point2(digCtx.primLen, -digCtx.width * 0.5f),
      Point2(digCtx.primLen, digCtx.width * 0.5f), Point2(0, digCtx.width * 0.5f)};

    IPoint2 digMin = IPoint2(tform.getResolution().x, tform.getResolution().y);
    IPoint2 digMax = IPoint2(0, 0);
    IPoint2 digPoints[4];
    for (int pointNo = 0; pointNo < countof(digPoints); ++pointNo)
    {
      IPoint2 coord = tform.getCoordFromPos(pos + digCorners[pointNo].x * digCtx.dir + digCorners[pointNo].y * up);
      digPoints[pointNo] = coord;
      digMin.x = min(digMin.x, coord.x);
      digMin.y = min(digMin.y, coord.y);
      digMax.x = max(digMax.x, coord.x);
      digMax.y = max(digMax.y, coord.y);
    }

    digCtx.worldBox = IBBox2(digMin, digMax);

    Terraform::QuadData quad;
    quad.vertices[0] = digPoints[0];
    quad.vertices[1] = digPoints[1];
    quad.vertices[2] = digPoints[2];
    quad.vertices[3] = digPoints[3];
    quad.diffAlt = digCtx.diffAlt;
    digCtx.changed |= doSubmitQuad(quad, digCtx.primId);
  }
}

Point3 TerraformDig::getDiggingPrimSize(TerraformDigCtx *dig_ctx) const
{
  G_ASSERT_RETURN(dig_ctx, Point3(0, 0, 0));
  TerraformDigCtx &digCtx = *dig_ctx;
  if (!digCtx.isPrimValid())
    return Point3(0, 0, 0);
  return Point3(digCtx.primLen, digCtx.primH, digCtx.width);
}

float TerraformDig::getDiggingMaxAlt(TerraformDigCtx *dig_ctx) const
{
  G_ASSERT_RETURN(dig_ctx, 0);
  TerraformDigCtx &digCtx = *dig_ctx;
  if (!digCtx.isPrimValid())
    return 0;
  return tform.getPrimMaxAlt(digCtx.primId);
}

void TerraformDig::getDiggingMass(TerraformDigCtx *dig_ctx, float &out_total_mass, float &out_left_mass) const
{
  out_total_mass = 0;
  out_left_mass = 0;
  G_ASSERT_RETURN(dig_ctx, );
  TerraformDigCtx &digCtx = *dig_ctx;
  if (!digCtx.isPrimValid())
    return;
  out_total_mass = digCtx.diffAlt;
  out_left_mass = tform.getPrimLeftAlt(digCtx.primId);
}

BBox2 TerraformDig::getDigRegion(TerraformDigCtx *dig_ctx) const
{
  G_ASSERT_RETURN(dig_ctx, BBox2());
  const TerraformDigCtx &digCtx = *dig_ctx;
  BBox2 box = BBox2(tform.getPosFromCoord(digCtx.worldBox[0]), tform.getPosFromCoord(digCtx.worldBox[1]));
  box.inflate(BLUR_RADIUS * max(tform.getWorldSize().x / tform.getResolution().x, tform.getWorldSize().y / tform.getResolution().y));
  return box;
}

dag::ConstSpan<TerraformDigCtx *> TerraformDig::getDigContexts() const { return digContexts; }

void TerraformDig::getBBChanges(Tab<BBox2> &out_boxes) const
{
  dag::ConstSpan<int> patchChanges = tform.getPatchChanges();
  for (int patchNo : patchChanges)
  {
    IBBox2 boxChanges = tform.getPatchBBChanges(patchNo);
    if (boxChanges.isEmpty())
      continue;
    BBox2 wbox = BBox2(tform.getPosFromCoord(boxChanges.lim[0]), tform.getPosFromCoord(boxChanges.lim[1]));
    wbox.inflate(
      BLUR_RADIUS * max(tform.getWorldSize().x / tform.getResolution().x, tform.getWorldSize().y / tform.getResolution().y));
    bool found = false;
    for (BBox2 &box : out_boxes)
      if (box & wbox)
      {
        box += wbox;
        found = true;
        break;
      }
    if (!found)
      out_boxes.push_back(wbox);
  }

  for (TerraformDigCtx *digCtx : digContexts)
  {
    if (!digCtx->changed)
      continue;
    BBox2 boxDig = getDigRegion(digCtx);
    if (boxDig.isempty())
      continue;
    bool found = false;
    for (BBox2 &box : out_boxes)
      if (box & boxDig)
      {
        box += boxDig;
        found = true;
        break;
      }
    if (!found)
      out_boxes.push_back(boxDig);
  }
}

void TerraformDig::clearBBChanges()
{
  tform.clearPatchChanges();
  for (TerraformDigCtx *digCtx : digContexts)
    digCtx->changed = false;
}

void TerraformDig::makeBombCrater(const Point2 &pos, float inner_radius, float inner_depth, float outer_radius, float outer_alt)
{
  makeBombCraterPart(pos, inner_radius, inner_depth, outer_radius, outer_alt, pos, outer_radius);
}

void TerraformDig::makeBombCraterPart(const Point2 &pos, float inner_radius, float inner_depth, float outer_radius, float outer_alt,
  const Point2 &part_pos, float part_radius)
{
  TIME_PROFILE(TerraformDig__makeBombCraterPart);

  G_ASSERT(outer_radius >= inner_radius);
  IPoint2 coordStart = tform.getCoordFromPos(Point2(part_pos.x - part_radius, part_pos.y - part_radius));
  IPoint2 coordEnd = tform.getCoordFromPos(Point2(part_pos.x + part_radius, part_pos.y + part_radius));
  float outerRadSq = sqr(outer_radius);
  float innerRadSq = sqr(inner_radius);
  float midRadSq = sqr(inner_radius + (outer_radius - inner_radius) * 0.25f);

  for (int x = coordStart.x; x < coordEnd.x; ++x)
    for (int y = coordStart.y; y < coordEnd.y; ++y)
    {
      IPoint2 coord(x, y);
      Point2 cellPos = tform.getPosFromCoord(coord);
      float distSq = lengthSq(pos - cellPos);
      if (distSq > outerRadSq)
        continue;
      Terraform::Pcd pcd = tform.makePcdFromCoord(coord);
      if (!tform.isValidPcd(pcd))
        continue;
      bool isZeroOrHighAlt = tform.getPatchAlt(pcd) >= tform.getZeroAlt();
      float alt = distSq > innerRadSq ? (distSq < midRadSq ? outer_alt * (1.f - (midRadSq - distSq) / (midRadSq - innerRadSq))
                                                           : outer_alt * (1.f - (distSq - midRadSq) / (outerRadSq - midRadSq)))
                                      : inner_depth * (1.f - distSq / innerRadSq);
      Terraform::PrimMode mode = distSq > innerRadSq && isZeroOrHighAlt ? Terraform::DYN_MAX : Terraform::DYN_MIN;
      doStorePatchAlt(pcd, tform.getAltModePacked(pcd, alt, mode));
    }
}

void TerraformDig::makeAreaPlate(const TMatrix &area_tm, float smoothing_area_width, float delta, float max_noise)
{
  TMatrix itm = inverse(area_tm);
  float scaleTm = max(area_tm.getcol(0).length(), area_tm.getcol(2).length());
  BBox3 bbox(area_tm.getcol(3), 0.f);
  for (int i = 0; i < 4; ++i)
    bbox += area_tm.getcol(3) + area_tm.getcol(i % 2 ? 0 : 2) * (i > 1 ? -1.f : 1.f);
  IPoint2 coordStart = tform.getCoordFromPos(Point2::xz(bbox.lim[0]));
  IPoint2 coordEnd = tform.getCoordFromPos(Point2::xz(bbox.lim[1]));
  float level = tform.sampleHmapHeightOrig(tform.getCoordFromPos(Point2::xz(area_tm.getcol(3)))) + delta;
  BBox3 bboxLocal(Point3(-.5f, -.5f, -.5f), Point3(.5f, .5f, .5f));
  float smSize = 0.5f - smoothing_area_width / scaleTm;
  BBox3 bboxLocalSmooth(Point3(-smSize, -smSize, -smSize), Point3(smSize, smSize, smSize));
  int initialSeed = grnd();
  for (int x = coordStart.x; x < coordEnd.x; ++x)
    for (int y = coordStart.y; y < coordEnd.y; ++y)
    {
      IPoint2 coord(x, y);
      Point2 cellPos = tform.getPosFromCoord(coord);
      Point3 posLocal = itm * Point3::xVy(cellPos, area_tm.getcol(3).y);
      if (!(bboxLocal & posLocal))
        continue;
      bool atSmoothArea = false;
      float dist = 0.f;
      if (!(bboxLocalSmooth & posLocal))
      {
        atSmoothArea = true;
        dist = sqrt(point_to_box_distance_sq(posLocal, bboxLocalSmooth)) * scaleTm;
      }
      Terraform::Pcd pcd = tform.makePcdFromCoord(coord);
      if (!tform.isValidPcd(pcd))
        continue;
      float hmapPos = tform.sampleHmapHeightOrig(coord);
      int seed = initialSeed + (x >> 2) * 2048 + (y >> 2); // pseudo random seed for noise by 4x4 cells
      float dAlt = level - hmapPos;
      float alt = atSmoothArea ? cvt(dist / smoothing_area_width, 1.f, .0f, tform.getPatchAltUnpacked(pcd), dAlt) : dAlt;
      alt += delta * dagor_random::_rnd_float(seed, -max_noise, max_noise);
      doStorePatchAlt(pcd, tform.getAltModePacked(pcd, alt, Terraform::DYN_REPLACE));
    }
}

void TerraformDig::makeAreaCylinder(const TMatrix &area_tm, float smoothing_area_width, float delta, float max_noise)
{
  Point2 center = Point2::xz(area_tm.getcol(3));
  float targetLevel = tform.sampleHmapHeightOrig(tform.getCoordFromPos(center)) + delta;
  float radius = area_tm.getcol(0).length();
  float radiusSmooth = radius - smoothing_area_width;
  IPoint2 coordStart = tform.getCoordFromPos(Point2(center.x - radius, center.y - radius));
  IPoint2 coordEnd = tform.getCoordFromPos(Point2(center.x + radius, center.y + radius));
  float radSq = sqr(radius);
  float radSmoothSq = sqr(radiusSmooth);
  int initialSeed = grnd();

  IPoint2 coord(0, 0);
  for (coord.x = coordStart.x; coord.x <= coordEnd.x; ++coord.x)
    for (coord.y = coordStart.y; coord.y <= coordEnd.y; ++coord.y)
    {
      Point2 cellPos = tform.getPosFromCoord(coord);
      float distSq = lengthSq(center - cellPos);
      if (distSq > radSq)
        continue;
      Terraform::Pcd pcd = tform.makePcdFromCoord(coord);
      if (!tform.isValidPcd(pcd))
        continue;
      float origLevel = tform.sampleHmapHeightOrig(coord);
      float levelDiff = targetLevel - origLevel;
      if (distSq > radSmoothSq)
      {
        float smoothDist = sqrt(distSq) - radiusSmooth;
        levelDiff = cvt(smoothDist / smoothing_area_width, 1.f, .0f, tform.getPatchAltUnpacked(pcd), levelDiff);
      }
      int seed = initialSeed + (coord.x >> 2) * 2048 + (coord.y >> 2); // pseudo random seed for noise by 4x4 cells
      levelDiff += delta * dagor_random::_rnd_float(seed, -max_noise, max_noise);
      doStorePatchAlt(pcd, tform.getAltModePacked(pcd, levelDiff, Terraform::DYN_REPLACE));
    }
}

void TerraformDig::drawDebug()
{
  const float MAIN_Z = 1.0f;

#if DIG_TRACK
  Tab<Point3> vertices(framemem_ptr());
  Tab<uint16_t> indices(framemem_ptr());
#endif

  for (TerraformDigCtx *digCtx : getDigContexts())
  {
#if DIG_TRACK
    if (digCtx->track.size() >= 2)
      for (int trackNo = 0; trackNo < digCtx->track.size(); ++trackNo)
      {
        const Point2 &trackPoint = digCtx->track[trackNo];
        Point2 avgDir;
        Point2 curDir;
        if (trackNo == 0)
        {
          curDir = digCtx->track[trackNo + 1] - trackPoint;
          normalizeDef(curDir, Point2(1, 0));
          avgDir = curDir;
        }
        else if (trackNo == digCtx->track.size() - 1)
        {
          curDir = trackPoint - digCtx->track[trackNo - 1];
          normalizeDef(curDir, Point2(1, 0));
          avgDir = curDir;
        }
        else
        {
          const Point2 &prevTrack = digCtx->track[trackNo - 1];
          const Point2 &nextTrack = digCtx->track[trackNo + 1];
          curDir = trackPoint - prevTrack;
          normalizeDef(curDir, Point2(1, 0));
          avgDir = normalize(curDir) + normalize(nextTrack - trackPoint);
          normalizeDef(avgDir, Point2(1, 0));
        }
        Point2 curNormal = Point2(-curDir.y, curDir.x);
        Point2 avgNormal = Point2(-avgDir.y, avgDir.x);
        float avgScale = 1.0f / max(curNormal * avgNormal, ALT_EPS);

        float ht = tform.sampleHmapHeightOrig(tform.getCoordFromPosF(trackPoint)) + MAIN_Z;
        vertices.push_back(Point3::xVy(trackPoint - avgNormal * digCtx->width * 0.5f * avgScale, ht));
        vertices.push_back(Point3::xVy(trackPoint + avgNormal * digCtx->width * 0.5f * avgScale, ht));

        if (trackNo >= 1)
        {
          indices.push_back((trackNo - 1) * 2 + 0);
          indices.push_back((trackNo - 1) * 2 + 2);
          indices.push_back((trackNo - 1) * 2 + 3);
          indices.push_back((trackNo - 1) * 2 + 0);
          indices.push_back((trackNo - 1) * 2 + 3);
          indices.push_back((trackNo - 1) * 2 + 1);
        }
      }
#endif

    float ht = tform.sampleHmapHeightOrig(tform.getCoordFromPosF(digCtx->pos)) + MAIN_Z;
    draw_debug_solid_sphere(Point3::xVy(digCtx->pos, ht), digCtx->width * 0.5f, TMatrix::IDENT, Color4(0, 0, 1, 0.5f));
  }

#if DIG_TRACK
  if (indices.size() > 0)
    draw_debug_solid_mesh(indices.data(), indices.size() / 3, (const float *)vertices.data(), sizeof(Point3), vertices.size(),
      TMatrix::IDENT, Color4(0, 1, 0, 0.5f));
#endif
}

void TerraformDig::testDebug(const BBox3 &box, const TMatrix &tm)
{
  static TerraformDigCtx *myDigCtx = NULL;
  if (is_keyboard_btn_down(HumanInput::DKEY_R) || is_keyboard_btn_down(HumanInput::DKEY_T) || is_keyboard_btn_down(HumanInput::DKEY_F))
  {
    float yLevel = is_keyboard_btn_down(HumanInput::DKEY_F) ? 0.0f : is_keyboard_btn_down(HumanInput::DKEY_T) ? 0.3f : 0.2f;
    Point2 dir = Point2::xz(tm.getcol(0));
    Point2 targetPos = Point2::xz(tm.getcol(3)) + dir * (box.lim[1].x - 0.5f);
    if (!myDigCtx)
      myDigCtx = startDigging(targetPos, dir, box.width().z + 1.0f, yLevel);
    advanceDigging(myDigCtx, targetPos, dir);
  }
  else if (myDigCtx)
  {
    stopDigging(myDigCtx);
    myDigCtx = NULL;
  }
}

void TerraformDig::doStorePatchAlt(const Pcd &pcd, uint8_t alt) { tform.storePatchAlt(pcd, alt); }

bool TerraformDig::doSubmitQuad(const Terraform::QuadData &quad, uint16_t &prim_id) { return tform.submitQuad(quad, prim_id); }

void TerraformDig::doRemovePrim(uint16_t prim_id) { tform.removePrim(prim_id); }

void TerraformDig::storePrim(TerraformDigCtx *dig_ctx)
{
  if (!dig_ctx->isPrimValid())
    return;

  Terraform::alt_hash_map_t altMap(dag::MemPtrAllocator{framemem_ptr()});
  tform.getPrimData(dig_ctx->primId, altMap);
  for (const auto &pcdAlt : altMap)
    doStorePatchAlt(pcdAlt.first, tform.packAlt(pcdAlt.second));
  doRemovePrim(dig_ctx->primId);
  dig_ctx->primId = 0;
  dig_ctx->diffAlt = 0;
  dig_ctx->worldBox = IBBox2();
  dig_ctx->primLen = 0;
#if DIG_TRACK
  clear_and_shrink(dig_ctx->track);
  dig_ctx->trackDir = Point2(1, 0);
#endif
}