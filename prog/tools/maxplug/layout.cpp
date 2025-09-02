// Copyright (C) Gaijin Games KFT.  All rights reserved.

// temporarily disabled due to visible artifacts
#if defined(ENABLE_LAYOUT) && defined(MAX_RELEASE_R22) && MAX_RELEASE >= MAX_RELEASE_R22

#include <max.h>
#include "layout.h"

extern HINSTANCE hInstance;

#define IDT_LAYOUT_TIMER       (1)
#define LAYOUT_TIMEOUT         (250)
#define PROP_LAYOUT            (L"~Layout~")
#define PROP_LAYOUT_SIZE       (L"~LayoutSize~")
#define PROP_ORIGINAL_DLG_PROC (L"~OriginalWndProc~")

struct DialogLayout
{
  int dlg_w, dlg_h;
  std::vector<int> items;
  std::vector<uint8_t> layout;
};

static inline uint8_t afx_clamp(const WORD val) { return val < 100 ? val : 100; }

static inline const uint8_t *dword_align(const uint8_t *ptr)
{
  static const uintptr_t alignment_minus_one = sizeof(DWORD) - 1;
  return reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(ptr + alignment_minus_one) & ~alignment_minus_one);
}

static void update_layout(const DialogLayout *self, HWND hWnd, LPARAM lParam)
{
  if (!self || !lParam)
    return;

  if (self->items.empty() || self->layout.empty())
    return;

  assert((self->items.size() % 5) == 0);
  assert((self->layout.size() % 4) == 0);
  assert((self->items.size() / 5) == (self->layout.size() / 4));

  RECT r;
  r.left = r.top = 0;
  r.right = self->dlg_w;
  r.bottom = self->dlg_h;
  MapDialogRect(hWnd, &r);

  int deltaW = LOWORD(lParam) - r.right;
  int deltaH = HIWORD(lParam) - r.bottom;
  if (deltaW < 0 || deltaH < 0)
    return;

  for (size_t i = 0; i < self->items.size() / 5; ++i)
  {
    const int *d_it = &self->items[i * 5];
    const uint8_t *l_it = &self->layout[i * 4];

    if (d_it[0] == IDC_STATIC)
      continue;

    HWND widget = GetDlgItem(hWnd, d_it[0]);
    if (!widget)
      continue;

    r.left = d_it[1];
    r.top = d_it[2];
    r.right = d_it[3];
    r.bottom = d_it[4];
    MapDialogRect(hWnd, &r);

    int x = r.left + (deltaW * l_it[0]) / 100;
    int y = r.top + (deltaH * l_it[1]) / 100;
    int w = r.right + (deltaW * l_it[2]) / 100;
    int h = r.bottom + (deltaH * l_it[3]) / 100;

    MoveWindow(widget, x, y, w, h, true);
    InvalidateRect(widget, NULL, TRUE);
    InvalidateRgn(widget, NULL, TRUE);
    UpdateWindow(widget);
  }

  UpdateWindow(hWnd);
}

void update_layout(HWND hWnd, LPARAM lParam) { update_layout((DialogLayout *)GetProp(hWnd, PROP_LAYOUT), hWnd, lParam); }

struct Resource
{
  const uint8_t *ptr;
  size_t num;
};

static Resource load_resource(LPCWSTR lpName, LPCWSTR type)
{
  static const Resource nores{0, 0};

  HRSRC hResInfo = FindResourceW(hInstance, lpName, type);
  if (!hResInfo)
    return nores;

  HGLOBAL hResData = LoadResource(hInstance, hResInfo);
  if (!hResData)
    return nores;

  DWORD size = SizeofResource(hInstance, hResInfo);
  if (!size)
    return nores;

  void *ptr = LockResource(hResData);
  if (!ptr)
    return nores;

  return {static_cast<uint8_t *>(ptr), size};
}

static std::vector<uint8_t> read_afx_dialog_layout(Resource res)
{
  std::vector<uint8_t> layout;
  if (res.ptr && res.num > sizeof(WORD))
  {
    res.num = res.num / sizeof(WORD) - 1;
    layout.reserve(res.num);
    const WORD *p = reinterpret_cast<const WORD *>(res.ptr);
    WORD version = *p++;
    if (version == 0)
      while (res.num--)
        layout.push_back(afx_clamp(*p++));
  }
  return layout;
}

