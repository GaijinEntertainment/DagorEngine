// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <windows.h>
#include "hmlSplineObject.h"
#include "hmlSplinePoint.h"
#include "hmlObjectsEditor.h"
#include "crossRoad.h"
#include "roadBuilderIface.h"
#include "materialMgr.h"
#include "roadUtil.h"
#include <de3_splineClassData.h>
#include <de3_lightService.h>
#include <math/dag_mathAng.h>
#include <math/dag_quatInterp.h>
#include <math/dag_math2d.h>
#include <perfMon/dag_cpuFreq.h>
#include <EditorCore/ec_IEditorCore.h>

using editorcore_extapi::dagGeom;
using editorcore_extapi::make_full_start_path;

using namespace roadbuildertool;

typedef RoadBuilder *(*get_new_road_builder_t)(void);
typedef void (*delete_road_builder_t)(RoadBuilder *);

static HMODULE hDll = NULL;
static get_new_road_builder_t get_new_road_builder = NULL;
static delete_road_builder_t delete_road_builder = NULL;
static RoadBuilder *road_builder = NULL;
static bool bad_road_builder = false;

#define TRANSIENT_PART 0.1

static bool loadRoadBuilderDll()
{
  if (hDll)
    return true;

  String fn = ::make_full_start_path("roadBuilder.dll");

  hDll = LoadLibrary(fn);
  if (!hDll)
  {
    DAEDITOR3.conError("Can't load road builder: %s", (char *)fn);
    return false;
  }
  get_new_road_builder = (get_new_road_builder_t)GetProcAddress(hDll, "get_new_road_builder");
  delete_road_builder = (delete_road_builder_t)GetProcAddress(hDll, "delete_road_builder");
  if (!get_new_road_builder || !delete_road_builder)
  {
    DAEDITOR3.conError("Can't init road builder");
    get_new_road_builder = NULL;
    delete_road_builder = NULL;
    FreeLibrary(hDll);
    hDll = NULL;
    return false;
  }

  bad_road_builder = false;
  return true;
}
static bool prepareRoadBuilder()
{
  if (bad_road_builder)
    return false;

  if (!road_builder)
  {
    road_builder = get_new_road_builder();
    if (!road_builder)
    {
      bad_road_builder = true;
      return false;
    }
    if (!road_builder->checkVersion())
    {
      DAEDITOR3.conError("Road builder version mismatch: has=%p needed=%p", road_builder->getVersion(), road_builder->VERSION);
      road_builder = NULL;
      bad_road_builder = true;
      return false;
    }
    road_builder->init();
  }
  return true;
}

void HmapLandObjectEditor::unloadRoadBuilderDll()
{
  if (!hDll)
    return;

  // if ( road_builder )
  //   delete_road_builder ( road_builder );

  road_builder = NULL;
  get_new_road_builder = NULL;
  delete_road_builder = NULL;
  FreeLibrary(hDll);
  hDll = NULL;
}

