#include <sceneBuilder/nsb_LightmappedScene.h>
#include <ioSys/dag_fileIo.h>
#include <util/dag_globDef.h>
#include "lmsPriv.h"


void StaticSceneBuilder::LightmappedScene::Object::save(IGenSave &cb, LightmappedScene &scene)
{
  cb.writeString(name);
  cb.writeInt(flags);
  cb.writeReal(visRange);
  cb.writeInt(transSortPriority);

  cb.writeInt(topLodObjId);
  cb.writeReal(lodRange);

  cb.writeReal(lodNearVisRange);
  cb.writeReal(lodFarVisRange);

  cb.writeInt(mats.size());
  for (int i = 0; i < mats.size(); ++i)
    cb.writeInt(scene.getMaterialId(mats[i]));

  mesh.saveData(cb);

  cb.writeTab(lmTc);
  cb.writeTab(lmFaces);
  cb.writeTab(lmIds);

  cb.writeInt(vlmIndex);
  cb.writeInt(vlmSize);
  cb.writeTab(vltmapFaces);

  cb.writeReal(lmSampleSize);
}


void StaticSceneBuilder::LightmappedScene::Lightmap::save(IGenSave &cb)
{
  cb.writeInt(size.x);
  cb.writeInt(size.y);
  cb.writeInt(offset.x);
  cb.writeInt(offset.y);
}


void StaticSceneBuilder::LightmappedScene::save(IGenSave &cb, IGenericProgressIndicator &pbar)
{
  pbar.setActionDesc("Saving lightmapped scene");
  pbar.setDone(0);
  pbar.setTotal(1 + 1 + objects.size() + 1);

  cb.writeInt(MAKE4C('L', 'M', 'S', '1'));
  cb.writeInt(VERSION_NUMBER);
  cb.write(&uuid, sizeof(uuid)); // to bind with LTi3 and LTo2

  // save textures
  pbar.incDone();
  cb.writeInt(textures.size());
  for (int i = 0; i < textures.size(); ++i)
    if (textures[i])
      cb.writeString(textures[i]->fileName);

  // save materials
  pbar.incDone();
  cb.writeInt(materials.size());
  for (int i = 0; i < materials.size(); ++i)
    materials[i]->save(cb, *this);

  // save objects
  cb.writeInt(objects.size());
  for (int i = 0; i < objects.size(); ++i)
  {
    pbar.incDone();
    objects[i].save(cb, *this);
  }

  // save lightmaps
  pbar.incDone();
  cb.writeInt(lightmaps.size());
  for (int i = 0; i < lightmaps.size(); ++i)
    lightmaps[i].save(cb);

  cb.writeInt(vltmapSize);
  cb.writeInt(totalLmSize.x);
  cb.writeInt(totalLmSize.y);

  cb.writeReal(defLmSampleSize);
  cb.writeReal(lmSampleSizeScale);
}


bool StaticSceneBuilder::LightmappedScene::save(const char *filename, IGenericProgressIndicator &pbar)
{
  FullFileSaveCB cb(filename);
  if (!cb.fileHandle)
    return false;

  try
  {
    save(cb, pbar);
  }
  catch (IGenSave::SaveException)
  {
    return false;
  }

  return true;
}
