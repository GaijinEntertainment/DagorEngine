// Copyright (C) Gaijin Games KFT.  All rights reserved.

using splineclass::Attr;

static void recalcLoftLighting(StaticGeometryContainer &geom)
{
  ISceneLightService *ltService = EDITORCORE->queryEditorInterface<ISceneLightService>();
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

      if (!mesh.vertnorm.size())
      {
        mesh.calc_vertnorms();
        mesh.calc_ngr();
      }

      mesh.cvert.resize(mesh.face.size() * 3);
      mesh.cface.resize(mesh.face.size());

      for (int f = 0; f < mesh.face.size(); ++f)
      {
        for (unsigned v = 0; v < 3; ++v)
        {
          normal = -mesh.vertnorm[mesh.facengr[f][v]];

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

void SplineGenEntity::generateLoftSegments(BezierSpline3d &effSpline, const char *name, int start_idx, int end_idx,
  bool place_on_collision, dag::ConstSpan<splineclass::Attr> splineScales, float scaleTcAlong)
{
  const splineclass::LoftGeomGenData *asset = splineClass ? splineClass->genGeom : nullptr, *assetEnd = NULL;
  IAssetService *assetSrv = EDITORCORE->queryEditorInterface<IAssetService>();

  if (!asset)
  {
    removeLoftGeom();
    return;
  }

  if (hasNonZeroLayers)
    loftChangesCount++;
  hasNonZeroLayers = false;

  if (place_on_collision)
  {
    if (!asset->colliders.size())
      DAEDITOR3.conError("<%s>: empty collders list in loft asset <%s>", name, assetSrv->getSplineClassDataName(splineClass));
  }
  if (asset->colliders.size())
    EDITORCORE->setColliders(asset->colliders, asset->collFilter);

  GeomObject &loftGeometry = getClearedLoftGeom();
  StaticGeometryContainer *g = new StaticGeometryContainer;
  bool segs_stored = false;
  if (!assetSrv)
  {
    updateLoftBox();
    goto final_release;
  }
  // debug("genLoftSeg(%d, %d, %s, cls=%s) segs=%d", start_idx, end_idx, name,
  //   EDITORCORE->queryEditorInterface<IAssetService>()->getSplineClassDataName(splineClass), effSpline.segs.size());

  for (int j = 0; j < asset->loft.size(); j++)
  {
    if (!assetSrv->isLoftCreatable(asset, j))
      continue;
    const splineclass::LoftGeomGenData::Loft &loft = asset->loft[j];
    if (loft.makeDelaunayPtCloud || loft.waterSurface)
      continue;

    {
      float zeroOpacityDistAtEnds = loft.zeroOpacityDistAtEnds;
      for (int materialNo = 0; materialNo < loft.matNames.size(); materialNo++)
      {
        Ptr<MaterialData> material;
        material = assetSrv->getMaterialData(loft.matNames[materialNo]);
        if (!material.get())
        {
          DAEDITOR3.conError("<%s>: invalid material <%s> for loft", name, loft.matNames[materialNo].str());
          goto final_release;
        }

        Mesh *themesh = new Mesh;
        Mesh &mesh = *themesh;

        bool store_segs = !segs_stored && loft.storeSegs;
        if (!assetSrv->createLoftMesh(mesh, asset, j, effSpline, start_idx, end_idx, place_on_collision, scaleTcAlong, materialNo,
              splineScales, store_segs ? &loftSeg : NULL, name,
              (start_idx == 0 && zeroOpacityDistAtEnds > 0) ? zeroOpacityDistAtEnds : 0,
              (end_idx == effSpline.segs.size() && zeroOpacityDistAtEnds > 0) ? zeroOpacityDistAtEnds : 0, //
              loft.marginAtStart, loft.marginAtEnd))
        {
          del_it(themesh);
          goto final_release;
        }

        if (store_segs)
          segs_stored = true;

        MaterialDataList mat;
        mat.addSubMat(material);

        ObjCreator3d::addNode(name, themesh, &mat, *g);

        for (int i = 0; i < g->nodes.size(); ++i)
        {
          StaticGeometryNode *node = g->nodes[i];

          node->flags = loft.flags;
          node->normalsDir = loft.normalsDir;

          node->calcBoundBox();
          node->calcBoundSphere();

          if (loft.loftLayerOrder)
          {
            node->script.setInt("layer", loft.loftLayerOrder);
            hasNonZeroLayers = true;
          }
          if (loft.stage)
            node->script.setInt("stage", loft.stage);
          if (!loft.layerTag.empty())
            node->script.setStr("layerTag", loft.layerTag);
          loftGeometry.getGeometryContainer()->addNode(new (tmpmem) StaticGeometryNode(*node));
        }

        g->clear();
      }
    }
  }
  loftGeometry.setTm(TMatrix::IDENT);
  recalcLoftLighting(*loftGeometry.getGeometryContainer());
  loftGeometry.notChangeVertexColors(true);
  loftGeometry.recompile();
  loftGeometry.notChangeVertexColors(false);
  updateLoftBox();

final_release:
  del_it(g);
  if (asset->colliders.size())
    EDITORCORE->restoreEditorColliders();
  if (hasNonZeroLayers)
    loftChangesCount++;
}

static bool make_spline_x0z(BezierSpline3d &dest, BezierSpline2d &bsp2, float water_level)
{
  clear_and_resize(dest.segs, bsp2.segs.size());
  for (int k = 0; k < dest.segs.size(); k++)
  {
    dest.segs[k].len = bsp2.segs[k].len;
    dest.segs[k].tlen = bsp2.segs[k].tlen;
    for (int l = 0; l < 3; l++)
      dest.segs[k].s2T[l] = bsp2.segs[k].s2T[l];
    for (int l = 0; l < 4; l++)
      dest.segs[k].sk[l].set_x0y(bsp2.segs[k].sk[l]);
    dest.segs[k].sk[0].y = water_level;
  }
  return true;
}

void SplineGenEntity::gatherLoftLandPts(Tab<Point3> &loft_pt_cloud, BezierSpline3d &effSpline, BezierSpline2d &splineXZ,
  const char *name, int start_idx, int end_idx, bool place_on_collision, dag::ConstSpan<splineclass::Attr> splineScales,
  Tab<Point3> &water_border_polys, Tab<Point2> &hmap_sweep_polys, bool water_surf, float water_level)
{
  IAssetService *assetSrv = EDITORCORE->queryEditorInterface<IAssetService>();
  if (!assetSrv)
    return;

  const splineclass::LoftGeomGenData *asset = splineClass ? splineClass->genGeom : nullptr;

  Tab<Point2> tmp_l2(tmpmem), tmp_r2(tmpmem);
  Tab<Point3> tmp_l3(tmpmem), tmp_r3(tmpmem);
  Tab<Tab<Point3>> tmp_ls(tmpmem);

  BezierSpline3d bspw;
  bool bspw_inited = false;

  int loft_cnt = asset ? asset->loft.size() : 0;
  for (int j = 0; j < loft_cnt; j++)
  {
    if (!assetSrv->isLoftCreatable(asset, j))
      continue;
    const splineclass::LoftGeomGenData::Loft &loft = asset->loft[j];
    if (!loft.makeDelaunayPtCloud)
      continue;
    if (!water_surf && loft.waterSurface)
      continue;

    bool zero_sweep = (loft.shapePts.size() > 1 && fabsf(loft.shapePts[0].x - loft.shapePts.back().x) < 1e-3f);
    float zeroOpacityDistAtEnds = loft.zeroOpacityDistAtEnds;

    {
      if (loft.waterSurface && !bspw_inited)
        bspw_inited = make_spline_x0z(bspw, splineXZ, water_level);

      Mesh *themesh = new Mesh;
      Mesh &mesh = *themesh;

      assetSrv->createLoftMesh(mesh, asset, j, loft.waterSurface ? bspw : effSpline, start_idx, end_idx, place_on_collision, 1.0f, -1,
        splineScales, NULL, name, (start_idx == 0 && zeroOpacityDistAtEnds > 0) ? zeroOpacityDistAtEnds : 0,
        (end_idx == effSpline.segs.size() && zeroOpacityDistAtEnds > 0) ? zeroOpacityDistAtEnds : 0, //
        loft.marginAtStart, loft.marginAtEnd);

      int prev_c = loft_pt_cloud.size();

      for (int k = 0; k < tmp_ls.size(); k++)
        tmp_ls[k].clear();

      for (int k = 0; k < mesh.vert.size(); k++)
        if (int(mesh.tvert[2][k].y) & 1)
        {
          int index = int(mesh.tvert[2][k].x) + 1;
          if (index >= tmp_ls.size())
            tmp_ls.resize(index + 1);
          if (index >= 0)
            tmp_ls[index].push_back(mesh.vert[k]);
        }

      for (int k = 0; k < tmp_ls.size(); k++)
        if (tmp_ls[k].size())
        {
          loft_pt_cloud.push_back().set(1.1e12f, tmp_ls[k].size(), 0);
          append_items(loft_pt_cloud, tmp_ls[k].size(), tmp_ls[k].data());
        }

      if (zero_sweep)
        clear_and_shrink(mesh.vert);

      tmp_l2.clear();
      tmp_l2.reserve(mesh.vert.size() / 2);
      tmp_r2.clear();
      tmp_r2.reserve(mesh.vert.size() / 2);
      for (int k = 0; k < mesh.vert.size(); k++)
      {
        int index = int(mesh.tvert[2][k].x);
        if (index == 0)
          tmp_l2.push_back().set_xz(mesh.vert[k]);
        else if (index == -1)
          tmp_r2.push_back().set_xz(mesh.vert[k]);
      }
      if (tmp_l2.size() && tmp_r2.size())
      {
        G_ASSERT(tmp_l2.size() == tmp_r2.size());
        hmap_sweep_polys.reserve(tmp_l2.size() * 2 + 1);
        hmap_sweep_polys.push_back().set(1.1e12f, tmp_l2.size() * 2);
        append_items(hmap_sweep_polys, tmp_l2.size(), tmp_l2.data());
        for (int k = tmp_r2.size() - 1; k >= 0; k--)
          hmap_sweep_polys.push_back() = tmp_r2[k];
      }

      // DAEDITOR3.conNote("make %p.%d in range (%d,%d) of %s -> %d pts",
      //   asset, j, start_idx, pi-1, getName(), loft_pt_cloud.size()-prev_c);
    }
  }

  for (int j = 0; j < loft_cnt; j++)
  {
    if (!water_surf)
      continue;

    if (!assetSrv->isLoftCreatable(asset, j))
      continue;
    const splineclass::LoftGeomGenData::Loft &loft = asset->loft[j];
    if (loft.makeDelaunayPtCloud || !loft.waterSurface)
      continue;

    float zeroOpacityDistAtEnds = loft.zeroOpacityDistAtEnds;
    if (!bspw_inited)
      bspw_inited = make_spline_x0z(bspw, splineXZ, water_level);

    {
      Mesh *themesh = new Mesh;
      Mesh &mesh = *themesh;

      assetSrv->createLoftMesh(mesh, asset, j, bspw, start_idx, end_idx, place_on_collision, 1.0f, -1, splineScales, NULL, name,
        (start_idx == 0 && zeroOpacityDistAtEnds > 0) ? zeroOpacityDistAtEnds : 0,
        (end_idx == effSpline.segs.size() && zeroOpacityDistAtEnds > 0) ? zeroOpacityDistAtEnds : 0, //
        loft.marginAtStart, loft.marginAtEnd);

      tmp_l3.clear();
      tmp_l3.reserve(mesh.vert.size() / 2);
      tmp_r3.clear();
      tmp_r3.reserve(mesh.vert.size() / 2);
      for (int k = 0; k < mesh.vert.size(); k++)
      {
        int index = int(mesh.tvert[2][k].x);
        if (index == 0)
          tmp_l3.push_back().set_xVz(mesh.vert[k], water_level);
        else if (index == -1)
          tmp_r3.push_back().set_xVz(mesh.vert[k], water_level);
      }
      if (tmp_l3.size() && tmp_r3.size())
      {
        G_ASSERT(tmp_l3.size() == tmp_r3.size());
        water_border_polys.reserve(tmp_l3.size() * 2 + 1);
        water_border_polys.push_back().set(1.1e12f, tmp_l3.size() * 2, loft.waterSurfaceExclude);
        append_items(water_border_polys, tmp_l3.size(), tmp_l3.data());
        for (int k = tmp_r3.size() - 1; k >= 0; k--)
          water_border_polys.push_back() = tmp_r3[k];
      }
    }
  }
}
