// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_string.h>
#include <math/dag_math3d.h>
#include <generic/dag_tab.h>

#include <EditorCore/ec_IGizmoObject.h>

struct SCNotification;
class AutoCompletition;
class SquirrelObject;
struct SubClass;
struct SQObjectPtr;

struct SquirrelVMSys;

typedef struct SQVM *HSQUIRRELVM;

#if !HANDLE2_DEFINED
// #ifdef STRICT
#if 1
// struct HWND__ { int unused; }
typedef struct HWND__ *HWND2;
typedef struct HINSTANCE__ *HINSTANCE2;
typedef struct HICON__ *HICON2;
typedef struct HACCEL__ *HACCEL2;
#else
typedef void *HWND2;
typedef void *HINSTANCE2;
typedef void *HICON2;
typedef void *HACCEL2;
#endif

#define HANDLE2_DEFINED 1
#endif


class ScriptEditor;

class ScriptObjInteractor : public IGizmoObject
{
public:
  ScriptObjInteractor(const TMatrix &tm, ScriptEditor *editor, const IPoint2 &text_range);
  ScriptObjInteractor(const Point3 &pos, ScriptEditor *editor, const IPoint2 &text_range);
  ScriptObjInteractor(const BSphere3 &sph, ScriptEditor *editor, const IPoint2 &text_range);

  TMatrix getTm() const override { return tm; }
  void setTm(const TMatrix &new_tm) override;
  void onStartChange() override;

  const char *getUndoName() const override
  {
    // return objType==TYPE_POINT3?"Script Point3":"Script TMatrix";
    return "Script object";
  }

  int getAvailableAxis() const override { return GizmoEventFilter::AXIS_X | GizmoEventFilter::AXIS_Y | GizmoEventFilter::AXIS_Z; }

  enum
  {
    TYPE_POINT3,
    TYPE_TMATRIX,
    TYPE_BSPHERE
  } objType;

public:
  IPoint2 textRange;

private:
  TMatrix tm;
  ScriptEditor *editor;
};

struct IScriptObjectManager
{
  virtual int getNumObjects() = 0;
  virtual const char *getObjName(int index) = 0;
  virtual BBox3 getObjBBox(int index) = 0;
  virtual void startEditingPoint3(IGizmoObject *interactor) = 0;
  virtual void startEditingTMatrix(IGizmoObject *interactor) = 0;
  virtual void stopEditing() = 0;

  //  //== temp
  //  virtual void  addObject(TMatrix &tm) = 0;
  //  virtual void  addObject(const char *name, ScriptObjInteractor *oi) = 0;
  //  virtual void  removeAllObjects() = 0;
};

struct IScriptObjectManager2
{
  virtual void addScriptObject(const char *name, ScriptObjInteractor *oi) = 0;
  virtual void removeAllScriptObjects() = 0;
  virtual void getInteractors(Tab<ScriptObjInteractor *> &list) = 0;
  virtual void setObjVisible(ScriptObjInteractor *oi, bool on) = 0;
};

class ScriptEditor
{
public:
  void *scintilaDefProc;

  ScriptEditor(HWND2 parent_wnd, IScriptObjectManager *obj_man, IScriptObjectManager2 *ext_obj_manager = NULL);
  ~ScriptEditor();
  bool initEditor();
  bool initPropEditor();
  void destroyEditor();
  void showEditor(bool activate = false);
  void hideEditor() const;
  bool isVisible() const;
  bool isTopmost() const;

  // for plugin
  void addObjectNames(const char *scheme, const char *game_script);

  // for autocompletition
  int sendEditor(unsigned Msg, unsigned wParam = 0, unsigned lParam = 0);
  int getCaretInLine();
  String getLine(int line = 0);
  void getRange(int start, int end, char *text);

  void updateInteractorsTextRange(int change_pos, int old_len, int new_len);

  int askForSave(bool allow_cancel);

  const char *getEditingFileName() { return fullPath; }
  void setEditingFileName(const char *fn);

