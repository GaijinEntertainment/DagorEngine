// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_object.h>
#include <EditorCore/ec_interface.h>

#include <libTools/util/undo.h>

#include <propPanel/control/container.h>


class ObjectEditor;

struct EcRect;


// Hit-test helper functions.
bool is_sphere_hit_by_point(const Point3 &world_center, real world_rad, IGenViewportWnd *vp, int x, int y);

bool is_sphere_hit_by_rect(const Point3 &world_center, real world_rad, IGenViewportWnd *vp, const EcRect &rect);

real get_world_rad(const Point3 &size);


// RenderableEditableObject class ID
static const int CID_RenderableEditableObject = 0x6437E6D5;


/// Base class for Editor objects with render, undo / redo etc possibilities.
/// Has rendering functions, parameters set methods, some undo / redo
/// operations, base operations for working with #PropertyPanel.
/// Generally used by class #ObjectEditor.
/// @ingroup EditorCore
/// @ingroup EditorBaseClasses
/// @ingroup EditableObject
class RenderableEditableObject : public EditableObject
{
public:
  EO_IMPLEMENT_RTTI_EX(CID_RenderableEditableObject, EditableObject)

  /// Object flags.
  enum RenderableEditableObjectFlags
  {
    FLG_SELECTED = 1 << 0,     ///< Object is selected.
    FLG_HIDDEN = 1 << 1,       ///< Object is hidden.
    FLG_FROZEN = 1 << 2,       ///< Object is "frozen".
    FLG_HIGHLIGHTED = 1 << 3,  ///< Object is highlighted.
    FLG_WANTRESELECT = 1 << 4, ///< Object wants receive every select (even if already selected)
  };

  /// Constructor 1.
  RenderableEditableObject() : objFlags(0), objEditor(NULL) {}

  /// Constructor 2.
  RenderableEditableObject(const RenderableEditableObject *fo) : EditableObject(*fo), objFlags(fo->objFlags), objEditor(NULL) {}

  /// Destructor.
  virtual ~RenderableEditableObject();

  /// Remove object from #ObjectEditor.
  void removeFromEditor();

  //*****************************************************************
  /// @name Render methods.
  //@{
  /// Object's act stage.
  /// If object plays animation or performs some actions on itself it
  /// does that in this method.
  /// @param[in] dt - the time passed from last call to update()
  virtual void update(real dt) = 0;

  /// The function is called before render().
  /// In this method object prepares for rendering if needed.
  virtual void beforeRender() = 0;

  /// Render object.
  /// Rendered objects may be in selected and frozen status only,
  /// render() is not called for hidden objects.
  virtual void render() = 0;

  /// Render transparent object elements.
  virtual void renderTrans() = 0;
  //@}

  //*****************************************************************
  /// @name Flags get / set methods.
  /// see. enum RenderableEditableObject::RenderableEditableObjectFlags
  //@{
  /// Test whether the object is hidden.
  /// @return @b true if object is hidden, @b false in other case
  virtual bool isHidden() const { return objFlags & FLG_HIDDEN; }

  /// Test whether the object is frozen.
  /// @return @b true if object is frozen, @b false in other case
  virtual bool isFrozen() const { return objFlags & FLG_FROZEN; }

  /// Test whether the object is selected.
  /// @return @b true if object is selected, @b false in other case
  inline bool isSelected() const { return objFlags & FLG_SELECTED; }

  /// Test whether the object is highlighted.
  /// @return @b true if object is highlighted, @b false in other case
  inline bool isHighlighted() const { return objFlags & FLG_HIGHLIGHTED; }


  /// Get object flags.
  /// @return object flags
  int getFlags() const { return objFlags; }

  /// Set object flags.
  /// @param[in] value - flags
  /// @param[in] mask - mask used
  /// @param[in] use_undo - if @b true a corresponding record will be created
  ///                       in undo system
  virtual void setFlags(int value, int mask, bool use_undo = true);

  /// Hide / show object.
  /// @param[in] hide - <b>true / false</b>: hide / show object
  virtual void hideObject(bool hide = true) { setFlags(hide ? FLG_HIDDEN : 0, FLG_HIDDEN | (hide ? FLG_SELECTED : 0)); }

  /// Freeze / unfreeze object.
  /// @param[in] freeze - <b>true / false: freeze / unfreeze</b> object
  virtual void freezeObject(bool freeze = true) { setFlags(freeze ? FLG_FROZEN : 0, FLG_FROZEN | (freeze ? FLG_SELECTED : 0)); }

  /// Set object highlighting on / off.
  /// @param[in] highlight - <b>true / false</b>: set object highlight
  ///                        <b>on / off</b>
  virtual void highlightObject(bool highlight = true) { setFlags(highlight ? FLG_HIGHLIGHTED : 0, FLG_HIGHLIGHTED); }

  /// Set object selection on / off.
  /// @param[in] select - <b>true / false</b>: set object selection
  ///                     <b>on / off</b>
  virtual void selectObject(bool select = true) { setFlags(select ? FLG_SELECTED : 0, FLG_SELECTED); }
  //@}

