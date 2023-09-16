#include <max.h>
#include <utilapi.h>
#include <meshadj.h>
#include <mnmath.h>
#include <templt.h>
#include <stdmat.h>
#include "dagor.h"
#include "resource.h"
#include "math3d.h"
#include <string>
#include "debug.h"

std::string wideToStr(const TCHAR *sw);
M_STD_STRING strToWide(const char *sz);

#define ERRMSG_DELAY 3000

#define MAX_CLSNAME 64

typedef int FaceNGr[3];
enum
{
  WORLD_SPACE,
  OBJECT_SPACE,
  TANGENT_SPACE,
};
class PolyBumpUtil : public UtilityObj
{
public:
  TCHAR clsname[MAX_CLSNAME];
  TCHAR output_path[256];
  IUtil *iu;
  Interface *ip;
  HWND hPolyPanel;
  ICustEdit *forwdist_p_eid;
  ICustEdit *backdist_p_eid;
  int antialias;
  int poly_width, poly_height;
  int highpolyid, lowpolyid;
  real backdist_p, forwdist_p;
  real back_weight;
  bool latest_hit;
  int expanddist;
  int space;
  bool selected_faces;
  int map_channel;
  PolyBumpUtil();
  void BeginEditParams(Interface *ip, IUtil *iu);
  void EndEditParams(Interface *ip, IUtil *iu);
  void DeleteThis() {}
  void update_ui() { update_poly_ui(); }
  void update_vars() { update_poly_vars(); }
  void update_poly_dist_ui();
  void update_poly_mapchannels();
  void update_poly_ui();
  void update_poly_vars();

  void InitPoly(HWND hWnd);
  void DestroyPoly(HWND hWnd);
  BOOL polybumpdlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam);

  bool BrowseOutFile(TSTR &file);
  bool select_output();

  void build_normal_map();
  void build_normal_map(BitArray *facesel, Face *face, int numf, Point3 *vert, int numv, TVFace *tf, Point3 *tv, FaceNGr *facengr,
    Point3 *vertnorm, Matrix3 &ltm, int wd, int ht, unsigned int *pixels, class Mesh &hm, Matrix3 &tm, real fordist, real backdist,
    bool nearest, real backw, int expands, INode *high);

  // Compute per-vertex normal in 3dsMax way
  void select_highpoly_model();
  void select_lowpoly_model();
  ULONG get_selected_trinode_id();
};
static PolyBumpUtil util;


class PolyBumpUtilDesc : public ClassDesc
{
public:
  int IsPublic() { return 1; }
  void *Create(BOOL loading = FALSE) { return &util; }
  const TCHAR *ClassName() { return GetString(IDS_POLYBUMPUTIL_NAME); }
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
  SClass_ID SuperClassID() { return UTILITY_CLASS_ID; }
  Class_ID ClassID() { return PolyBumpUtil_CID; }
  const TCHAR *Category() { return GetString(IDS_UTIL_CAT); }
  BOOL NeedsToSave() { return TRUE; }
  IOResult Save(ISave *);
  IOResult Load(ILoad *);
};

//== save bump params
enum
{
  CH_POLYBUMP,
  CH_POLYBUMP2
};

IOResult PolyBumpUtilDesc::Save(ISave *io)
{
  util.update_vars();
  ULONG nw;
  io->BeginChunk(CH_POLYBUMP);
  if (io->Write(&util.poly_width, 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.poly_height, 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.expanddist, 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.forwdist_p, sizeof(real), &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.backdist_p, sizeof(real), &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.back_weight, sizeof(real), &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.latest_hit, sizeof(bool), &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.highpolyid, sizeof(util.highpolyid), &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.lowpolyid, sizeof(util.lowpolyid), &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.space, sizeof(util.space), &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.selected_faces, sizeof(bool), &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.map_channel, sizeof(int), &nw) != IO_OK)
    return IO_ERROR;
  if (io->WriteCString(util.output_path) != IO_OK)
    return IO_ERROR;
  io->EndChunk();
  return IO_OK;
}

IOResult PolyBumpUtilDesc::Load(ILoad *io)
{
  ULONG nr;
  TCHAR *str;
  while (io->OpenChunk() == IO_OK)
  {
    switch (io->CurChunkID())
    {
      case CH_POLYBUMP:
      {
        if (io->Read(&util.poly_width, 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.poly_height, 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.expanddist, 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.forwdist_p, sizeof(real), &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.backdist_p, sizeof(real), &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.back_weight, sizeof(real), &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.latest_hit, sizeof(bool), &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.highpolyid, sizeof(util.highpolyid), &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.lowpolyid, sizeof(util.lowpolyid), &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.space, sizeof(util.space), &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.selected_faces, sizeof(bool), &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.map_channel, sizeof(int), &nr) != IO_OK)
          return IO_ERROR;
        memset(util.output_path, 0, sizeof(util.output_path));
        if (io->ReadCStringChunk(&str) != IO_OK)
          return IO_ERROR;
        _tcsncpy(util.output_path, str, 255);
      }
      break;
      case CH_POLYBUMP2:
      {
        if (io->Read(&util.antialias, sizeof(int), &nr) != IO_OK)
          return IO_ERROR;
      }
      break;
    }
    io->CloseChunk();
  }
  util.update_ui();
  return IO_OK;
}

static PolyBumpUtilDesc utilDesc;
ClassDesc *GetPolyBumpCD() { return &utilDesc; }

bool get_edint(ICustEdit *e, int &a);
bool get_edfloat(ICustEdit *e, float &a);
bool get_chkbool(int id, char &chk, HWND pan = NULL);
bool get_chkbool(int id, bool &chk, HWND pan = NULL)
{
  char a;
  if (get_chkbool(id, a, pan))
  {
    chk = (a == BST_CHECKED ? true : false);
    return true;
  }
  chk = (a == BST_CHECKED ? true : false);
  return false;
}


static INT_PTR CALLBACK PolyBumpUtilPolyDlgProc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return util.polybumpdlg_proc(hw, msg, wParam, lParam);
}


BOOL PolyBumpUtil::polybumpdlg_proc(HWND hw, UINT msg, WPARAM wpar, LPARAM lpar)
{
  switch (msg)
  {
    case WM_INITDIALOG: InitPoly(hw); break;
    case WM_DESTROY: DestroyPoly(hw); break;
    case WM_CUSTEDIT_ENTER:
      update_poly_vars();
      update_poly_dist_ui();
      break;
    case WM_COMMAND:
      switch (LOWORD(wpar))
      {
        case IDC_START: build_normal_map(); break;
        case IDC_LOWPOLY: select_lowpoly_model(); break;
        case IDC_HIGHPOLY: select_highpoly_model(); break;
        case IDC_SET_OUTPUTPATH: select_output(); break;
        case IDC_MAPCHANNEL:
          if (HIWORD(wpar) == CBN_SELCHANGE)
          {
            char buf[256];
            buf[255] = 0;
            SendMessage(GetDlgItem(hPolyPanel, IDC_MAPCHANNEL), WM_GETTEXT, 255, (LPARAM)buf);
            map_channel = atoi(buf);
          }
          break;
      }
      break;
    default: return FALSE;
  }
  return TRUE;
}

PolyBumpUtil::PolyBumpUtil()
{
  _tcscpy(clsname, _T(""));
  iu = NULL;
  ip = NULL;
  hPolyPanel = NULL;
  forwdist_p_eid = backdist_p_eid = NULL;
  poly_height = poly_width = 256;
  highpolyid = lowpolyid = 0;
  expanddist = 32;
  backdist_p = 3;
  forwdist_p = 10;
  back_weight = 0.3f;
  latest_hit = false;
  space = OBJECT_SPACE;
  selected_faces = false;
  map_channel = 1;
  antialias = 0;
  memset(output_path, 0, sizeof(output_path));
}

void PolyBumpUtil::BeginEditParams(Interface *ip, IUtil *iu)
{
  this->iu = iu;
  this->ip = ip;
  hPolyPanel =
    ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_POLYBUMP), PolyBumpUtilPolyDlgProc, GetString(IDS_POLYBUMPUTIL_NAME), 0);
}

void PolyBumpUtil::EndEditParams(Interface *ip, IUtil *iu)
{
  this->iu = NULL;
  this->ip = NULL;
  if (hPolyPanel)
    ip->DeleteRollupPage(hPolyPanel);
  hPolyPanel = NULL;
}

