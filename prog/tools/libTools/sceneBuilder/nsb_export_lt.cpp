#include <libTools/util/de_TextureName.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/util/strUtil.h>
#include <sceneBuilder/nsb_LightmappedScene.h>
#include <sceneBuilder/nsb_export_lt.h>
#include <shaders/dag_shaderCommon.h>
#include <ioSys/dag_ioUtils.h>
#include <libTools/staticGeom/matFlags.h>
#include <libTools/dtx/dtxSave.h>
#include <libTools/shaderResBuilder/matSubst.h>
#include <util/dag_texMetaData.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_debug.h>

#include <libTools/util/makeBindump.h>
#include <libTools/util/binDumpUtil.h>
#include <assets/assetHlp.h>


class StaticSceneBuilder::LtinputExporter : public LightmappedScene
{
public:
  void exportTexture(IGenSave &cb, StaticGeometryTexture *tex, ILogWriter &log, int dtx_quality);
  void exportMaterial(IGenSave &cb, StaticGeometryMaterial *mat, ILogWriter &log);
  void exportObject(IGenSave &cb, Object &obj, ILogWriter &log);
  void exportScene(IGenSave &cb, bool save_lightmaps, ILogWriter &log, int dtx_quality, IGenericProgressIndicator &pbar);
};


void StaticSceneBuilder::LtinputExporter::exportTexture(IGenSave &cb, StaticGeometryTexture *tex, ILogWriter &log, int dtx_quality)
{
  TextureMetaData tmd;
  SimpleString filename(tmd.decode(tex->fileName));
  MemorySaveCB cwr(4 << 20);
  int data_size = 0;

  cb.writeString(filename);

  if (strlen(filename) > 0 && filename[strlen(filename) - 1] == '*')
  {
    if (!texconvcache::get_tex_asset_built_raw_dds_by_name(filename, cwr, _MAKE4C('PC'), NULL, &log))
    {
      log.addMessage(ILogWriter::WARNING, "Convertible tex (%s) is not converted yet; exported as empty/black", filename.str());
      cb.writeInt(0);
      return;
    }

    log.addMessage(ILogWriter::NOTE, "Convertible tex (%s) was used", filename.str());
  }
  else
  {
    const char *ext = dd_get_fname_ext(filename);
    if (ext && strnicmp(ext, ".dds", 4) == 0)
      copy_file_to_stream(filename, cwr);
    else
    {
      cb.writeInt(0);
      log.addMessage(ILogWriter::ERROR, "Texture file not convertible or DDS '%s'", filename);
      return;
    }
  }

  data_size = cwr.tell();
  MemoryLoadCB loadcb(cwr.getMem(), false);

  ddstexture::Header hdr;

  hdr.addr_mode_u = tmd.d3dTexAddr(tmd.addrU);
  hdr.addr_mode_v = tmd.d3dTexAddr(tmd.addrV);

  hdr.HQ_mip = tmd.hqMip;
  hdr.MQ_mip = tmd.mqMip;
  hdr.LQ_mip = tmd.lqMip;

  if (!ddstexture::write_cb_to_cb(cb, loadcb, &hdr, data_size))
  {
    log.addMessage(ILogWriter::ERROR, "Can't open texture file '%s'", filename);
    cb.writeInt(0);
  }
}


void StaticSceneBuilder::LtinputExporter::exportMaterial(IGenSave &cb, StaticGeometryMaterial *mat, ILogWriter &log)
{
  cb.beginBlock();
  cb.writeString(mat->name);
  cb.writeInt(mat->flags);
  cb.writeInt(getGiTextureId(mat->textures[0]));
  cb.write(&mat->diff, sizeof(mat->diff));
  cb.write(&mat->emis, sizeof(mat->emis));
  cb.write(&mat->trans, sizeof(mat->trans));
  cb.write(&mat->amb, sizeof(mat->amb));
  cb.writeInt(mat->atest);
  cb.writeReal(mat->cosPower);
  cb.writeString(static_geom_mat_subst.substMatClassName(mat->className));
  cb.writeString(mat->scriptText);
  cb.endBlock();
}


