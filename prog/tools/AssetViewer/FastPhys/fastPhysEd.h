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
  ~FastPhysEditor();

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

  virtual UndoSystem *getUndoSystem() { return undoSystem; }

  // ObjectEditor
  virtual void update(real dt);
  virtual void render();

  virtual void selectionChanged();
  virtual void onClick(int pcb_id, PropPanel2 *panel);

  void refillPanel();
  void updateToolbarButtons();
  void fillToolBar(PropPanel2 *toolbar);
  void addButton(PropPanel2 *toolbar, int id, const char *bmp_name, const char *hint, bool check = false);

  void setWind(const Point3 &vel, float power, float turb);
  void getWind(Point3 &vel, float &power, float &turb);

  virtual void removeObjects(RenderableEditableObject **obj, int num, bool use_undo = true);
  // void removeDeadActions(FpdAction *action);

  // IFpdLoad implementation
  virtual FpdAction *loadAction(const DataBlock &blk) { return load_action(blk, *this); }

  GeomNodeTree &getGeomTree() override { return nodeTree; }
  virtual IFPObject *getWrapObjectByName(const char *name);
  virtual FpdObject *getEdObjectByName(const char *name);

  virtual FpdPoint *getPointByName(const char *name);
  virtual FpdBone *getBoneByName(const char *name);

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

  void updateSelection();

  void save(DataBlock &blk);
  void exportBinaryDump();
  void export_fast_phys(mkbindump::BinDumpSaveCB &cb);
  void revert();
  bool loadSkeleton(DagorAssetMgr &mgr, SimpleString sa_name, ILogWriter &log);
  bool loadCharacter(DagorAssetMgr &mgr, SimpleString sa_name, ILogWriter &log);
};

//------------------------------------------------------------------