void PolyBumpUtil::update_poly_dist_ui()
{
  if (!hPolyPanel)
    return;
  char buf[256];
  SendMessage(GetDlgItem(hPolyPanel, IDC_LOWPOLYNAME), WM_SETTEXT, 0, (LPARAM) "(none)");
  if (lowpolyid)
  {
    INode *node = ip->GetINodeByHandle(lowpolyid);
    if (node)
    {
      SendMessage(GetDlgItem(hPolyPanel, IDC_LOWPOLYNAME), WM_SETTEXT, 0, (LPARAM)node->GetName());
      Object *ob = node->GetObjectRef();
      if (ob)
      {
        Matrix3 tm = node->GetObjectTM(ip->GetTime());
        Box3 bbox;
        ob->GetDeformBBox(ip->GetTime(), bbox, &tm);
        real lg = Length(bbox.Width()) * 0.5f;
        real raylg = forwdist_p * lg;
        if (raylg >= 1000)
        {
          sprintf(buf, "= %.2f m", raylg / 100);
        }
        else
        {
          sprintf(buf, "= %.2f cm", raylg);
        }
        SendMessage(GetDlgItem(hPolyPanel, IDC_FDIST_CM), WM_SETTEXT, 0, LPARAM(buf));
        raylg = backdist_p * lg;
        if (raylg >= 1000)
        {
          sprintf(buf, "= %.2f m", raylg / 100);
        }
        else
        {
          sprintf(buf, "= %.2f cm", raylg);
        }
        SendMessage(GetDlgItem(hPolyPanel, IDC_BDIST_CM), WM_SETTEXT, 0, LPARAM(buf));
      }
    }
  }
}

void PolyBumpUtil::update_poly_mapchannels()
{
  if (!hPolyPanel)
    return;
  char buf[256];
  if (lowpolyid)
  {
    INode *node = ip->GetINodeByHandle(lowpolyid);
    if (!node)
      return;
    Object *ob = node->GetObjectRef();
    if (!ob)
      return;
    TriObject *lowtri = (TriObject *)ob->ConvertToType(ip->GetTime(), Class_ID(TRIOBJ_CLASS_ID, 0));
    if (!lowtri)
      return;
    Mesh &lowm = ((TriObject *)lowtri)->mesh;

    SendMessage(GetDlgItem(hPolyPanel, IDC_MAPCHANNEL), CB_RESETCONTENT, 0, 0);
    int nearest_supported = MAX_MESHMAPS * 2;
    for (int i = 1; i < MAX_MESHMAPS; ++i)
    {
      if (lowm.mapSupport(i))
      {
        if (abs(map_channel - i) < abs(map_channel - nearest_supported))
          nearest_supported = i;
        sprintf(buf, "%d", i);
        SendMessage(GetDlgItem(hPolyPanel, IDC_MAPCHANNEL), CB_ADDSTRING, 0, (LPARAM)buf);
      }
    }
    if (!lowm.mapSupport(map_channel))
    {
      map_channel = nearest_supported;
      if (map_channel >= MAX_MESHMAPS)
        map_channel = MAX_MESHMAPS - 1;
    }
    sprintf(buf, "%d", map_channel);
    SendMessage(GetDlgItem(hPolyPanel, IDC_MAPCHANNEL), WM_SETTEXT, 0, (LPARAM)buf);

    if (lowtri != ob)
      lowtri->DeleteMe();
  }
}

void PolyBumpUtil::update_poly_ui()
{
  if (!hPolyPanel)
    return;
  char buf[256];
  if (space == TANGENT_SPACE)
  {
    SendMessage(GetDlgItem(hPolyPanel, IDC_SPACE), WM_SETTEXT, 0, (LPARAM) "Tangent space");
  }
  else if (space == OBJECT_SPACE)
  {
    space = OBJECT_SPACE;
    SendMessage(GetDlgItem(hPolyPanel, IDC_SPACE), WM_SETTEXT, 0, (LPARAM) "Object space");
  }
  else
  {
    space = WORLD_SPACE;
    SendMessage(GetDlgItem(hPolyPanel, IDC_SPACE), WM_SETTEXT, 0, (LPARAM) "World space");
  }

  sprintf(buf, "%d", poly_width);
  SendMessage(GetDlgItem(hPolyPanel, IDC_WIDTH), WM_SETTEXT, 0, (LPARAM)buf);
  sprintf(buf, "%d", poly_height);
  SendMessage(GetDlgItem(hPolyPanel, IDC_HEIGHT), WM_SETTEXT, 0, (LPARAM)buf);

  if (expanddist)
    sprintf(buf, "%d", expanddist);
  else
    sprintf(buf, "None");

  SendMessage(GetDlgItem(hPolyPanel, IDC_EXPAND), WM_SETTEXT, 0, (LPARAM)buf);
  SendMessage(GetDlgItem(hPolyPanel, IDC_HIGHPOLYNAME), WM_SETTEXT, 0, (LPARAM) "(none)");

  if (highpolyid)
  {
    INode *node = ip->GetINodeByHandle(highpolyid);
    if (node)
    {
      SendMessage(GetDlgItem(hPolyPanel, IDC_HIGHPOLYNAME), WM_SETTEXT, 0, (LPARAM)node->GetName());
    }
  }
  if (forwdist_p_eid)
  {
    forwdist_p_eid->SetText(forwdist_p);
  }
  if (backdist_p_eid)
  {
    backdist_p_eid->SetText(backdist_p);
  }
  update_poly_dist_ui();

  /*int i;
  for(i=0;i<12;++i) if( (1<<i)>=poly_width) break;
  SendMessage(GetDlgItem(hPolyPanel,IDC_WIDTH),CB_SETCURSEL,i-4,0);
  for(i=0;i<12;++i) if( (1<<i)>=poly_height) break;
  SendMessage(GetDlgItem(hPolyPanel,IDC_HEIGHT),CB_SETCURSEL,i-4,0);*/
  CheckDlgButton(hPolyPanel, IDC_LATEST, latest_hit ? BST_CHECKED : BST_UNCHECKED);
  CheckDlgButton(hPolyPanel, IDC_SELFACES, selected_faces ? BST_CHECKED : BST_UNCHECKED);
  SendMessage(GetDlgItem(hPolyPanel, IDC_BACK_WEIGHT), TBM_SETPOS, (WPARAM)TRUE, (LPARAM) int(back_weight * 1000));
  SendMessage(GetDlgItem(hPolyPanel, IDC_OUTPUTPATH), WM_SETTEXT, 0, (LPARAM)output_path);
  update_poly_mapchannels();
}

void PolyBumpUtil::update_poly_vars()
{
  if (!hPolyPanel)
    return;
  // debug("try get");
  char buf[256];
  buf[255] = 0;
  SendMessage(GetDlgItem(hPolyPanel, IDC_WIDTH), WM_GETTEXT, 255, (LPARAM)buf);
  poly_width = atoi(buf);
  SendMessage(GetDlgItem(hPolyPanel, IDC_HEIGHT), WM_GETTEXT, 255, (LPARAM)buf);
  poly_height = atoi(buf);
  SendMessage(GetDlgItem(hPolyPanel, IDC_EXPAND), WM_GETTEXT, 255, (LPARAM)buf);
  expanddist = atoi(buf);
  SendMessage(GetDlgItem(hPolyPanel, IDC_SPACE), WM_GETTEXT, 255, (LPARAM)buf);
  if (strnicmp(buf, "tangent", strlen("tangent")) == 0)
  {
    space = TANGENT_SPACE;
  }
  else if (strnicmp(buf, "object", strlen("object")) == 0)
  {
    space = OBJECT_SPACE;
  }
  else
  {
    space = WORLD_SPACE;
  }
  SendMessage(GetDlgItem(hPolyPanel, IDC_MAPCHANNEL), WM_GETTEXT, 255, (LPARAM)buf);
  map_channel = atoi(buf);
  get_chkbool(IDC_SELFACES, selected_faces, hPolyPanel);
  get_chkbool(IDC_LATEST, latest_hit, hPolyPanel);
  get_edfloat(forwdist_p_eid, forwdist_p);
  get_edfloat(backdist_p_eid, backdist_p);
  /*poly_width=1<<(4+SendMessage( GetDlgItem(hPolyPanel,IDC_WIDTH),CB_GETCURSEL,0,0));
  poly_height=1<<(4+SendMessage( GetDlgItem(hPolyPanel,IDC_HEIGHT),CB_GETCURSEL,0,0));*/
  // debug("got %d %d",poly_width,poly_height);
  back_weight = SendMessage(GetDlgItem(hPolyPanel, IDC_BACK_WEIGHT), TBM_GETPOS, (WPARAM)0, (LPARAM)0) / 1000.0;
}

