//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class ObjectEditor;
class TMatrix;
class Point3;
class SqModules;
class EditableObject;
struct Frustum;


struct IDaEditor4EmbeddedComponent
{
  virtual ~IDaEditor4EmbeddedComponent() {}

  virtual void setStaticTraceRay(bool (*trace_ray)(const Point3 &p0, const Point3 &dir, float &dist, Point3 *out_norm)) = 0;
  virtual void setStaticTraceRayEx(
    bool (*trace_ray)(const Point3 &p0, const Point3 &dir, float &dist, EditableObject *exclude_obj, Point3 *out_norm)) = 0;
  virtual void setStaticPerformPointAction(void (*perform_point_action)(bool trace, const Point3 &p0, const Point3 &dir,
    const Point3 &traced_pos, const char *op, int mod)) = 0;
  virtual void setObjectEditor(ObjectEditor *ed) = 0;
  virtual void setActivationHandler(bool (*on_active_changed)(bool activate)) = 0;
  virtual void setUpdateHandler(void (*on_update)(float)) = 0;
  virtual void setChangedHandler(void (*on_state_changed)()) = 0;
  virtual bool isActive() const = 0;
  virtual bool isFreeCameraActive() const = 0;
  virtual const TMatrix *getCameraForcedItm() const = 0;

  virtual void act(float dt) = 0;
  virtual void beforeRender() = 0;
  virtual void render3d(const Frustum &, const Point3 &) = 0;
  virtual void renderUi() = 0;
};

IDaEditor4EmbeddedComponent *create_da_editor4(const char *mouse_cursor_texname);

void register_da_editor4_script(SqModules *module_mgr);

struct IDaEditor4StubEC : public IDaEditor4EmbeddedComponent
{
  virtual void setStaticTraceRay(bool (*)(const Point3 &, const Point3 &, float &, Point3 *)) {}
  virtual void setStaticTraceRayEx(bool (*)(const Point3 &, const Point3 &, float &, EditableObject *, Point3 *)) {}
  virtual void setStaticPerformPointAction(void (*)(bool, const Point3 &, const Point3 &, const Point3 &, const char *, int)) {}
  virtual void setObjectEditor(ObjectEditor *) {}
  virtual void setActivationHandler(bool (*)(bool activate)) {}
  virtual void setUpdateHandler(void (*)(float)) {}
  virtual void setChangedHandler(void (*)()) {}
  virtual bool isActive() const { return false; }
  virtual bool isFreeCameraActive() const override { return false; }
  virtual const TMatrix *getCameraForcedItm() const { return NULL; }

  virtual void act(float /*dt*/) {}
  virtual void beforeRender() {}
  virtual void render3d(const Frustum &, const Point3 &) {}
  virtual void renderUi() {}
};
