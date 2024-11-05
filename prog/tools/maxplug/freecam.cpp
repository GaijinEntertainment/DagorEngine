// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include <utilapi.h>
#include <winuser.h>
#include "resource.h"
#include "dagor.h"
#include "debug.h"

#define MAX_CLSNAME 64

extern HINSTANCE hInstance;

ViewExp *getViewport(HWND hwnd)
{
#if defined(MAX_RELEASE_R15) && MAX_RELEASE >= MAX_RELEASE_R15
  return GetCOREInterface()->GetViewExp(hwnd).ToPointer();
#else
  return GetCOREInterface()->GetViewport(hwnd);
#endif
}

ViewExp *getActiveViewport()
{
#if defined(MAX_RELEASE_R15) && MAX_RELEASE >= MAX_RELEASE_R15
  return GetCOREInterface()->GetActiveViewExp().ToPointer();
#else
  return GetCOREInterface()->GetActiveViewport();
#endif
}

void releaseViewport(ViewExp *vp)
{
#if defined(MAX_RELEASE_R15) && MAX_RELEASE >= MAX_RELEASE_R15
#else
  GetCOREInterface()->ReleaseViewport(vp);
#endif
}

struct FreeCamera
{
  char on;
  char modon;
  bool changed;
  float move_speed, rot_speed, turbo;
  void init(HWND hw, Interface *ip) {}
  FreeCamera()
  {
    move_speed = 1;
    rot_speed = 0.004f;
    hang = 0;
    vang = PI / 2;
    on = 0;
    modon = 1;
    turbo = 5;
    changed = true;
  }
  void destroy(HWND hw, Interface *ip) {}
  Matrix3 curviewtm, curviewitm;
  Point3 curviewpos;
  float hang, vang;

  void InitView(ViewExp *viewexp)
  {
    viewexp->GetAffineTM(curviewtm);
    curviewitm = Inverse(curviewtm);
    curviewpos = curviewitm.GetRow(3);
    hang = 0, vang = PI / 2;
    DPoint3 viewdir = curviewitm.GetRow(2);
    DPoint3 vdh = Normalize(DPoint3(viewdir.x, viewdir.y, 0));
    if (Length(vdh) != 0)
    {
      hang = atan2(viewdir.x, -viewdir.y);
    }
    if (viewdir.z != 0)
    {
      vang = asin(-viewdir.z) + PI / 2;
    }
    curviewitm = RotateXMatrix(vang) * RotateZMatrix(hang);
    curviewitm.SetRow(3, curviewpos);
    changed = true;
  }

  bool MoveViewByRow(int ch, float dh)
  {
    if (ch < 0 || ch > 2 || dh == 0)
      return false;
    Point3 movedir = curviewitm.GetRow(ch);
    curviewpos += move_speed * dh * movedir;
    changed = true;
    return true;
  }
  void SetView(ViewExp *viewexp)
  {
    while (hang >= TWOPI)
      hang -= TWOPI;
    while (hang < 0)
      hang += TWOPI;
    // while(vang>PI) vang-=TWOPI;
    // while(vang<-PI) vang+=TWOPI;
    if (vang > PI)
      vang = PI;
    if (vang < 0)
      vang = 0;
    curviewitm = RotateXMatrix(vang) * RotateZMatrix(hang);
    curviewitm.SetRow(3, curviewpos);
    curviewtm = Inverse(curviewitm);
    viewexp->SetAffineTM(curviewtm);
    changed = false;
  }
};
static FreeCamera freecam;

#define NumElements(array) (sizeof(array) / sizeof(array[0]))

