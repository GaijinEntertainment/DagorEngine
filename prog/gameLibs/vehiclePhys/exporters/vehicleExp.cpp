// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/daBuildExpPluginChain.h>
#include <assets/assetPlugin.h>
#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <vehiclePhys/physCarGameRes.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <math/dag_boundingSphere.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_string.h>

BEGIN_DABUILD_PLUGIN_NAMESPACE(vehicle)

class VehicleDescExporter : public IDagorAssetExporter
{
public:
  virtual const char *__stdcall getExporterIdStr() const { return "vehicleDesc exp"; }

  virtual const char *__stdcall getAssetType() const { return "vehicleDesc"; }
  virtual unsigned __stdcall getGameResClassId() const { return VehicleDescGameResClassId; }
  virtual unsigned __stdcall getGameResVersion() const { return 3; }

  virtual void __stdcall onRegister() {}
  virtual void __stdcall onUnregister() {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override { files.clear(); }

  virtual bool __stdcall isExportableAsset(DagorAsset &a) { return true; }

  virtual bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log)
  {
    PhysCarSettings2 cd;

    // init with defaults
    cd.frontSusp.upperPt = Point3(1, 0.15, 2);
    cd.frontSusp.springAxis = Point3(0, -1, 0);
    cd.frontSusp.fixPtOfs = 0.15;
    cd.frontSusp.upTravel = 0.10;
    cd.frontSusp.maxTravel = 0.20;
    cd.frontSusp.springK = 40000;
    cd.frontSusp.springKdUp = 7000;
    cd.frontSusp.springKdDown = 7000;
    cd.frontSusp.arbK = 0;
    cd.frontSusp.wRad = 0.4;
    cd.frontSusp.wMass = 16.0;
    cd.frontSusp.flags = cd.frontSusp.FLG_POWERED | cd.frontSusp.FLG_STEERED;

    cd.rearSusp = cd.frontSusp;
    cd.rearSusp.upperPt.z = -2.5;
    cd.rearSusp.flags = 0;

    // read settings from BLK
    const DataBlock &blk = *a.props.getBlockByNameEx("suspension");
    Point3 springAxis = blk.getPoint3("springAxis", cd.frontSusp.springAxis);
    cd.logicTm = blk.getTm("logicTm", TMatrix::IDENT);

    PhysCarSuspensionParams *s[2] = {&cd.frontSusp, &cd.rearSusp};
    const DataBlock *b[2] = {blk.getBlockByName("front"), blk.getBlockByName("rear")};
    for (int i = 0; i < 2; i++)
    {
      if (!b[i])
        continue;

      Point2 slim = b[i]->getPoint2("susp_limits", Point2(0.25, 0.45));
      Point3 fixpt = b[i]->getPoint3("fixPt", s[i]->upperPt);
      float w_nat = b[i]->getReal("spring_w_nat", 0);

      s[i]->wRad = b[i]->getReal("wheelRad", s[i]->wRad);
      s[i]->wMass = b[i]->getReal("wheelMass", s[i]->wMass);

      s[i]->springAxis = b[i]->getPoint3("springAxis", springAxis);
      s[i]->upperPt = b[i]->getPoint3("upperPt", fixpt + (slim[0] - s[i]->wRad) * springAxis);
      s[i]->fixPtOfs = b[i]->getReal("fixPtOfs", -(slim[0] - s[i]->wRad));
      s[i]->maxTravel = b[i]->getReal("maxTravel", slim[1] - slim[0]);
      s[i]->upTravel = b[i]->getReal("upTravel", s[i]->maxTravel / 2);

      if (w_nat > 0)
      {
        s[i]->springK = w_nat;
        s[i]->setSpringKAsW0(true);
      }
      else
        s[i]->springK = b[i]->getReal("spring_k", s[i]->springK);
      s[i]->springKdUp = b[i]->getReal("spring_kd_up", s[i]->springKdUp);
      s[i]->springKdDown = b[i]->getReal("spring_kd_down", s[i]->springKdDown);
      s[i]->arbK = b[i]->getReal("anti_roll_bar_k", s[i]->arbK);

      s[i]->setPowered(b[i]->getBool("powered", s[i]->powered()));
      s[i]->setControlled(b[i]->getBool("steered", s[i]->controlled()));
      s[i]->setRigidAxle(b[i]->getBool("rigid_axle", s[i]->rigidAxle()));
    }

    // write resource
    cwr.writeInt32e(cd.RES_VERSION);
    cwr.write32ex(&cd, sizeof(cd));
    return true;
  }
};