void PolyBumpUtil::InitPoly(HWND hw)
{
  // Width
  hPolyPanel = hw;
  char buf[256];
  int i;
  for (i = 4; i <= 11; ++i)
  {
    sprintf(buf, "%d", 1 << i);
    SendMessage(GetDlgItem(hw, IDC_WIDTH), CB_ADDSTRING, 0, (LPARAM)buf);
    SendMessage(GetDlgItem(hw, IDC_HEIGHT), CB_ADDSTRING, 0, (LPARAM)buf);
  }
  SendMessage(GetDlgItem(hw, IDC_SPACE), CB_ADDSTRING, 0, (LPARAM) "World space");
  SendMessage(GetDlgItem(hw, IDC_SPACE), CB_ADDSTRING, 0, (LPARAM) "Object space");
  SendMessage(GetDlgItem(hw, IDC_SPACE), CB_ADDSTRING, 0, (LPARAM) "Tangent space");

  // SendMessage(GetDlgItem(hw,IDC_WIDTH),  WM_SETTEXT, 0, (LPARAM) str[8-4]);
  SendMessage(GetDlgItem(hPolyPanel, IDC_WIDTH), CB_SETCURSEL, 4, 0);
  SendMessage(GetDlgItem(hPolyPanel, IDC_HEIGHT), CB_SETCURSEL, 4, 0);
  // SendMessage(GetDlgItem(hw,IDC_HEIGHT), WM_SETTEXT, 0, (LPARAM) str[8-4]);

  sprintf(buf, "None");
  SendMessage(GetDlgItem(hw, IDC_EXPAND), CB_ADDSTRING, 0, (LPARAM)buf);
  for (i = 0; i <= 6; ++i)
  {
    sprintf(buf, "%d", 1 << i);
    SendMessage(GetDlgItem(hw, IDC_EXPAND), CB_ADDSTRING, 0, (LPARAM)buf);
  }
  sprintf(buf, "%d", 2048);
  SendMessage(GetDlgItem(hw, IDC_EXPAND), CB_ADDSTRING, 0, (LPARAM)buf);

  forwdist_p_eid = GetICustEdit(GetDlgItem(hw, IDC_FDIST_PERCENT));
  backdist_p_eid = GetICustEdit(GetDlgItem(hw, IDC_BDIST_PERCENT));

  SendMessage(GetDlgItem(hPolyPanel, IDC_BACK_WEIGHT), TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 1000));
  SendMessage(GetDlgItem(hPolyPanel, IDC_BACK_WEIGHT), TBM_SETTIC, (WPARAM)0, (LPARAM)500);

  update_poly_ui();
}
void PolyBumpUtil::DestroyPoly(HWND hw)
{
  update_poly_vars();
#define reled(e)         \
  if (e)                 \
  {                      \
    ReleaseICustEdit(e); \
    e = NULL;            \
  }
  reled(forwdist_p_eid);
  reled(backdist_p_eid);
#undef reled
}

struct FaceArea
{
  int p0, p1;
  bool lit;
};

__forceinline real lengthSq(const Point2 &p) { return p.x * p.x + p.y * p.y; }
__forceinline real length(const Point2 &p) { return Length(p); }

template <typename To, typename From>
To bitwise_cast(const From &from)
{
  union
  {
    To t;
    From f;
  } x;
  x.f = from;
  return x.t;
}

INLINE int real2int(float x)
{
  const int magic = (150 << 23) | (1 << 22);
  x += bitwise_cast<float, int>(magic);
  return bitwise_cast<int, float>(x) - magic;
}
typedef unsigned int uint;

static inline uint store_point2(const Point2 &p) { return (real2int(p.x * (1 << 14)) << 16) | (real2int(p.y * (1 << 14)) & 0xFFFF); }

static inline Point2 get_point2(uint a) { return Point2((int(a) >> 16) / real(1 << 14), short(a & 0xFFFF) / real(1 << 14)); }

__declspec(dllexport) int save_tga32(const char *fn, char *ptr, int wd, int ht, int stride)
{
  if (!ptr)
    return 0;
  FILE *h = fopen(fn, "wb");
  if (!h)
    return 0;
  fwrite("\0\0\2\0\0\0\0\0\0\0\0\0", 12, 1, h);
  fwrite(&wd, 2, 1, h);
  fwrite(&ht, 2, 1, h);
  fwrite("\x20\x00", 2, 1, h);
  char *p = (char *)ptr + stride * (ht - 1);
  for (int i = 0; i < ht; ++i, p -= stride)
    fwrite(p, wd * 4, 1, h);
  fclose(h);
  return 1;
}

class VNormal
{
public:
  Point3 norm;
  DWORD smooth;
  VNormal *next;
  BOOL init;
  int index, fi;
  VNormal()
  {
    smooth = 0;
    next = NULL;
    init = FALSE;
    index = 0;
    norm = Point3(0, 0, 0);
  }

  VNormal(Point3 &n, DWORD s, int f)
  {
    next = NULL;
    init = TRUE;
    norm = n;
    smooth = s;
    fi = f;
  }

  ~VNormal()
  {
    if (next)
      delete next;
    next = NULL;
  }

  void AddNormal(Point3 &n, DWORD s, int fi);

  Point3 &GetNormal(DWORD s, int f);

  void Normalize();
  void SetIndex(int &ind)
  {
    index = ind;
    ind++;
    if (next)
      next->SetIndex(ind);
  }
  int GetIndex(DWORD s, int f)
  {
    if (!next)
      return index;
    if (!smooth && f == fi)
      return index;
    if (smooth & s)
      return index;
    else
      return next->GetIndex(s, f);
  }
};

// Add a normal to the list if the smoothing group bits overlap,
// otherwise create a new vertex normal in the list
void VNormal::AddNormal(Point3 &n, DWORD s, int f)
{

  if (!(s & smooth) && init)
  {
    if (next)
      next->AddNormal(n, s, f);
    else
    {
      next = new VNormal(n, s, f);
    }
  }
  else
  {
    norm += n;
    smooth |= s;
    fi = f;
    init = TRUE;
  }
}

// Retrieves a normal if the smoothing groups overlap or there is
// only one in the list

Point3 &VNormal::GetNormal(DWORD s, int f)
{
  if (!next)
    return norm;
  if (!smooth && f == fi)
    return norm;
  if (smooth & s)
    return norm;
  else
    return next->GetNormal(s, f);
}


// Normalize each normal in the list

void VNormal::Normalize()
{
  VNormal *ptr = next, *prev = this;
  while (ptr)
  {
    if (ptr->smooth & smooth)
    {
      norm += ptr->norm;
      prev->next = ptr->next;
      delete ptr;
      ptr = prev->next;
    }
    else
    {
      prev = ptr;
      ptr = ptr->next;
    }
  }

  norm = ::Normalize(norm);
  if (next)
    next->Normalize();
}

// Compute the face and vertex normals

void ComputeVertexNormals(Mesh *mesh, Tab<Point3> &vnrm, Tab<FaceNGr> &fngr, Point3 *tverts)
{

  Face *face;
  Point3 *verts;
  Point3 v0, v1, v2;
  Tab<VNormal> vnorms;

  Tab<Point3> fnorms;
  fngr.SetCount(mesh->getNumFaces());


  face = mesh->faces;

  verts = tverts ? tverts : mesh->verts;

  vnorms.SetCount(mesh->getNumVerts());

  fnorms.SetCount(mesh->getNumFaces());


  // Compute face and vertex surface normals

  int i;
  for (i = 0; i < mesh->getNumVerts(); i++)
  {
    vnorms[i] = VNormal();
  }

  for (i = 0; i < mesh->getNumFaces(); i++, face++)
  {
    // Calculate the surface normal
    v0 = verts[face->v[0]];
    v1 = verts[face->v[1]];
    v2 = verts[face->v[2]];

    fnorms[i] = (v1 - v0) ^ (v2 - v1);

    fnorms[i] = Normalize(fnorms[i]);
    for (int j = 0; j < 3; j++)
    {
      vnorms[face->v[j]].AddNormal(fnorms[i], face->smGroup, i);
    }
  }

  int totalnorms = 0;
  for (i = 0; i < mesh->getNumVerts(); i++)
  {
    vnorms[i].Normalize();
    vnorms[i].SetIndex(totalnorms);
  }
  vnrm.SetCount(totalnorms);

  face = mesh->faces;
  for (i = 0; i < mesh->getNumFaces(); i++, face++)
  {
    for (int j = 0; j < 3; j++)
    {
      fngr[i][j] = vnorms[face->v[j]].GetIndex(face->smGroup, i);
      vnrm[fngr[i][j]] = vnorms[face->v[j]].GetNormal(face->smGroup, i);
    }
  }

  for (i = 0; i < mesh->getNumVerts(); i++)
  {
    vnorms[i].~VNormal();
  }
}

static BOOL file_exists(TCHAR *filename)
{
  HANDLE findhandle;
  WIN32_FIND_DATA file;
  findhandle = FindFirstFile(filename, &file);
  FindClose(findhandle);
  if (findhandle == INVALID_HANDLE_VALUE)
    return FALSE;
  else
    return TRUE;
}