  HWND2 getWindowHandle() { return hWnd; }

  void endGoToLineDialog(bool ok, int line = -1);

  void addTMatrix(const TMatrix &tm);
  void addBSphere3(const TMatrix &tm);
  bool isTextChanged() { return isDirty; };

private:
  struct NamedSqObject;

  // IDagorSqCB impl
  virtual void onNewSlot(int source_line, const SQObjectPtr &self, const SQObjectPtr &key, const SQObjectPtr &val, bool bstatic);

  void buildFullObjName(NamedSqObject *obj, String &out_name);
  void createPoint3Interactor(NamedSqObject *obj, int source_text_pos);
  void createTMatrixInteractor(NamedSqObject *obj, int source_text_pos);
  void createBSphereInteractor(NamedSqObject *obj, int source_text_pos);
  void createCollectedObjects();

  void onCompileError(const char *desc, int line, int col);

  void switchChild();
  void showPropEditor(bool activate = false);
  void hidePropEditor(bool ok);
  static void compileErrorHandler(HSQUIRRELVM v, const char *desc, const char *source, int64_t line, int64_t column);
  void compileScript();
  bool compile(char *data, bool need_run);
  void onStyleNeeded();
  void newScript();
  void openScript();
  void saveScript();
  void openFile(const char *file_name, bool show_error = true);
  void saveScriptAs();
  void saveFile(const char *fileName);
  void setupScintilla();
  void notify(SCNotification *notification);
  void setAStyle(int style, unsigned long fore, unsigned long back = 0xFFFFFF, int size = -1, const char *face = 0);

  void setTitle();
  void alignControls();
  bool registerWndClass();
  bool registerPropWndClass();
  static intptr_t __stdcall wndProc(HWND2 wnd, unsigned msg, uintptr_t w_param, intptr_t l_param);
  static intptr_t __stdcall propWndProc(HWND2 wnd, unsigned msg, uintptr_t w_param, intptr_t l_param);
  void getClassesTab(SquirrelObject &classes);
  void clickSubclass(const char *class_name, int pos);
  void clickAppend(const char *class_name, int pos);

  void loadScheme(const char *fn);
  void loadGameScript(const char *fn);

  void onAddSetCameraCmd();
  void onGoToCamera();
  void expand(int &line, bool doExpand, bool force = false, int visLevels = 0, int level = -1);
  void onExpandCollapseAll(bool expand);
  void onUpdateUIRequested();
  char *getSelectedWord(int *start_pos = NULL);
  template <class T>
  bool getValueFromScript(int text_pos, T *out_val = NULL, int *end_pos = NULL);
  bool getTextRangeInParenthesis(int text_pos, int &start_pos, int &end_pos);

  void updateObjectsVisibility();

  void showFindDialog();

  void setStatusMessage(const char *msg);

  HWND2 hWnd;
  HWND2 parentWnd;
  HWND2 hwndScintilla;
  HWND2 hwndPropCombo;
  HWND2 hwndProps;
  HWND2 hwndOk;
  HWND2 hwndCancel;

  HWND2 hwndDebug;

  HWND2 hwndGoToLineDlg;

  struct SearchDlg
  {
    char *fr;
    HWND2 hdlg;
  } searchDlg;

  static int edCount;
  static unsigned wndClassId;
  static unsigned propWndClassId;
  static void *sciLexerDll;
  String fullPath;
  String appName;

  HACCEL2 accelTable;

  Tab<SubClass *> subClasses;
  String startStr;

  bool isDirty;
  AutoCompletition *ac;
  bool lastVisible;

  SubClass *lastClass;
  int lastCaretPos;
  int editorType;

  IScriptObjectManager *scriptObjManager;
  IScriptObjectManager2 *extScriptObjManager;
  ScriptObjInteractor *interactor;

  Tab<NamedSqObject *> namedSqObjects;

  bool gizmoLock;

  SquirrelVMSys *sqVM;

  unsigned int msgIdFindString;

  friend class ScriptObjInteractor;
};