void StaticSceneBuilder::LtinputExporter::exportObject(IGenSave &cb, Object &obj, ILogWriter &log)
{
  cb.beginBlock();

  cb.writeString(obj.name);

  cb.writeInt(obj.flags);

  cb.writeInt(obj.vssDivCount);

  cb.writeInt(obj.topLodObjId);
  cb.writeReal(obj.lodRange);

  cb.writeInt(obj.mats.size());
  for (int i = 0; i < obj.mats.size(); ++i)
    cb.writeInt(getMaterialId(obj.mats[i]));

  {
    Tab<GiFace> faces(tmpmem);
    faces.resize(obj.mesh.getFaceCount());

    for (int i = 0; i < faces.size(); ++i)
    {
      faces[i].v[0] = obj.mesh.getFace()[i].v[0];
      faces[i].v[1] = obj.mesh.getFace()[i].v[1];
      faces[i].v[2] = obj.mesh.getFace()[i].v[2];
      faces[i].mat = obj.mesh.getFaceMaterial(i);
    }

    cb.writeTab(obj.mesh.vert);
    cb.writeTab(faces);
  }

  cb.writeTab(obj.mesh.vertnorm);
  cb.writeTab(obj.mesh.facengr);

  cb.writeTab(obj.mesh.tvert[0]);
  cb.writeTab(obj.mesh.tface[0]);

  if (obj.flags & ObjFlags::FLG_BILLBOARD_MESH)
  {
    // pass billboard deltas in vertex color

    int chi = obj.mesh.find_extra_channel(SCUSAGE_EXTRA, 0);
    G_ASSERT(chi >= 0);

    MeshData::ExtraChannel &ch = obj.mesh.extra[chi];
    G_ASSERT(ch.type == MeshData::CHT_FLOAT3);

    cb.writeInt(ch.numverts());
    cb.write(&ch.vt[0], ch.numverts() * sizeof(Point3));

    cb.writeTab(ch.fc);
  }
  else
  {
    cb.writeInt(obj.mesh.cvert.size());
    for (int i = 0; i < obj.mesh.cvert.size(); i++)
      cb.write(&obj.mesh.cvert[i], sizeof(Point3));

    cb.writeTab(obj.mesh.cface);
  }

  // convert LM TC
  {
    Tab<Point2> tc(tmpmem);
    Tab<TFace> faces(tmpmem);

    tc.reserve(obj.lmTc.size());
    faces.resize(obj.lmFaces.size());

    for (int i = 0; i < faces.size(); ++i)
    {
      IPoint2 ofs(0, 0);
      IPoint2 size(0, 0);

      if (obj.lmIds[i] != BAD_LMID)
      {
        ofs = lightmaps[obj.lmIds[i]].offset;
        size = lightmaps[obj.lmIds[i]].size;
      }
      else
      {
        // debug("obj %s face[%i] has BAD_LMID", (char*)obj.name, i );
        // debug_flush(false);
        faces[i].t[0] = faces[i].t[1] = faces[i].t[2] = -1;
        continue;
      }

      if (obj.lmFaces[i].t[0] >= obj.lmTc.size() || obj.lmFaces[i].t[1] >= obj.lmTc.size() || obj.lmFaces[i].t[2] >= obj.lmTc.size())
      {
        logerr("obj.lmFaces[%d].t=%d,%d,%d", i, obj.lmFaces[i].t[0], obj.lmFaces[i].t[1], obj.lmFaces[i].t[2]);
        erase_items(faces, i, 1);
        i--;
        continue;
      }
      for (int j = 0; j < 3; ++j)
      {
        Point2 p = obj.lmTc[obj.lmFaces[i].t[j]];
        p.x *= size.x;
        p.y *= size.y;
        p += Point2(ofs.x - 0.5f, ofs.y - 0.5f);

        int k;
        for (k = 0; k < tc.size(); ++k)
          if (tc[k] == p)
            break;

        if (k >= tc.size())
          tc.push_back(p);

        faces[i].t[j] = k;
      }
    }

    cb.writeTab(tc);
    cb.writeTab(faces);
  }

  cb.writeInt(obj.vlmIndex);
  cb.writeInt(obj.vlmSize);
  cb.writeTab(obj.vltmapFaces);

  cb.endBlock();
}


void StaticSceneBuilder::LtinputExporter::exportScene(IGenSave &cb, bool save_lightmaps, ILogWriter &log, int dtx_quality,
  IGenericProgressIndicator &pbar)
{
  if (save_lightmaps)
  {
    // lightmaps
    cb.beginBlock();
    cb.writeInt(MAKE4C('l', 'm', 'a', 'p'));

    cb.writeInt(totalLmSize.x);
    cb.writeInt(totalLmSize.y);
    cb.writeInt(vltmapSize);

    cb.writeInt(lightmaps.size());
    for (int i = 0; i < lightmaps.size(); ++i)
    {
      Lightmap &lm = lightmaps[i];
      cb.writeInt(lm.size.x);
      cb.writeInt(lm.size.y);
      cb.writeInt(lm.offset.x);
      cb.writeInt(lm.offset.y);
    }

    cb.endBlock();
  }

  // textures
  if (giTextures.size())
  {
    pbar.setActionDesc("Export to LT: textures");
    pbar.setDone(0);
    pbar.setTotal(giTextures.size());

    cb.beginBlock();
    cb.writeInt(MAKE4C('t', 'e', 'x', 's'));
    cb.writeInt(giTextures.size());

    for (int i = 0; i < giTextures.size(); ++i)
    {
      pbar.incDone();
      exportTexture(cb, giTextures[i], log, dtx_quality);
    }

    cb.endBlock();
  }

  // materials
  if (materials.size())
  {
    pbar.setActionDesc("Export to LT: materials");
    pbar.setDone(0);
    pbar.setTotal(materials.size());

    cb.beginBlock();
    cb.writeInt(MAKE4C('m', 'a', 't', 's'));
    cb.writeInt(materials.size());

    for (int i = 0; i < materials.size(); ++i)
    {
      pbar.incDone();
      exportMaterial(cb, materials[i], log);
    }

    cb.endBlock();
  }

  // objects
  if (objects.size())
  {
    pbar.setActionDesc("Export to LT: objects");
    pbar.setDone(0);
    pbar.setTotal(totalFaces);

    cb.beginBlock();
    cb.writeInt(MAKE4C('o', 'b', 'j', 's'));
    cb.writeInt(objects.size());

    for (int i = 0; i < objects.size(); ++i)
    {
      pbar.incDone(objects[i].mesh.getFaceCount());
      exportObject(cb, objects[i], log);
    }

    cb.endBlock();
  }
}

