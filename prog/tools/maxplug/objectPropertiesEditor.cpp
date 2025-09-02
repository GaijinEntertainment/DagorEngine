// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include <utilapi.h>
#include <meshadj.h>
#include <mnmath.h>
#include <stdmat.h>
#include "dagor.h"
#include "resource.h"
#include "math3d.h"
#include "rolluppanel.h"
#include "enumnode.h"

class ObjectPropertiesEditorDesc : public ClassDesc
{
public:
  int IsPublic() override { return 1; }
  void *Create(BOOL loading = FALSE) override;
  const TCHAR *ClassName() override { return GetString(IDS_OBJECT_PROPERTIES_EDITOR_NAME); }
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  const MCHAR *NonLocalizedClassName() override { return ClassName(); }
#else
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
#endif
  SClass_ID SuperClassID() override { return UTILITY_CLASS_ID; }
  Class_ID ClassID() override { return OBJECT_PROPERTIES_EDITOR_CID; }
  const TCHAR *Category() override { return GetString(IDS_UTIL_CAT); }
  void ResetClassParams(BOOL fileReset) override {}
};


class ObjectPropertiesEditor : public UtilityObj
{
public:
  ObjectPropertiesEditor();
  void Init();
  void Destroy();

  void BeginEditParams(Interface *, IUtil *) override;
  void EndEditParams(Interface *, IUtil *) override;
  void DeleteThis() override;
  void UpdateControls(Interface *interface_pointer);

  static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


protected:
  RollupPanel *panel;

  HWND m_hPanel;
  Interface *m_pInterface;
  Tab<bool> selectedNodes;
};


class ObjectPropertiesEditorRedrawViewsCallback : public RedrawViewsCallback
{
public:
  void proc(Interface *interface_pointer) override;
};


static ObjectPropertiesEditorDesc object_properties_editor_desc;
static ObjectPropertiesEditor object_properties_editor;
static ObjectPropertiesEditorRedrawViewsCallback object_properties_editor_redraw_views_callback;


ClassDesc *GetObjectPropertiesEditorCD() { return &object_properties_editor_desc; }


void *ObjectPropertiesEditorDesc::Create(BOOL loading) { return &object_properties_editor; }


void ObjectPropertiesEditorRedrawViewsCallback::proc(Interface *interface_pointer)
{
  object_properties_editor.UpdateControls(interface_pointer);
}


ObjectPropertiesEditor::ObjectPropertiesEditor()
{
  m_hPanel = NULL;
  m_pInterface = NULL;
  panel = NULL;
}


void ObjectPropertiesEditor::Init() { m_pInterface->RegisterRedrawViewsCallback(&object_properties_editor_redraw_views_callback); }


void ObjectPropertiesEditor::Destroy()
{
  if (panel)
  {
    delete panel;
    panel = NULL;
  }
  m_pInterface->UnRegisterRedrawViewsCallback(&object_properties_editor_redraw_views_callback);
  m_pInterface->UnRegisterDlgWnd(m_hPanel);
  m_hPanel = NULL;
}


void ObjectPropertiesEditor::BeginEditParams(Interface *in_pInterface, IUtil *util)
{
  m_pInterface = in_pInterface;

  if (m_hPanel)
  {
    SetActiveWindow(m_hPanel);
  }
  else
  {
    Init();
    m_hPanel = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_OBJECT_PROPERTIES_FROM_BLK), m_pInterface->GetMAXHWnd(),
      ObjectPropertiesEditor::DialogProc, (LPARAM)this);
    m_pInterface->RegisterDlgWnd(m_hPanel);
  }

  util->CloseUtility();
}


void ObjectPropertiesEditor::EndEditParams(Interface *in_pInterface, IUtil *)
{
  // Do nothing.
}


void ObjectPropertiesEditor::DeleteThis()
{
  // Do nothing.
}


INT_PTR CALLBACK ObjectPropertiesEditor::DialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  ObjectPropertiesEditor *pThis = (ObjectPropertiesEditor *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
  if ((!pThis || !pThis->m_hPanel) && msg != WM_INITDIALOG)
  {
    return FALSE;
  }


  switch (msg)
  {
    case WM_INITDIALOG:
    {
      ObjectPropertiesEditor *pThis = (ObjectPropertiesEditor *)lParam;
      SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
      pThis->m_hPanel = hWnd;
      CenterWindow(hWnd, GetParent(hWnd));
      pThis->panel = new RollupPanel(pThis->m_pInterface, hWnd);
      pThis->panel->fillPanel();
      return TRUE;
    }
    break;

    case WM_CLOSE:
    {
      EndDialog(pThis->m_hPanel, FALSE);
      pThis->Destroy();
    }
    break;

    default: return FALSE;
  }
  return TRUE;
}

class SelectionChanged : public ENodeCB
{
public:
  SelectionChanged(Tab<bool> &selected_nodes) : selectedNodes(selected_nodes), i(0), needUpdate(false) {}
  ~SelectionChanged() override {}
  bool need() { return needUpdate; }
  void resize()
  {
    if (i < selectedNodes.Count())
    {
      selectedNodes.Resize(i);
      needUpdate = true;
    }
  }
  int proc(INode *n) override
  {
    if (i < selectedNodes.Count() && selectedNodes[i] != (n->Selected() != 0))
    {
      selectedNodes[i] = n->Selected() != 0;
      needUpdate = true;
    }
    if (i >= selectedNodes.Count())
    {
      bool b = n->Selected() != 0;
      selectedNodes.Append(1, &b);
      needUpdate = true;
    }
    i++;
    return ECB_CONT;
  }

private:
  Tab<bool> &selectedNodes;
  bool needUpdate;
  int i;
};

void ObjectPropertiesEditor::UpdateControls(Interface *ip)
{
  if (panel)
  {
    SelectionChanged cb(selectedNodes);
    enum_nodes(ip->GetRootNode(), &cb);
    cb.resize();
    if (cb.need())
    {
      panel->fillPanel();
    }
  }
}