struct DlgExInfo
{
  short w, h;
  WORD num_items;
};

static DlgExInfo read_dialog_ex(Resource &res)
{
  struct DLGTEMPLATEEX
  {
    WORD dlgVer;
    WORD signature;
    DWORD helpID;
    DWORD exStyle;
    DWORD style;
    WORD cDlgItems;
    short x;
    short y;
    short cx;
    short cy;
  };
  assert(res.num >= sizeof(DLGTEMPLATEEX));

  const DLGTEMPLATEEX *tpl = reinterpret_cast<const DLGTEMPLATEEX *>(res.ptr);
  if (tpl->signature != 0xFFFF)
  {
    DebugPrint(L"Layout: Only DIALOGEX is supported");
    return {0, 0, 0};
  }

  DlgExInfo dlg_info{tpl->cx, tpl->cy, tpl->cDlgItems};
  res.ptr += sizeof(DLGTEMPLATEEX);

  const WORD *wp = reinterpret_cast<const WORD *>(res.ptr);

  // skip menu
  if (*wp == 0xFFFF)
    wp += 2;
  else
    while (*wp++)
      ;

  // skip class name
  if (*wp == 0xFFFF)
    wp += 2;
  else
    while (*wp++)
      ;

  // skip title
  while (*wp++)
    ;

  // skip font definition (if defined)
  if (tpl->style & DS_SETFONT)
  {
    // skip pointsize, weight, italic + charset
    wp += 3;
    // skip typeface (suppose it's null-terminated)
    while (*wp++)
      ;
  }

  res.ptr = dword_align(reinterpret_cast<const uint8_t *>(wp));

  return dlg_info;
}

static std::vector<int> read_dialog_ex_items(Resource res, WORD cDlgItems)
{
  std::vector<int> items;
  if (!res.ptr || !res.num || !cDlgItems)
    return items;

  items.reserve(5ULL * cDlgItems);

  struct DLGITEMTEMPLATEEX
  {
    DWORD helpID;
    DWORD exStyle;
    DWORD style;
    short x;
    short y;
    short cx;
    short cy;
    DWORD id;
  };
  assert(res.num >= sizeof(DLGITEMTEMPLATEEX));

  for (WORD i = 0; i < cDlgItems; ++i)
  {
    const DLGITEMTEMPLATEEX *tpl = reinterpret_cast<const DLGITEMTEMPLATEEX *>(res.ptr);

    items.push_back(tpl->id);
    items.push_back(tpl->x);
    items.push_back(tpl->y);
    items.push_back(tpl->cx);
    items.push_back(tpl->cy);
    res.ptr += sizeof(DLGITEMTEMPLATEEX);

    const WORD *wp = reinterpret_cast<const WORD *>(res.ptr);

    // skip window class name
    if (*wp == 0xFFFF)
      wp += 2;
    else
      while (*wp++)
        ;

    // skip title
    if (*wp == 0xFFFF)
      wp += 2;
    else
      while (*wp++)
        ;

    // skip extra data
    WORD num = *wp++;

    res.ptr = dword_align(reinterpret_cast<const uint8_t *>(wp) + num);
  }

  return items;
}

static DialogLayout *load_dialog_layout(LPCWSTR dialog_name)
{
  Resource res = load_resource(dialog_name, RT_DIALOG);
  if (!res.ptr || !res.num)
    return 0;

  DlgExInfo dlg_info = read_dialog_ex(res);
  if (!dlg_info.w || !dlg_info.h || !dlg_info.num_items)
    return 0;

  DialogLayout *layout = new DialogLayout;
  layout->dlg_w = dlg_info.w;
  layout->dlg_h = dlg_info.h;
  layout->items = read_dialog_ex_items(res, dlg_info.num_items);
  if (layout->items.empty())
    return 0;

  res = load_resource(dialog_name, L"AFX_DIALOG_LAYOUT");
  if (!res.ptr || !res.num)
    return 0;

  layout->layout = read_afx_dialog_layout(res);
  if (layout->layout.empty())
    return 0;

  return layout;
}