class VehicleExporter : public IDagorAssetExporter
{
public:
  virtual const char *__stdcall getExporterIdStr() const { return "vehicle exp"; }

  virtual const char *__stdcall getAssetType() const { return "vehicle"; }
  virtual unsigned __stdcall getGameResClassId() const { return VehicleGameResClassId; }
  virtual unsigned __stdcall getGameResVersion() const { return 4; }

  virtual void __stdcall onRegister() {}
  virtual void __stdcall onUnregister() {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override { files.clear(); }

  virtual bool __stdcall isExportableAsset(DagorAsset &a) { return true; }

  virtual bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log) { return true; }
};

class VehicleRefs : public IDagorAssetRefProvider
{
public:
  virtual const char *__stdcall getRefProviderIdStr() const { return "vehicle refs"; }
  virtual const char *__stdcall getAssetType() const { return "vehicle"; }

  virtual void __stdcall onRegister() {}
  virtual void __stdcall onUnregister() {}

  void __stdcall getAssetRefs(DagorAsset &a, Tab<Ref> &refs) override
  {
    static constexpr int REF_NUM = 4;
    static const char *atype_nm[REF_NUM] = {"physObj", "dynModel", "dynModel", "vehicleDesc"};
    static const char *ref_nm[REF_NUM] = {"physObj", "frontWheel", "rearWheel", "vehicleDesc"};
    static int atype[REF_NUM] = {-2};

    if (atype[0] == -2)
      for (int i = 0; i < REF_NUM; i++)
        atype[i] = a.getMgr().getAssetTypeId(atype_nm[i]);

    refs.clear();
    for (int i = 0; i < REF_NUM; i++)
    {
      if (atype[i] < 0)
      {
        IDagorAssetRefProvider::Ref &r = refs.push_back();
        r.flags = RFLG_EXTERNAL | RFLG_OPTIONAL;
        r.setBrokenRef("");
        continue;
      }

      const char *r_name = a.props.getStr(ref_nm[i], "");
      DagorAsset *ra = a.getMgr().findAsset(r_name, atype[i]);

      IDagorAssetRefProvider::Ref &r = refs.push_back();
      r.flags = RFLG_EXTERNAL;
      if (!ra)
        r.setBrokenRef(String(64, "%s:%s", r_name, atype_nm[i]));
      else
        r.refAsset = ra;
    }
  }
};


class VehicleExporterPlugin : public IDaBuildPlugin
{
public:
  virtual bool __stdcall init(const DataBlock &appblk) { return true; }
  virtual void __stdcall destroy() { delete this; }

  virtual int __stdcall getExpCount() { return 2; }
  virtual const char *__stdcall getExpType(int idx)
  {
    switch (idx)
    {
      case 0: return "vehicleDesc";
      case 1: return "vehicle";
      default: return NULL;
    }
  }
  virtual IDagorAssetExporter *__stdcall getExp(int idx)
  {
    switch (idx)
    {
      case 0: return &expDesc;
      case 1: return &exp;
      default: return NULL;
    }
  }

  virtual int __stdcall getRefProvCount() { return 1; }
  virtual const char *__stdcall getRefProvType(int idx) { return getExpType(idx); }
  virtual IDagorAssetRefProvider *__stdcall getRefProv(int idx) { return idx == 0 ? &refs : NULL; }

protected:
  VehicleDescExporter expDesc;
  VehicleExporter exp;
  VehicleRefs refs;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) VehicleExporterPlugin; }
END_DABUILD_PLUGIN_NAMESPACE(vehicle)
REGISTER_DABUILD_PLUGIN(vehicle, nullptr)