static void addGeometryNode(StaticGeometryContainer &geom, MaterialDataList *mat, Geometry &geom_out, const char *node_name)
{
  StaticGeometryContainer *g = dagGeom->newStaticGeometryContainer();

  Mesh *n_mesh = dagGeom->newMesh(), *n_wlMesh = dagGeom->newMesh();
  MeshData &mesh = n_mesh->getMeshData(), &wlMesh = n_wlMesh->getMeshData();

  mesh.vert = make_span_const(geom_out.vertices, geom_out.vertCount);
  mesh.tvert[0] = make_span_const(geom_out.textureCoordinates, geom_out.tcCount);
  mesh.vertnorm = make_span_const(geom_out.normals, geom_out.normalCount);

  mesh.face.resize(geom_out.faceCount);
  mesh.tface[0].resize(geom_out.faceCount);
  mesh.facengr.resize(geom_out.faceCount);

  wlMesh.vert = make_span_const(geom_out.vertices, geom_out.vertCount);
  wlMesh.tvert[0] = make_span_const(geom_out.textureCoordinates, geom_out.tcCount);
  wlMesh.vertnorm = make_span_const(geom_out.normals, geom_out.normalCount);

  wlMesh.face.resize(geom_out.faceCount);
  wlMesh.tface[0].resize(geom_out.faceCount);
  wlMesh.facengr.resize(geom_out.faceCount);

  int fc = 0, wlfc = 0;
  bool has_def_mat = false;

  for (int fi = 0; fi < geom_out.faceCount; fi++)
  {
    const roadbuildertool::FaceIndices &f = geom_out.geomFaces[fi];

    if (f.v[0] == f.v[1] || f.v[0] == f.v[2] || f.v[1] == f.v[2])
      continue;

    MeshData *m;
    int i;

    if (geom_out.details[fi].type == roadbuildertool::GeometryFaceDetails::TYPE_WHITELINES)
    {
      m = &wlMesh;
      i = wlfc++;
    }
    else
    {
      m = &mesh;
      i = fc++;
    }

    memcpy(m->face[i].v, f.v, sizeof(int) * 3);
    m->face[i].smgr = geom_out.details[fi].smoothingGroup;

    if (m->face[i].smgr == 0)
      DAEDITOR3.conError("face %d, smoothing groop = 0, material: %s, class: %s", i,
        (char *)mat->getSubMat(geom_out.details[fi].materialId + 1)->matName,
        (char *)mat->getSubMat(geom_out.details[fi].materialId + 1)->className);

    if (geom_out.details[fi].materialId == -1)
      has_def_mat = true;
    m->face[i].mat = geom_out.details[fi].materialId + 1;

    memcpy(&m->facengr[i][0], geom_out.normalIndices[fi].v, sizeof(int) * 3);
    memcpy(m->tface[0][i].t, geom_out.textureFaces[fi].v, sizeof(int) * 3);
  }
  if (has_def_mat && !mat->list[0].get())
  {
    ObjLibMaterialListMgr::setDefaultMat0(*mat);
    DAEDITOR3.conError("roadGeom generated with default(bad) material");
  }

  mesh.facengr.resize(fc);
  mesh.tface[0].resize(fc);
  mesh.face.resize(fc);

  mesh.facengr.shrink_to_fit();
  mesh.tface[0].shrink_to_fit();
  mesh.face.shrink_to_fit();

  wlMesh.facengr.resize(wlfc);
  wlMesh.tface[0].resize(wlfc);
  wlMesh.face.resize(wlfc);

  wlMesh.facengr.shrink_to_fit();
  wlMesh.tface[0].shrink_to_fit();
  wlMesh.face.shrink_to_fit();

  mesh.calc_facenorms();
  wlMesh.calc_facenorms();

  ObjLibMaterialListMgr::complementDefaultMat0(*mat);
  dagGeom->objCreator3dAddNode(node_name, n_mesh, mat, *g);
  ObjLibMaterialListMgr::reclearDefaultMat0(*mat);

  for (int i = 0; i < g->nodes.size(); ++i)
  {
    StaticGeometryNode *node = g->nodes[i];

    node->flags |= StaticGeometryNode::FLG_CASTSHADOWS | StaticGeometryNode::FLG_CASTSHADOWS_ON_SELF |
                   StaticGeometryNode::FLG_COLLIDABLE | StaticGeometryNode::FLG_NO_RECOMPUTE_NORMALS;

    dagGeom->staticGeometryNodeCalcBoundBox(*node);
    dagGeom->staticGeometryNodeCalcBoundSphere(*node);

    geom.addNode(dagGeom->newStaticGeometryNode(*node, tmpmem));
  }

  dagGeom->staticGeometryContainerClear(*g);

  dagGeom->objCreator3dAddNode(String(node_name) + "_whiteline", n_wlMesh, mat, *g);

  for (int i = 0; i < g->nodes.size(); ++i)
  {
    StaticGeometryNode *node = g->nodes[i];
    node->flags = StaticGeometryNode::FLG_RENDERABLE | StaticGeometryNode::FLG_NO_RECOMPUTE_NORMALS;

    dagGeom->staticGeometryNodeCalcBoundBox(*node);
    dagGeom->staticGeometryNodeCalcBoundSphere(*node);

    geom.addNode(dagGeom->newStaticGeometryNode(*node, tmpmem));
  }

  dagGeom->deleteStaticGeometryContainer(g);
}

