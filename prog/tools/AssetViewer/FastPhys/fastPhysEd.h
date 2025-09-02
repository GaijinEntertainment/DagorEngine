// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <anim/dag_animDecl.h>

#include <EditorCore/ec_ObjectEditor.h>

#include <assets/assetMgr.h>

#include <libTools/fastPhysData/fp_data.h>

class FastPhysPlugin;


//------------------------------------------------------------------

class FastPhysSystem;
class IFPObject;
class TreeBaseWindow;
class FastPhysCharRoot;

class FastPhysEditor : public ObjectEditor, public IFpdLoad
{
public:
  PtrTab<FpdPoint> points;
  Ptr<FpdAction> initAction, updateAction;
  GeomNodeTree nodeTree;

  bool selProcess;
  bool isSimulationActive, isSimDebugVisible, isObjectDebugVisible, isSkeletonVisible;

public:
  FastPhysEditor(FastPhysPlugin &plugin);
  ~FastPhysEditor() override;

  void clearAll();

  IFPObject *addObject(FpdObject *o);
  void load(const DagorAsset &asset);
  void saveResource();

  static bool hasSubAction(FpdContainerAction *container, FpdAction *action);
  static FpdContainerAction *getContainerAction(FpdAction *action);

  void onCharWtmChanged();

  void startSimulation();
  void stopSimulation();
  void renderGeometry(int stage);

  void renderSkeleton();

  UndoSystem *getUndoSystem() override { return undoSystem; }

  // ObjectEditor
  void update(real dt) override;
  void render() override;

  void selectionChanged() override;
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  void refillPanel();
  void updateToolbarButtons() override;
  void fillToolBar(PropPanel::ContainerPropertyControl *toolbar) override;
  void addButton(PropPanel::ContainerPropertyControl *toolbar, int id, const char *bmp_name, const char *hint,
    bool check = false) override;

  void setWind(const Point3 &vel, float power, float turb);
  void getWind(Point3 &vel, float &power, float &turb);

  void removeObjects(RenderableEditableObject **obj, int num, bool use_undo = true) override;
  // void removeDeadActions(FpdAction *action);

  // IFpdLoad implementation
  FpdAction *loadAction(const DataBlock &blk) override { return load_action(blk, *this); }

  GeomNodeTree &getGeomTree() override { return nodeTree; }
  FpdObject *getEdObjectByName(const char *name) override;

  IFPObject *getWrapObjectByName(const char *name);
  FpdPoint *getPointByName(const char *name);
  FpdBone *getBoneByName(const char *name);

  IFPObject *getSelObject();

  bool showNodeList(Tab<int> &sels);

  IFPObject *createPoint(const char *from_name, const Point3 &pos, dag::Index16 node);
  void createPointsAtPivots();

  void linkSelectedPointsWithBone();

  void createClippersAtPivots();
  IFPObject *createClipper(const char *from_name, const Point3 &pos, dag::Index16 node);

  AnimV20::IAnimCharacter2 *getAnimChar() const;

  TMatrix getRootWtm();

protected:
  FastPhysSystem *physSystem;
  UndoSystem *undoSystem;
  Ptr<class FastPhysCharRoot> charRoot;
  SmallTab<mat44f, TmpmemAlloc> savedNodeTms;

  FastPhysPlugin &mPlugin;

  SimpleString mSkeletonName;

  void updateSelection() override;

  void save(DataBlock &blk);
  void exportBinaryDump();
  void export_fast_phys(mkbindump::BinDumpSaveCB &cb);
  void revert();
  bool loadSkeleton(DagorAssetMgr &mgr, SimpleString sa_name, ILogWriter &log);
  bool loadCharacter(DagorAssetMgr &mgr, SimpleString sa_name, ILogWriter &log);
};

//------------------------------------------------------------------
