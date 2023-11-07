#include "riGen/riGenData.h"

#include <gameRes/dag_gameResSystem.h>
#include <3d/dag_materialData.h>
#include <shaders/dag_rendInstRes.h>
#include <image/dag_texPixel.h>
#include <math/dag_mesh.h>
#include <generic/dag_tabWithLock.h>


static void build_stub_ri_mesh(Mesh &m)
{
  return; // don't generate mesh since this stub is not ShaderResUnitedVdata-compatible and cannot be rendered
  m.vert.resize(8);
  m.vert[0].set(-0.1, 0, 0.1);
  m.vert[1].set(-0.5, 1, 0.5);
  m.vert[2].set(0.5, 1, 0.5);
  m.vert[3].set(0.1, 0, 0.1);
  m.vert[4].set(-0.1, 0, -0.1);
  m.vert[5].set(0.1, 0, -0.1);
  m.vert[6].set(0.5, 1, -0.5);
  m.vert[7].set(-0.5, 1, -0.5);

  m.face.resize(12);
  m.face[0].set(2, 1, 0, 0, 0);
  m.face[1].set(0, 3, 2, 0, 0);
  m.face[2].set(6, 5, 4, 0, 0);
  m.face[3].set(4, 7, 6, 0, 0);
  m.face[4].set(5, 6, 2, 0, 0);
  m.face[5].set(2, 3, 5, 0, 0);
  m.face[6].set(5, 3, 0, 0, 0);
  m.face[7].set(0, 4, 5, 0, 0);
  m.face[8].set(7, 4, 0, 0, 0);
  m.face[9].set(0, 1, 7, 0, 0);
  m.face[10].set(6, 7, 1, 0, 0);
  m.face[11].set(1, 2, 6, 0, 0);

  m.tvert[0].resize(m.vert.size());
  for (int i = 0; i < m.vert.size(); i++)
    m.tvert[0][i].set_xz(m.vert[i]);

  m.tface[0].resize(m.face.size());
  for (int i = 0; i < m.face.size(); i++)
    for (int k = 0; k < 3; k++)
      m.tface[0][i].t[k] = m.face[i].v[k];

  m.calc_ngr();
  m.calc_vertnorms();
  for (int i = 0; i < m.vertnorm.size(); i++)
    m.vertnorm[i] = -m.vertnorm[i];
}

class StubRiReleasingFactory : public GameResourceFactory
{
  struct StubInstShaderMeshResource : public InstShaderMeshResource
  {
    StubInstShaderMeshResource(TEXTUREID tid)
    {
      resSize = 0;

      Mesh m;
      build_stub_ri_mesh(m);

      MaterialData md;
      md.className = "rendinst_simple";
      md.mtex[0] = tid;
      Ptr<ShaderMaterial> sm = new_shader_material(md);

      cubeSM = ShaderMesh::createSimple(m, sm);
      memcpy(&mesh, cubeSM, sizeof(mesh));
    }
    ~StubInstShaderMeshResource()
    {
      memset(&mesh, 0, sizeof(mesh));
      del_it(cubeSM);
    }

    ShaderMesh *cubeSM;
  };

  struct StubRendInst : public RenderableInstanceLodsResource
  {
    Lod lod0;
    struct StubLod : public RenderableInstanceResource
    {
      StubLod(InstShaderMeshResource *m)
      {
        memset(&rigid, 0, sizeof(rigid));
        addRef();
        rigid.mesh = m;
      }
      ~StubLod() { rigid.mesh = nullptr; }
    };
    StubRendInst(InstShaderMeshResource *m)
    {
      packedFields = sizeof(*this) - dumpStartOfs(); // Set all other fields to zero.
      extraFlags = 0;
      bbox[0].set(-0.5, 0, -0.5);
      bbox[1].set(0.5, 1, 0.5);
      bsphCenter.set(0, 0.5, 0);
      bsphRad = 1;
      bound0rad = 1;

      lods.init(&lod0, 1);
      lod0.scene.setPtr(new StubLod(m));
      lod0.range = 100;
    }
  };

public:
  StubRiReleasingFactory()
  {
    stubMesh = nullptr;
    stubTex = nullptr;
    stubTexId = BAD_TEXTUREID;
  }
  virtual ~StubRiReleasingFactory() {}
  virtual unsigned getResClassId() { return 0x12345678; } // non-existing class
  virtual const char *getResClassName() { return "*"; }   // non-existing class name
  virtual bool isResLoaded(int) { return false; }
  virtual bool checkResPtr(GameResource *gr)
  {
    if (!stubMesh.get())
      return false;
    TabWithLock<StubRendInst *>::AutoLock al(resList);
    return find_value_idx(resList, (StubRendInst *)gr) >= 0;
  }