static ActionDescription spShortcuts[] = {
  IDR_FREE_BACKW,
  IDS_FREE_BACKW,
  IDS_FREE_BACKW,
  IDS_FREECAM,
  IDR_FREE,
  IDS_FREECAM,
  IDS_FREECAM,
  IDS_FREECAM,
  IDR_FREE_FORW,
  IDS_FREE_FORW,
  IDS_FREE_FORW,
  IDS_FREECAM,
  IDR_FREE_LEFT,
  IDS_FREE_LEFT,
  IDS_FREE_LEFT,
  IDS_FREECAM,
  IDR_FREE_RIGHT,
  IDS_FREE_RIGHT,
  IDS_FREE_RIGHT,
  IDS_FREECAM,

  IDR_FREE_UP,
  IDS_FREE_UP,
  IDS_FREE_UP,
  IDS_FREECAM,
  IDR_FREE_DOWN,
  IDS_FREE_DOWN,
  IDS_FREE_DOWN,
  IDS_FREECAM,

  IDR_FREET_BACKW,
  IDS_FREET_BACKW,
  IDS_FREET_BACKW,
  IDS_FREECAM,
  IDR_FREET_FORW,
  IDS_FREET_FORW,
  IDS_FREET_FORW,
  IDS_FREECAM,
  IDR_FREET_LEFT,
  IDS_FREET_LEFT,
  IDS_FREET_LEFT,
  IDS_FREECAM,
  IDR_FREET_RIGHT,
  IDS_FREET_RIGHT,
  IDS_FREET_RIGHT,
  IDS_FREECAM,

  IDR_FREET_UP,
  IDS_FREET_UP,
  IDS_FREET_UP,
  IDS_FREECAM,
  IDR_FREET_DOWN,
  IDS_FREET_DOWN,
  IDS_FREET_DOWN,
  IDS_FREECAM,

};

const ActionContextId FreeCamContext = 0x2bfc51d6;
const ActionTableId FreeCamShortcuts = 0x2bfc51d6;

class FreeCamCommandMode : public CommandMode, public MouseCallBack
{
public:
  HCURSOR prevcur;
  FreeCamCommandMode() { prevcur = NULL; }
  int Class() { return 1101; } // VIEWPORT_COMMAND
  int ID() { return CID_USER + 0x777; }
  MouseCallBack *MouseProc(int *numPoints)
  {
    *numPoints = 200000000;
    return this;
  }
  BOOL ChangeFG(CommandMode *oldMode) { return (oldMode->ChangeFGProc() != CHANGE_FG_SELECTED); }

  ChangeForegroundCallback *ChangeFGProc() { return CHANGE_FG_SELECTED; }
  void EnterMode()
  {
    started = 0;
    ViewExp *ve = getActiveViewport();
    HWND hwnd = ve->GetHWnd();
    RECT Rect;
    bool gr = true;
    if (!GetWindowRect(hwnd, &Rect))
      gr = false;
    releaseViewport(ve);
    RECT desktopr;
    GetWindowRect(GetDesktopWindow(), &desktopr);
    if (gr)
      mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, 65535 * (Rect.right + Rect.left) / (2 * desktopr.right),
        65535 * (Rect.top + Rect.bottom) / (2 * desktopr.bottom), 0, NULL);
    prevcur = SetCursor(NULL);
  }
  void ExitMode()
  {
    if (prevcur)
      SetCursor(prevcur);
    prevcur = NULL;
  }

  // Mouse

  IPoint2 op, startop;
  int started;
  int proc(HWND hwnd, int msg, int point, int flags, IPoint2 m)
  {
    if (msg == MOUSE_MOVE)
    {
      // DWORD dx,dy,dwData;
      // mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_MOVE,dx,dy, dwData, NULL);
      if (op.x != m.x || op.y != m.y)
      {
        if (!started)
        {
          op = m;
          startop = m;
          started = 1;
          return TRUE;
        }
        RECT desktopr, Rect;
        bool gr = true;
        GetWindowRect(GetDesktopWindow(), &desktopr);
        ViewExp *vpt = getViewport(hwnd);
        if (!GetWindowRect(vpt->GetHWnd(), &Rect))
          gr = false;
        float h = freecam.rot_speed * (op.x - m.x);
        float v = freecam.rot_speed * (op.y - m.y);
        freecam.hang += h;
        freecam.vang += v;
        if (gr)
        {
          if (m.x == -Rect.left)
          {
            m.x = desktopr.right - Rect.left - 2;
            gr = false;
          }
          if (m.y == -Rect.top)
          {
            m.y = desktopr.bottom - Rect.top - 2;
            gr = false;
          }
          if (m.x == desktopr.right - 1 - Rect.left)
          {
            m.x = -Rect.left + 2;
            gr = false;
          }
          if (m.y == desktopr.bottom - 1 - Rect.top)
          {
            m.y = -Rect.top + 2;
            gr = false;
          }
          if (!gr)
            mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, (m.x + Rect.left) * 65535 / desktopr.right,
              (m.y + Rect.top) * 65535 / desktopr.bottom, 0, NULL);
        }
        freecam.changed = true;
        // camera->SetView(vpt);
        // vpt->getGW()->enlargeUpdateRect(NULL);
        releaseViewport(vpt);
        // GetCOREInterface()->ForceCompleteRedraw(FALSE);
        op = m;
      }
    }
    else if (msg == MOUSE_POINT)
    {
      op = m;
    }
    else if (msg == MOUSE_PROPCLICK || msg == MOUSE_ABORT)
    {
      op = m;
      started = 0;
      freecam.on = 0;
      // mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_MOVE,startop.x,startop.y, 0, NULL);
      GetCOREInterface()->DeleteMode(this);
    }
    else if (msg == MOUSE_INIT)
    {
      // SetCapture(hwnd);
      op = m;
      started = 0;
    }
    else if (msg == MOUSE_UNINIT)
    {
      RECT desktopr, Rect;
      bool gr = true;
      GetWindowRect(GetDesktopWindow(), &desktopr);
      ViewExp *vpt = getViewport(hwnd);
      if (!GetWindowRect(vpt->GetHWnd(), &Rect))
        gr = false;
      releaseViewport(vpt);
      if (gr)
        mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, (startop.x + Rect.left) * 65535 / desktopr.right,
          (startop.y + Rect.top) * 65535 / desktopr.bottom, 0, NULL);
      // debug("%d %d ",startop.x,startop.y);
    }
    return true;
  }
  int override(int mode)
  {
    return mode;
    // return CLICK_DRAG_CLICK;
  } // Override the mouse manager's mode!
};

