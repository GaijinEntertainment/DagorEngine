#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <ioSys/dag_genIo.h>
#include <generic/dag_tab.h>
#include <generic/dag_initOnDemand.h>
#include <3d/dag_materialData.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <gameRes/dag_dumpResRefCountImpl.h>
#include <osApiWrappers/dag_critSec.h>

/****************************************/
/****************************************/


class MaterialGameResFactory : public GameResourceFactory
{
public:
  struct MatData
  {
    int resId = -1;

    SimpleString className;
    SimpleString matScript;
    uint32_t flags = 0;
    D3dMaterial mat = D3dMaterial();
  };

  struct Material
  {
    int resId = -1;
    Ptr<MaterialData> mat;
  };

  Tab<MatData> matData;
  Tab<Material> matList;

  MaterialGameResFactory() : matData(midmem_ptr()), matList(midmem_ptr()) {}

  int findResData(int res_id)
  {
    for (int i = 0; i < matData.size(); ++i)
      if (matData[i].resId == res_id)
        return i;

    return -1;
  }

  int findRes(int res_id)
  {
    for (int i = 0; i < matList.size(); ++i)
      if (matList[i].resId == res_id)
        return i;

    return -1;
  }

  int findMatData(GameResource *mat)
  {
    for (int i = 0; i < matList.size(); ++i)
      if (matList[i].mat == (MaterialData *)mat)
        return i;

    return -1;
  }

  virtual unsigned getResClassId() { return MaterialGameResClassId; }

  virtual const char *getResClassName() { return "Material"; }

  virtual bool isResLoaded(int res_id) { return findRes(res_id) >= 0; }
  virtual bool checkResPtr(GameResource *res) { return findMatData(res) >= 0; }

  virtual GameResource *getGameResource(int res_id)
  {
    int id = findRes(res_id);
    if (id < 0)
      ::load_game_resource_pack(res_id);

    id = findRes(res_id);
    if (id < 0)
      return NULL;

    matList[id].mat.addRef();

    return (GameResource *)matList[id].mat.get();
  }

  virtual bool addRefGameResource(GameResource *resource)
  {
    int id = findMatData(resource);
    if (id >= 0)
      matList[id].mat.addRef();

    return id >= 0;
  }

  void releaseTextures(Material &material)
  {
    for (int i = 0; i < MAXMATTEXNUM; ++i)
      if (material.mat->mtex[i] != BAD_TEXTUREID)
      {
        release_managed_tex(material.mat->mtex[i]);
        material.mat->mtex[i] = BAD_TEXTUREID;
      }
  }

  void delRef(int id)
  {
    if (id < 0)
      return;

    G_ASSERT(matList[id].mat);
    G_ASSERT(matList[id].mat->getRefCount() >= 2);

    if (matList[id].mat->getRefCount() == 2)
      releaseTextures(matList[id]);

    matList[id].mat.delRef();
  }

  virtual void releaseGameResource(int res_id)
  {
    int id = findRes(res_id);
    delRef(id);
  }

  virtual bool releaseGameResource(GameResource *resource)
  {
    int id = findMatData(resource);
    delRef(id);
    return id >= 0;
  }

  virtual bool freeUnusedResources(bool forced_free_unref_packs, bool once /*= false*/)
  {
    bool result = false;

    for (int i = matList.size() - 1; i >= 0; --i)
    {
      if (!matList[i].mat || get_refcount_game_resource_pack_by_resid(matList[i].resId) > 0)
        continue;

      if (matList[i].mat->getRefCount() > 1)
      {
        if (!forced_free_unref_packs)
          continue;
        else
          FATAL_ON_UNLOADING_USED_RES(matList[i].resId, matList[i].mat->getRefCount());
      }

      releaseTextures(matList[i]);
      erase_items(matList, i, 1);
      result = true;
      if (once)
        break;
    }

    return result;
  }

  void readColor4(Color4 &color, IGenLoad &crd)
  {
    crd.readReal(color.r);
    crd.readReal(color.g);
    crd.readReal(color.b);
    crd.readReal(color.a);
  }

  void load(MatData &data, IGenLoad &crd)
  {
    crd.readString(data.className);
    crd.readString(data.matScript);
    data.flags = crd.readInt();

    crd.readReal(data.mat.power);
    readColor4(data.mat.diff, crd);
    readColor4(data.mat.amb, crd);
    readColor4(data.mat.spec, crd);
    readColor4(data.mat.emis, crd);
  }

  void loadGameResourceData(int res_id, IGenLoad &cb) override
  {
    WinAutoLock lock(get_gameres_main_cs());
    int id = findResData(res_id);
    if (id >= 0)
      return;

    MatData &mat = matData.push_back();
    lock.unlockFinal();
    load(mat, cb);
    mat.resId = res_id;
  }

  void createGameResource(int res_id, const int *reference_ids, int num_refs) override
  {
    WinAutoLock lock(get_gameres_main_cs());
    int id = findRes(res_id);
    if (id >= 0)
      return;

    String name;
    get_game_resource_name(res_id, name);

    id = findResData(res_id);
    if (id < 0)
    {
      logerr("BUG: no Material resource %s", name.str());
      return;
    }

    int gdi = append_items(matList, 1);
    lock.unlockFinal();

    matList[gdi].mat = new MaterialData();
    matList[gdi].mat->className = matData[id].className;
    matList[gdi].mat->matScript = matData[id].matScript;
    matList[gdi].mat->flags = matData[id].flags;
    matList[gdi].mat->mat = matData[id].mat;

    if (num_refs > MAXMATTEXNUM)
    {
      logerr("BUG: too many textures for material resource %s", name.str());
      num_refs = MAXMATTEXNUM;
    }

    for (int i = 0; i < num_refs; ++i)
    {
      String texName;
      ::get_game_resource_name(reference_ids[i], texName);
      matList[gdi].mat->mtex[i] = ::get_tex_gameres(texName.str());
    }

    matList[gdi].mat->matName = name;
    matList[gdi].resId = res_id;
  }

  virtual void reset()
  {
    matList.clear();
    matData.clear();
  }

  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(matList, resId, mat->getRefCount())
};


static InitOnDemand<MaterialGameResFactory> mat_factory;

void register_material_gameres_factory()
{
  mat_factory.demandInit();
  ::add_factory(mat_factory);
}