static int autoRoadNodeIdx = 0;
static int autoCrossNodeIdx = 0;
static void buildSegments(Geometry &geom_out, StaticGeometryContainer &geom, MaterialDataList *mat,
  dag::Span<LinkProperties> build_links, dag::ConstSpan<LampProperties> lamps, dag::ConstSpan<PointProperties> build_points,
  dag::ConstSpan<DecalProperties> decals)
{
  for (int i = 0; i < build_links.size(); i++)
    build_links[i].lamps = &lamps[(unsigned)(uintptr_t)build_links[i].lamps];

  Roads roads;

  roads.links = build_links.data();
  roads.linksCount = build_links.size();
  roads.points = build_points.data();
  roads.pointsCount = build_points.size();
  roads.decals = decals.data();
  roads.decalsCount = decals.size();
  roads.boundLinks = NULL;
  roads.boundLinksCount = 0;
  roads.transitionPart = TRANSIENT_PART;

  // build roads
  // debug("roads: points = %d; links = %d", roads.pointsCount, roads.linksCount);

  // build roads
  memset(&geom_out, 0, sizeof(geom_out));
  bool ret = road_builder->build(roads, geom_out);
  /*debug("geom_out/roads: ret=%d faceCount=%d, vertCount=%d, tcCount=%d, normalCount=%d\n"
        "                lamps=%p lampsCount=%d", ret, geom_out.faceCount, geom_out.vertCount,
        geom_out.tcCount, geom_out.normalCount, geom_out.lamps, geom_out.lampsCount);*/
  if (ret && geom_out.faceCount && geom_out.vertCount)
  {
    String name;
    if (build_links.size() == 1 && build_points.size() == 2)
      name.printf(1024, "road_%d", autoRoadNodeIdx++);
    else
      name.printf(1024, "cross_%d", autoCrossNodeIdx++);

    addGeometryNode(geom, mat, geom_out, name);
    // lamp_pos.reserve(lamp_pos.size() + geom_out.lampsCount);
    for (int i = 0; i < geom_out.lampsCount; ++i)
    {
      Quat quat;
      euler_to_quat(geom_out.lamps[i].pitchYawRow.x, geom_out.lamps[i].pitchYawRow.y, geom_out.lamps[i].pitchYawRow.z, quat);
      // lamp_pos.push_back(AnimV20Math::makeTM(geom_out.lamps[i].position, quat, Point3(1,1,1)));
      // lamp_types.push_back(geom_out.lamps[i].typeId);
    }
  }

  road_builder->endBuild(geom_out);
}

static void addInvariantPointProps(const splineclass::RoadData &road, PointProperties &props, ObjLibMaterialListMgr &mat_mgr)
{
  memset(&props, 0, sizeof(props));

  props.linesNumber = road.linesCount;
  props.lineWidth = road.lineWidth;
  props.angleRadius = road.crossTurnRadius;
  props.updir = Point3(0, 1, 0);
  props.crossRoadMatId = mat_mgr.getMaterialId(road.mat.crossRoad);
  props.crossRoadUScale = road.crossRoadUScale;
  props.crossRoadVScale = road.crossRoadVScale;
  props.roadFlipUV = road.flipCrossRoadUV;
}
static void addPointProps(const splineclass::RoadData &road, PointProperties &props, ObjLibMaterialListMgr &mat_mgr,
  SplinePointObject *p)
{
  addInvariantPointProps(road, props, mat_mgr);
  props.pos = p->getPt();
  props.inHandle = p->getBezierIn();
  props.outHandle = p->getBezierOut();

  //! upDir assumed to be computed before while generated straight segments!
  props.updir = p->tmpUpDir;
}