static class FreeCamShortcut *fccb = NULL;

static FreeCamCommandMode freecamcm;

static VOID CALLBACK TimerFunc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
  if (!freecam.modon || !freecam.on)
    return;
  if (!freecam.changed)
    return;
  ViewExp *ve = getActiveViewport();
  freecam.SetView(ve);
  GetCOREInterface()->ForceCompleteRedraw(FALSE);
  releaseViewport(ve);
  /*static int called=0;
  if(++called>10){
  GetCOREInterface()->ForceCompleteRedraw(FALSE);
  called=0;
  }*/
}
static UINT_PTR timer_id = NULL;

class FreeCamShortcut : public ActionCallback
{
public:
  BOOL ExecuteAction(int id)
  {
    if (!freecam.modon)
      return FALSE;
    if (!freecam.on && id != IDR_FREE)
      return FALSE;
    ViewExp *ve = NULL;
    bool set = false;
    if ((freecam.on && id != IDR_FREE) || (!freecam.on && id == IDR_FREE))
    {
      set = true;
      ve = getActiveViewport();
    }
    switch (id)
    {
      case IDR_FREE_UP: freecam.MoveViewByRow(1, 1); break;
      case IDR_FREE_DOWN: freecam.MoveViewByRow(1, -1); break;
      case IDR_FREE_LEFT: freecam.MoveViewByRow(0, -1); break;
      case IDR_FREE_RIGHT: freecam.MoveViewByRow(0, 1); break;
      case IDR_FREE_BACKW: freecam.MoveViewByRow(2, 1); break;
      case IDR_FREE_FORW: freecam.MoveViewByRow(2, -1); break;
      case IDR_FREET_BACKW: freecam.MoveViewByRow(2, freecam.turbo); break;
      case IDR_FREET_FORW: freecam.MoveViewByRow(2, -freecam.turbo); break;
      case IDR_FREET_LEFT: freecam.MoveViewByRow(0, -freecam.turbo); break;
      case IDR_FREET_RIGHT: freecam.MoveViewByRow(0, freecam.turbo); break;
      case IDR_FREET_UP: freecam.MoveViewByRow(1, freecam.turbo); break;
      case IDR_FREET_DOWN: freecam.MoveViewByRow(1, -freecam.turbo); break;
      case IDR_FREE:
      {
        freecam.on = !freecam.on;
        if (freecam.on)
        {
          freecam.InitView(ve);
          GetCOREInterface()->SetCommandMode(&freecamcm);
          timer_id = SetTimer(NULL, NULL, 10, (TIMERPROC)&TimerFunc);
          ActionContext *uimain = GetCOREInterface()->GetActionManager()->FindContext(kActionMainUIContext);
          if (uimain)
            uimain->SetActive(false);
        }
        else
        {
          if (timer_id)
            KillTimer(NULL, timer_id);
          timer_id = NULL;
          GetCOREInterface()->DeleteMode(&freecamcm);
          ActionContext *uimain = GetCOREInterface()->GetActionManager()->FindContext(kActionMainUIContext);
          if (uimain)
            uimain->SetActive(true);
        }
        return TRUE;
      }
      break;
    }
    if (set)
    {
      // freecam.SetView(ve);
      releaseViewport(ve);
      // GetCOREInterface()->ForceCompleteRedraw(FALSE);
      // GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
    }
    return TRUE;
  };

}; // freecam_callback;