  //*****************************************************************
  /// @name Object properties get / set methods
  //@{
  /// Test whether part of the object is inside rectangle.
  /// @param[in] vp - pointer to viewport where test is performed
  /// @param[in] rect - rectangle
  /// @return @b true if object is inside rectangle, @b false in other case
  virtual bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const = 0;

  /// Get object bounding box.
  /// @param[out] box - object bounding box
  /// @return @b true if object has bounding box
  virtual bool getWorldBox(BBox3 &box) const = 0;

  /// Set object name.
  /// @param[in] nm - pointer to name string
  /// @return @b true if operation successful, @b false in other case
  virtual bool setName(const char *nm)
  {
    name = nm;
    return true;
  }

  /// Set object position.
  /// @param[in] p - object position
  /// @return @b true if operation successful, @b false in other case
  virtual bool setPos(const Point3 &p);

  /// Set object scaling in local X,Y,Z-directions.
  /// @param[in] p - object scalings
  /// @return @b true if operation successful, @b false in other case
  virtual bool setSize(const Point3 &p);

  /// Get object scaling in local X,Y,Z-directions.
  /// @return object scalings
  virtual Point3 getSize() const;

  /// Rotate object (with #Matrix3).
  /// @param[in] tm - rotation matrix (see #Matrix3)
  /// @return @b true if operation successful, @b false in other case
  virtual bool setRotM3(const Matrix3 &tm);
  /// Rotate object (with #TMatrix).
  /// @param[in] tm - rotation matrix (see #TMatrix)
  /// @return @b true if operation successful, @b false in other case
  virtual bool setRotTm(const TMatrix &tm);

  /// Get object rotate matrix (#Matrix3).
  /// @return object rotate matrix (#Matrix3)
  virtual Matrix3 getRotM3() const;
  /// Get object rotate matrix (#TMatrix).
  /// @return object rotate matrix (#TMatrix)
  virtual TMatrix getRotTm() const;

  /// Get object matrix (#Matrix3).
  /// @return object matrix (#Matrix3)
  virtual Matrix3 getMatrix() const;
  /// Set object matrix (from #Matrix3).
  /// @param[in] tm - new matrix
  virtual void setMatrix(const Matrix3 &tm);

  /// Get object matrix (#TMatrix).
  /// return object matrix (#TMatrix)
  virtual const TMatrix &getWtm() const { return matrix; }
  /// Set object matrix (#TMatrix).
  /// @param[in] wtm - new matrix
  virtual void setWtm(const TMatrix &wtm);
  //@}

  //*****************************************************************
  /// @name Object edit methods.
  //@{
  /// Move object.
  /// @param[in] delta - object movement relative to current position
  /// @param[in] basis - Gizmo basis (see #IEditorCoreEngine::BasisType)
  virtual void moveObject(const Point3 &delta, IEditorCoreEngine::BasisType basis);

  /// Move object over surface.
  /// @param[in] delta - object movement relative to current position
  /// @param[in] basis - Gizmo basis (see #IEditorCoreEngine::BasisType)
  virtual void moveSurfObject(const Point3 &delta, IEditorCoreEngine::BasisType basis);