  virtual GameResource *getGameResource(int) { return nullptr; }
  virtual bool addRefGameResource(GameResource *gr)
  {
    if (!stubMesh.get())
      return false;
    TabWithLock<StubRendInst *>::AutoLock al(resList);
    if (!checkResPtr(gr))
      return false;
    ((StubRendInst *)gr)->addRef();
    return true;
  }
  virtual void releaseGameResource(int) {}
  virtual bool releaseGameResource(GameResource *gr)
  {
    if (!stubMesh.get())
      return false;
    TabWithLock<StubRendInst *>::AutoLock al(resList);
    if (!checkResPtr(gr))
      return false;
    if (((StubRendInst *)gr)->getRefCount() == 1)
      erase_item_by_value(resList, (StubRendInst *)gr);
    ((StubRendInst *)gr)->delRef();
    return true;
  }
  virtual bool freeUnusedResources(bool, bool) { return false; }
  void loadGameResourceData(int, IGenLoad &) override {}
  void createGameResource(int, const int *, int) override {}
  virtual void reset() {}

  void init()
  {
    if (stubMesh)
      return;
    debug("rendInst: init stub mesh");
    TexImage32 image[2];
    image[0].w = image[0].h = 1;
    *(E3DCOLOR *)(image + 1) = E3DCOLOR(255, 0, 0, 255);
    stubTex = d3d::create_tex(image, 1, 1, TEXCF_RGB | TEXCF_LOADONCE | TEXCF_SYSTEXCOPY, 1, "RI:stubTex");
    d3d_err(stubTex);

    stubTexId = register_managed_tex("__ri_stub_tex#", stubTex);
    stubMesh = new StubInstShaderMeshResource(stubTexId);

    if (!find_gameres_factory(getResClassId()))
      add_factory(this);
    debug("rendInst: init stub mesh done");
  }
  void term()
  {
    if (!stubMesh)
      return;
    if (!resList.size() || stubMesh->getRefCount() == 1)
    {
      resList.clear();
      resList.shrink_to_fit();
      debug("rendInst: term stub mesh");
      stubMesh = nullptr;
      ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(stubTexId, stubTex);
      remove_factory(this);
    }
    else
      logerr("rendInst: cannot term stub mesh due to %d (%d) refs", resList.size(), stubMesh->getRefCount());
  }

  RenderableInstanceLodsResource *getStub()
  {
    if (!RendInstGenData::renderResRequired) // Note: This stub can't work without shaders
      return nullptr;
    if (!stubMesh)
      init();
    TabWithLock<StubRendInst *>::AutoLock al(resList);
    StubRendInst *gr = new StubRendInst(stubMesh);
    gr->addRef();
    resList.push_back(gr);
    logwarn("rendinst::get_stub_res used");
    return gr;
  }

public:
  Texture *stubTex;
  TEXTUREID stubTexId;
  Ptr<StubInstShaderMeshResource> stubMesh;
  TabWithLock<StubRendInst *> resList;
};

static StubRiReleasingFactory fac;

RenderableInstanceLodsResource *rendinst::get_stub_res() { return fac.getStub(); }
void rendinst::term_stub_res() { fac.term(); }
