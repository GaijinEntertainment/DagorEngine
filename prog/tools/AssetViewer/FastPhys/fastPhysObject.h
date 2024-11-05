// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/c_control_event_handler.h>

#include <EditorCore/ec_rendEdObject.h>

class FpdObject;
class FastPhysEditor;

//------------------------------------------------------------------

class IFPObject : public RenderableEditableObject, public PropPanel::ControlEventHandler
{
public:
  static constexpr unsigned HUID = 0x8DCB76A6u; // IFPObject

  static E3DCOLOR frozenColor;

public:
  IFPObject(FpdObject *obj, FastPhysEditor &editor);
  virtual ~IFPObject();

  FpdObject *getObject() const { return mObject.get(); }
  FastPhysEditor *getObjEditor() const;

  static IFPObject *createFPObject(FpdObject *obj, FastPhysEditor &editor); // factory for wrapper object

  virtual void refillPanel(PropPanel::ContainerPropertyControl *panel) = 0;

  // RenderableEditableObject

  virtual void update(real dt) {}
  virtual void beforeRender() {}
  virtual void renderTrans() {}

  virtual void render() {}
  virtual bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const { return true; }
  virtual bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const { return true; }
  virtual bool getWorldBox(BBox3 &box) const { return true; }
  virtual void onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
    dag::ConstSpan<RenderableEditableObject *> objects){};

  EO_IMPLEMENT_RTTI(HUID)

protected:
  Ptr<FpdObject> mObject;
  FastPhysEditor &mFPEditor;
};


//------------------------------------------------------------------