static RECT get_local_rect(HWND hWnd)
{
  RECT rect;
  GetWindowRect(hWnd, &rect);
  MapWindowPoints(HWND_DESKTOP, GetParent(hWnd), (LPPOINT)&rect, 2);
  return rect;
}

static INT_PTR CALLBACK parent_proc_wrapper(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  WNDPROC originalDlgProc = (WNDPROC)GetProp(hWnd, PROP_ORIGINAL_DLG_PROC);
  assert(originalDlgProc);
  switch (message)
  {
    case WM_MOVE:
    case WM_SIZE: return TRUE;

    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
      for (HWND hChild = GetWindow(hWnd, GW_CHILD); hChild; hChild = GetWindow(hChild, GW_HWNDNEXT))
      {
        RECT rect = get_local_rect(hChild);
        WINDOWPOS *wp = (WINDOWPOS *)lParam;
        MoveWindow(hChild, 1, rect.top, wp->cx - 2, rect.bottom - rect.top, TRUE);
      }
      return TRUE;

    default: break;
  }
  return (*originalDlgProc)(hWnd, message, wParam, lParam);
}

static void propagate_update_layout(HWND hWnd, LPARAM lParam)
{
  update_layout(hWnd, lParam);
  for (HWND hChild = GetWindow(hWnd, GW_CHILD); hChild; hChild = GetWindow(hChild, GW_HWNDNEXT))
    propagate_update_layout(hChild, lParam);
}

static INT_PTR CALLBACK rollup_proc_wrapper(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  WNDPROC originalDlgProc = (WNDPROC)GetProp(hWnd, PROP_ORIGINAL_DLG_PROC);
  assert(originalDlgProc);
  switch (message)
  {
    case WM_MOVE:
    case WM_SIZE:
    case WM_WINDOWPOSCHANGING: return TRUE;

    case WM_WINDOWPOSCHANGED:
    {
      WINDOWPOS *wp = (WINDOWPOS *)lParam;
      SetProp(hWnd, PROP_LAYOUT_SIZE, (HANDLE)MAKELPARAM(wp->cx, wp->cy));
      SetTimer(hWnd, IDT_LAYOUT_TIMER, LAYOUT_TIMEOUT, NULL);
    }
      return TRUE;

    case WM_TIMER:
      if (wParam == IDT_LAYOUT_TIMER)
      {
        PostMessage(hWnd, LAYOUT_MESSAGE, 0, (LPARAM)GetProp(hWnd, PROP_LAYOUT_SIZE));
        KillTimer(hWnd, IDT_LAYOUT_TIMER);
      }
      break;

    case LAYOUT_MESSAGE: propagate_update_layout(hWnd, lParam); break;

    case WM_DESTROY: delete (DialogLayout *)GetProp(hWnd, PROP_LAYOUT); break;

    default: break;
  }
  return (*originalDlgProc)(hWnd, message, wParam, lParam);
}

static void hook_dlg_proc(HWND hWnd, DLGPROC dlgProc)
{
  if (!hWnd)
    return;
  WNDPROC orgDlgProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)dlgProc);
  SetProp(hWnd, PROP_ORIGINAL_DLG_PROC, (HANDLE)orgDlgProc);
}

void attach_layout_to_rollup(HWND hWnd, LPCWSTR dialog_name)
{
  DialogLayout *layout = load_dialog_layout(dialog_name);
  hook_dlg_proc(GetParent(hWnd), parent_proc_wrapper);
  hook_dlg_proc(hWnd, rollup_proc_wrapper);
  SetProp(hWnd, PROP_LAYOUT, (HANDLE)layout);
}

void attach_layout_to_dialog(HWND hWnd, LPCWSTR dialog_name)
{
  DialogLayout *layout = load_dialog_layout(dialog_name);
  SetProp(hWnd, PROP_LAYOUT, (HANDLE)layout);
}

#endif