static void computePointUpDir(const splineclass::RoadData *road_before, const splineclass::RoadData *road_after,
  PointProperties &props, SplinePointObject *p, BezierSplineInt3d *seg_before, BezierSplineInt3d *seg_after)
{
  if (p->isCross)
    props.updir = Point3(0, 1, 0);
  else
  {
    float r1 = 0, r2 = 0;

    // calculate integral curvature characteristics of splines
    if (seg_before)
      for (float t = 1; t > 0; t -= 0.02)
        r1 += t * seg_before->k2s(t);

    if (seg_after)
      for (float t = 0; t < 1; t += 0.02)
        r2 += (1 - t) * seg_after->k2s(t);

    // clamp curvature characteristics by road::maxPointR
    if (road_before && fabs(r1) > road_before->maxPointR)
      r1 = r1 >= 0 ? road_before->maxPointR : -road_before->maxPointR;
    if (road_after && fabs(r2) > road_after->maxPointR)
      r2 = r2 >= 0 ? road_after->maxPointR : -road_after->maxPointR;

    // convert curvature characteristics into upDir rotation (in radians) using multipliers of both splines
    float rot = road_before ? -r1 * road_before->pointRtoUpdirRot : 0;
    if (road_after)
      rot -= r2 * road_after->pointRtoUpdirRot;
    if (road_before && road_after)
      rot *= 0.5;

    // clamp maximum upDir rotation angle
    if (road_before)
    {
      if (rot < -road_before->maxUpdirRot)
        rot = -road_before->maxUpdirRot;
      else if (rot > road_before->maxUpdirRot)
        rot = road_before->maxUpdirRot;
    }

    if (road_after)
    {
      if (rot < -road_after->maxUpdirRot)
        rot = -road_after->maxUpdirRot;
      else if (rot > road_after->maxUpdirRot)
        rot = road_after->maxUpdirRot;
    }

    // if rotation angle is small engough, use vertical-up upDir
    if (fabs(rot) < 1e-4)
      props.updir = Point3(0, 1, 0);
    else
    {
      // clamp minimum upDir rotation angle
      if (road_before && fabs(rot) < road_before->minUpdirRot)
        rot = rot >= 0 ? road_before->minUpdirRot : -road_before->minUpdirRot;
      if (road_after && fabs(rot) < road_after->minUpdirRot)
        rot = rot >= 0 ? road_after->minUpdirRot : -road_after->minUpdirRot;

      // finally compute upDir form upDir rotation angle
      Point3 spline_dir = normalize(Point3(p->getProps().relOut.x, 0, p->getProps().relOut.z));
      props.updir = makeTM(spline_dir, rot) * Point3(0, 1, 0);
    }
  }
  p->tmpUpDir = props.updir;
}

