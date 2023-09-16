#include <sceneBuilder/nsb_LightmappedScene.h>
#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/staticGeom/matFlags.h>
#include <libTools/util/de_TextureName.h>
#include <libTools/dagFileRW/textureNameResolver.h>
#include <util/dag_texMetaData.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_fileIo.h>
#include <debug/dag_debug.h>

void StaticSceneBuilder::LightmappedScene::Object::load(IGenLoad &cb, LightmappedScene &scene, int version)
{
  cb.readString(name);
  cb.readInt(flags);
  vssDivCount = 0; //== clear on load
  cb.readReal(visRange);
  cb.readInt(transSortPriority);

  cb.readInt(topLodObjId);
  cb.readReal(lodRange);

  if (version >= 4)
  {
    cb.readReal(lodNearVisRange);
    cb.readReal(lodFarVisRange);
  }
  else
  {
    lodNearVisRange = -1;
    lodFarVisRange = -1;
  }

  mats.resize(cb.readInt());
  for (int i = 0; i < mats.size(); ++i)
  {
    const int id = cb.readInt();
    if (id >= 0 && id < scene.materials.size())
    {
      mats[i] = scene.materials[id];

      if (mats[i]->flags & MatFlags::FLG_BILLBOARD_MASK)
        flags |= ObjFlags::FLG_BILLBOARD_MESH;
    }
  }

  mesh.loadData(cb);

  cb.readTab(lmTc);
  cb.readTab(lmFaces);
  cb.readTab(lmIds);

  cb.readInt(vlmIndex);
  cb.readInt(vlmSize);
  cb.readTab(vltmapFaces);

  cb.readReal(lmSampleSize);
}

void StaticSceneBuilder::LightmappedScene::Lightmap::load(IGenLoad &cb)
{
  cb.readInt(size.x);
  cb.readInt(size.y);
  cb.readInt(offset.x);
  cb.readInt(offset.y);
}


void StaticSceneBuilder::LightmappedScene::load(IGenLoad &cb, IGenericProgressIndicator &pbar)
{
  if (cb.readInt() != MAKE4C('L', 'M', 'S', '1'))
    throw IGenLoad::LoadException("not LMS1", cb.tell());

  int version = cb.readInt();
  // if (version != VERSION_NUMBER)
  //   throw IGenLoad::LoadException("incorrect ver", cb.tell());
  if (version < 3 || version > VERSION_NUMBER)
    throw IGenLoad::LoadException("incorrect ver", cb.tell());

  clear();
  cb.read(&uuid, sizeof(uuid)); // to bind with LTi3 and LTo2

  pbar.setActionDesc("Loading lightmapped scene");

  // load textures
  textures.resize(cb.readInt());
  for (int i = 0; i < textures.size(); ++i)
  {
    String src_fn, fn;
    ITextureNameResolver *resv = get_global_tex_name_resolver();
    cb.readString(src_fn);
    if (!resv || !resv->resolveTextureName(src_fn, fn))
      fn = src_fn;

    const char *fname = TextureMetaData::decodeFileName(fn);
    if (!trail_strcmp(fn, "*") && !dd_file_exist(fname))
    {
      debug("can't find texture [%s]", fn.str());
      pbar.setActionDesc(String(512, "can't find texture [%s]", fn.str()));
      // throw IGenLoad::LoadException(String(512, "can't find tetxure [%s]", (const char*)fn));
      textures[i] = NULL;
    }
    else
      textures[i] = new StaticGeometryTexture(dd_get_fname(fname));
  }

  // load materials
  materials.resize(cb.readInt());
  for (int i = 0; i < materials.size(); ++i)
  {
    materials[i] = new StaticGeometryMaterial;
    materials[i]->load(cb, *this);
    if (materials[i]->textures[0])
      addGiTexture(materials[i]->textures[0]);
  }

  // load objects
  objects.resize(cb.readInt());
  pbar.setDone(0);
  pbar.setTotal(objects.size());
  for (int i = 0; i < objects.size(); ++i)
  {
    pbar.incDone();
    objects[i].load(cb, *this, version);
    totalFaces += objects[i].mesh.getFaceCount();
  }

  // load lightmaps
  lightmaps.resize(cb.readInt());
  for (int i = 0; i < lightmaps.size(); ++i)
    lightmaps[i].load(cb);

  cb.readInt(vltmapSize);
  cb.readInt(totalLmSize.x);
  cb.readInt(totalLmSize.y);

  cb.seekrel(4); // defaultLmSampleSize;
  cb.readReal(lmSampleSizeScale);
}


bool StaticSceneBuilder::LightmappedScene::load(const char *filename, IGenericProgressIndicator &pbar)
{
  FullFileLoadCB cb(filename);
  if (!cb.fileHandle)
  {
    debug("Cannot open file %s", filename);
    return false;
  }

  try
  {
    load(cb, pbar);
  }
  catch (IGenLoad::LoadException &e)
  {
    debug("LoadException: %s; %s", e.excDesc, DAGOR_EXC_STACK_STR(e));
    return false;
  }

  return true;
}