bool StaticSceneBuilder::export_to_light_tool(const char *ltinput, const char *lmsfile, int tex_quality, int lm_size,
  int target_lm_num, float pack_fidelity_ratio, StaticGeometryContainer &geom, StaticGeometryContainer &envi_geom,
  IAdditionalLTExport *add, IGenericProgressIndicator &pbar, ILogWriter &log, float &lm_scale)
{
  int i;
  for (i = 0; i < geom.nodes.size(); ++i)
  {
    if (!geom.nodes[i] || !(geom.nodes[i]->flags & StaticGeometryNode::FLG_RENDERABLE))
    {
      del_it(geom.nodes[i]);
      erase_items(geom.nodes, i--, 1);
    }
  }

  for (i = 0; i < envi_geom.nodes.size(); ++i)
  {
    if (!envi_geom.nodes[i] || !(envi_geom.nodes[i]->flags & StaticGeometryNode::FLG_RENDERABLE))
    {
      del_it(envi_geom.nodes[i]);
      erase_items(envi_geom.nodes, i--, 1);
    }
  }

  LtinputExporter scene, envi;

  scene.addGeom(geom.getNodes(), &log);
  envi.addGeom(envi_geom.getNodes(), &log);
  if (!scene.calcLightmapMapping(lm_size, target_lm_num, pack_fidelity_ratio, pbar, lm_scale))
    return false;

  scene.makeSingleLightmap();

  if (scene.objects.size() > 32767)
  {
    log.addMessage(ILogWriter::ERROR, "Objects count(%d) exceeded limit 32767.", scene.objects.size());
  }

  log.addMessage(ILogWriter::NOTE, "%d lightmaps, %d vltmap", scene.lightmaps.size(), scene.vltmapSize);
  log.addMessage(ILogWriter::NOTE, "%dx%d total lightmap size", scene.totalLmSize.x, scene.totalLmSize.y);

  log.addMessage(ILogWriter::NOTE, "scene: %d textures, %d GI textures, %d materials, %d objects", scene.textures.size(),
    scene.giTextures.size(), scene.materials.size(), scene.objects.size());

  log.addMessage(ILogWriter::NOTE, "envi:  %d textures, %d GI textures, %d materials, %d objects", envi.textures.size(),
    envi.giTextures.size(), envi.materials.size(), envi.objects.size());

  // generate unique 128-bit signature for this task
  DagUUID::generate(scene.uuid);
  if (!scene.save(lmsfile, pbar))
  {
    log.addMessage(ILogWriter::FATAL, "Error saving lightmapped scene '%s'", lmsfile);
    return false;
  }

  FullFileSaveCB cb(ltinput);
  if (!cb.fileHandle)
  {
    log.addMessage(ILogWriter::FATAL, "Can't create LTinput file '%s'", ltinput);
    return false;
  }

  try
  {
    cb.writeInt(_MAKE4C('LTi5'));
    cb.write(&scene.uuid, sizeof(scene.uuid)); // to bind with LMS1 and LTo2

    cb.beginBlock();

    cb.beginTaggedBlock(MAKE4C('e', 'n', 'v', 'i'));
    envi.exportScene(cb, false, log, tex_quality, pbar);
    cb.endBlock();

    cb.beginTaggedBlock(MAKE4C('s', 'c', 'e', 'n'));
    scene.exportScene(cb, true, log, tex_quality, pbar);
    cb.endBlock();
    if (add)
    {
      cb.beginTaggedBlock(MAKE4C('a', 'd', 'd', ' '));
      add->write(cb);
      cb.endBlock();
    }

    cb.endBlock();
  }
  catch (IGenSave::SaveException)
  {
    log.addMessage(ILogWriter::FATAL, "Error writing LT input file '%s'", ltinput);
    return false;
  }

  return true;
}

bool StaticSceneBuilder::export_envi_to_LMS1(StaticGeometryContainer &geom, const char *lms_file, IGenericProgressIndicator &pbar,
  ILogWriter &log)
{
  LtinputExporter envi;

  envi.addGeom(geom.getNodes(), &log);
  return envi.save(lms_file, pbar);
}