static void addInvariantLinkProps(splineclass::RoadData &road, /* non const as we modify it's 'roads' member */
  const splineclass::RoadData *road_prev, const splineclass::RoadData *road_next, float seg_len, float seg_len_prev,
  float seg_len_next, LinkProperties &link, Tab<LampProperties> &lamps, ObjLibMaterialListMgr &mat_mgr)
{
#define GET_MAT_COND(mat_name, cond) (cond) ? mat_mgr.getMaterialId(road.mat.mat_name) : -1

  memset(link.types, 0, sizeof(link.types));
  for (int j = 0; j < splineclass::RoadData::MAX_SEP_LINES; ++j)
    link.types[j] = road.sepLineType[j];

  link.flags = 0;
  if (road.hasLeftSide)
    link.flags |= LinkProperties::LSIDE;
  if (road.hasRightSide)
    link.flags |= LinkProperties::RSIDE;
  if (road.hasLeftSideBorder)
    link.flags |= LinkProperties::LSIDE_BORDERS;
  if (road.hasRightSideBorder)
    link.flags |= LinkProperties::RSIDE_BORDERS;
  if (road.hasLeftVertWall)
    link.flags |= LinkProperties::LVERTICAL_WALL;
  if (road.hasRightVertWall)
    link.flags |= LinkProperties::RVERTICAL_WALL;
  if (road.hasCenterBorder)
    link.flags |= LinkProperties::CENTER_BORDER;
  if (road.hasHammerSides)
    link.flags |= LinkProperties::HAMMER_SIDES;
  if (road.hasHammerCenter)
    link.flags |= LinkProperties::HAMMER_CENTER;
  if (road.hasPaintings)
    link.flags |= LinkProperties::PAINTINGS;
  if (road.hasStopLine)
    link.flags |= LinkProperties::STOP_LINE;
  if (road.isBridge)
    link.flags |= LinkProperties::BRIDGE;
  if (road.isBridgeSupport)
    link.flags |= LinkProperties::BRIDGE_SUPPORT;
  if (road.isCustomBridge)
    link.flags |= LinkProperties::CUSTOM_BRIDGE;
  if (road.flipRoadUV)
    link.flags |= LinkProperties::FLIP_ROAD_TEX;

  link.roadMatId = mat_mgr.getMaterialId(road.mat.road);
  link.sideMatId = GET_MAT_COND(sides, road.hasLeftSide | road.hasRightSide);
  link.verticalWallsMatId = GET_MAT_COND(vertWalls, road.hasLeftVertWall | road.hasRightVertWall);
  link.centerBorderMatId = GET_MAT_COND(centerBorder, road.hasLeftSideBorder | road.hasCenterBorder | road.hasRightSideBorder);
  link.hammerSidesMatId = GET_MAT_COND(hammerSides, road.hasHammerSides);
  link.hammerCenterMatId = GET_MAT_COND(hammerCenter, road.hasHammerCenter);
  link.paintingsMatId = GET_MAT_COND(paintings, road.hasPaintings);

  link.sideSplineProperties.scale[1] = road.sideSlope;
  link.wallSplineProperties.scale[1] = road.wallSlope;
  link.sideSplineProperties.scale[0] = road.sideSlope;
  link.wallSplineProperties.scale[0] = road.wallSlope;
  link.sideSplineProperties.scale[2] = road.sideSlope;
  link.wallSplineProperties.scale[2] = road.wallSlope;

  float len = TRANSIENT_PART * seg_len;
  if (road_prev && TRANSIENT_PART > 0.001)
  {
    float len1 = TRANSIENT_PART * seg_len_prev;
    float t = len1 / (len1 + len);
    link.sideSplineProperties.scale[0] = t * road.sideSlope + (1 - t) * road_prev->sideSlope;
    link.wallSplineProperties.scale[0] = t * road.wallSlope + (1 - t) * road_prev->wallSlope;
  }
  if (road_next && TRANSIENT_PART > 0.001)
  {
    float len2 = TRANSIENT_PART * seg_len_next;
    float t = len / (len + len2);
    link.sideSplineProperties.scale[2] = t * road_next->sideSlope + (1 - t) * road.sideSlope;
    link.wallSplineProperties.scale[2] = t * road_next->wallSlope + (1 - t) * road.wallSlope;
  }

  link.leftMint = 0.0;
  link.leftMaxt = 1.0;
  link.rightMint = 0.0;
  link.rightMaxt = 1.0;

  link.curvatureStrength = road.curvatureStrength;
  link.minStep = road.minStep;
  link.maxStep = road.maxStep;


  link.lamps = (LampProperties *)(uintptr_t)lamps.size();
  link.lampsCount = road.lamps.size();

  for (int j = 0; j < road.lamps.size(); j++)
  {
    splineclass::RoadData::Lamp &lm = road.lamps[j];
    LampProperties &lamp = lamps.push_back();
    lamp.roadOffset = lm.roadOffset;
    lamp.eachSize = lm.eachSize;
    lamp.typeId = (int)(intptr_t)lm.entity;
    lamp.rotateToRoad = lm.rotateToRoad;
    lamp.additionalHeightOffset = lm.additionalHeightOffset;
    lamp.placementType = lm.placementType;
  }

  if (link.lampsCount)
    link.flags |= LinkProperties::GENERATE_LAMPS;

  link.leftWallHeight = road.leftWallHeight;
  link.rightWallHeight = road.rightWallHeight;
  link.centerBorderHeight = road.centerBorderHeight;
  link.leftBorderHeight = road.leftBorderHeight;
  link.rightBorderHeight = road.rightBorderHeight;
  link.leftHammerYOffs = road.leftHammerYOffs;
  link.leftHammerHeight = road.leftHammerHeight;
  link.rightHammerYOffs = road.rightHammerYOffs;
  link.rightHammerHeight = road.rightHammerHeight;
  link.centerHammerYOffs = road.centerHammerYOffs;
  link.centerHammerHeight = road.centerHammerHeight;
  link.bridgeRoadThickness = road.bridgeRoadThickness;
  link.bridgeStandHeight = road.bridgeStandHeight;

  link.walkablePartVScale = road.walkablePartVScale;
  link.walkablePartUScale = road.walkablePartUScale;

#undef GET_MAT_COND
}
static void addLinkProps(splineclass::RoadData &road, LinkProperties &link, Tab<LampProperties> &lamps, ObjLibMaterialListMgr &mat_mgr,
  int pid1, int pid2)
{
  addInvariantLinkProps(road, NULL, NULL, 0, 0, 0, link, lamps, mat_mgr);
  link.pointId1 = pid1;
  link.pointId2 = pid2;
}

