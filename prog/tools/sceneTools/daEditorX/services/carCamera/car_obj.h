// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_simpleString.h>

class DagorAsset;
class IObjEntity;
class DynamicPhysObjectData;
class IPhysCar;
class TestDriver;

class IGenViewportWnd;

class VehicleViewer
{
public:
  VehicleViewer();
  ~VehicleViewer();


  virtual bool begin(IGenViewportWnd *_vpw);
  virtual bool end();

  virtual bool getSelectionBox(BBox3 &box) const;

  virtual void actObjects(float dt);

  virtual void beforeRenderObjects();
  virtual void renderObjects();
  virtual void renderTransObjects();
  virtual void renderInfo();

  virtual bool restartSim();

  void getCarTM(TMatrix &tm);
  IPhysCar *getCarObj() { return car; }

  bool isSimulation() { return nowSimulated; }

protected:
  virtual void renderInfoTest(const char *text, Point2 pos, E3DCOLOR color);

  SimpleString assetName;
  DagorAsset *dasset;
  IObjEntity *entity;
  DynamicPhysObjectData *dynPhysObjData;
  int physType;
  float simulationTime;
  bool nowSimulated;

  IPhysCar *car;
  TestDriver *driver;
  bool steady;
  bool rwd, fwd, rsa, fsa;

  IGenViewportWnd *vpw;
};
