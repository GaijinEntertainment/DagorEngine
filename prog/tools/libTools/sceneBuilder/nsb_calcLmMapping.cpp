#include <sceneBuilder/nsb_LightmappedScene.h>
#include <libTools/staticGeom/matFlags.h>
#include <unwrapMapping/autoUnwrap.h>
#include <libTools/util/iLogWriter.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>
#include "lmsPriv.h"


struct SecondPassLmMappingData
{
  StaticSceneBuilder::LightmappedScene::Object *obj;
  int mappingHandle;
  Mesh *lmOnlyMesh;
};

bool StaticSceneBuilder::LightmappedScene::calcLightmapMapping(int lm_size, int target_lm_num, float pack_fidelity_ratio,
  IGenericProgressIndicator &pbar, float &lm_scale)
{
  SimpleProgressCB progressCB;

  pbar.startProgress(&progressCB);
  pbar.setActionDesc("Lightmap mapping");
  pbar.setDone(0);
  pbar.setTotal(totalFaces);

  AutoUnwrapMapping::start_mapping(lm_size);
  Tab<SecondPassLmMappingData> splmd(tmpmem);
  int ltmap_face_count = 0;
  int vltmap_face_count = 0;

  // map objects
  for (int oi = 0; oi < objects.size(); ++oi)
  {
    if (progressCB.mustStop)
    {
      AutoUnwrapMapping::finish_mapping();
      return false;
    }

    Object &obj = objects[oi];

    pbar.incDone(obj.mesh.getFaceCount());

    obj.lmIds.resize(obj.mesh.getFaceCount());

    bool needlm = false, hasnonlm = false;

    // check for LM/VLM requirements
    for (int f = 0; f < obj.mesh.getFaceCount();)
    {
      int mi = obj.mesh.getFaceMaterial(f);
      int ef;
      for (ef = f + 1; ef < obj.mesh.getFaceCount(); ++ef)
        if (obj.mesh.getFaceMaterial(ef) != mi)
          break;

      int flg = mi < obj.mats.size() && obj.mats[mi] ? obj.mats[mi]->flags : 0;

      uint16_t lmid;
      if (flg & MatFlags::FLG_USE_LM)
      {
        needlm = true;
        lmid = 0;
      }
      else
      {
        lmid = BAD_LMID;
        hasnonlm = true;
      }

      for (int i = f; i < ef; ++i)
        obj.lmIds[i] = lmid;

      if (flg & MatFlags::FLG_USE_VLM)
      {
        if (!obj.vltmapFaces.size())
        {
          obj.vltmapFaces.resize(obj.mesh.getFaceCount());
          mem_set_ff(obj.vltmapFaces);
        }
        vltmap_face_count += ef - f;
      }

      f = ef;
    }


    if (needlm)
    {
      real ss = obj.lmSampleSize;
      if (ss <= 0)
        ss = defLmSampleSize;

      SecondPassLmMappingData md;
      md.obj = &obj;
      md.lmOnlyMesh = hasnonlm ? new Mesh(obj.mesh) : NULL;

      if (md.lmOnlyMesh)
      {
        pbar.setActionDesc(String(256, "object %s: mesh with mixed ltmap/non-ltmap faces", obj.name.str()));
        for (int f = md.lmOnlyMesh->getFaceCount() - 1; f >= 0;)
        {
          while (f >= 0 && obj.lmIds[f] != BAD_LMID)
            f--;
          if (f < 0)
            break;

          int ef = f;
          while (ef >= 0 && obj.lmIds[ef] == BAD_LMID)
            ef--;
          md.lmOnlyMesh->erasefaces(ef + 1, f - ef);
          pbar.setActionDesc(String(256, "  erased non-ltmap faces %d..%d", ef + 1, f));
          f = ef;
        }
      }

      Mesh &m = md.lmOnlyMesh ? *md.lmOnlyMesh : obj.mesh;
      md.mappingHandle = AutoUnwrapMapping::request_mapping(m.face.data(), m.getFaceCount(), m.vert.data(), m.getVertCount(), ss, true,
        obj.unwrapScheme);

      splmd.push_back(md);
      ltmap_face_count += m.getFaceCount();
    }


    if (obj.vltmapFaces.size())
    {
      // map VLM
      Tab<VertNormPair> vn(tmpmem);
      vn.reserve(obj.mesh.getFaceCount() * 3);

      VertToFaceVertMap f2v;

      obj.mesh.buildVertToFaceVertMap(f2v);

      for (int j = 0; j < obj.mesh.getVertCount(); ++j)
      {
        VertNormPair v;
        v.v = j;
        int lv = vn.size();

        int numf = f2v.getNumFaces(j);

        for (int k = 0; k < numf; ++k)
        {
          int findex = f2v.getVertFaceIndex(j, k);

          int f = findex / 3;
          int mat = obj.mesh.getFaceMaterial(f);

          if (obj.mats.size() <= mat || !obj.mats[mat] || !(obj.mats[mat]->flags & MatFlags::FLG_USE_VLM))
            continue;

          int vi = findex % 3;

          v.n = obj.mesh.facengr[f][vi];

          int l;
          for (l = lv; l < vn.size(); ++l)
            if (vn[l].n == v.n)
              break;
          if (l >= vn.size())
            vn.push_back(v);

          obj.vltmapFaces[f].t[vi] = l;
        }
      }

      obj.vlmSize = vn.size();

      obj.vlmIndex = vltmapSize;
      vltmapSize += obj.vlmSize;
    }
  }

  // unwrap meshes and prepare facegroups
  pbar.setActionDesc(
    String(128, "Preparing face groups (%d faces on ltmap, %d faces use vertex lighting)", ltmap_face_count, vltmap_face_count));
  AutoUnwrapMapping::prepare_facegroups(pbar, progressCB);

  // perform facegroup packing
  float cur_ratio;
  int cur_num;

  float lower_s = 0.0, upper_s = -1;

  debug_flush(true);
  if (target_lm_num > splmd.size())
    target_lm_num = splmd.size();

  for (;;) // eternal loop with explicit exit
  {
    if (progressCB.mustStop)
    {
      AutoUnwrapMapping::finish_mapping();
      return false;
    }

    if (splmd.size() < 1)
      break;

    pbar.setActionDesc(String(256, "Packing face groups into lightmaps (s=%.5f)", lm_scale));

    cur_num = AutoUnwrapMapping::repack_facegroups(lm_scale, &cur_ratio, target_lm_num, pbar, progressCB);

    debug_ctx("x%.6f: %d lightmaps, ratio=%.6f", lm_scale, cur_num, cur_ratio / cur_num);

    if (cur_num == -1)
    {
      AutoUnwrapMapping::finish_mapping();
      return false;
    }

    if (cur_num <= 1 && cur_ratio < 1e-3)
      break;

    if (cur_num == target_lm_num && cur_ratio >= pack_fidelity_ratio * cur_num)
      break;

    if (cur_num > target_lm_num)
    {
      lower_s = lm_scale;
      if (upper_s < 0)
      {
        lm_scale *= 2;
        continue;
      }
    }
    else
      upper_s = lm_scale;

    if (upper_s - lower_s < 1.0e-3 * lm_scale)
    {
      lm_scale = upper_s;
      if (cur_num != target_lm_num)
      {
        pbar.setActionDesc("Packing face groups into lightmaps(final)");
        cur_num = AutoUnwrapMapping::repack_facegroups(lm_scale, &cur_ratio, target_lm_num, pbar, progressCB);

        if (cur_num == -1)
        {
          AutoUnwrapMapping::finish_mapping();
          return false;
        }
      }
      break;
    }

    lm_scale = (upper_s + lower_s) / 2;
  }

  debug_ctx("x%.6f: %d lightmaps, ratio=%.6f", lm_scale, cur_num, cur_ratio / cur_num);
  debug_flush(false);

  // receive final mapping for lightmapped objects
  Tab<TFace> tmpLmFaces(tmpmem);
  Tab<uint16_t> tmpLmIds(tmpmem);

  for (int i = 0; i < splmd.size(); ++i)
  {
    Object &obj = *splmd[i].obj;
    Point2 *lmtc;
    int numlmtc;

    obj.lmFaces.resize(obj.mesh.getFaceCount());

    if (!splmd[i].lmOnlyMesh)
    {
      AutoUnwrapMapping::get_final_mapping(splmd[i].mappingHandle, obj.lmIds.data(), elem_size(obj.lmIds), obj.lmFaces.data(), lmtc,
        numlmtc);
    }
    else
    {
      mem_set_ff(obj.lmFaces);

      tmpLmIds.resize(splmd[i].lmOnlyMesh->getFaceCount());
      tmpLmFaces.resize(splmd[i].lmOnlyMesh->getFaceCount());

      AutoUnwrapMapping::get_final_mapping(splmd[i].mappingHandle, tmpLmIds.data(), elem_size(obj.lmIds), tmpLmFaces.data(), lmtc,
        numlmtc);

      TFace *pLmFaces = tmpLmFaces.data();
      uint16_t *pLmIds = tmpLmIds.data();
      for (int j = 0; j < obj.lmIds.size(); j++)
        if (obj.lmIds[j] != BAD_LMID)
        {
          obj.lmIds[j] = *pLmIds;
          obj.lmFaces[j] = *pLmFaces;
          pLmIds++;
          pLmFaces++;
        }
      del_it(splmd[i].lmOnlyMesh);
    }
    obj.lmTc = make_span_const(lmtc, numlmtc);
  }

  // create lightmaps
  lightmaps.resize(AutoUnwrapMapping::num_lightmaps());
  for (int i = 0; i < lightmaps.size(); ++i)
    AutoUnwrapMapping::get_lightmap_size(i, lightmaps[i].size.x, lightmaps[i].size.y);

  AutoUnwrapMapping::finish_mapping();

  return true;
}

void StaticSceneBuilder::LightmappedScene::makeSingleLightmap()
{
  //== better layout

  IPoint2 ofs(0, 0);

  totalLmSize = IPoint2(0, 0);

  for (int i = 0; i < lightmaps.size(); ++i)
  {
    Lightmap &lm = lightmaps[i];

    lm.offset = ofs;
    ofs.y += lm.size.y;
    if (lm.size.x > totalLmSize.x)
      totalLmSize.x = lm.size.x;
  }

  totalLmSize.y = ofs.y;
}