static void recalcRoadsLighting(StaticGeometryContainer &geom)
{
  ISceneLightService *ltService = DAGORED2->queryEditorInterface<ISceneLightService>();
  if (!ltService)
    return;

  Color3 ltCol1, ltCol2, ambCol;
  Point3 ltDir1, ltDir2;
  Point3 normal;

  ltService->getDirectLight(ltDir1, ltCol1, ltDir2, ltCol2, ambCol);

  for (int ni = 0; ni < geom.nodes.size(); ++ni)
  {
    const StaticGeometryNode *node = geom.nodes[ni];

    if (node && node->mesh)
    {
      Mesh &mesh = node->mesh->mesh;

      mesh.cvert.resize(mesh.face.size() * 3);
      mesh.cface.resize(mesh.face.size());

      for (int f = 0; f < mesh.face.size(); ++f)
      {
        for (unsigned v = 0; v < 3; ++v)
        {
          normal = mesh.vertnorm[mesh.facengr[f][v]];

          const int vi = f * 3 + v;

          Color3 resColor = ambCol;
          real k = normal * ltDir1;
          if (k > 0)
            resColor += ltCol1 * k;
          k = normal * ltDir2;
          if (k > 0)
            resColor += ltCol2 * k;

          mesh.cvert[vi] = ::color4(resColor, 1);
          mesh.cface[f].t[v] = vi;
        }
      }
    }
  }
}

static void applyRoadShapes(const splineclass::RoadData *asset)
{
#define SET_SHAPE(x, y)                                                        \
  if (x)                                                                       \
  {                                                                            \
    roadbuildertool::RoadShape shape;                                          \
    shape.isClosed = x->isClosed;                                              \
    shape.uScale = x->uScale;                                                  \
    shape.vScale = x->vScale;                                                  \
    shape.pointsCount = x->points.size();                                      \
    shape.points = (roadbuildertool::RoadShape::ShapePoint *)x->points.data(); \
    road_builder->setRoadShape(y, &shape);                                     \
  }                                                                            \
  else                                                                         \
    road_builder->setRoadShape(y, NULL);

  SET_SHAPE(asset->borderShape, SHAPE_BORDER);
  SET_SHAPE(asset->sideShape, SHAPE_SIDE);
  SET_SHAPE(asset->roadShape, SHAPE_ROAD);
  SET_SHAPE(asset->sideHammerShape, SHAPE_SIDE_HAMMER);
  SET_SHAPE(asset->centerHammerShape, SHAPE_CENTER_HAMMER);
  SET_SHAPE(asset->centerBorderShape, SHAPE_CENTER_BORDER);
  SET_SHAPE(asset->wallShape, SHAPE_WALL);
  SET_SHAPE(asset->bridgeShape, SHAPE_BRIDGE);

#undef SET_SHAPE
}