ActionTable *GetActions()
{
  static ActionTable *pTab = NULL;
  if (pTab)
    return NULL;
  TSTR name = GetString(IDS_FREECAM);
  HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_FREECAM_ACC));
  int numOps = NumElements(spShortcuts);
  pTab = new ActionTable(FreeCamShortcuts, FreeCamContext, name, hAccel, numOps, spShortcuts, hInstance);

  fccb = new FreeCamShortcut;

  GetCOREInterface()->GetActionManager()->RegisterActionContext(FreeCamContext, name.data());
  GetCOREInterface()->GetActionManager()->RegisterActionTable(pTab);
  GetCOREInterface()->GetActionManager()->ActivateActionTable(fccb, FreeCamShortcuts);
  // pTab->SetCallback(&freecam_callback);
  return pTab;
}

extern bool get_edint(ICustEdit *e, int &a);
extern bool get_edfloat(ICustEdit *e, float &a);

class DagFreeCamUtil : public UtilityObj
{
public:
  TCHAR clsname[MAX_CLSNAME];
  IUtil *iu;
  Interface *ip;
  HWND hFreeCamPanel;
  ICustEdit *cammovespeedid, *camrotspeedid, *turboid;
  // FreeCamCommandMode freecamcm;
  // FreeCamera freecam;


  DagFreeCamUtil();
  void BeginEditParams(Interface *ip, IUtil *iu);
  void EndEditParams(Interface *ip, IUtil *iu);
  void DeleteThis() {}
  void update_freecam_ui();
  void update_freecam_vars();

  void Init(HWND hWnd);
  void Destroy(HWND hWnd);
  BOOL dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam);
  void set_cam(char);
};
static DagFreeCamUtil futil;


class DagFreeCamUtilDesc : public ClassDesc
{
public:
  int IsPublic() { return 1; }
  void *Create(BOOL loading = FALSE) { return &futil; }
  const TCHAR *ClassName() { return GetString(IDS_DAGFREECAM_UTIL_NAME); }
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
  SClass_ID SuperClassID() { return UTILITY_CLASS_ID; }
  Class_ID ClassID() { return DagUtilFreeCam_CID; }
  const TCHAR *Category() { return GetString(IDS_UTIL_CAT); }
  BOOL NeedsToSave() { return TRUE; }
  IOResult Save(ISave *);
  IOResult Load(ILoad *);
  int NumActionTables()
  {
    GetActions();
    return 0;
  } // GetActions();
  // ActionTable*  GetActionTable(int i) { return GetActions(); }
};

static const int CH_FREE_CAM = 1;

IOResult DagFreeCamUtilDesc::Save(ISave *io)
{
  futil.update_freecam_vars();
  ULONG nw;
  io->BeginChunk(CH_FREE_CAM);
  if (io->Write(&freecam.modon, 1, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&freecam.rot_speed, sizeof(float), &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&freecam.move_speed, sizeof(float), &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&freecam.vang, sizeof(float), &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&freecam.hang, sizeof(float), &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&freecam.turbo, sizeof(float), &nw) != IO_OK)
    return IO_ERROR;
  io->EndChunk();
  return IO_OK;
}

IOResult DagFreeCamUtilDesc::Load(ILoad *io)
{
  ULONG nr;
  while (io->OpenChunk() == IO_OK)
  {
    switch (io->CurChunkID())
    {
      case CH_FREE_CAM:
      {
        if (io->Read(&freecam.modon, 1, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&freecam.rot_speed, sizeof(float), &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&freecam.move_speed, sizeof(float), &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&freecam.vang, sizeof(float), &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&freecam.hang, sizeof(float), &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&freecam.turbo, sizeof(float), &nr) != IO_OK)
          return IO_ERROR;
        futil.set_cam(freecam.modon);
      }
      break;
    }
    io->CloseChunk();
  }
  futil.update_freecam_ui();
  return IO_OK;
}