bool PolyBumpUtil::BrowseOutFile(TSTR &file)
{
  // BOOL silent = TheManager->SetSilentMode(TRUE);
  // BitmapInfo biOutFile;
  FilterList fl;

  OPENFILENAME ofn;
  // get OPENFILENAME items
  HWND hWnd = GetCOREInterface()->GetMAXHWnd();
  TCHAR file_name[260];
  _tcsncpy(file_name, file, 260);
  TSTR cap_str = GetString(IDS_SELECT_OUTPUT);

  fl.Append(GetString(IDS_FILTER_TARGA));
  fl.Append(GetString(IDS_FILTER_TARGA_DESC));
  fl.Append(GetString(IDS_FILTER_ALL));
  fl.Append(GetString(IDS_FILTER_ALL_DESC));

  // setup OPENFILENAME
  memset(&ofn, 0, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hWnd;
  ofn.hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
  // WIN64 Cleanup: Shuler
  ofn.lpstrFile = file_name;
  ofn.nMaxFile = sizeof(file_name) / sizeof(TCHAR);
  ofn.lpstrFilter = fl;
  ofn.nFilterIndex = 1;
  ofn.lpstrTitle = cap_str;
  ofn.lpstrFileTitle = _T("");
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = _T("");
  ofn.Flags = OFN_HIDEREADONLY;
  // ofn.lpstrDefExt=_T("tga");

  // throw up the dialog
  while (GetSaveFileName(&ofn))
  {
    TCHAR *fn = new TCHAR[_tcslen(ofn.lpstrFile) + 4 + 1], *p;
    DWORD i;
    _tcscpy(fn, ofn.lpstrFile);
    TCHAR type[_MAX_PATH];
    // BMMSplitFilename(ofn.lpstrFile, NULL, NULL, type);
    // char drv[_MAX_DRIVE],dir[_MAX_DIR],fn[_MAX_PATH];
    _tsplitpath(ofn.lpstrFile, NULL, NULL, NULL, type);
    if (type[0] == 0 || _tcschr(type, _T('\\')) != NULL) // no suffix, add from filter types if not *.*
    {
      for (i = 1, p = (TCHAR *)ofn.lpstrFilter; i < ofn.nFilterIndex; i++)
      {
        while (*p)
          p++;
        p++;
        while (*p)
          p++;
        p++; // skip to indexed type
      }
      while (*p)
        p++;
      p++;
      p++;                                  // jump descripter, '*'
      if (*p == _T('.') && p[1] != _T('*')) // add if .xyz
        _tcscat(fn, p);
    }

    if (file_exists(fn))
    {
      // file already exists, query owverwrite
      TCHAR buf[256];
      MessageBeep(MB_ICONEXCLAMATION);
      _stprintf(buf, _T("\"%s\" %s"), fn, GetString(IDS_FILE_REPLACE));
      if (MessageBox(hWnd, buf, GetString(IDS_SAVE_AS), MB_ICONQUESTION | MB_YESNO) == IDNO)
      {
        // don't overwrite, remove the path from the name, try again
        _tcscpy(buf, file_name + ofn.nFileOffset);
        _tcscpy(file_name, buf);
        delete[] fn;
        continue;
      }
    }
    file = fn;
    delete[] fn;

    return true;
    break;
  }
  return false;
}

bool PolyBumpUtil::select_output()
{
  TSTR file = output_path;
  if (BrowseOutFile(file))
  {
    _tcsncpy(output_path, file, 255);
    SendMessage(GetDlgItem(hPolyPanel, IDC_OUTPUTPATH), WM_SETTEXT, 0, (LPARAM)output_path);
    return true;
  }
  return false;
}

static DWORD WINAPI dummyprogressfn(LPVOID arg) { return 0; }

void PolyBumpUtil::build_normal_map()
{
  update_poly_vars();
  TimeValue time = ip->GetTime();
  INode *lownode = NULL, *highnode = NULL;
  if (lowpolyid)
  {
    lownode = ip->GetINodeByHandle(lowpolyid);
    if (lownode)
    {
      SendMessage(GetDlgItem(hPolyPanel, IDC_LOWPOLYNAME), WM_SETTEXT, 0, (LPARAM)lownode->GetName());
    }
  }
  if (highpolyid)
  {
    highnode = ip->GetINodeByHandle(highpolyid);
    if (highnode)
    {
      SendMessage(GetDlgItem(hPolyPanel, IDC_HIGHPOLYNAME), WM_SETTEXT, 0, (LPARAM)highnode->GetName());
    }
  }

  if (!lownode)
  {
    ip->DisplayTempPrompt(GetString(IDS_INVALID_LOWPOLYNODE));
    return;
  }
  Object *lowo = lownode->GetObjectRef();
  if (!lowo)
  {
    ip->DisplayTempPrompt(GetString(IDS_INVALID_LOWPOLYNODE));
    return;
  }
  TriObject *lowtri = (TriObject *)lowo->ConvertToType(time, Class_ID(TRIOBJ_CLASS_ID, 0));
  if (!lowtri)
  {
    ip->DisplayTempPrompt(GetString(IDS_CANTCONVERT_LOWPOLYNODE));
    return;
  }

  if (!highnode)
  {
    ip->DisplayTempPrompt(GetString(IDS_INVALID_HIGHPOLYNODE));
    if (lowtri != lowo)
      lowtri->DeleteMe();
    return;
  }
  Object *higho = highnode->GetObjectRef();
  if (!higho)
  {
    ip->DisplayTempPrompt(GetString(IDS_INVALID_HIGHPOLYNODE));
    if (lowtri != lowo)
      lowtri->DeleteMe();
    return;
  }
  TriObject *hightri = (TriObject *)higho->ConvertToType(time, Class_ID(TRIOBJ_CLASS_ID, 0));
  if (!hightri)
  {
    ip->DisplayTempPrompt(GetString(IDS_CANTCONVERT_HIGHPOLYNODE));
    if (lowtri != lowo)
      lowtri->DeleteMe();
    return;
  }
  if (!output_path[0])
  {
    if (!select_output())
    {
      ip->DisplayTempPrompt(GetString(IDS_CANCELLED));
      if (lowtri != lowo)
        lowtri->DeleteMe();
      if (hightri != higho)
        hightri->DeleteMe();
      return;
    }
  }

  Mesh &lowm = ((TriObject *)lowtri)->mesh;
  Mesh &highm = ((TriObject *)hightri)->mesh;
  if (!lowm.mapSupport(map_channel) || !lowm.mapFaces(map_channel))
  {
    ip->DisplayTempPrompt(GetString(IDS_INVALIDMAPCHANNEL));
    if (lowtri != lowo)
      lowtri->DeleteMe();
    if (hightri != higho)
      hightri->DeleteMe();
    return;
  }

  uint *pixels = new uint[poly_width * poly_height];
  Tab<Point3> lowvnrm;
  Tab<FaceNGr> lowfngr;
  Tab<Point3> lowvert;
  lowvert.SetCount(lowm.numVerts);

  Matrix3 ltm = lownode->GetObjTMAfterWSM(time);
  Point3 scale(Length(ltm.GetRow(0)), Length(ltm.GetRow(1)), Length(ltm.GetRow(2)));
  if (space != WORLD_SPACE)
  {
    if (fabs(scale.x - scale.y) >= scale.x * 1e-3 || fabs(scale.z - scale.y) >= scale.y * 1e-3)
    {
      int retval = MessageBox(ip->GetMAXHWnd(), _T("Non-unirform scale applied to object. Continue anyway?"),
        _T("Non-uniform scale warning"), MB_ICONQUESTION | MB_YESNO);
      if (retval == IDYES)
        ; // no-op
      else if (retval == IDNO)
      {
        if (lowtri != lowo)
          lowtri->DeleteMe();
        if (hightri != higho)
          hightri->DeleteMe();
        delete[] pixels;
        return;
      }
    }
  }

  for (int v = 0; v < lowm.numVerts; ++v)
    lowvert[v] = ltm.PointTransform(lowm.verts[v]);
  ComputeVertexNormals(&lowm, lowvnrm, lowfngr, lowvert.Addr(0));


  memset(pixels, 0, poly_width * poly_height * 4);

  Box3 bbox;
  lowo->GetDeformBBox(time, bbox, &ltm);
  real lg = Length(bbox.Width()) * 0.5f;

  if (selected_faces && lowm.FaceSel().NumberSet() == 0)
  {
    ip->DisplayTempPrompt(GetString(IDS_EMPTYSELECTION));
  }
  else
  {
    ip->ProgressStart(_T("Working..."), TRUE, dummyprogressfn, NULL);
    build_normal_map(selected_faces ? &lowm.FaceSel() : NULL, lowm.faces, lowm.numFaces, lowvert.Addr(0), lowm.numVerts,
      lowm.mapFaces(map_channel), lowm.mapVerts(map_channel), lowfngr.Addr(0), lowvnrm.Addr(0), Inverse(ltm), poly_width, poly_height,
      pixels, highm, highnode->GetObjTMAfterWSM(time), lg * forwdist_p / 100.0f, lg * backdist_p / 100.0f, !latest_hit, back_weight,
      expanddist, highnode);
    if (ip->GetCancel())
    {
      ip->ProgressEnd();
      ip->DisplayTempPrompt(GetString(IDS_CANCELLED));
    }
    else
    {
      ip->ProgressEnd();

      std::string s = wideToStr(output_path);

      if (!save_tga32(s.c_str(), (char *)pixels, poly_width, poly_height, poly_width * 4))
      {
        ip->DisplayTempPrompt(GetString(IDS_CANTWRITE_FILE));
      }
      else
      {
        ip->DisplayTempPrompt(GetString(IDS_SUCCESS));
      }
    }
  }

  if (lowtri != lowo)
    lowtri->DeleteMe();
  if (hightri != higho)
    hightri->DeleteMe();
  delete[] pixels;
}

static inline Point2 Point3to2(const Point3 &p) { return Point2(p.x, 1 - p.y); }

ULONG PolyBumpUtil::get_selected_trinode_id()
{
  int n = ip->GetSelNodeCount();
  if (n != 1)
  {
    ip->DisplayTempPrompt(_T("Multiply objects selected"));
    return 0;
  }
  INode *node = ip->GetSelNode(0);
  if (!node)
    return 0;
  Object *o = node->GetObjectRef();
  if (!o)
  {
    ip->DisplayTempPrompt(_T("Helper Node!"));
    return 0;
  }
  if (!o->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
  {
    ip->DisplayTempPrompt(_T("Not a Editable Mesh convertable"));
    return 0;
  }
  return node->GetHandle();
}

void PolyBumpUtil::select_lowpoly_model()
{
  lowpolyid = get_selected_trinode_id();
  update_poly_ui();
}

void PolyBumpUtil::select_highpoly_model()
{
  highpolyid = get_selected_trinode_id();
  update_poly_ui();
}

bool expand(Point3 *nrms, BitArray &hits, int wd, int ht)
{
  BitArray setnow(wd * ht);
  setnow.ClearAll();
  int unset = 0;
  for (int y = 0, i = 0; y < ht; ++y)
  {
    for (int x = 0; x < wd; ++x, ++i)
    {
      if (nrms[i].x > 2)
      {
        // Unset Normal
        int w_hits_n = 0, wo_hits_n = 0;
        Point3 w_hits(0, 0, 0), w_any, wo_hits(0, 0, 0), wo_any;
        for (int ly = (y == 0 ? 0 : y - 1); ly <= (y == ht - 1 ? ht - 1 : y + 1); ++ly)
        {
          for (int lx = (x == 0 ? 0 : x - 1); lx <= (x == wd - 1 ? wd - 1 : x + 1); ++lx)
            if (abs(lx - x) + abs(ly - y) == 1)
            {
              int ind = lx + ly * wd;
              if (nrms[ind].x > 2 || setnow[ind])
                continue;
              if (hits[ind])
              {
                w_hits_n++;
                w_hits += nrms[ind];
                w_any = nrms[ind];
              }
              else
              {
                wo_hits_n++;
                wo_hits += nrms[ind];
                wo_any = nrms[ind];
              }
            }
        }
        if (w_hits_n)
        {
          float lg = Length(w_hits);
          if (lg < 1e-5)
            nrms[i] = w_any;
          else
            nrms[i] = w_hits / lg;
          setnow.Set(i);
        }
        else if (wo_hits_n)
        {
          float lg = Length(wo_hits);
          if (lg < 1e-5)
            nrms[i] = wo_any;
          else
            nrms[i] = wo_hits / lg;
          setnow.Set(i);
        }
        else
          unset++;
      }
    }
  }
  if (unset == 0 || unset == wd * ht)
    return false;
  return true;
}

StaticMeshRayTracer *create_staticmeshraytracer(Mesh &m);


static Point3 pabs(const Point3 &a) { return Point3(fabs(a.x), fabs(a.y), fabs(a.z)); }
float ComputeFaceCurvature(Point3 *n, Point3 *v);

void compute_bump_vectors(const Point3 tv[3], const Point3 v[3], Point3 bvec[3]);

class BumpMapSC : public ShadeContext
{
public:
  TimeValue curtime;
  INode *node;
  Mesh *mesh;
  int face;
  // Matrix3 cam2obj,obj2cam;
  Box3 objbox;
  Point3 bary, norm, normal, facenorm, view, orgview, pt, dpt, dpt_dx, dpt_dy, vp[3], bv[3], bo;
  Point3 tv[MAX_MESHMAPS][3];
  float curve, face_sz, sz_ratio, facecurve;
  Tab<Point3> fnorm;
  // Tab<Point3[3]> vnorm;
  Point3 *highvnrm;
  FaceNGr *facengr;
  bool hastv[MAX_MESHMAPS];

  BumpMapSC(TimeValue t, Interface *ip)
  {
    curtime = t;
    doMaps = TRUE;
    filterMaps = TRUE;
    backFace = FALSE;
    node = NULL;
    mesh = NULL;
    face = -1;
    ambientLight = Color(0, 0, 0);
  }
  void setnode(INode *n)
  {
    node = n;
    /*if(n) {
      obj2cam=n->GetObjectTM(curtime);
      cam2obj=Inverse(obj2cam);
    }*/
  }
  void setmesh(Mesh &m, Point3 *vnr, FaceNGr *ngr)
  {
    mesh = &m;
    objbox = m.getBoundingBox();
    fnorm.SetCount(m.numFaces);
    int i;
    for (i = 0; i < m.numFaces; ++i)
      fnorm[i] =
        Normalize(CrossProd(m.verts[m.faces[i].v[1]] - m.verts[m.faces[i].v[0]], m.verts[m.faces[i].v[2]] - m.verts[m.faces[i].v[0]]));
    highvnrm = vnr;
    facengr = ngr;
  }
  void gettv(int ch)
  {
    if (hastv[ch])
      return;
    if (mesh->mapSupport(ch))
    {
      Point3 *t = mesh->mapVerts(ch);
      TVFace &f = *(mesh->mapFaces(ch) + face);
      for (int i = 0; i < 3; ++i)
        tv[ch][i] = t[f.t[i]];
    }
    else
    {
      tv[ch][0] = Point3(0, 0, 0);
      tv[ch][1] = Point3(0, 0, 0);
      tv[ch][2] = Point3(0, 0, 0);
    }
    hastv[ch] = true;
  }
  void setface(int fi, int ch)
  {
    face = fi;
    Face &f = mesh->faces[fi];
    mtlNum = f.getMatID();
    int i;
    for (i = 0; i < 3; ++i)
      vp[i] = mesh->verts[f.v[i]]; //*obj2cam;
    for (i = 0; i < MAX_MESHMAPS; ++i)
      hastv[i] = false;
    face_sz = fabsf(vp[1].x - vp[0].x);
    face_sz += fabsf(vp[1].y - vp[0].y);
    face_sz += fabsf(vp[1].z - vp[0].z);
    face_sz += fabsf(vp[2].x - vp[0].x);
    face_sz += fabsf(vp[2].y - vp[0].y);
    face_sz += fabsf(vp[2].z - vp[0].z);
    face_sz /= 6;
    facenorm = fnorm[fi];
    Point3 vnorm[3] = {highvnrm[facengr[fi][0]], highvnrm[facengr[fi][1]], highvnrm[facengr[fi][2]]};
    facecurve = ComputeFaceCurvature(vnorm, vp);
    gettv(ch);
    compute_bump_vectors(tv[ch], vp, bv);
    bo = vp[0] - (bv[0] * tv[ch][0].x + bv[1] * tv[ch][0].y);
  }
  void setpt(float u, float v, float du, float dv)
  {
    pt = bv[0] * u + bv[1] * v + bo;
    dpt_dx = bv[0] * du;
    dpt_dy = bv[1] * dv;
    dpt = (pabs(dpt_dx) + pabs(dpt_dy)) * .5f;
    sz_ratio = (fabsf(dpt.x) + fabsf(dpt.y) + fabsf(dpt.z)) / (face_sz * 3);
    curve = facecurve * (fabsf(du) + fabsf(dv)) * .5f;
    bary = BaryCoords(vp[0], vp[1], vp[2], pt);
    normal =
      Normalize(highvnrm[facengr[face][0]] * bary[0] + highvnrm[facengr[face][1]] * bary[1] + highvnrm[facengr[face][2]] * bary[2]);
    orgview = -normal;
    norm = normal;
    view = orgview;
  }
  BOOL InMtlEditor() { return TRUE; }
  int Antialias() { return TRUE; }
  int ProjType() { return 0; }
  LightDesc *Light(int n) { return NULL; }
  TimeValue CurTime() { return curtime; }
  INode *Node() { return node; }
  Object *GetEvalObject()
  {
    if (!node)
      return NULL;
    return node->EvalWorldState(curtime).obj;
  }
  Point3 BarycentricCoords() { return bary; }
  int FaceNumber() { return face; }
  Point3 Normal() { return norm; }
  void SetNormal(Point3 p) { norm = p; }
  Point3 OrigNormal() { return normal; }
  float Curve() { return curve; }
  Point3 GNormal() { return facenorm; }
  Point3 CamPos() { return Point3(0, 0, 0); }
  Point3 V() { return view; }
  void SetView(Point3 p) { view = p; }
  Point3 OrigView() { return orgview; }
  Point3 ReflectVector()
  {
    float VN = -DotProd(view, norm);
    return Normalize(2.0f * VN * norm + view);
  }
  Point3 RefractVector(float ior)
  {
    Point3 N = Normal();
    float VN, nur, k;
    VN = DotProd(-view, N);
    if (backFace)
      nur = ior;
    else
      nur = (ior != 0.0f) ? 1.0f / ior : 1.0f;
    k = 1.0f - nur * nur * (1.0f - VN * VN);
    if (k <= 0.0f)
    {
      // Total internal reflection:
      return ReflectVector();
    }
    else
    {
      return (nur * VN - sqrtf(k)) * N + nur * view;
    }
  }
  Point3 P() { return pt; }
  Point3 DP() { return dpt; }
  void DP(Point3 &dx, Point3 &dy)
  {
    dx = dpt_dx;
    dy = dpt_dy;
  }
  Point3 PObj() { return pt; } // *cam2obj
  Point3 DPObj()
  {
    // return VectorTransform(dpt,cam2obj);
    return dpt;
  }
  Box3 ObjectBox() { return objbox; }
  Point3 PObjRelBox()
  {
    Point3 w = objbox.Width() * .5f, c = objbox.Center();
    Point3 p = PObj();
    p.x = (p.x - c.x) / w.x;
    p.y = (p.y - c.y) / w.y;
    p.z = (p.z - c.z) / w.z;
    return p;
  }
  Point3 DPObjRelBox()
  {
    Point3 w = objbox.Width() * .5f;
    Point3 p = DPObj();
    p.x = (p.x) / w.x;
    p.y = (p.y) / w.y;
    p.z = (p.z) / w.z;
    return p;
  }
  void ScreenUV(Point2 &uv, Point2 &duv)
  {
    uv = Point2(0.f, 0.f);
    duv = Point2(0.f, 0.f);
  }
  IPoint2 ScreenCoord() { return IPoint2(0, 0); }
  Point3 UVW(int ch)
  {
    gettv(ch);
    return tv[ch][0] * bary[0] + tv[ch][1] * bary[1] + tv[ch][2] * bary[2];
  }
  Point3 DUVW(int ch)
  {
    gettv(ch);
    return (pabs(tv[ch][1] - tv[ch][0]) + pabs(tv[ch][2] - tv[ch][0])) * .5f * sz_ratio;
  }
  int BumpBasisVectors(Point3 dP[2], int axis, int channel) { return 0; }
  Point3 UVWNormal(int channel = 0) { return 0; }

  void DPdUVW(Point3 dP[3], int ch)
  {
    gettv(ch);
    Point3 bvec[3];
    ComputeBumpVectors(tv[ch], vp, bvec);
    for (int i = 0; i < 3; ++i)
      dP[i] = Normalize(bvec[i]);
  }
  void GetBGColor(Color &co, Color &tr, int fogBG)
  {
    co.Black();
    tr.Black();
  }
  Point3 PointTo(const Point3 &p, RefFrame ito)
  {
    switch (ito)
    {
      case REF_WORLD:
      case REF_CAMERA:
        return p;
        // case REF_OBJECT:
        //	return p*cam2obj;
    }
    return p;
  }
  Point3 PointFrom(const Point3 &p, RefFrame ito)
  {
    switch (ito)
    {
      case REF_WORLD:
      case REF_CAMERA:
        return p;
        // case REF_OBJECT:
        //	return p*obj2cam;
    }
    return p;
  }
  Point3 VectorTo(const Point3 &p, RefFrame ito)
  {
    switch (ito)
    {
      case REF_WORLD:
      case REF_CAMERA:
        return p;
        // case REF_OBJECT:
        //	return VectorTransform(p,cam2obj);
    }
    return p;
  }
  Point3 VectorFrom(const Point3 &p, RefFrame ito)
  {
    switch (ito)
    {
      case REF_WORLD:
      case REF_CAMERA:
        return p;
        // case REF_OBJECT:
        //	return VectorTransform(p,obj2cam);
    }
    return p;
  }
  Matrix3 MatrixTo(RefFrame ito) { return Matrix3(); }
  Matrix3 MatrixFrom(RefFrame ifrom) { return Matrix3(); }
};


void PolyBumpUtil::build_normal_map(BitArray *facesel, Face *face, int numf, Point3 *vert, int numv, TVFace *tf, Point3 *tv,
  FaceNGr *facengr, Point3 *vertnorm, Matrix3 &lowtm, int reswd, int resht, unsigned int *respixels, class Mesh &hm,
  class Matrix3 &htm, real fordist, real backdist, bool nearest, real backw, int expands, INode *high)
{

  Point3 lowtm_scale(Length(lowtm.GetRow(0)), Length(lowtm.GetRow(1)), Length(lowtm.GetRow(2)));
  Tab<FaceArea> farea;
  Tab<int> fi;
  farea.SetCount(numf);
  const unsigned EFLG = 0x80000000;
  Tab<unsigned> fm;
  Tab<uint> pofs;
  int wd = reswd, ht = resht;
  if (antialias > 1)
  {
    wd *= antialias;
    ht *= antialias;
  }
  fm.Resize(wd * ht);
  fm.SetCount(wd * ht);
  pofs.Resize(wd * ht);
  pofs.SetCount(wd * ht);
  memset(&fm[0], 0xFF, (wd * ht) * sizeof(unsigned));
  Tab<Point3> pos;
  Tab<Point3> nrm;
  pos.Resize(wd * ht);
  pos.SetCount(wd * ht);
  nrm.Resize(wd * ht);
  nrm.SetCount(wd * ht);

  Tab<Point3[3][3]> tg_face;
  Tab<Point3> lowbc;
  lowbc.SetCount(wd * ht);

  if (space == TANGENT_SPACE)
  {
    Tab<Point3[2]> face_T;
    face_T.SetCount(numf);
    int i, f;
    for (f = 0; f < numf; ++f)
    {
      if (facesel && !((*facesel)[f]))
        continue;
      Point2 t0 = Point3to2(tv[tf[f].t[0]]);
      Point2 t1 = Point3to2(tv[tf[f].t[1]]);
      Point2 t2 = Point3to2(tv[tf[f].t[2]]);
      Point2 te1 = t1 - t0;
      Point2 te2 = t2 - t0;
      Point3 ve1 = vert[face[f].v[1]] - vert[face[f].v[0]];
      Point3 ve2 = vert[face[f].v[2]] - vert[face[f].v[0]];
      Point3 cp;

      cp = CrossProd(Point3(ve1.x, te1.x, te1.y), Point3(ve2.x, te2.x, te2.y));
      if (fabsf(cp.x) > REAL_EPS)
      {
        face_T[f][0].x = -cp.y / cp.x;
        face_T[f][1].x = -cp.z / cp.x;
      }
      else
        face_T[f][0].x = face_T[f][1].x = 0; // face_bumptex[i][1].x=0;

      cp = CrossProd(Point3(ve1.y, te1.x, te1.y), Point3(ve2.y, te2.x, te2.y));
      if (fabsf(cp.x) > REAL_EPS)
      {
        face_T[f][0].y = -cp.y / cp.x;
        face_T[f][1].y = -cp.z / cp.x;
      }
      else
        face_T[f][0].y = face_T[f][1].y = 0; // face_bumptex[i][1].y=0;

      cp = CrossProd(Point3(ve1.z, te1.x, te1.y), Point3(ve2.z, te2.x, te2.y));
      if (fabsf(cp.x) > REAL_EPS)
      {
        face_T[f][0].z = -cp.y / cp.x;
        face_T[f][1].z = -cp.z / cp.x;
      }
      else
        face_T[f][0].z = face_T[f][1].z = 0;
    }
    Tab<Tab<int>> map;

    map.SetCount(numv);
    for (i = 0; i < numv; ++i)
      map[i].Init();
    for (f = 0; f < numf; ++f)
    {
      map[face[f].v[0]].Append(1, &f, 8);
      map[face[f].v[1]].Append(1, &f, 8);
      map[face[f].v[2]].Append(1, &f, 8);
    }

    tg_face.SetCount(numf);
    // memset(tg_face.Addr(0),0xFF,numf*sizeof(FaceNGr));

    for (i = 0; i < numv; ++i)
    {
      for (int j = 0; j < map[i].Count(); ++j)
      {
        int f = map[i][j];
        int vi, vi2;
        for (vi = 0; vi < 2; ++vi)
          if (face[f].v[vi] == i)
            break;
        Point3 S(0, 0, 0), T(0, 0, 0);
        for (int j2 = 0; j2 < map[i].Count(); ++j2)
        {
          int f2 = map[i][j2];
          for (vi2 = 0; vi2 < 2; ++vi2)
            if (face[f2].v[vi2] == i)
              break;
          if (tf[f].t[vi] == tf[f2].t[vi2] && facengr[f][vi] == facengr[f2][vi2])
          {
            S += face_T[map[i][j2]][0];
            T += face_T[map[i][j2]][1];
          }
        }
        S = Normalize(S);
        T = Normalize(T);
        tg_face[f][vi][0] = S;
        tg_face[f][vi][1] = T; // CrossProd(S,vertnorm[facengr[f][vi]]);
        tg_face[f][vi][2] = Normalize(CrossProd(S, T));
        if (DotProd(tg_face[f][vi][2], vertnorm[facengr[f][vi]]) < 0)
          tg_face[f][vi][2] = -tg_face[f][vi][2];
        // tg_face[f][vi][2]=vertnorm[facengr[f][vi]];
      }
    }
    for (i = 0; i < numv; ++i)
      map[i].~Tab();
  }

  real du = .5f / wd, dv = .5f / ht;
  for (int f = 0; f < numf; ++f)
  {
    if (facesel && !((*facesel)[f]))
      continue;


    farea[f].p0 = fm.Count();
    farea[f].p1 = 0;
    Point2 t0 = Point3to2(tv[tf[f].t[0]]) - Point2(du, dv);
    Point2 t1 = Point3to2(tv[tf[f].t[1]]) - Point2(du, dv);
    Point2 t2 = Point3to2(tv[tf[f].t[2]]) - Point2(du, dv);
    Point2 tp[3], tedir[3], ted[3];
    tp[0] = Point2(t0.x * wd, t0.y * ht);
    tp[1] = Point2(t1.x * wd, t1.y * ht);
    tp[2] = Point2(t2.x * wd, t2.y * ht);
    tedir[0] = tp[1] - tp[0];
    ted[0] = tedir[0] / lengthSq(tedir[0]);
    tedir[1] = tp[2] - tp[1];
    ted[1] = tedir[1] / lengthSq(tedir[1]);
    tedir[2] = tp[0] - tp[2];
    ted[2] = tedir[2] / lengthSq(tedir[2]);
    // calc interpolation data
    real tu0 = t0.x, tv0 = t0.y;
    real tu1 = t1.x - tu0, tv1 = t1.y - tv0;
    real tu2 = t2.x - tu0, tv2 = t2.y - tv0;
    real det = tu1 * tv2 - tu2 * tv1;
    if (det == 0)
      det = 1;
    else
      det = 1 / det;
    Point3 p0 = vert[face[f].v[0]];
    Point3 p1 = vert[face[f].v[1]] - p0;
    Point3 p2 = vert[face[f].v[2]] - p0;
    Point3 n0 = vertnorm[facengr[f][0]];
    Point3 n1 = vertnorm[facengr[f][1]] - n0;
    Point3 n2 = vertnorm[facengr[f][2]] - n0;


    // calc face box
    BBox2 box;
    box += t0;
    box += t1;
    box += t2;
    int u0 = floorf(box[0].x * wd - 1);
    int v0 = floorf(box[0].y * ht - 1);
    int u1 = ceilf(box[1].x * wd + 1);
    int v1 = ceilf(box[1].y * ht + 1);
    // calc edge lines
    real A[3], B[3], C[3];
#define MAKELINE(p0, p1, A, B, C)         \
  {                                       \
    real a = -(p1.y - p0.y) * ht;         \
    real b = (p1.x - p0.x) * wd;          \
    real l = sqrtf(a * a + b * b);        \
    if (l == 0)                           \
      A = B = 0;                          \
    else                                  \
    {                                     \
      A = a / l;                          \
      B = b / l;                          \
    }                                     \
    C = -(A * p0.x * wd + B * p0.y * ht); \
  }
    MAKELINE(t0, t1, A[0], B[0], C[0]);
    MAKELINE(t1, t2, A[1], B[1], C[1]);
    MAKELINE(t2, t0, A[2], B[2], C[2]);
#undef MAKELINE
    if (A[0] * t2.x * wd + B[0] * t2.y * ht + C[0] < 0)
      for (int i = 0; i < 3; ++i)
      {
        A[i] = -A[i];
        B[i] = -B[i];
        C[i] = -C[i];
      }
    // for each point in the box
    int y = v0 % ht;
    if (y < 0)
      y += ht;
    int x0 = u0 % wd;
    if (x0 < 0)
      x0 += wd;
    for (int v = v0; v <= v1; ++v, y = (++y >= ht ? 0 : y))
    {
      real tv = v / real(ht); //+dv;
      real de[3];
      int i;
      for (i = 0; i < 3; ++i)
        de[i] = B[i] * v + C[i];
      for (int u = u0, x = x0; u <= u1; ++u, x = (++x >= wd ? 0 : x))
      {
        // test edge lines
        int p = y * wd + x;
        if (!(fm[p] & EFLG))
          continue;
        int be = -1;
        real bed = MIN_REAL, bet = -1;
        for (i = 0; i < 3; ++i)
        {
          real d = A[i] * u + de[i];
          if (d >= 0)
            continue;
          if (d <= -1.5f)
            break;
          if (d < bed)
            continue;
          Point2 dp = Point2(real(u), real(v)) - tp[i];
          real t = dp.DotProd(ted[i]);
          if (t < 0)
          {
            t = 0;
            d = -length(dp);
          }
          else if (t > 1)
          {
            t = 1;
            d = -length(Point2(real(u), real(v)) - tp[i == 3 ? 0 : i + 1]);
          }
          if (d > bed)
          {
            bed = d;
            be = i;
            bet = t;
          }
        }
        if (i < 3)
          continue;
        if (be < 0)
        {
          // point on the face
          real tu = u / real(wd); //+du;
          real k1 = ((tu - tu0) * tv2 - (tv - tv0) * tu2) * det;
          real k2 = ((tv - tv0) * tu1 - (tu - tu0) * tv1) * det;
          pos[p] = p1 * k1 + p2 * k2 + p0;
          nrm[p] = n1 * k1 + n2 * k2 + n0;
          lowbc[p] = Point3(1 - k1 - k2, k1, k2);
          fm[p] = (be >= 0 ? f | EFLG : f);
          if (p < farea[f].p0)
            farea[f].p0 = p;
          if (p > farea[f].p1)
            farea[f].p1 = p;
          /*if ( p < fm0 )
          fm0 = p;
          if ( p > fm1 )
      fm1 = p;*/
        }
        else
        {
          // point out of the face - get point on nearest edge
          Point2 t = tedir[be] * bet + tp[be];
          Point2 dp = t - Point2(real(u), real(v));
          if (rabs(dp.x) > 1 || rabs(dp.y) > 1)
            continue;
          if (fm[p] != 0xFFFFFFFF)
          {
            if (lengthSq(dp) >= lengthSq(get_point2(pofs[p])))
              continue;
          }
          // if(f==7) debug(" uv=%d,%d dp=" FMT_P2 "",u,v,P2D(dp));
          pofs[p] = store_point2(dp);
          t.x /= wd;
          t.y /= ht;
          real k1 = ((t.x - tu0) * tv2 - (t.y - tv0) * tu2) * det;
          real k2 = ((t.y - tv0) * tu1 - (t.x - tu0) * tv1) * det;
          pos[p] = p1 * k1 + p2 * k2 + p0;
          nrm[p] = n1 * k1 + n2 * k2 + n0;
          lowbc[p] = Point3(1 - k1 - k2, k1, k2);
          fm[p] = (f | EFLG);
          // lmcol[p] = ambcol;
          if (p < farea[f].p0)
            farea[f].p0 = p;
          if (p > farea[f].p1)
            farea[f].p1 = p;
          /*if ( p < fm0 )
          fm0 = p;
          if ( p > fm1 )
      fm1 = p;*/
        }
      }
    }
  }

  //==tracerays
  Mesh trhm = hm;
  /*for(f=0;f<trhm.numFaces;++f) {
  int v0=trhm.faces[f].v[0];
  trhm.faces[f].v[0]=trhm.faces[f].v[1];
  trhm.faces[f].v[1]=v0;
  }*/
  Tab<Point3> highvnrm;
  Tab<FaceNGr> highfngr;
  Tab<Point3> highvert;
  highvert.SetCount(hm.numVerts);
  for (int v = 0; v < trhm.numVerts; ++v)
    highvert[v] = htm.PointTransform(trhm.verts[v]);
  ComputeVertexNormals(&trhm, highvnrm, highfngr, highvert.Addr(0));
  memcpy(trhm.verts, highvert.Addr(0), trhm.numVerts * sizeof(Point3));

  Tab<Point3> releasenrm;
  releasenrm.SetCount(wd * ht);
  BitArray releasehit(wd * ht);
  releasehit.ClearAll();
  StaticMeshRayTracer *raytracer = create_staticmeshraytracer(trhm);
  if (backdist < 1.0e-5)
    backw = 0;
  BumpMapSC sc(ip->GetTime(), ip);
  sc.setnode(high);
  sc.setmesh(hm, highvnrm.Addr(0), highfngr.Addr(0));
  Mtl *mtl = high->GetMtl();
  TimeValue time = ip->GetTime();
  if (mtl)
  {
    mtl->Update(time, FOREVER);
    if (mtl->IsMultiMtl())
    {
      for (int i = 0; i < mtl->NumSubMtls(); ++i)
      {
        Mtl *submtl = mtl->GetSubMtl(i);
        if (!submtl)
          continue;
        submtl->Update(time, FOREVER);
        Class_ID cid = submtl->ClassID();
        if (cid == Class_ID(DMTL_CLASS_ID, 0) && !((StdMat *)submtl)->MapEnabled(ID_BU))
        {
          continue;
        }
        Texmap *btex = submtl->GetSubTexmap(ID_BU);
        btex->Update(time, FOREVER);
        btex->LoadMapFiles(time);
      }
    }
    else
    {
      Class_ID cid = mtl->ClassID();
      if (cid != Class_ID(DMTL_CLASS_ID, 0) || ((StdMat *)mtl)->MapEnabled(ID_BU))
      {
        Texmap *btex = mtl->GetSubTexmap(ID_BU);
        btex->Update(time, FOREVER);
        btex->LoadMapFiles(time);
      }
    }
  }

  for (int i = 0; i < wd * ht; ++i)
  {
    if (!(i % wd))
    {
      ip->ProgressUpdate(i * 100 / (wd * ht));
      if (ip->GetCancel())
      {
        int retval = MessageBox(ip->GetMAXHWnd(), _T("Stop Normal Map Calculating?"), _T("Question"), MB_ICONQUESTION | MB_YESNO);
        if (retval == IDYES)
        {
          delete raytracer;
          return;
        }
        else if (retval == IDNO)
        {
          ip->SetCancel(FALSE);
        }
      }
    }
    if (fm[i] != 0xFFFFFFFF)
    {
      Ray ray;
      Point3 n;
      ray.dir = n = Normalize(nrm[i]);
      ray.p = pos[i];
      // DWORD fi;
      // Point3 nrm;
      float forat = fordist;
      int forfi;
      Point3 forbary;
      // if(trhm.IntersectRay(ray,at,nrm,fi,bary)){
      bool forhit;

      float backat = backdist;
      int backfi = -1;
      Point3 backbary(0, 0, 0);
      bool backhit = false;

      if (nearest)
      {
        forhit = raytracer->traceray_back(ray.p, ray.dir, forat, forfi, forbary);
        if (backw > 0)
          backhit = raytracer->traceray(ray.p, -ray.dir, backat, backfi, backbary);
      }
      else
      {
        forhit = raytracer->traceray(ray.p + ray.dir * forat, -ray.dir, forat, forfi, forbary);
        if (backw > 0)
        {
          backhit = raytracer->traceray_back(ray.p - ray.dir * backat, ray.dir, backat, backfi, backbary);
        }
      }
      Point3 bary;
      int fi;
      if (!forhit && backhit)
      {
        fi = backfi;
        bary = backbary;
      }
      else if (forhit && !backhit)
      {
        fi = forfi;
        bary = forbary;
      }
      else if (!forhit && !backhit)
      {
        releasenrm[i] = n;
        if (space == OBJECT_SPACE)
        {
          releasenrm[i] = lowtm.VectorTransform(releasenrm[i]) / lowtm_scale;
          ;
        }
        continue;
      }
      else
      {
        real bw, fw;
        if (nearest)
        {
          bw = (1 - backw) * backat;
          fw = backw * forat;
          if (fw <= bw)
          {
            fi = forfi;
            bary = forbary;
          }
          else
          {
            fi = backfi;
            bary = backbary;
          }
        }
        else
        {
          backat = backdist - backat;
          forat = fordist - forat;
          bw = backw * backat;
          fw = (1 - backw) * forat;
          if (fw >= bw)
          {
            fi = forfi;
            bary = forbary;
          }
          else
          {
            fi = backfi;
            bary = backbary;
          }
        }
      }
      releasenrm[i] =
        Normalize(bary.x * highvnrm[highfngr[fi][0]] + bary.y * highvnrm[highfngr[fi][1]] + bary.z * highvnrm[highfngr[fi][2]]);
      if (mtl)
      {
        Texmap *btex = NULL;

        Mtl *submtl = mtl;
        if (mtl->IsMultiMtl())
        {
          submtl = mtl->GetSubMtl(hm.faces[fi].getMatID());
        }
        if (submtl)
        {
          Class_ID cid = submtl->ClassID();
          if (cid != Class_ID(DMTL_CLASS_ID, 0) || ((StdMat *)submtl)->MapEnabled(ID_BU))
          {
            btex = submtl->GetSubTexmap(ID_BU);
          }
        }
        if (btex)
        {
          int mapch = btex->GetMapChannel();
          if (hm.mapSupport(mapch))
          {
            UVVert *tv = hm.mapVerts(mapch);
            TVFace *tf = hm.mapFaces(mapch);
            sc.setface(fi, mapch);
            UVVert uv = bary.x * tv[tf[fi].t[0]] + bary.y * tv[tf[fi].t[1]] + bary.z * tv[tf[fi].t[2]];
            sc.setpt(uv.x, uv.y, 1.0f / wd, 1.0f / ht);
            Point3 bn = btex->EvalNormalPerturb(sc) * ((StdMat *)submtl)->GetTexmapAmt(ID_BU, time);
            // if(!((BMTex*)btex)->thebm) debug("None");
            // debug("a %g %g %g",bn.x,bn.y,bn.z);
            releasenrm[i] += bn;
            releasenrm[i] = Normalize(releasenrm[i]);
          }
        }
      }
      if (space == OBJECT_SPACE)
      {
        releasenrm[i] = lowtm.VectorTransform(releasenrm[i]) / lowtm_scale;
      }
      releasehit.Set(i);
    }
    else
    {
      releasenrm[i] = Point3(MAX_REAL, 0.0f, 0.0f);
    }
  }
  delete raytracer;

  for (int ei = 0; ei < expands; ++ei)
  {
    if (!expand(releasenrm.Addr(0), releasehit, wd, ht))
      break;
  }
  int samples = antialias < 1 ? 1 : antialias;
  for (int ri = 0, y = 0; y < resht; ++y)
  {
    for (int x = 0; x < reswd; ++x, ++ri)
    {
      // i=x+y*wd;
      Point3 nr(0, 0, 0);
      int nc = 0;
      int a = 0;
      for (int j = 0, rj = y * samples; j < samples; ++j, ++rj)
      {
        for (int xi = 0, ri = x * samples; xi < samples; ++xi, ++ri)
        {
          int i = ri + rj * wd;
          Point3 n = releasenrm[i];
          if (n.x > 2)
            continue;
          if (fm[i] != 0xFFFFFFFF && space == TANGENT_SPACE)
          {
            Point3 S = lowbc[i].x * tg_face[fm[i] & (~EFLG)][0][0] + lowbc[i].y * tg_face[fm[i] & (~EFLG)][1][0] +
                       lowbc[i].z * tg_face[fm[i] & (~EFLG)][2][0];
            Point3 T = lowbc[i].x * tg_face[fm[i] & (~EFLG)][0][1] + lowbc[i].y * tg_face[fm[i] & (~EFLG)][1][1] +
                       lowbc[i].z * tg_face[fm[i] & (~EFLG)][2][1];
            Point3 SxT = lowbc[i].x * tg_face[fm[i] & (~EFLG)][0][2] + lowbc[i].y * tg_face[fm[i] & (~EFLG)][1][2] +
                         lowbc[i].z * tg_face[fm[i] & (~EFLG)][2][2];
            Matrix3 m(S, T, SxT, Point3(0.0f, 0.0f, 0.0f));
            n = Inverse(m).VectorTransform(n);
          }
          if (fm[i] == 0xFFFFFFFF)
            ;
          else if (releasehit[i])
            a += 255;
          else
            a += 128;
          nr += n;
          nc++;
        }
      }
      if (!nc)
        continue;
      a /= nc;
      nr = Normalize(nr);
      respixels[ri] =
        real2uchar(0.5f * (nr.z + 1)) | (real2uchar(0.5f * (nr.y + 1)) << 8) | (real2uchar(0.5f * (nr.x + 1)) << 16) | (a << 24);
    }
  }
}