void SplineObject::generateRoadSegments(int start_idx, int end_idx, const splineclass::RoadData *asset_prev,
  splineclass::RoadData *asset, const splineclass::RoadData *asset_next)
{
  if (!asset_prev && !asset && !asset_next) // roads not used?
  {
    for (int i = start_idx; i < end_idx; i++)
      points[i]->removeRoadGeom();
    return;
  }

  if (!loadRoadBuilderDll())
    return;
  if (!prepareRoadBuilder())
    return;

  LinkProperties link;
  PointProperties pts[2];
  Tab<LampProperties> lamps(tmpmem);
  Geometry geomOut;
  ObjLibMaterialListMgr matMgr;
  MaterialDataList *mat = NULL;
  BezierSplineInt3d *seg_cur = NULL, *seg_prev = NULL, *seg_next = NULL;

  if (asset)
  {
    addInvariantPointProps(*asset, pts[0], matMgr);
    addInvariantPointProps(*asset, pts[1], matMgr);
    addInvariantLinkProps(*asset, NULL, NULL, 0, 0, 0, link, lamps, matMgr);
    mat = matMgr.buildMaterialDataList();
    applyRoadShapes(asset);
  }

  for (int i = start_idx; i < end_idx; i++)
  {
    if (!asset || points[i]->isCross || points[i + 1]->isCross)
    {
      if (points[i]->getRoadGeom())
        HmapLandObjectEditor::geomBuildCntRoad++;
      points[i]->removeRoadGeom();
      continue;
    }
    if (!points[i]->segChanged && !points[i + 1]->segChanged)
      continue;

    // DAEDITOR3.conNote("build segment %p/%d", this, i);
    lamps.clear();

    seg_cur = &bezierSpline.segs[i];
    if (i > 0)
      seg_prev = &bezierSpline.segs[i - 1];
    else
      seg_prev = isClosed() ? &bezierSpline.segs.back() : NULL;
    if (i + 1 < bezierSpline.segs.size())
      seg_next = &bezierSpline.segs[i + 1];
    else
      seg_next = isClosed() ? &bezierSpline.segs[0] : NULL;

    addInvariantLinkProps(*asset, i == start_idx ? asset_prev : NULL, i + 1 == end_idx ? asset_next : NULL, seg_cur->len,
      seg_prev ? seg_prev->len : 0.0, seg_next ? seg_next->len : 0.0, link, lamps, matMgr);
    link.pointId1 = 0;
    link.pointId2 = 1;

    pts[0].pos = points[i]->getPt();
    pts[0].inHandle = points[i]->getBezierIn();
    pts[0].outHandle = points[i]->getBezierOut();
    computePointUpDir(i == start_idx ? asset_prev : asset, asset, pts[0], points[i], seg_prev, seg_cur);

    pts[1].pos = points[i + 1]->getPt();
    pts[1].inHandle = points[i + 1]->getBezierIn();
    pts[1].outHandle = points[i + 1]->getBezierOut();
    if (i + 1 == end_idx && asset_next)
    {
      pts[1].linesNumber = asset_next->linesCount;
      pts[1].lineWidth = asset_next->lineWidth;
    }
    computePointUpDir(asset, i + 1 == end_idx ? asset_next : asset, pts[1], points[i + 1], seg_cur, seg_next);

    GeomObject &roadGeom = points[i]->getClearedRoadGeom();

    buildSegments(geomOut, *roadGeom.getGeometryContainer(), mat, make_span(&link, 1), lamps, make_span_const(pts, 2), {});
    roadGeom.setTm(TMatrix::IDENT);

    recalcRoadsLighting(*roadGeom.getGeometryContainer());
    roadGeom.notChangeVertexColors(true);
    dagGeom->geomObjectRecompile(roadGeom);
    roadGeom.notChangeVertexColors(false);
    HmapLandObjectEditor::geomBuildCntRoad++;
    points[i]->updateRoadBox();
  }

  del_it(mat);
  road_builder->release();
}