  /// Rotate object.
  /// @param[in] delta - object rotation in Euler angles relative to
  ///                    current position
  /// @param[in] origin - center of rotation (if Gizmo's basis != BASIS_Local)
  /// @param[in] basis - Gizmo basis (see #IEditorCoreEngine::BasisType)
  virtual void rotateObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis);

  /// Scale object.
  /// @param[in] delta - object scalings relative to current sizes
  /// @param[in] origin - center of scaling (if Gizmo's basis != BASIS_Local)
  /// @param[in] basis - Gizmo basis (see #IEditorCoreEngine::BasisType)
  virtual void scaleObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis);
  //@}

  //*****************************************************************
  /// @name Undo / redo methods.
  //@{
  /// Put 'undo move' data to undo system.
  virtual void putMoveUndo();
  /// Put 'undo rotate' data to undo system.
  virtual void putRotateUndo();
  /// Put 'undo scale' data to undo system.
  virtual void putScaleUndo();
  //@}


  /// Test whether object may be deleted.
  /// @return @b true if object may be deleted, @b false in other case
  virtual bool mayDelete() { return true; }

  /// Test whether object may be renamed.
  /// @return @b true if object may be renamed, @b false in other case
  virtual bool mayRename() { return true; }

  /// Called when renaming object is performed.
  /// @param[in] obj - pointer to object to be renamed
  /// @param[in] old_name - pointer to old name string
  /// @param[in] new_name - pointer to new name string
  virtual void onObjectNameChange(RenderableEditableObject *obj, const char *old_name, const char *new_name) {}

  /// Get ObjectEditor.
  /// @return pointer to ObjectEditor instance whose functions the object uses
  ObjectEditor *getObjEditor() const { return objEditor; }

  //*****************************************************************
  /// @name Property Panel methods.
  //@{
  /// RTTI method\n
  /// Get Common Class Id.
  /// @param[in] objects - array of pointers to objects
  /// @param[in] num - number of objects in array
  /// @return @b CID_RenderableEditableObject if all objects are of class
  /// #RenderableEditableObject or derived from it, @b NullCID in other case
  virtual DClassID getCommonClassId(RenderableEditableObject **objects, int num);

  /// Property Panel common group and property PID values
  enum
  {
    PID_COMMON_GROUP = 100000,
    PID_TRANSFORM_GROUP = 200000,
    PID_SEED_GROUP = 300000,
  };

  /// Fill Property Panel with object properties.
  /// Used to place object (group of objects) properties on Property Panel.
  /// @param[in] panel - Property Panel
  /// @param[in] for_class_id - class ID of object(s)
  /// @param[in] objects - array of pointers to objects
  virtual void fillProps(PropPanel::ContainerPropertyControl &panel, DClassID for_class_id,
    dag::ConstSpan<RenderableEditableObject *> objects);

  /// Called when parameter changing on Property Panel is performed.
  /// @param[in] pid - parameter ID
  /// @param[in] edit_finished - @b true if edit finished, @b false in other case
  /// @param[in] panel - Property Panel
  /// @param[in] objects - array of pointers to objects
  virtual void onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
    dag::ConstSpan<RenderableEditableObject *> objects) = 0;

  /// Called when button 'Close' on Property Panel is pressed.
  /// @param[in] panel - Property Panel
  /// @param[in] objects - array of pointers to objects
  virtual void onPPClose(PropPanel::ContainerPropertyControl &panel, dag::ConstSpan<RenderableEditableObject *> objects) {}

  /// Called before Property Panel will be cleared.
  /// @param[in] panel - Property Panel
  /// @param[in] objects - array of pointers to objects
  virtual void onPPClear(PropPanel::ContainerPropertyControl &panel, dag::ConstSpan<RenderableEditableObject *> objects) {}

  /// Called when button on Property Panel is pressed.
  /// @param[in] pid - button ID
  /// @param[in] panel - Property Panel
  /// @param[in] objects - array of pointers to objects
  virtual void onPPBtnPressed(int pid, PropPanel::ContainerPropertyControl &panel, dag::ConstSpan<RenderableEditableObject *> objects)
  {}
  //@}

  virtual void onRemove(ObjectEditor *objEditor) {}
  virtual void onAdd(ObjectEditor *objEditor) {}

  virtual void rememberSurfaceDist();
  virtual void zeroSurfaceDist() { surfaceDist = 0.0; }
  virtual real getSurfaceDist() const { return surfaceDist; }

  const Point3 &getSavedPlacementNormal() const { return savedPlacementNormal; }
  void setSavedPlacementNormal(const Point3 &n) { savedPlacementNormal = n; }
  virtual bool usesRendinstPlacement() const { return false; }
  virtual void setCollisionIgnored() {}
  virtual void resetCollisionIgnored() {}

protected:
  friend class ObjectEditor;

  int objFlags;
  ObjectEditor *objEditor;

  real surfaceDist;
  Point3 savedPlacementNormal = Point3(0, 1, 0);

  class UndoObjFlags : public UndoRedoObject
  {
  public:
    Ptr<RenderableEditableObject> obj;
    int prevFlags, redoFlags;


    UndoObjFlags(RenderableEditableObject *o) : obj(o), prevFlags(o->getFlags()) { redoFlags = prevFlags; }

    virtual void restore(bool save_redo)
    {
      if (save_redo)
        redoFlags = obj->getFlags();
      obj->setFlags(prevFlags, ~0);
    }

    virtual void redo() { obj->setFlags(redoFlags, ~0); }

    virtual size_t size() { return sizeof(*this); }
    virtual void accepted() {}
    virtual void get_description(String &s) { s = "UndoObjFlags"; }
  };


  class UndoMove : public UndoRedoObject
  {
  public:
    Ptr<RenderableEditableObject> obj;
    Point3 oldPos, redoPos;


    UndoMove(RenderableEditableObject *o) : obj(o) { oldPos = redoPos = obj->getPos(); }

    virtual void restore(bool save_redo)
    {
      if (save_redo)
        redoPos = obj->getPos();
      obj->setPos(oldPos);
    }

    virtual void redo() { obj->setPos(redoPos); }

    virtual size_t size() { return sizeof(*this); }
    virtual void accepted() {}
    virtual void get_description(String &s) { s = "UndoMove"; }
  };


  class UndoMatrix : public UndoRedoObject
  {
  public:
    Ptr<RenderableEditableObject> obj;
    Matrix3 oldMatrix, redoMatrix;


    UndoMatrix(RenderableEditableObject *o) : obj(o) { oldMatrix = redoMatrix = obj->getMatrix(); }

    virtual void restore(bool save_redo)
    {
      if (save_redo)
        redoMatrix = obj->getMatrix();
      obj->setMatrix(oldMatrix);
    }

    virtual void redo() { obj->setMatrix(redoMatrix); }

    virtual size_t size() { return sizeof(*this); }
    virtual void accepted() {}
    virtual void get_description(String &s) { s = "UndoMatrix"; }
  };
};

#define EO_IMPLEMENT_RTTI(CID) EO_IMPLEMENT_RTTI_EX(CID, RenderableEditableObject)