static DagFreeCamUtilDesc futilDesc;
ClassDesc *GetDagFreeCamUtilCD() { return &futilDesc; }


static INT_PTR CALLBACK FreeCamDlgProc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return futil.dlg_proc(hw, msg, wParam, lParam);
}

BOOL DagFreeCamUtil::dlg_proc(HWND hw, UINT msg, WPARAM wpar, LPARAM lpar)
{
  switch (msg)
  {
    case WM_INITDIALOG: Init(hw); break;
    case WM_DESTROY: Destroy(hw); break;
    case WM_COMMAND:
      switch (LOWORD(wpar))
      {
        case IDC_FREECAM_ON:
          freecam.modon = !freecam.modon;
          set_cam(freecam.modon);
          break;
      }
      break;
    default: return FALSE;
  }
  return TRUE;
}

void DagFreeCamUtil::BeginEditParams(Interface *ip, IUtil *iu)
{
  this->iu = iu;
  this->ip = ip;

  hFreeCamPanel = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_FREECAM), FreeCamDlgProc, GetString(IDS_FREECAM), 0);
}

void DagFreeCamUtil::EndEditParams(Interface *ip, IUtil *iu)
{
  this->iu = NULL;
  this->ip = NULL;
  if (hFreeCamPanel)
    ip->DeleteRollupPage(hFreeCamPanel);
  hFreeCamPanel = NULL;
}


void DagFreeCamUtil::update_freecam_ui()
{
  HWND hw = hFreeCamPanel;
  if (!hw)
    return;
  CheckDlgButton(hw, IDC_FREECAM_ON, freecam.modon);
  if (cammovespeedid)
    cammovespeedid->SetText(freecam.move_speed);
  if (camrotspeedid)
    camrotspeedid->SetText(freecam.rot_speed);
  if (turboid)
    turboid->SetText(freecam.turbo);
}

void DagFreeCamUtil::set_cam(char fcmon)
{
  if (fcmon)
    ; // no-op
  else if (!fcmon)
  {
    if (freecam.on)
    {
      freecam.on = 0;
      GetCOREInterface()->DeleteMode(&freecamcm);
      if (timer_id)
        KillTimer(NULL, timer_id);
      timer_id = NULL;
      ActionContext *uimain = GetCOREInterface()->GetActionManager()->FindContext(kActionMainUIContext);
      if (uimain)
        uimain->SetActive(true);
    }
  }
}

void DagFreeCamUtil::update_freecam_vars()
{
  HWND hw = hFreeCamPanel;
  if (!hw)
    return;
  freecam.modon = IsDlgButtonChecked(hw, IDC_FREECAM_ON);
  set_cam(freecam.modon);
  get_edfloat(cammovespeedid, freecam.move_speed);
  get_edfloat(camrotspeedid, freecam.rot_speed);
  get_edfloat(turboid, freecam.turbo);
}

void DagFreeCamUtil::Init(HWND hw)
{
  hFreeCamPanel = hw;
  freecam.init(hw, ip);
  cammovespeedid = GetICustEdit(GetDlgItem(hw, IDC_MOVE_SPEED));
  camrotspeedid = GetICustEdit(GetDlgItem(hw, IDC_ROT_SPEED));
  turboid = GetICustEdit(GetDlgItem(hw, IDC_TURBO));
  update_freecam_ui();
}

void DagFreeCamUtil::Destroy(HWND hw)
{
  update_freecam_vars();
#define reled(e)         \
  if (e)                 \
  {                      \
    ReleaseICustEdit(e); \
    e = NULL;            \
  }
  reled(cammovespeedid);
  reled(camrotspeedid);
  reled(turboid);
#undef reled
  freecam.destroy(hw, ip);
}


DagFreeCamUtil::DagFreeCamUtil()
{
  _tcscpy(clsname, _T(""));
  iu = NULL;
  ip = NULL;
  hFreeCamPanel = NULL;
  turboid = cammovespeedid = camrotspeedid = NULL;
}