void HmapLandObjectEditor::updateCrossRoadGeom()
{
  if (!crossRoads.size())
    return;

  if (!loadRoadBuilderDll())
    return;
  if (!prepareRoadBuilder())
    return;

  Tab<LinkProperties> links(tmpmem);
  Tab<PointProperties> pts(tmpmem);
  Tab<LampProperties> lamps(tmpmem);
  Geometry geomOut;
  ObjLibMaterialListMgr matMgr;
  MaterialDataList *mat = NULL;
  splineclass::RoadData *asset = NULL, *assetPrev = NULL, *central_asset = NULL;

  for (int i = 0; i < crossRoads.size(); i++)
  {
    if (!crossRoads[i]->changed)
      continue;
    // DAEDITOR3.conNote("build cross %d", i);
    crossRoads[i]->changed = false;

    links.clear();
    pts.clear();
    lamps.clear();
    int idc = -1, c_rank = -1;
    for (int j = crossRoads[i]->points.size() - 1; j >= 0; j--)
    {
      SplinePointObject *p = crossRoads[i]->points[j];
      SplineObject *s = p->spline;

      if (!getRoad(p))
        continue;

      assetPrev = asset = NULL;
      for (int k = 0; k < s->points.size(); k++)
      {
        assetPrev = asset;
        asset = getRoad(s->points[k]);

        if (s->points[k] == p && (asset || assetPrev))
        {
          if (idc == -1)
          {
            central_asset = asset ? asset : assetPrev;
            addPointProps(*central_asset, pts.push_back(), matMgr, p);
            c_rank = asset ? asset->roadRankId : assetPrev->roadRankId;
            idc = pts.size() - 1;
          }
          else if (c_rank < (asset ? asset->roadRankId : assetPrev->roadRankId))
          {
            central_asset = asset ? asset : assetPrev;
            addPointProps(*central_asset, pts[idc], matMgr, p);
            c_rank = asset ? asset->roadRankId : assetPrev->roadRankId;
          }

          if (k > 0 && assetPrev)
          {
            addPointProps(*assetPrev, pts.push_back(), matMgr, s->points[k - 1]);
            addLinkProps(*assetPrev, links.push_back(), lamps, matMgr, pts.size() - 1, idc);
          }
          if (k + 1 < s->points.size() && asset)
          {
            asset = getRoad(s->points[k]);
            if (asset)
            {
              addPointProps(*asset, pts.push_back(), matMgr, s->points[k + 1]);
              addLinkProps(*asset, links.push_back(), lamps, matMgr, idc, pts.size() - 1);
            }
          }
          break;
        }
      }
    }

    if (idc == -1)
    {
      // not really crossroads - just cross of 2 splines
      if (crossRoads[i]->getRoadGeom())
        HmapLandObjectEditor::geomBuildCntRoad++;
      crossRoads[i]->removeRoadGeom();
      continue;
    }

    mat = matMgr.buildMaterialDataList();
    applyRoadShapes(central_asset);

    GeomObject &roadGeom = crossRoads[i]->getClearedRoadGeom();
    buildSegments(geomOut, *roadGeom.getGeometryContainer(), mat, make_span(links), lamps, pts, {});
    roadGeom.setTm(TMatrix::IDENT);

    recalcRoadsLighting(*roadGeom.getGeometryContainer());
    roadGeom.notChangeVertexColors(true);
    dagGeom->geomObjectRecompile(roadGeom);
    roadGeom.notChangeVertexColors(false);

    del_it(mat);
    HmapLandObjectEditor::geomBuildCntRoad++;
  }
}
