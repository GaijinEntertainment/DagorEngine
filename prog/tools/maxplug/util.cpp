#include <map>
#include <max.h>
#include <utilapi.h>
#include <bmmlib.h>
#include <stdmat.h>
#include <notetrck.h>
#include <meshadj.h>
#include <mnmath.h>
#include <splshape.h>
#include <ilayer.h>
#include <ilayerproperties.h>

#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <memory>

#include "dagor.h"
#include "enumnode.h"
#include "mater.h"
#include "texmaps.h"
#include "resource.h"
#include "cfg.h"
#include "mathAng.h"
#include "dag_auto_ptr.h"


#define ERRMSG_DELAY 3000

#define MAX_CLSNAME 64

static Tab<char *> dagpath;

char dagor_path[256] = "";
void debug(char *s, ...);

M_STD_STRING strToWide(const char *sz)
{
#ifdef _UNICODE
  size_t n = strlen(sz) + 1;
  TCHAR *sw = new TCHAR[n];
  mbstowcs(sw, sz, n);

  M_STD_STRING res(sw);
  delete[] sw;
#else
  M_STD_STRING res(sz);
#endif
  return res;
}


std::string wideToStr(const TCHAR *sw)
{
#ifdef _UNICODE
  size_t n = _tcslen(sw) * 3 + 1;
  char *sz = new char[n];
  wcstombs(sz, sw, n);

  std::string res(sz);
  delete[] sz;
#else
  std::string res(sw);
#endif
  return res;
}


void import_milkshape_anim(Interface *ip, HWND hpanel);

class Dag2DagnewCB : public ENodeCB
{
public:
  Interface *ip;
  TimeValue time;
  bool onImport;
  CfgShader *cfg;

  std::map<Mtl *, Mtl *> convertedMat;

  Dag2DagnewCB(Interface *iptr, bool on_import)
  {
    ip = iptr;
    time = ip->GetTime();

    onImport = on_import;

    if (onImport)
    {
      TCHAR filename[MAX_PATH];
      CfgShader::GetCfgFilename(_T("DagorShaders.cfg"), filename);
      cfg = new CfgShader(filename);
    }
    else
    {
      cfg = NULL;
    }
  }

  ~Dag2DagnewCB() {}

  Mtl *conv_mat(Mtl *&m)
  {
    Class_ID cid = m->ClassID();
    if (cid == DagorMat_CID)
    {
      IDagorMat *d = (IDagorMat *)m->GetInterface(I_DAGORMAT);

      if (onImport && d->get_classname() != NULL && d->get_classname()[0] != 0)
      {
        StringVector *shaders = cfg->GetShaderNames();
        bool found = false;
        for (unsigned int shaderNo = 0; shaderNo < shaders->size(); shaderNo++)
        {
          if (!_tcsicmp(d->get_classname(), shaders->at(shaderNo).c_str()))
          {
            found = true;
            break;
          }
        }
        if (!found)
          return NULL;
      }

      Mtl *nm = (Mtl *)CreateInstance(MATERIAL_CLASS_ID, DagorMat2_CID);
      IDagorMat *nd = (IDagorMat *)nm->GetInterface(I_DAGORMAT);

      nd->set_2sided(d->get_2sided());
      nd->set_amb(d->get_amb());
      nd->set_classname(d->get_classname());
      nd->set_diff(d->get_diff());
      nd->set_emis(d->get_emis());
      nd->set_power(d->get_power());
      nd->set_spec(d->get_spec());
      nd->set_script(d->get_script());

      if (onImport && (d->get_classname() == NULL || d->get_classname()[0] == 0))
      {
        nd->set_classname(_T("simple"));
        nd->set_script(_T("lighting=lightmap\r\n"));
      }

      nm->SetName(m->GetName());


      for (int i = 0; i < NUMTEXMAPS; ++i)
      {
        Texmap *tex = m->GetSubTexmap(i);
        if (tex)
          nm->SetSubTexmap(i, tex);

        // nd->set_param(i, d->get_param(i));
        // nd->set_texname(i, d->get_texname(i));
      }

      m->ReleaseInterface(I_DAGORMAT, d);

      return nm;
    }
    return NULL;
  }

  int proc(INode *n)
  {
    if (!n)
      return ECB_CONT;
    Mtl *m = n->GetMtl();
    char cnv = 0;
    if (m)
    {
      if (m->IsMultiMtl())
      {
        int num = m->NumSubMtls();
        for (int i = 0; i < num; ++i)
        {
          Mtl *sm = m->GetSubMtl(i);
          if (sm)
          {
            std::map<Mtl *, Mtl *>::iterator itr = convertedMat.find(sm);
            if (itr != convertedMat.end())
            {
              m->SetSubMtl(i, itr->second);
            }
            else
            {
              Mtl *nmtl = conv_mat(sm);
              if (nmtl)
              {
                convertedMat[sm] = nmtl;
                m->SetSubMtl(i, nmtl);
              }
            }
          }
        }
      }
      else
      {
        std::map<Mtl *, Mtl *>::iterator itr = convertedMat.find(m);
        if (itr != convertedMat.end())
        {
          n->SetMtl(itr->second);
        }
        else
        {
          Mtl *nmtl = conv_mat(m);
          if (nmtl)
          {
            convertedMat[m] = nmtl;
            n->SetMtl(nmtl);
          }
        }
      }
    }
    return ECB_CONT;
  }
};

void convertnew(Interface *ip, bool on_import)
{
  if (!ip)
    return;

  Dag2DagnewCB cbv(ip, on_import);
  enum_nodes(ip->GetRootNode(), &cbv);
  ip->RedrawViews(ip->GetTime());
}

enum TCCOPY
{
  CTC_COPY,
  CTC_MOVE,
  CTC_SWAP,
  CTC_FLIPU,
  CTC_FLIPV,
  CTC_XCHG,
  CTC_MUL,
  CTC_ADD,
  CTC_KILL,
  CTC_INFO,
};

struct MappingInfo
{
  Tab<TVFace> tface;
  Tab<UVVert> tvert;
  void clear()
  {
    tface.SetCount(0);
    tvert.SetCount(0);
  }
};

typedef int FaceNGr[3];
// typedef float real;
#define INLINE   __forceinline
#define MAX_REAL FLT_MAX
#define MIN_REAL (-FLT_MAX)
//__forceinline real rabs(real a){return fabsf(a);}

/*INLINE unsigned real2uchar(float p) {
  float _n=p+1.0f;
  unsigned i=*(unsigned*)&_n;
  if(i>=0x40000000) i=0xFF;
  else if(i<=0x3F800000) i=0;
  else i=(i>>15)&0xFF;
  return i;
}*/

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


static void spaces_to_underscores(TCHAR *string)
{
  for (TCHAR *cur = string; *cur; cur++)
    if (*cur == ' ')
      *cur = '_';
}


class TopologyAdapter
{
public:
  Mesh *mesh;

  TopologyAdapter(Mesh *in_mesh) { mesh = in_mesh; }
  virtual unsigned int getNumVerts() = 0;
  virtual void setNumVerts(unsigned int num_verts) = 0;
  virtual unsigned int getIndex(unsigned int face_no, unsigned int index_no) = 0;
  virtual void setIndex(unsigned int face_no, unsigned int index_no, unsigned int index) = 0;
  virtual void copyVertex(unsigned int from, unsigned int to) = 0;
  virtual bool countRemovedVertices() { return false; }
};


class DagUtil : public UtilityObj
{
public:
  IUtil *iu;
  Interface *ip;

  // selection params
  HWND hBrightness;
  // dagor util
  HWND dagorUtilRoll;
  NoteKeyTab ntrk_buf;

  // uv utils
  HWND uvUtilsRoll;
  int tc_srcch, tc_dstch, tc_mapach, sel_fid;
  float tc_uk, tc_vk, tc_uo, tc_vo;
  ICustEdit *edsrctc, *edmapatc, *eddsttc, *etc_uk, *etc_vk, *etc_uo, *etc_vo, *edfid;
  char tc_facesel;
  MappingInfo chanelbuf[MAX_MESHMAPS];

  // scatter
  HWND scatterRoll;
  ICustEdit *egs_seed, *egs_objnum, *egs_sname, *egs_slope;
  ICustEdit *egs_xydiap[2], *egs_zdiap[2];
  char gs_set2norm, gs_usesmgr, gs_rotatez, gs_selfaces, gs_scalexy, gs_scalez;
  float gs_xydiap[2], gs_zdiap[2];
  float gs_slope;
  int gs_seed, gs_objnum;
  TCHAR gs_sname[512];

  // splines & smooth
  HWND splinesAndSmoothRoll;
  char mesh_smoothtu, use_smooth_grps, turn_edge, smooth_selected;
  ICustEdit *egrassht, *egrasstile, *egrassg, *egrasssegs, *egrasssteps, *epjs_maxseglen, *epjs_optseglen, *epjs_optang, *edaround,
    *edsharp, *edsmthiterations;
  float grass_ht, grass_tile, grass_g, pjspl_maxseglen, pjspl_optseglen, pjspl_optang;
  int grass_segs, grass_steps;
  char grass_up, grass_usetang;
  float merge_around, sharpness;
  int iterations;

  // materials
  HWND materialsRoll;
  TCHAR clsname[MAX_CLSNAME];
  ICustEdit *eclsname;

protected:
  bool editableMeshFound;
  unsigned int degenerateFacesRemovedNum;
  unsigned int isolatedVerticesRemovedNum;

public:
  DagUtil();
  void BeginEditParams(Interface *ip, IUtil *iu);
  void EndEditParams(Interface *ip, IUtil *iu);
  void DeleteThis() {}
  void update_ui();
  void update_vars();

  BOOL dagor_util_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam);
  BOOL materials_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam);
  BOOL uv_util_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam);
  BOOL spline_and_smooth_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam);
  BOOL scatter_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam);
  BOOL selection_brightness_dlg_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  // dagor util
  void get_ntrk();
  void put_ntrk();
  void convertSpaces();
  void convertSpacesRecursive(INode *node);
  void selectFloorFaces();
  void removeDegenerates();
  void removeDegeneratesRecursive(INode *node);
  void removeIsolatedVertices(TopologyAdapter &adapter);
  bool isDegenerate(Mesh *mesh, unsigned int face_no, const Matrix3 &otm);

  // uv utils
  void copy_tch();
  bool copy_tch1(Mesh &m, TCCOPY tc, int src, int dst, int mapa, bool sel_faces);
  bool copy_tch_buf(Mesh &m, MappingInfo &mi, int c);
  void paste_tch();
  bool paste_tch_buf(Mesh &m, MappingInfo &mi, int c);
  void copy_tch(TCCOPY tc);
  void select_face(bool vert);

  // scatter
  void genobjonsurf();

  // splines & smooth
  void mesh_smooth();
  void smooth_mesh(INode *node, Mesh &msh);
  void make_grass();
  void project_spline();

  // materials
  void show_all_maps();
  void hide_all_maps();
  void stdmat_to_dagmat();
  void dagmat_to_stdmat();
  void maxtures2materials();
  void maxtures2materials(Mesh &m, INode *node);
  void collapse();
  void convert();
  void enumerate();

protected:
  void dagor_util_update_ui(HWND hw);
  void materials_update_ui(HWND hw);
  void uv_utils_update_ui(HWND hw);
  void splines_and_smooth_update_ui(HWND hw);
  void scatter_update_ui(HWND hw);
  void materials_update_vars(HWND hw);
  void uv_utils_update_vars(HWND hw);
  void splines_and_smooth_update_vars(HWND hw);
  void scatter_update_vars(HWND hw);
};

static DagUtil util;
// Compute per-vertex normal in 3dsMax way
void ComputeVertexNormals(Mesh *mesh, Tab<Point3> &vnrm, Tab<FaceNGr> &fngr, Point3 *trvert = NULL);


class DagUtilDesc : public ClassDesc
{
public:
  int IsPublic() { return 1; }
  void *Create(BOOL loading = FALSE) { return &util; }
  const TCHAR *ClassName() { return GetString(IDS_DAGUTIL_NAME); }
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
  SClass_ID SuperClassID() { return UTILITY_CLASS_ID; }
  Class_ID ClassID() { return DagUtil_CID; }
  const TCHAR *Category() { return GetString(IDS_UTIL_CAT); }
  BOOL NeedsToSave() { return TRUE; }
  IOResult Save(ISave *);
  IOResult Load(ILoad *);
};

//== save grass params
enum
{
  CH_RMAP_FNAME = 1,
  CH_LTMAP,
  CH_RENDER_MAP,
  CH_EPSPL,
  CH_GS,
  CH_GS2
};

IOResult DagUtilDesc::Save(ISave *io)
{
  util.update_vars();
  ULONG nw;
  io->BeginChunk(CH_EPSPL);
  if (io->Write(&util.grass_ht, 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.grass_tile, 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.grass_g, 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.grass_segs, 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.grass_steps, 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write((BYTE *)&util.grass_up, 1, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write((BYTE *)&util.grass_usetang, 1, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.pjspl_maxseglen, 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.pjspl_optseglen, 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.pjspl_optang, 4, &nw) != IO_OK)
    return IO_ERROR;
  io->EndChunk();
  io->BeginChunk(CH_GS);
  if (io->Write((BYTE *)&util.gs_set2norm, 1, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write((BYTE *)&util.gs_usesmgr, 1, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write((BYTE *)&util.gs_rotatez, 1, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.gs_slope, 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.gs_seed, 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.gs_objnum, 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(util.gs_sname, (int)_tcslen(util.gs_sname), &nw) != IO_OK)
    return IO_ERROR;
  io->EndChunk();
  io->BeginChunk(CH_GS2);
  if (io->Write((BYTE *)&util.gs_selfaces, 1, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.gs_xydiap[0], 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.gs_xydiap[1], 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.gs_zdiap[0], 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.gs_zdiap[1], 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write((BYTE *)&util.gs_scalexy, 1, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write((BYTE *)&util.gs_scalez, 1, &nw) != IO_OK)
    return IO_ERROR;
  io->EndChunk();
  return IO_OK;
}

IOResult DagUtilDesc::Load(ILoad *io)
{
  ULONG nr;
  while (io->OpenChunk() == IO_OK)
  {
    switch (io->CurChunkID())
    {
      case CH_EPSPL:
      {
        if (io->Read(&util.grass_ht, 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.grass_tile, 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.grass_g, 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.grass_segs, 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.grass_steps, 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read((BYTE *)&util.grass_up, 1, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read((BYTE *)&util.grass_usetang, 1, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.pjspl_maxseglen, 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.pjspl_optseglen, 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.pjspl_optang, 4, &nr) != IO_OK)
          return IO_ERROR;
      }
      break;
      case CH_GS:
      {
        if (io->Read((BYTE *)&util.gs_set2norm, 1, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read((BYTE *)&util.gs_usesmgr, 1, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read((BYTE *)&util.gs_rotatez, 1, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.gs_slope, 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.gs_seed, 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.gs_objnum, 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(util.gs_sname, sizeof(util.gs_sname), &nr) != IO_OK)
          return IO_ERROR;
      }
      break;
      case CH_GS2:
      {
        if (io->Read((BYTE *)&util.gs_selfaces, 1, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.gs_xydiap[0], 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.gs_xydiap[1], 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.gs_zdiap[0], 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.gs_zdiap[1], 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read((BYTE *)&util.gs_scalexy, 1, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read((BYTE *)&util.gs_scalez, 1, &nr) != IO_OK)
          return IO_ERROR;
      }
      break;
    }
    io->CloseChunk();
  }
  util.update_ui();
  return IO_OK;
}

static DagUtilDesc utilDesc;
ClassDesc *GetDagUtilCD() { return &utilDesc; }

bool get_edint(ICustEdit *e, int &a)
{
  if (!e)
    return false;
  BOOL ok = TRUE;
  int val = e->GetInt(&ok);
  if (!ok)
    return false;
  a = val;
  return true;
}

bool get_edfloat(ICustEdit *e, float &a)
{
  if (!e)
    return false;
  BOOL ok = TRUE;
  float val = e->GetFloat(&ok);
  if (!ok)
    return false;
  a = val;
  return true;
}
bool get_chkbool(int id, char &chk, HWND pan = NULL)
{
  if (!pan)
    return false;
  chk = IsDlgButtonChecked(pan, id);
  return true;
}


static const TCHAR *stripbasepath(const TCHAR *s, const TCHAR *base)
{
  const TCHAR *dp = base;
  for (; *dp && *s; ++dp, ++s)
  {
    if (*dp == '/' || *dp == '\\')
      if (*s == '/' || *s == '\\')
        continue;
    if (_totupper(*dp) != _totupper(*s))
      break;
  }
  if (!*dp)
    return s;
  return NULL;
}

const TCHAR *make_path_rel(const TCHAR *p)
{
  if (!p)
    return p;
  if (*p == 0)
    return p;
  for (int i = 0; i < dagpath.Count(); ++i)
  {
    const TCHAR *s = stripbasepath(p, strToWide(dagpath[i]).c_str());
    if (s)
      return s;
  }
  /*
      if(p[1]==':') p+=2;
      for(;*p=='\\' || *p=='/';++p);
  */


  return p;
}

static void add_dagpath(char *p, bool first)
{
  int i;
  for (i = 0; i < dagpath.Count(); ++i)
    if (stricmp(p, dagpath[i]) == 0)
      break;
  if (i < dagpath.Count())
  {
    if (first)
    {
      char *dp = dagpath[i];
      dagpath.Delete(i, 1);
      dagpath.Insert(0, 1, &dp);
    }
    return;
  }
  p = strdup(p);
  assert(p);
  if (first)
    dagpath.Insert(0, 1, &p);
  else
    dagpath.Append(1, &p);
}


void AppendSlash(char *p)
{
  char last = p[strlen(p) - 1];
  if (last != '\\' && last != '/')
    strcat(p, "\\");
}

void load_dagorpath_cfg()
{
  TCHAR fname[256];

  CfgShader::GetCfgFilename(_T("DagorPath.cfg"), fname);
  FILE *h = _tfopen(fname, _T("rt"));
  if (h)
  {
    int i;
    for (i = 0; i < dagpath.Count(); ++i)
      free(dagpath[i]);
    dagpath.ZeroCount();


    char fn[256];

    while (fgets(fn, 256, h))
    {
      for (i = (int)strlen(fn) - 1; i >= 0; --i)
        if (fn[i] != '\r' && fn[i] != '\n' && fn[i] != '\t' && fn[i] != ' ')
          break;
      fn[i + 1] = 0;
      char *p;
      for (p = fn; *p; ++p)
        if (*p != '\r' && *p != '\n' && *p != '\t' && *p != ' ')
          break;
      if (!*p)
        continue;


      // BMMAppendSlash(p);

      AppendSlash(p);

      add_dagpath(p, false);
    }
    fclose(h);
    if (dagpath.Count())
      strcpy(dagor_path, dagpath[0]);
    else
      strcpy(dagor_path, "");
  }
}


void set_dagor_path(const char *p)
{
  if (!p)
    return;
  load_dagorpath_cfg();

  strcpy(dagor_path, p);
  // BMMAppendSlash(dagor_path);

  AppendSlash(dagor_path);
  add_dagpath(dagor_path, true);

  TCHAR fn[256];
  CfgShader::GetCfgFilename(_T("DagorPath.cfg"), fn);
  FILE *h = _tfopen(fn, _T("wt"));
  if (h)
  {
    for (int i = 0; i < dagpath.Count(); ++i)
    {
      fputs(dagpath[i], h);
      fputs("\n", h);
    }
    fclose(h);
  }
}


static INT_PTR CALLBACK dagor_util_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return util.dagor_util_dlg_proc(hw, msg, wParam, lParam);
}

BOOL DagUtil::selection_brightness_dlg_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  DagUtil *pThis = (DagUtil *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
  if ((!pThis || !pThis->hBrightness) && msg != WM_INITDIALOG)
  {
    return FALSE;
  }
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      DagUtil *pThis = (DagUtil *)lParam;
      SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
      pThis->hBrightness = hWnd;
      CenterWindow(hWnd, GetParent(hWnd));
      ICustEdit *iEdit = GetICustEdit(GetDlgItem(hWnd, IDC_CUSTOM));
      iEdit->SetText(30);
      ReleaseICustEdit(iEdit);
    }
    break;
    case WM_DESTROY: return FALSE;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_CALC:
        {
          InvalidateRect(hWnd, NULL, true);
          UpdateWindow(hWnd);
        }
        break;
      }
      break;

    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hdc;
      hdc = BeginPaint(hWnd, &ps);
      ICustEdit *iEdit = GetICustEdit(GetDlgItem(hWnd, IDC_CUSTOM));
      real alfa = iEdit->GetInt() * DEG_TO_RAD;
      if (alfa > 0.01 && alfa < HALFPI)
      {
        real part = 0.9f, br;
        int d = get_cos_power_from_ang(alfa, part, br);
        const int steps = 50;
        const float w = 400;
        Tab<Point2> fp;
        float move = PI / 2 / (float)steps;
        for (int i = 0; i < steps; i++)
        {
          float x = (float)i * move;
          float y = 1;
          for (int j = 0; j < d; j++)
            y *= cos(x);
          fp.Append(1, &Point2(x, y));
        }
        POINT ip[steps];
        for (int i = 0; i < steps; i++)
        {
          ip[i].x = fp[i].x * w * 2.f / PI;
          ip[i].y = w - (fp[i].y * w);
        }
        Polyline(hdc, ip, steps);

        MoveToEx(hdc, alfa * w * 2.f / PI, 0, NULL);
        LineTo(hdc, alfa * w * 2.f / PI, 400);
        SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
        TCHAR buf[32];
        _stprintf(buf, _T("alfa=%.3frad -> d=%i"), alfa, d);
        // debug(buf);
        TextOut(hdc, 150, 200 + 40, buf, (int)_tcslen(buf));
        _stprintf(buf, _T("%.6f>%.6f"), br, part);
        TextOut(hdc, 150, 220 + 40, buf, (int)_tcslen(buf));
        // debug(buf);
      }
      ReleaseICustEdit(iEdit);
      EndPaint(hWnd, &ps);
      ReleaseDC(hWnd, hdc);
    }
    break;

    case WM_CLOSE:
    {
      EndDialog(pThis->hBrightness, FALSE);
      pThis->ip->UnRegisterDlgWnd(pThis->hBrightness);
      pThis->hBrightness = NULL;
    }
    break;

    default: return FALSE;
  }
  return TRUE;
}

static INT_PTR CALLBACK selection_brightness_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return util.selection_brightness_dlg_proc(hw, msg, wParam, lParam);
}

BOOL DagUtil::dagor_util_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam)
{
#define reled(e)         \
  if (e)                 \
  {                      \
    ReleaseICustEdit(e); \
    e = NULL;            \
  }
  switch (msg)
  {
    case WM_INITDIALOG: dagor_util_update_ui(hw); break;

    case WM_DESTROY: break;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_EDIT_BRIGHTNESS:
        {
          if (hBrightness)
          {
            SetActiveWindow(hBrightness);
          }
          else
          {
            hBrightness = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_DAGOR_BRIGHTNESS), ip->GetMAXHWnd(),
              ::selection_brightness_dlg_proc, (LPARAM)this);
            ip->RegisterDlgWnd(hBrightness);
          }
        }
        break;
        case IDC_GET_NTRK:
        {
          get_ntrk();
          break;
        }
        case IDC_PUT_NTRK:
        {
          put_ntrk();
          break;
        }
        case IDC_CONVERT_SPACES:
        {
          convertSpaces();
          break;
        }
        case IDC_IMPORT_MS_ANIM:
        {
          import_milkshape_anim(ip, hw);
          break;
        }
        case IDC_REMOVE_DEGENERATES:
        {
          removeDegenerates();
          break;
        }
        case IDC_SELECT_FLOOR: selectFloorFaces(); break;
        case IDC_SET_DAGORPATH:
        {
          TCHAR dir[256];
          _tcscpy(dir, strToWide(dagor_path).c_str());

          ip->ChooseDirectory(hw, GetString(IDS_CHOOSE_DAGOR_PATH), dir);
          if (dir[0])
          {
            set_dagor_path(wideToStr(dir).c_str());
            SetDlgItemText(hw, IDC_DAGORPATH, strToWide(dagor_path).c_str());
          }
          break;
        }
      }
      break;

    default: return FALSE;
  }
  return TRUE;
#undef reled
}


static INT_PTR CALLBACK materials_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return util.materials_dlg_proc(hw, msg, wParam, lParam);
}

BOOL DagUtil::materials_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam)
{
#define reled(e)         \
  if (e)                 \
  {                      \
    ReleaseICustEdit(e); \
    e = NULL;            \
  }
  switch (msg)
  {
    case WM_INITDIALOG:
      eclsname = GetICustEdit(GetDlgItem(hw, IDC_CLSNAME));
      materials_update_ui(hw);
      break;

    case WM_DESTROY:
      materials_update_vars(hw);
      reled(eclsname);
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_MAX2MAT:
        {
          maxtures2materials();
          break;
        }
        case IDC_SHOWALLMAPS:
        {
          show_all_maps();
          break;
        }
        case IDC_HIDEMAPS:
        {
          hide_all_maps();
          break;
        }
        case IDC_STD2DAGMAT:
        {
          stdmat_to_dagmat();
          break;
        }
        case IDC_DAGMAT2STDMAT:
        {
          dagmat_to_stdmat();
          break;
        }
        case IDC_ENUMERATE:
        {
          enumerate();
          break;
        }
        case IDC_COLLAPSE:
        {
          collapse();
          break;
        }
        case IDC_CONVERT:
        {
          convert();
          break;
        }
        case IDC_CONVERT_NEW:
        {
          convertnew(ip, false);
          break;
        }
        case IDC_CLSNAME:
        {
          eclsname->GetText(clsname, MAX_CLSNAME);
          break;
        }
      }
      break;

    default: return FALSE;
  }
  return TRUE;
#undef reled
}

static INT_PTR CALLBACK uv_util_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return util.uv_util_dlg_proc(hw, msg, wParam, lParam);
}

BOOL DagUtil::uv_util_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam)
{
#define reled(e)         \
  if (e)                 \
  {                      \
    ReleaseICustEdit(e); \
    e = NULL;            \
  }
  switch (msg)
  {
    case WM_INITDIALOG:
      edfid = GetICustEdit(GetDlgItem(hw, IDC_FID));
      edmapatc = GetICustEdit(GetDlgItem(hw, IDC_TCMAPA));
      edsrctc = GetICustEdit(GetDlgItem(hw, IDC_TCSRC));
      eddsttc = GetICustEdit(GetDlgItem(hw, IDC_TCDST));
      etc_uk = GetICustEdit(GetDlgItem(hw, IDC_TCUK));
      etc_vk = GetICustEdit(GetDlgItem(hw, IDC_TCVK));
      etc_uo = GetICustEdit(GetDlgItem(hw, IDC_TCUO));
      etc_vo = GetICustEdit(GetDlgItem(hw, IDC_TCVO));
      uv_utils_update_ui(hw);
      break;

    case WM_DESTROY:
      uv_utils_update_vars(hw);
      reled(edfid);
      reled(edmapatc);
      reled(edsrctc);
      reled(eddsttc);
      reled(etc_uk);
      reled(etc_vk);
      reled(etc_uo);
      reled(etc_vo);
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_TCCOPYBUF:
        {
          copy_tch();
          break;
        }
        case IDC_TCPASTEBUF:
        {
          paste_tch();
          break;
        }
        case IDC_TCSWAP:
        {
          copy_tch(CTC_SWAP);
          break;
        }
        case IDC_TCCOPY:
        {
          copy_tch(CTC_COPY);
          break;
        }
        case IDC_TCMOVE:
        {
          copy_tch(CTC_MOVE);
          break;
        }
        case IDC_TCKILL:
        {
          copy_tch(CTC_KILL);
          break;
        }
        case IDC_TCINFO:
        {
          copy_tch(CTC_INFO);
          break;
        }
        case IDC_TCXCHG:
        {
          copy_tch(CTC_XCHG);
          break;
        }
        case IDC_TCFLIPU:
        {
          copy_tch(CTC_FLIPU);
          break;
        }
        case IDC_TCFLIPV:
        {
          copy_tch(CTC_FLIPV);
          break;
        }
        case IDC_TCMUL:
        {
          copy_tch(CTC_MUL);
          break;
        }
        case IDC_TCADD:
        {
          copy_tch(CTC_ADD);
          break;
        }
        case IDC_SELFACE:
        {
          select_face(false);
          break;
        }
        case IDC_SELVERT:
        {
          select_face(true);
          break;
        }
      }
      break;

    default: return FALSE;
  }
  return TRUE;
#undef reled
}


static INT_PTR CALLBACK spline_and_smooth_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return util.spline_and_smooth_dlg_proc(hw, msg, wParam, lParam);
}

BOOL DagUtil::spline_and_smooth_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam)
{
#define reled(e)         \
  if (e)                 \
  {                      \
    ReleaseICustEdit(e); \
    e = NULL;            \
  }
  switch (msg)
  {
    case WM_INITDIALOG:
      egrassht = GetICustEdit(GetDlgItem(hw, IDC_GRASSHT));
      egrasstile = GetICustEdit(GetDlgItem(hw, IDC_GRASSTILE));
      egrassg = GetICustEdit(GetDlgItem(hw, IDC_GRASSGRAVITY));
      egrasssegs = GetICustEdit(GetDlgItem(hw, IDC_GRASSSEGS));
      epjs_optang = GetICustEdit(GetDlgItem(hw, IDC_PJS_OPTANGLE));
      epjs_optseglen = GetICustEdit(GetDlgItem(hw, IDC_PJS_OPTSEGLEN));
      epjs_maxseglen = GetICustEdit(GetDlgItem(hw, IDC_PJS_MAXSEGLEN));
      egrasssteps = GetICustEdit(GetDlgItem(hw, IDC_GRASSSTEPS));
      edsharp = GetICustEdit(GetDlgItem(hw, IDC_SHARPNESS));
      edsmthiterations = GetICustEdit(GetDlgItem(hw, IDC_SMTHITERATIONS));
      edaround = GetICustEdit(GetDlgItem(hw, IDC_AROUND));
      splines_and_smooth_update_ui(hw);
      break;

    case WM_DESTROY:
      splines_and_smooth_update_vars(hw);
      reled(edaround);
      reled(edsmthiterations);
      reled(edsharp);
      reled(egrassht);
      reled(egrasstile);
      reled(egrassg);
      reled(egrasssegs);
      reled(egrasssteps);
      reled(epjs_maxseglen);
      reled(epjs_optseglen);
      reled(epjs_optang);
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_MESHSMOOTH:
        {
          mesh_smooth();
          break;
        }
        case IDC_MAKEGRASS:
        {
          make_grass();
          break;
        }
        case IDC_PROJSPL:
        {
          project_spline();
          break;
        }
      }
      break;

    default: return FALSE;
  }
  return TRUE;
#undef reled
}


static INT_PTR CALLBACK scatter_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return util.scatter_dlg_proc(hw, msg, wParam, lParam);
}

BOOL DagUtil::scatter_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam)
{
#define reled(e)         \
  if (e)                 \
  {                      \
    ReleaseICustEdit(e); \
    e = NULL;            \
  }
  switch (msg)
  {
    case WM_INITDIALOG:
      egs_objnum = GetICustEdit(GetDlgItem(hw, IDC_OBJNUM));
      egs_seed = GetICustEdit(GetDlgItem(hw, IDC_OBJGENSEED));
      egs_sname = GetICustEdit(GetDlgItem(hw, IDC_SELSET));
      egs_slope = GetICustEdit(GetDlgItem(hw, IDC_GS_SLOPE));
      egs_xydiap[0] = GetICustEdit(GetDlgItem(hw, IDC_GS_SCALEXYS));
      egs_xydiap[1] = GetICustEdit(GetDlgItem(hw, IDC_GS_SCALEXYE));
      egs_zdiap[0] = GetICustEdit(GetDlgItem(hw, IDC_GS_SCALEZS));
      egs_zdiap[1] = GetICustEdit(GetDlgItem(hw, IDC_GS_SCALEZE));
      scatter_update_ui(hw);
      break;

    case WM_DESTROY:
      scatter_update_vars(hw);
      reled(egs_objnum);
      reled(egs_seed);
      reled(egs_sname);
      reled(egs_slope);
      reled(egs_xydiap[0]);
      reled(egs_xydiap[1]);
      reled(egs_zdiap[0]);
      reled(egs_zdiap[1]);
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_GENOBJSURF:
        {
          genobjonsurf();
          break;
        }
      }
      break;

    default: return FALSE;
  }
  return TRUE;
#undef reled
}


DagUtil::DagUtil()
{
  iu = NULL;
  ip = NULL;

  // dagor utils
  dagorUtilRoll = NULL;
  editableMeshFound = false;
  degenerateFacesRemovedNum = 0;
  isolatedVerticesRemovedNum = 0;

  // uv utils
  uvUtilsRoll = NULL;
  edsrctc = edmapatc = eddsttc = etc_uk = etc_vk = etc_uo = etc_vo = edfid = NULL;
  tc_mapach = tc_srcch = tc_dstch = 1;
  tc_uk = tc_vk = 2;
  tc_uo = tc_vo = .5f;
  tc_facesel = 0;
  sel_fid = 1;

  // scatter
  scatterRoll = NULL;
  egs_seed = egs_objnum = egs_sname = egs_slope = NULL;
  egs_xydiap[0] = egs_xydiap[1] = NULL;
  egs_zdiap[0] = egs_zdiap[1] = NULL;
  gs_set2norm = true;
  gs_usesmgr = false;
  gs_selfaces = false;
  gs_rotatez = true;
  gs_slope = 0;
  gs_seed = 1000;
  gs_xydiap[0] = gs_xydiap[1] = 1;
  gs_zdiap[0] = gs_zdiap[1] = 1;
  gs_scalexy = 0;
  gs_scalez = 0;
  gs_objnum = 0;

  // splines & smooth
  splinesAndSmoothRoll = NULL;
  egrassht = egrasstile = egrassg = egrasssegs = egrasssteps = NULL;
  epjs_maxseglen = epjs_optang = epjs_optseglen = NULL;
  grass_ht = 1;
  grass_tile = 3;
  grass_g = 0;
  grass_segs = 1;
  grass_steps = -1;
  grass_up = 1;
  grass_usetang = 1;
  pjspl_maxseglen = 1;
  pjspl_optang = 2;
  pjspl_optseglen = 1;
  turn_edge = 0;
  smooth_selected = 0;
  iterations = 1;
  sharpness = 1;
  use_smooth_grps = 1;
  mesh_smoothtu = 0;
  merge_around = 0.000001f;
  edaround = edsharp = edsmthiterations = NULL;

  // materials
  materialsRoll = NULL;
  _tcscpy(clsname, _T(""));
  eclsname = NULL;
}

void DagUtil::BeginEditParams(Interface *ip, IUtil *iu)
{
  this->ip = ip;
  this->iu = iu;

  dagorUtilRoll = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_DAGUTIL), ::dagor_util_dlg_proc, GetString(IDS_DAGUTIL_ROLL), 0);

  materialsRoll = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_MATERIALS), ::materials_dlg_proc, GetString(IDS_MATERIALS_ROLL), 0,
    APPENDROLL_CLOSED);

  uvUtilsRoll = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_UV_UTILS), ::uv_util_dlg_proc, GetString(IDS_UV_UTILS_ROLL), 0,
    APPENDROLL_CLOSED);

  splinesAndSmoothRoll = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_SPLINES_AND_SMOOTH), ::spline_and_smooth_dlg_proc,
    GetString(IDS_SPLINES_AND_SMOOTH_ROLL), 0, APPENDROLL_CLOSED);

  scatterRoll =
    ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_SCATTER), ::scatter_dlg_proc, GetString(IDS_SCATTER_ROLL), 0, APPENDROLL_CLOSED);
}

void DagUtil::EndEditParams(Interface *ip, IUtil *iu)
{
  this->iu = NULL;
  this->ip = NULL;

  if (dagorUtilRoll)
    ip->DeleteRollupPage(dagorUtilRoll);
  dagorUtilRoll = NULL;
  if (materialsRoll)
    ip->DeleteRollupPage(materialsRoll);
  materialsRoll = NULL;
  if (uvUtilsRoll)
    ip->DeleteRollupPage(uvUtilsRoll);
  uvUtilsRoll = NULL;
  if (splinesAndSmoothRoll)
    ip->DeleteRollupPage(splinesAndSmoothRoll);
  splinesAndSmoothRoll = NULL;
  if (scatterRoll)
    ip->DeleteRollupPage(scatterRoll);
  scatterRoll = NULL;
  if (hBrightness)
  {
    EndDialog(hBrightness, FALSE);
    ip->UnRegisterDlgWnd(hBrightness);
    hBrightness = NULL;
  }
}

void DagUtil::dagor_util_update_ui(HWND hw)
{
  if (hw)
  {
    SetDlgItemText(hw, IDC_DAGORPATH, strToWide(dagor_path).c_str());
  }
}


void DagUtil::materials_update_ui(HWND hw)
{
  if (eclsname)
    eclsname->SetText(clsname);
}

void DagUtil::materials_update_vars(HWND hw)
{
  if (eclsname)
    eclsname->GetText(clsname, sizeof(clsname));
}


void DagUtil::uv_utils_update_ui(HWND hw)
{
  if (edfid)
    edfid->SetText(sel_fid);
  if (edmapatc)
    edmapatc->SetText(tc_mapach);
  if (edsrctc)
    edsrctc->SetText(tc_srcch);
  if (eddsttc)
    eddsttc->SetText(tc_dstch);
  if (etc_uk)
    etc_uk->SetText(tc_uk);
  if (etc_vk)
    etc_vk->SetText(tc_vk);
  if (etc_uo)
    etc_uo->SetText(tc_uo);
  if (etc_vo)
    etc_vo->SetText(tc_vo);

  if (hw)
    CheckDlgButton(hw, IDC_APPLYSELECTEDFACES, tc_facesel);
}

void DagUtil::uv_utils_update_vars(HWND hw)
{
  get_edint(edfid, sel_fid);
  get_edint(edmapatc, tc_mapach);
  get_edint(edsrctc, tc_srcch);
  get_edint(eddsttc, tc_dstch);
  get_edfloat(etc_uk, tc_uk);
  get_edfloat(etc_vk, tc_vk);
  get_edfloat(etc_uo, tc_uo);
  get_edfloat(etc_vo, tc_vo);

  if (hw)
    get_chkbool(IDC_APPLYSELECTEDFACES, tc_facesel, hw);
}


void DagUtil::splines_and_smooth_update_ui(HWND hw)
{
  if (edaround)
    edaround->SetText(merge_around, 9);
  if (edsharp)
    edsharp->SetText(sharpness, 4);
  if (edsmthiterations)
    edsmthiterations->SetText(iterations);
  if (egrassht)
    egrassht->SetText(grass_ht);
  if (egrasstile)
    egrasstile->SetText(grass_tile);
  if (egrassg)
    egrassg->SetText(grass_g);
  if (egrasssegs)
    egrasssegs->SetText(grass_segs);
  if (egrasssteps)
    egrasssteps->SetText(grass_steps);
  if (epjs_maxseglen)
    epjs_maxseglen->SetText(pjspl_maxseglen);
  if (epjs_optseglen)
    epjs_optseglen->SetText(pjspl_optseglen);
  if (epjs_optang)
    epjs_optang->SetText(pjspl_optang);

  if (hw)
  {
    CheckDlgButton(hw, IDC_GRASSUP, grass_up);
    CheckDlgButton(hw, IDC_GRASSUSETANG, grass_usetang);
    CheckDlgButton(hw, IDC_SMOOTHSEL, smooth_selected);
    CheckDlgButton(hw, IDC_TM_DIVIDEDIR, mesh_smoothtu);
    CheckDlgButton(hw, IDC_USE_SMOOTH, use_smooth_grps);
  }
}

void DagUtil::splines_and_smooth_update_vars(HWND hw)
{
  if (edaround)
    merge_around = edaround->GetFloat();
  if (edsharp)
    sharpness = edsharp->GetFloat();
  if (edsmthiterations)
    iterations = edsmthiterations->GetInt();
  if (iterations <= 0)
    iterations = 1;
  if (edsmthiterations)
    edsmthiterations->SetText(iterations);
  get_edfloat(egrassht, grass_ht);
  get_edfloat(egrasstile, grass_tile);
  get_edfloat(egrassg, grass_g);
  get_edint(egrasssegs, grass_segs);
  get_edint(egrasssteps, grass_steps);
  get_edfloat(epjs_maxseglen, pjspl_maxseglen);
  get_edfloat(epjs_optseglen, pjspl_optseglen);
  get_edfloat(epjs_optang, pjspl_optang);

  if (hw)
  {
    get_chkbool(IDC_GRASSUP, grass_up, hw);
    get_chkbool(IDC_GRASSUSETANG, grass_usetang, hw);
    get_chkbool(IDC_TM_DIVIDEDIR, mesh_smoothtu, hw);
    get_chkbool(IDC_USE_SMOOTH, use_smooth_grps, hw);
    get_chkbool(IDC_SMOOTHSEL, smooth_selected, hw);
  }
}


void DagUtil::scatter_update_ui(HWND hw)
{
  if (egs_sname)
    egs_sname->SetText(gs_sname);
  if (egs_objnum)
    egs_objnum->SetText(gs_objnum);
  if (egs_slope)
    egs_slope->SetText(gs_slope);
  if (egs_seed)
    egs_seed->SetText(gs_seed);
  if (egs_xydiap[0])
    egs_xydiap[0]->SetText(gs_xydiap[0]);
  if (egs_xydiap[1])
    egs_xydiap[1]->SetText(gs_xydiap[1]);
  if (egs_zdiap[0])
    egs_zdiap[0]->SetText(gs_zdiap[0]);
  if (egs_zdiap[1])
    egs_zdiap[1]->SetText(gs_zdiap[1]);

  if (hw)
  {
    CheckDlgButton(hw, IDC_GS_SCALEXY, gs_scalexy);
    CheckDlgButton(hw, IDC_GS_SCALEZ, gs_scalez);
    CheckDlgButton(hw, IDC_GS_SET2NORM, gs_set2norm);
    CheckDlgButton(hw, IDC_GS_USESMGR, gs_usesmgr);
    CheckDlgButton(hw, IDC_GS_ROTATEZ, gs_rotatez);
    CheckDlgButton(hw, IDC_GS_SELFACES, gs_selfaces);
  }
}

void DagUtil::scatter_update_vars(HWND hw)
{
  if (egs_sname)
    egs_sname->GetText(gs_sname, sizeof(gs_sname));
  get_edfloat(egs_xydiap[0], gs_xydiap[0]);
  get_edfloat(egs_xydiap[1], gs_xydiap[1]);
  get_edfloat(egs_zdiap[0], gs_zdiap[0]);
  get_edfloat(egs_zdiap[1], gs_zdiap[1]);
  get_edfloat(egs_slope, gs_slope);
  get_edint(egs_seed, gs_seed);
  get_edint(egs_objnum, gs_objnum);

  if (hw)
  {
    get_chkbool(IDC_GS_SET2NORM, gs_set2norm, hw);
    get_chkbool(IDC_GS_USESMGR, gs_usesmgr, hw);
    get_chkbool(IDC_GS_ROTATEZ, gs_rotatez, hw);
    get_chkbool(IDC_GS_SELFACES, gs_selfaces, hw);
    get_chkbool(IDC_GS_SCALEXY, gs_scalexy, hw);
    get_chkbool(IDC_GS_SCALEZ, gs_scalez, hw);
  }
}


void DagUtil::update_ui()
{
  dagor_util_update_ui(dagorUtilRoll);
  materials_update_ui(materialsRoll);
  uv_utils_update_ui(uvUtilsRoll);
  splines_and_smooth_update_ui(splinesAndSmoothRoll);
  scatter_update_ui(scatterRoll);
}


void DagUtil::update_vars()
{
  materials_update_vars(materialsRoll);
  uv_utils_update_vars(uvUtilsRoll);
  splines_and_smooth_update_vars(splinesAndSmoothRoll);
  scatter_update_vars(scatterRoll);
}


class ShowMapsCB : public ENodeCB
{
public:
  Interface *ip;
  ShowMapsCB(Interface *iptr) { ip = iptr; }
  void show_map(Mtl *m, Mtl *topm, int sub)
  {
    Texmap *tex = NULL;
    Class_ID cid = m->ClassID();
    if (cid == Class_ID(DMTL_CLASS_ID, 0))
      tex = m->GetSubTexmap(ID_DI);
    else
      tex = m->GetSubTexmap(0);
    if (tex)
      ip->ActivateTexture(tex, topm, sub);
  }
  int proc(INode *n)
  {
    if (!n)
      return ECB_CONT;
    if (!n->Selected())
      return ECB_CONT;
    Mtl *m = n->GetMtl();
    if (m)
    {
      if (m->IsMultiMtl())
      {
        int num = m->NumSubMtls();
        for (int i = 0; i < num; ++i)
        {
          Mtl *sm = m->GetSubMtl(i);
          if (sm)
            show_map(sm, m, i);
        }
      }
      else
        show_map(m, m, -1);
      n->InvalidateWS();
    }
    return ECB_CONT;
  }
};

void DagUtil::show_all_maps()
{
  if (!ip)
    return;
  ShowMapsCB cb(ip);
  enum_nodes(ip->GetRootNode(), &cb);
  ip->RedrawViews(ip->GetTime());
}

class HideMapsCB : public ENodeCB
{
public:
  Interface *ip;
  HideMapsCB(Interface *iptr) { ip = iptr; }
  void hide_map(Mtl *m, Mtl *topm, int sub)
  {
    Texmap *tex = NULL;
    Class_ID cid = m->ClassID();
    if (cid == Class_ID(DMTL_CLASS_ID, 0))
      tex = m->GetSubTexmap(ID_DI);
    else
      tex = m->GetSubTexmap(0);
    if (tex)
      ip->DeActivateTexture(tex, topm, sub);
  }
  int proc(INode *n)
  {
    if (!n)
      return ECB_CONT;
    if (!n->Selected())
      return ECB_CONT;
    Mtl *m = n->GetMtl();
    if (m)
    {
      if (m->IsMultiMtl())
      {
        int num = m->NumSubMtls();
        for (int i = 0; i < num; ++i)
        {
          Mtl *sm = m->GetSubMtl(i);
          if (sm)
            hide_map(sm, m, i);
        }
      }
      else
        hide_map(m, m, -1);
      n->InvalidateWS();
    }
    return ECB_CONT;
  }
};

void DagUtil::hide_all_maps()
{
  if (!ip)
    return;
  HideMapsCB cb(ip);
  enum_nodes(ip->GetRootNode(), &cb);
  ip->RedrawViews(ip->GetTime());
}


struct CnvMtl
{
  Mtl *om;
  Mtl *nm;
};

class Std2DagCB : public ENodeCB
{
public:
  Tab<CnvMtl> mat;
  Interface *ip;
  TimeValue time;

  Std2DagCB(Interface *iptr)
  {
    ip = iptr;
    time = ip->GetTime();
  }
  int conv_mat(Mtl *&m)
  {
    for (int i = 0; i < mat.Count(); ++i)
      if (mat[i].om == m)
      {
        m = mat[i].nm;
        return 1;
      }
    Class_ID cid = m->ClassID();
    if (cid == Class_ID(DMTL_CLASS_ID, 0))
    {
      CnvMtl cm;
      cm.om = m;
      StdMat *sm = (StdMat *)m;
      Mtl *dm = (Mtl *)ip->CreateInstance(MATERIAL_CLASS_ID, DagorMat_CID);
      assert(dm);
      IDagorMat *d = (IDagorMat *)dm->GetInterface(I_DAGORMAT);
      assert(d);
      dm->SetName(sm->GetName());
      d->set_2sided(sm->GetTwoSided());
      float si = sm->GetSelfIllum(time);
      d->set_amb(sm->GetAmbient(time) * (1 - si));
      Color diff = sm->GetDiffuse(time);
      d->set_diff(diff * (1 - si));
      d->set_spec(sm->GetSpecular(time) * sm->GetShinStr(time));
      d->set_emis(diff * si);
      d->set_power(powf(2.0f, sm->GetShininess(time) * 10.0f) * 4.0f);
      Texmap *tex = sm->GetSubTexmap(ID_DI);
      if (tex)
      {
        Color c = Color(1, 1, 1) * (1 - si);
        d->set_amb(c);
        d->set_diff(c);
        d->set_emis(Color(1, 1, 1) * si);
        dm->SetSubTexmap(0, tex);
      }
      d->set_classname(util.clsname);
      dm->ReleaseInterface(I_DAGORMAT, d);
      cm.nm = dm;
      mat.Append(1, &cm);
      m = dm;
      return 1;
    }
    return 0;
  }
  int proc(INode *n)
  {
    if (!n)
      return ECB_CONT;
    Mtl *m = n->GetMtl();
    char cnv = 0;
    if (m)
    {
      if (m->IsMultiMtl())
      {
        int num = m->NumSubMtls();
        for (int i = 0; i < num; ++i)
        {
          Mtl *sm = m->GetSubMtl(i);
          if (sm)
            if (conv_mat(sm))
            {
              m->SetSubMtl(i, sm);
              cnv = 1;
            }
        }
      }
      else
      {
        if (conv_mat(m))
        {
          n->SetMtl(m);
          cnv = 1;
        }
      }
    }
    if (cnv)
      n->InvalidateWS();
    return ECB_CONT;
  }
};

void DagUtil::stdmat_to_dagmat()
{
  if (!ip)
    return;
  Std2DagCB cb(ip);
  enum_nodes(ip->GetRootNode(), &cb);
  ip->RedrawViews(ip->GetTime());
}

class Dag2StdCB : public ENodeCB
{
public:
  Tab<CnvMtl> mat;
  Interface *ip;
  TimeValue time;

  Dag2StdCB(Interface *iptr)
  {
    ip = iptr;
    time = ip->GetTime();
  }
  int conv_mat(Mtl *&m)
  {
    for (int i = 0; i < mat.Count(); ++i)
      if (mat[i].om == m)
      {
        m = mat[i].nm;
        return 1;
      }
    Class_ID cid = m->ClassID();
    if (cid == DagorMat_CID || cid == DagorMat2_CID)
    {
      CnvMtl cm;
      cm.om = m;
      Mtl *newm = (Mtl *)ip->CreateInstance(MATERIAL_CLASS_ID, Class_ID(DMTL_CLASS_ID, 0));
      assert(newm);
      StdMat *sm = (StdMat *)newm;
      IDagorMat *d = (IDagorMat *)m->GetInterface(I_DAGORMAT);
      assert(d);

      sm->SetName(m->GetName());
      sm->SetTwoSided(d->get_2sided());
      Color c = d->get_emis();
      float si = (c.r + c.g + c.b) / 3;
      sm->SetSelfIllum(si, time);
      c = d->get_amb();
      if (si != 1)
        c /= 1 - si;
      sm->SetAmbient(c, time);
      c = d->get_diff();
      if (si != 1)
        c /= 1 - si;
      sm->SetDiffuse(c, time);
      sm->SetSpecular(d->get_spec(), time);
      sm->SetShinStr(1, time);

      // d->set_power( powf(2.0f,sm->GetShininess(time)*10.0f) *4.0f );
      float pw = d->get_power();
      if (pw != 0)
        pw = log((float)pw / 4) / log((float)2) / 10;
      else
        sm->SetShinStr(0, time);
      sm->SetShininess(pw, time);

      Texmap *tex = m->GetSubTexmap(0);
      if (tex)
        sm->SetSubTexmap(ID_DI, tex);

      m->ReleaseInterface(I_DAGORMAT, d);

      cm.nm = newm;
      mat.Append(1, &cm);
      m = newm;
      return 1;
    }
    return 0;
  }
  int proc(INode *n)
  {
    if (!n)
      return ECB_CONT;
    Mtl *m = n->GetMtl();
    char cnv = 0;
    if (m)
    {
      if (m->IsMultiMtl())
      {
        int num = m->NumSubMtls();
        for (int i = 0; i < num; ++i)
        {
          Mtl *sm = m->GetSubMtl(i);
          if (sm)
            if (conv_mat(sm))
            {
              m->SetSubMtl(i, sm);
              cnv = 1;
            }
        }
      }
      else
      {
        if (conv_mat(m))
        {
          n->SetMtl(m);
          cnv = 1;
        }
      }
    }
    if (cnv)
      n->InvalidateWS();
    return ECB_CONT;
  }
};

void DagUtil::dagmat_to_stdmat()
{
  if (!ip)
    return;
  Dag2StdCB cb(ip);
  enum_nodes(ip->GetRootNode(), &cb);
  ip->RedrawViews(ip->GetTime());
}

////////////////////////////////////////////////////////////

typedef dag_auto_ptr<Mtl> MtlPtr;

typedef std::vector<M_STD_STRING> StrVec;
typedef std::vector<MtlPtr> MtlVec;
typedef std::map<M_STD_STRING, M_STD_STRING> StrMap;


M_STD_STRING CfgReplace(M_STD_STRING str, M_STD_STRING dst, M_STD_STRING src)
{
  M_STD_STRING res;

  res = str;

  size_t pos = 0;

  while ((pos = res.find(dst, pos)) != std::string::npos)
  {
    res.replace(pos, dst.length(), src);
    pos += src.length();
  }

  return res;
}

class Dag2EnumeratorCB : public ENodeCB
{
public:
  StrVec materials;
  StrVec scripts;

  StrMap mats;

  Interface *ip;
  TimeValue time;

  Dag2EnumeratorCB(Interface *iptr)
  {
    ip = iptr;
    time = ip->GetTime();
  }

  ~Dag2EnumeratorCB() {}

  int enumerate(Mtl *&m)
  {
    Class_ID cid = m->ClassID();
    if (cid == DagorMat_CID)
    {
      IDagorMat *d = (IDagorMat *)m->GetInterface(I_DAGORMAT);
      M_STD_STRING name = d->get_classname();
      M_STD_STRING script = d->get_script();

      script = CfgReader::Replace(script, _T("\r"), _T("\\r"));
      script = CfgReader::Replace(script, _T("\n"), _T("\\n"));


      materials.push_back(name);
      scripts.push_back(script);
      M_STD_STRING mat;

      mat += _T("cn=\"");
      mat += name;
      mat += _T("\"");
      mat += _T("\n");
      mat += _T("sc=\"");
      mat += script;
      mat += _T("\"");
      mat += _T("\n");

      if (mats.find(mat) == mats.end())
      {
        mats[mat] = _T("ok");
      }


      /*

        IDagorMat *d = (IDagorMat *) m->GetInterface(I_DAGORMAT);
      std::string name = d->get_classname();
      std::string script = d->get_script();

      script = CfgReader::Replace(script, "\r", "\\r");
      script = CfgReader::Replace(script, "\n", "\\n");

      materials.push_back(name);
      scripts.push_back(script);
      std::string mat;

      mat += "cn=\"";
      mat += name;
      mat += "\"";
      mat += "\n";
      mat += "sc=\"";
      mat += script;
      mat += "\"";
      mat += "\n";

      if ( mats.find(mat) == mats.end() )
      {
        mats[mat] = "ok";
      }*/
    }
    return 0;
  }

  int proc(INode *n)
  {
    if (!n)
      return ECB_CONT;
    Mtl *m = n->GetMtl();
    char cnv = 0;
    if (m)
    {
      if (m->IsMultiMtl())
      {
        int num = m->NumSubMtls();
        for (int i = 0; i < num; ++i)
        {
          Mtl *sm = m->GetSubMtl(i);
          if (sm)
          {
            enumerate(sm);
          }
        }
      }
      else
      {
        enumerate(m);
      }
    }
    return ECB_CONT;
  }
};

class Dag2UniqueCB : public ENodeCB
{
public:
  CfgShader *cfg;

  Tab<Mtl *> mats;

  Interface *ip;
  TimeValue time;

  Dag2UniqueCB(Interface *iptr)
  {
    ip = iptr;
    time = ip->GetTime();
  }

  ~Dag2UniqueCB() {}

  Mtl *unique_mat(Mtl *mf)
  {
    bool same = false;
    IDagorMat *d = (IDagorMat *)mf->GetInterface(I_DAGORMAT);

    for (int pos = 0; pos < mats.Count(); pos++)
    {
      IDagorMat *c = (IDagorMat *)(mats[pos])->GetInterface(I_DAGORMAT);

      for (int i = 0; i < NUMTEXMAPS; i++)
      {
        M_STD_STRING texd = d->get_texname(i) ? d->get_texname(i) : M_STD_STRING();
        M_STD_STRING texc = c->get_texname(i) ? c->get_texname(i) : M_STD_STRING();

        if (texd.compare(texc) == 0 && d->get_param(i) == c->get_param(i))
        {
          same = true;
        }
        else
        {
          same = false;
          break;
        }
      }


      M_STD_STRING named = d->get_classname() ? d->get_classname() : M_STD_STRING();
      M_STD_STRING namec = c->get_classname() ? c->get_classname() : M_STD_STRING();

      M_STD_STRING scriptd = d->get_script() ? d->get_script() : M_STD_STRING();
      M_STD_STRING scriptc = c->get_script() ? c->get_script() : M_STD_STRING();

      if (!same)
        continue;

      if (d->get_2sided() == c->get_2sided() && d->get_amb() == c->get_amb() && named.compare(namec) == 0 &&
          d->get_diff() == c->get_diff() && d->get_emis() == c->get_emis() && scriptd.compare(scriptc) == 0 &&
          d->get_spec() == d->get_spec() && (fabs(d->get_power()) - fabs(c->get_power())) < 0.000001f)
      {
        return mats[pos];
      }
      else
      {
        same = false;
      }
    }

    mats.Append(1, &mf);

    return NULL;
  }

  int proc(INode *n)
  {
    if (!n)
      return ECB_CONT;
    Mtl *m = n->GetMtl();
    char cnv = 0;
    if (m)
    {
      if (m->IsMultiMtl())
      {
        int num = m->NumSubMtls();
        for (int i = 0; i < num; ++i)
        {
          Mtl *sm = m->GetSubMtl(i);
          if (sm)
          {
            Mtl *umtl = unique_mat(sm);
            if (umtl)
            {
              m->SetSubMtl(i, umtl);
            }
          }
        }
      }
      else
      {
        Mtl *umtl = unique_mat(m);
        if (umtl)
        {
          n->SetMtl(umtl);
        }
      }
    }
    return ECB_CONT;
  }
};

void collapse_materials(Interface *ip)
{
  if (!ip)
    return;

  Dag2UniqueCB cbv(ip);
  enum_nodes(ip->GetRootNode(), &cbv);
  ip->RedrawViews(ip->GetTime());
}


class Dag2DagCB : public ENodeCB
{
public:
  CfgShader *cfg;

  StrMap mats_name;
  StrMap mats_script;

  Interface *ip;
  TimeValue time;

  Dag2DagCB(Interface *iptr)
  {
    ip = iptr;
    time = ip->GetTime();

    TCHAR filename[MAX_PATH];
    CfgShader::GetCfgFilename(_T("DagorConvert.cfg"), filename);
    cfg = new CfgShader(filename);

    cfg->GetShaderNames();

    for (int pos = 0; pos < cfg->shader_names.size(); ++pos)
    {
      M_STD_STRING cn = cfg->GetKeyValue(_T("cn"), cfg->shader_names.at(pos));
      M_STD_STRING sc = cfg->GetKeyValue(_T("sc"), cfg->shader_names.at(pos));
      M_STD_STRING sh = cfg->GetKeyValue(_T("sh"), cfg->shader_names.at(pos));
      M_STD_STRING pm = cfg->GetKeyValue(_T("pm"), cfg->shader_names.at(pos));
      pm = CfgReader::Replace(pm, _T("\\r"), _T("\r"));
      pm = CfgReader::Replace(pm, _T("\\n"), _T("\n"));

      M_STD_STRING mat;

      mat += cn;
      mat += _T("/");
      mat += sc;


      mats_name[mat] = sh;
      mats_script[mat] = pm;
    }
  }

  ~Dag2DagCB() { delete cfg; }

  int conv_mat(Mtl *&m)
  {
    Class_ID cid = m->ClassID();
    if (cid == DagorMat_CID)
    {
      IDagorMat *d = (IDagorMat *)m->GetInterface(I_DAGORMAT);

      M_STD_STRING cn = d->get_classname();
      cn = CfgReader::Remove(cn, _T(" "));

      M_STD_STRING sc = d->get_script();
      sc = CfgReader::Replace(sc, _T("\r"), _T("\\r"));
      sc = CfgReader::Replace(sc, _T("\n"), _T("\\n"));
      //                      sc = CfgReader::Remove(sc, " ");

      M_STD_STRING mat;

      mat += cn;
      mat += _T("/");
      mat += sc;

      if (mats_name.find(mat) != mats_name.end() && mats_script.find(mat) != mats_script.end())
      {
        M_STD_STRING sh = mats_name[mat];
        M_STD_STRING pm = mats_script[mat];
        d->set_classname(sh.c_str());
        d->set_script(pm.c_str());
      } // else debug("<%s>",(LPSTR)mat.c_str());
    }
    return 0;
  }

  int proc(INode *n)
  {
    if (!n)
      return ECB_CONT;
    Mtl *m = n->GetMtl();
    char cnv = 0;
    if (m)
    {
      if (m->IsMultiMtl())
      {
        int num = m->NumSubMtls();
        for (int i = 0; i < num; ++i)
        {
          Mtl *sm = m->GetSubMtl(i);
          if (sm)
          {
            conv_mat(sm);
          }
        }
      }
      else
      {
        conv_mat(m);
      }
    }
    return ECB_CONT;
  }
};

void DagUtil::enumerate()
{
  if (!ip)
    return;
  Dag2EnumeratorCB cb(ip);
  enum_nodes(ip->GetRootNode(), &cb);
  ip->RedrawViews(ip->GetTime());
  int pos = 0;

  for (pos = 0; pos < cb.materials.size(); ++pos)
  {
    M_STD_STRING str1 = cb.materials.at(pos);
    M_STD_STRING str2 = cb.scripts.at(pos);
  }

  StrMap::iterator itr = cb.mats.begin();

  std::string buffer;
  pos = 0;
  while (itr != cb.mats.end())
  {
    std::ostringstream index;
    index << pos;

    buffer += "[";
    buffer += index.str();
    buffer += "]";
    buffer += "\n";
    buffer += "\n";

    buffer += wideToStr(itr->first.c_str());

    buffer += "\n";
    buffer += "sh=\"\"";
    buffer += "\n";
    buffer += "pm=\"\"";
    buffer += "\n";
    buffer += "\n";

    itr++;
    ++pos;
  }

  FILE *f = fopen("d:/materials.txt", "w+t");
  fwrite(buffer.c_str(), 1, buffer.length(), f);
  fclose(f);
}

void DagUtil::collapse()
{
  if (!ip)
    return;

  Dag2UniqueCB cbv(ip);
  enum_nodes(ip->GetRootNode(), &cbv);
  ip->RedrawViews(ip->GetTime());
}

void DagUtil::convert()
{
  if (!ip)
    return;

  Dag2DagCB cbv(ip);
  enum_nodes(ip->GetRootNode(), &cbv);
  ip->RedrawViews(ip->GetTime());

  Dag2EnumeratorCB cb(ip);
  enum_nodes(ip->GetRootNode(), &cb);
  ip->RedrawViews(ip->GetTime());
  int pos = 0;

  for (pos = 0; pos < cb.materials.size(); ++pos)
  {
    M_STD_STRING str1 = cb.materials.at(pos);
    M_STD_STRING str2 = cb.scripts.at(pos);
  }

  StrMap::iterator itr = cb.mats.begin();

  std::string buffer;
  pos = 0;
  while (itr != cb.mats.end())
  {
    std::ostringstream index;
    index << pos;

    buffer += "[";
    buffer += index.str();
    buffer += "]";
    buffer += "\n";
    buffer += "\n";

    buffer += wideToStr(itr->first.c_str());

    // buffer += itr->first;

    buffer += "\n";
    buffer += "sh=\"\"";
    buffer += "\n";
    buffer += "pm=\"\"";
    buffer += "\n";
    buffer += "\n";

    itr++;
    ++pos;
  }

  FILE *f = fopen("d:/materials_conv.txt", "w+t");
  fwrite(buffer.c_str(), 1, buffer.length(), f);
  fclose(f);
}

//////////////////////////////////////////////////////////////////////////////

static int intersects(const Box3 &a, const Box3 &b)
{
  if (a.Max().x < b.Min().x)
    return 0;
  if (a.Max().y < b.Min().y)
    return 0;
  if (a.Max().z < b.Min().z)
    return 0;
  if (a.Min().x > b.Max().x)
    return 0;
  if (a.Min().y > b.Max().y)
    return 0;
  if (a.Min().z > b.Max().z)
    return 0;
  return 1;
}

bool DagUtil::copy_tch_buf(Mesh &m, MappingInfo &mi, int c)
{
  mi.clear();
  if (!m.mapSupport(c))
  {
    return false;
  }
  mi.tface.Append(m.getNumFaces(), m.mapFaces(c));
  mi.tvert.Append(m.getNumMapVerts(c), m.mapVerts(c));
  return true;
}

bool DagUtil::paste_tch_buf(Mesh &m, MappingInfo &mi, int c)
{
  if (mi.tface.Count() != m.getNumFaces())
  {
    return false;
  }
  m.setMapSupport(c);
  m.setNumMapVerts(c, mi.tvert.Count());
  m.setNumMapFaces(c, mi.tface.Count());
  memcpy(m.mapVerts(c), &mi.tvert[0], sizeof(UVVert) * mi.tvert.Count());
  memcpy(m.mapFaces(c), &mi.tface[0], sizeof(TVFace) * mi.tface.Count());
  return true;
}

static struct
{
  int numv;
} tcinfo[MAX_MESHMAPS];

bool DagUtil::copy_tch1(Mesh &m, TCCOPY tc, int src, int dst, int mapa, bool sel_faces)
{
  // if(src<0) src=0;
  // if(dst<0) dst=0;
  // if(mapa<0) mapa=0;
  if (tc == CTC_COPY || tc == CTC_MOVE)
  {
    if (!m.mapSupport(src))
      return false;
    m.setMapSupport(dst);
    m.setNumMapVerts(dst, m.getNumMapVerts(src));
    m.setNumMapFaces(dst, m.getNumFaces());
    memcpy(m.mapVerts(dst), m.mapVerts(src), sizeof(Point3) * m.getNumMapVerts(src));
    memcpy(m.mapFaces(dst), m.mapFaces(src), sizeof(TVFace) * m.getNumFaces());
    if (tc == CTC_MOVE)
    {
      m.setMapSupport(src, FALSE);
    }
    return true;
  }
  else if (tc == CTC_INFO)
  {
    for (int i = 0; i < MAX_MESHMAPS; ++i)
      if (m.mapSupport(i))
      {
        if (tcinfo[i].numv < 0)
          tcinfo[i].numv = 0;
        tcinfo[i].numv += m.getNumMapVerts(i);
      }
    return true;
  }
  else if (tc == CTC_KILL)
  {
    m.setMapSupport(src, FALSE);
    return true;
  }
  else if (tc == CTC_SWAP)
  {
    if (!m.mapSupport(src))
      return false;
    if (!m.mapSupport(dst))
      return false;
    Tab<TVFace> dstfcs;
    dstfcs.Append(m.getNumFaces(), m.mapFaces(dst));
    Tab<UVVert> dstvrts;
    dstvrts.Append(m.getNumMapVerts(dst), m.mapVerts(dst));
    TVFace *srctf = m.mapFaces(src);
    m.setNumMapVerts(dst, m.getNumMapVerts(src));
    m.setNumMapFaces(dst, m.getNumFaces());
    memcpy(m.mapVerts(dst), m.mapVerts(src), sizeof(Point3) * m.getNumMapVerts(src));
    memcpy(m.mapFaces(dst), m.mapFaces(src), sizeof(TVFace) * m.getNumFaces());

    m.setNumMapVerts(src, dstvrts.Count());
    m.setNumMapFaces(src, dstfcs.Count());
    memcpy(m.mapFaces(src), &dstfcs[0], sizeof(TVFace) * m.getNumFaces());
    memcpy(m.mapVerts(src), &dstvrts[0], sizeof(Point3) * dstvrts.Count());
    return true;
  }
  else
  {
    src = mapa;
    if (!m.mapSupport(src))
      return false;
    Tab<char> sv;
    int nv = m.getNumMapVerts(src);
    sv.SetCount(nv);
    memset(&sv[0], 0, nv);
    int nf = m.numFaces;
    TVFace *tf = m.mapFaces(src);
    int i;
    for (i = 0; i < nf; ++i)
      if (!sel_faces || mesh_face_sel(m)[i])
      {
        for (int j = 0; j < 3; ++j)
          sv[tf[i].t[j]] |= 1;
      }
      else
      {
        for (int j = 0; j < 3; ++j)
          sv[tf[i].t[j]] |= 2;
      }
    int ncv = 0;
    for (i = 0; i < nv; ++i)
      if (sv[i] == 3)
        ++ncv;
    Tab<int> vmap;
    vmap.SetCount(nv);
    if (ncv)
    {
      m.setNumMapVerts(src, nv + ncv, TRUE);
      Point3 *tv = m.mapVerts(src);
      int j = nv;
      for (i = 0; i < nv; ++i)
        if (sv[i] == 3)
        {
          tv[j] = tv[i];
          vmap[i] = j++;
        }
        else
          vmap[i] = i;
    }
    for (i = 0; i < nf; ++i)
      if (!sel_faces || mesh_face_sel(m)[i])
      {
        for (int j = 0; j < 3; ++j)
          if (sv[tf[i].t[j]] == 3)
            tf[i].t[j] = vmap[tf[i].t[j]];
      }
    Tab<char> sv2;
    sv2.SetCount(nv + ncv);
    for (i = 0; i < nv; ++i)
      sv2[i] = sv[i];
    for (i = 0; i < ncv; ++i)
      sv2[nv + i] = 1;
    Point3 *tv = m.mapVerts(src);
    for (i = 0; i < nv + ncv; ++i, ++tv)
      if (sv2[i] == 1)
      {
        if (tc == CTC_MUL)
        {
          tv->x *= tc_uk;
          tv->y *= tc_vk;
        }
        else if (tc == CTC_ADD)
        {
          tv->x += tc_uo;
          tv->y += tc_vo;
        }
        else if (tc == CTC_XCHG)
        {
          float a = tv->x;
          tv->x = tv->y;
          tv->y = a;
        }
        else if (tc == CTC_FLIPU)
          tv->x = 1 - tv->x;
        else if (tc == CTC_FLIPV)
          tv->y = 1 - tv->y;
      }
    m.InvalidateTopologyCache();
    m.InvalidateGeomCache();
    return true;
  }
  return false;
}


// void DagUtil::fix_edgevis(){
//         update_vars();
//         TimeValue time=ip->GetTime();
//         int n=ip->GetSelNodeCount();
//         for(int i=0;i<n;++i) {
//                 INode *node=ip->GetSelNode(i);
//                 if(!node) continue;
//                 Object *o=node->GetObjectRef();
//                 if(!o) continue;
//                 if(!o->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID,0))) {continue;}
//                 Mesh &m=((TriObject*)o)->mesh;
//                 for(int i=0;i<m.numFaces;++i) {
//                         Face &f=m.faces[i];
//                         f.flags=(f.flags&~(EDGE_A|EDGE_C))|(f.flags&EDGE_A?EDGE_C:0)|(f.flags&EDGE_C?EDGE_A:0);
//                 }
//                 node->InvalidateWS();
//                 break;
//         }
//         ip->RedrawViews(time);
// }


void DagUtil::convertSpacesRecursive(INode *node)
{
  if (!node)
    return;

  if (node != ip->GetRootNode()) // 'Scene Root' shouldn't be exported anyway.
  {
    TCHAR newName[10000];
    _tcscpy(newName, node->GetName());
    spaces_to_underscores(newName);
    if (_tcscmp(node->GetName(), newName))
      node->SetName(newName);
  }

  for (int childNo = 0; childNo < node->NumberOfChildren(); childNo++)
    convertSpacesRecursive(node->GetChildNode(childNo));
}


void DagUtil::convertSpaces()
{
  update_vars();
  convertSpacesRecursive(ip->GetRootNode());
  TimeValue time = ip->GetTime();
  ip->RedrawViews(time);
}


void DagUtil::removeIsolatedVertices(TopologyAdapter &adapter)
{
  bool *isReferencedArray = new bool[adapter.getNumVerts()];
  unsigned int *newIndicesArray = new unsigned int[adapter.getNumVerts()];
  ZeroMemory(isReferencedArray, adapter.getNumVerts() * sizeof(bool));
  for (unsigned int faceNo = 0; faceNo < adapter.mesh->getNumFaces(); faceNo++)
  {
    isReferencedArray[adapter.getIndex(faceNo, 0)] = true;
    isReferencedArray[adapter.getIndex(faceNo, 1)] = true;
    isReferencedArray[adapter.getIndex(faceNo, 2)] = true;
  }
  unsigned int currentVertex = 0, vertexNo;
  for (vertexNo = 0; vertexNo < adapter.getNumVerts(); vertexNo++)
  {
    adapter.copyVertex(vertexNo, currentVertex);
    newIndicesArray[vertexNo] = currentVertex;
    if (isReferencedArray[vertexNo])
      currentVertex++;
  }
  adapter.setNumVerts(currentVertex);
  if (adapter.countRemovedVertices())
    isolatedVerticesRemovedNum += vertexNo - currentVertex;

  for (unsigned int faceNo = 0; faceNo < adapter.mesh->getNumFaces(); faceNo++)
  {
    adapter.setIndex(faceNo, 0, newIndicesArray[adapter.getIndex(faceNo, 0)]);
    adapter.setIndex(faceNo, 1, newIndicesArray[adapter.getIndex(faceNo, 1)]);
    adapter.setIndex(faceNo, 2, newIndicesArray[adapter.getIndex(faceNo, 2)]);
  }

  delete[] isReferencedArray;
  delete[] newIndicesArray;
}


class GeometryTopologyAdapter : public TopologyAdapter
{
public:
  GeometryTopologyAdapter(Mesh *in_mesh) : TopologyAdapter(in_mesh) {}

  unsigned int getNumVerts() { return mesh->getNumVerts(); }

  void setNumVerts(unsigned int num_verts) { mesh->setNumVerts(num_verts, TRUE, TRUE); }

  unsigned int getIndex(unsigned int face_no, unsigned int index_no) { return mesh->faces[face_no].v[index_no]; }

  void setIndex(unsigned int face_no, unsigned int index_no, unsigned int index) { mesh->faces[face_no].v[index_no] = index; }

  void copyVertex(unsigned int from, unsigned int to) { mesh->verts[to] = mesh->verts[from]; }

  bool countRemovedVertices() { return true; }
};


class TextureTopologyAdapter : public TopologyAdapter
{
  int channel;

public:
  TextureTopologyAdapter(Mesh *in_mesh, int ch) : TopologyAdapter(in_mesh), channel(ch) {}

  unsigned int getNumVerts() { return mesh->getNumMapVerts(channel); }

  void setNumVerts(unsigned int num_verts) { mesh->setNumMapVerts(channel, num_verts, TRUE); }

  unsigned int getIndex(unsigned int face_no, unsigned int index_no)
  {
    TVFace *tvFace = mesh->mapFaces(channel);
    assert(tvFace);
    return tvFace[face_no].t[index_no];
  }

  void setIndex(unsigned int face_no, unsigned int index_no, unsigned int index)
  {
    TVFace *tvFace = mesh->mapFaces(channel);
    assert(tvFace);
    tvFace[face_no].t[index_no] = index;
  }

  void copyVertex(unsigned int from, unsigned int to)
  {
    TVFace *tvFace = mesh->mapFaces(channel);
    assert(tvFace);
    tvFace[to] = tvFace[from];
  }
};


class ColorTopologyAdapter : public TopologyAdapter
{
public:
  ColorTopologyAdapter(Mesh *in_mesh) : TopologyAdapter(in_mesh) {}

  unsigned int getNumVerts() { return mesh->getNumVertCol(); }

  void setNumVerts(unsigned int num_verts) { mesh->setNumVertCol(num_verts, TRUE); }

  unsigned int getIndex(unsigned int face_no, unsigned int index_no) { return mesh->vcFace[face_no].t[index_no]; }

  void setIndex(unsigned int face_no, unsigned int index_no, unsigned int index) { mesh->vcFace[face_no].t[index_no] = index; }

  void copyVertex(unsigned int from, unsigned int to) { mesh->vertCol[to] = mesh->vertCol[from]; }
};


bool DagUtil::isDegenerate(Mesh *mesh, unsigned int face_no, const Matrix3 &otm)
{
  Point3 vertex1 = mesh->verts[mesh->faces[face_no].v[0]] * otm;
  Point3 vertex2 = mesh->verts[mesh->faces[face_no].v[1]] * otm;
  Point3 vertex3 = mesh->verts[mesh->faces[face_no].v[2]] * otm;

  if (is_equal_point(vertex1, vertex2, DEGENERATE_VERTEX_DELTA))
    return true;

  if (is_equal_point(vertex1, vertex3, DEGENERATE_VERTEX_DELTA))
    return true;

  if (is_equal_point(vertex2, vertex3, DEGENERATE_VERTEX_DELTA))
    return true;

  Point3 diff = vertex3 - vertex1;
  Point3 dir = vertex2 - vertex1;
  float sqrlen = dir.LengthSquared();
  float t = DotProd(diff, dir) / sqrlen;
  diff -= t * dir;
  float linedist = diff.Length();

  if (linedist < DEGENERATE_VERTEX_DELTA)
    return true;

  return false;
}


void DagUtil::removeDegeneratesRecursive(INode *node)
{
  if (!node)
    return;

  ObjectState state = node->EvalWorldState(0);
  if (state.obj && !node->IsNodeHidden())
  {
    TriObject *triObject = (TriObject *)state.obj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0));

    if (triObject && triObject == state.obj)
    {
      editableMeshFound = true;
      Mesh *mesh = &triObject->GetMesh();

      TimeValue time = GetCOREInterface()->GetTime();

      Matrix3 otm;
      otm = get_scaled_object_tm(node, time);
      otm = otm * Inverse(get_scaled_stretch_node_tm(node, time));
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
      otm *= (float)GetSystemUnitScale(UNITS_METERS);
#else
      otm *= (float)GetMasterScale(UNITS_METERS);
#endif

      // Remove degenerate faces.

      for (unsigned int faceNo = 0; faceNo < mesh->getNumFaces(); faceNo++)
      {
        if (isDegenerate(mesh, faceNo, otm))
        {
          degenerateFacesRemovedNum++;
          for (unsigned int faceToCopyNo = faceNo + 1; faceToCopyNo < mesh->getNumFaces(); faceToCopyNo++)
          {
            mesh->faces[faceToCopyNo - 1] = mesh->faces[faceToCopyNo];
            if (mesh->vcFace)
              mesh->vcFace[faceToCopyNo - 1] = mesh->vcFace[faceToCopyNo];

            for (int ch = 0; ch < MAX_MESHMAPS; ++ch)
            {
              if (!mesh->mapSupport(ch))
                continue;
              TVFace *tf = mesh->mapFaces(ch);
              if (!tf)
                continue;
              tf[faceToCopyNo - 1] = tf[faceToCopyNo];
            }
          }
          mesh->setNumFaces(mesh->getNumFaces() - 1, TRUE, TRUE);
          faceNo--;
        }
      }


      // Remove isolated vertices.

      removeIsolatedVertices(GeometryTopologyAdapter(mesh));

      /*  for ( int ch = 0; ch < MAX_MESHMAPS; ++ch )
        {
          if ( ! mesh->mapSupport (ch) || !  mesh->mapFaces (ch) )
            continue;
          removeIsolatedVertices(TextureTopologyAdapter(mesh, ch));
        }*/

      if (mesh->vertCol)
        removeIsolatedVertices(ColorTopologyAdapter(mesh));


      // Updates.

      mesh->InvalidateGeomCache();
      mesh->InvalidateTopologyCache();

      triObject->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
      triObject->NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED);

      node->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
      node->NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED);
    }

    if (triObject && triObject != state.obj)
      delete triObject;
  }


  for (int childNo = 0; childNo < node->NumberOfChildren(); childNo++)
    removeDegeneratesRecursive(node->GetChildNode(childNo));
}


void DagUtil::removeDegenerates()
{
  update_vars();
  editableMeshFound = false;
  degenerateFacesRemovedNum = 0;
  isolatedVerticesRemovedNum = 0;
  removeDegeneratesRecursive(ip->GetRootNode());
  TimeValue time = ip->GetTime();
  ip->RedrawViews(time);

  if (!editableMeshFound)
  {
    MessageBox(dagorUtilRoll, _T("No editable mesh found."), _T("Remove degenerates"), MB_ICONWARNING);
  }
  {
    TCHAR str[1000];
    _stprintf(str, _T("%d faces and %d vertices removed."), degenerateFacesRemovedNum, isolatedVerticesRemovedNum);
    MessageBox(dagorUtilRoll, str, _T("Remove degenerates"), MB_OK);
  }
}


void DagUtil::selectFloorFaces()
{
  TimeValue time = ip->GetTime();

  int n = ip->GetSelNodeCount();
  for (int i = 0; i < n; ++i)
  {
    INode *node = ip->GetSelNode(i);
    if (!node)
      continue;
    Object *o = node->GetObjectRef();
    if (!o)
      continue;
    if (!o->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID, 0)))
      continue;
    node->InvalidateWS();

    Matrix3 wtm = node->GetObjectTM(time);
    BOOL flip = wtm.Parity();

    Mesh &m = ((TriObject *)o)->mesh;
    mesh_face_sel(m).SetSize(m.numFaces);
    for (int fi = 0; fi < m.numFaces; ++fi)
    {
      Face &f = m.faces[fi];
      Point3 v0 = m.verts[f.v[0]] * wtm;
      Point3 v1 = m.verts[f.v[1]] * wtm;
      Point3 v2 = m.verts[f.v[2]] * wtm;
      Point3 normal = Normalize(CrossProd(v1 - v0, v2 - v0));
      if (flip)
        normal = -normal;
      mesh_face_sel(m).Set(fi, ((normal.z >= 0.02f) ? 1 : 0));
    }
  }

  ip->RedrawViews(time);
}


void DagUtil::copy_tch()
{
  update_vars();
  TimeValue time = ip->GetTime();
  int n = ip->GetSelNodeCount();
  int i;
  for (i = 0; i < n; ++i)
  {
    INode *node = ip->GetSelNode(i);
    if (!node)
      continue;
    Object *o = node->GetObjectRef();
    if (!o)
      continue;
    if (!o->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID, 0)))
    {
      continue;
    }
    node->InvalidateWS();
    if (tc_srcch >= 0)
    {
      if (!copy_tch_buf(((TriObject *)o)->mesh, chanelbuf[tc_srcch], tc_srcch))
      {
        TSTR s;
        s.printf(_T("Map not support"));
        ip->DisplayTempPrompt(s);
      }
      else
      {
        TSTR s;
        s.printf(_T("Map chanel %d was copied"), tc_srcch);
        ip->DisplayTempPrompt(s);
      }
    }
    else
    {
      for (int mi = 0; mi < MAX_MESHMAPS; ++mi)
      {
        copy_tch_buf(((TriObject *)o)->mesh, chanelbuf[mi], mi);
      }
      TSTR s;
      s.printf(_T("Map chanels were copied"));
      ip->DisplayTempPrompt(s);
    }
    node->InvalidateWS();
    break;
  }
  ip->RedrawViews(time);
}

void DagUtil::paste_tch()
{
  update_vars();
  TimeValue time = ip->GetTime();
  int n = ip->GetSelNodeCount();
  for (int i = 0; i < n; ++i)
  {
    INode *node = ip->GetSelNode(i);
    if (!node)
      continue;
    Object *o = node->GetObjectRef();
    if (!o)
      continue;
    if (!o->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID, 0)))
      continue;
    node->InvalidateWS();
    if (tc_dstch >= 0)
    {
      if (!paste_tch_buf(((TriObject *)o)->mesh, chanelbuf[tc_srcch], tc_dstch))
      {
        TSTR s;
        s.printf(_T("Invalid face count"));
        ip->DisplayTempPrompt(s);
      }
      else
      {
        TSTR s;
        s.printf(_T("Map chanel %d was pasted into map chanel %d"), tc_srcch, tc_dstch);
        ip->DisplayTempPrompt(s);
      }
    }
    else
    {
      bool res = false;
      for (int mi = 0; mi < MAX_MESHMAPS; ++mi)
      {
        res = res | paste_tch_buf(((TriObject *)o)->mesh, chanelbuf[mi], mi);
      }
      if (!res)
      {
        TSTR s;
        s.printf(_T("Invalid face count"));
        ip->DisplayTempPrompt(s);
      }
      else
      {
        TSTR s;
        s.printf(_T("Some map chanels were pasted"));
        ip->DisplayTempPrompt(s);
      }
    }
    node->InvalidateWS();
    break;
  }
  ip->RedrawViews(time);
}

void DagUtil::copy_tch(TCCOPY tc)
{
  update_vars();
  TimeValue time = ip->GetTime();
  int n = ip->GetSelNodeCount();
  bool result = n ? true : false;
  if (tc == CTC_INFO)
  {
    for (int i = 0; i < MAX_MESHMAPS; ++i)
      tcinfo[i].numv = -1;
  }
  for (int i = 0; i < n; ++i)
  {
    INode *node = ip->GetSelNode(i);
    if (!node)
      continue;
    Object *o = node->GetObjectRef();
    if (!o)
      continue;
    if (!o->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID, 0)))
    {
      result = false;
      continue;
    }
    node->InvalidateWS();
    result = (copy_tch1(((TriObject *)o)->mesh, tc, tc_srcch, tc_dstch, tc_mapach, bool(tc_facesel != 0)) && result);
    node->InvalidateWS();
  }
  ip->RedrawViews(time);
  if (result)
  {
    int nr = 1;
    TSTR s;
    s.printf(GetString(IDS_MAPOP_PERFORMED), nr);
    ip->DisplayTempPrompt(s);

    if (tc == CTC_INFO)
    {
      TSTR buf = _T("");
      for (int i = 0; i < MAX_MESHMAPS; ++i)
        if (tcinfo[i].numv >= 0)
        {
          TSTR s;
          s.printf(_T("#%d: %d verts\r\n"), i, tcinfo[i].numv);
          buf += s;
        }
      MessageBox(uvUtilsRoll, buf, _T("TC Info"), MB_OK);
    }
  }
  else
  {
    int nr = 1;
    TSTR s;
    s.printf(GetString(IDS_MAPOP_NOTPERFORMED), nr);
    ip->DisplayTempPrompt(s);
  }
}

void DagUtil::select_face(bool vert)
{
  update_vars();
  TimeValue time = ip->GetTime();
  if (ip->GetSelNodeCount() != 1)
  {
    ip->DisplayTempPrompt(GetString(IDS_SEL_1_OBJ));
    return;
  }
  INode *node = ip->GetSelNode(0);
  if (!node)
    return;
  Object *o = node->GetObjectRef();
  if (!o)
    return;
  if (!o->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID, 0)))
  {
    ip->DisplayTempPrompt(GetString(IDS_MUST_BE_EDMESH));
    return;
  }
  Mesh &m = ((TriObject *)o)->mesh;
  if (vert)
  {
    mesh_vert_sel(m).ClearAll();
    if (sel_fid >= 0 && sel_fid < m.numVerts)
      mesh_vert_sel(m).Set(sel_fid);
  }
  else
  {
    mesh_face_sel(m).ClearAll();
    if (sel_fid >= 0 && sel_fid < m.numFaces)
      mesh_face_sel(m).Set(sel_fid);
  }
  node->InvalidateWS();
  ip->RedrawViews(time);
}

class MultiBooleanCB : public ENodeCB
{
public:
  Interface *ip;
  Tab<INode *> nodes;
  MultiBooleanCB(Interface *iptr) { ip = iptr; }
  bool add(MNMesh &to, MNMesh &from, int inst)
  {
    // if(tm1) to.Transform(*tm1);
    // if(tm2) from.Transform(*tm2);
    to.PrepForBoolean();
    from.PrepForBoolean();
    MNMesh m3;
    if (!m3.MakeBoolean(to, from, MESHBOOL_UNION, NULL))
    {
      // if(inst){to=m3;}
      return false;
    }
    m3.selLevel = MNM_SL_FACE;
    BitArray selected(m3.numf);
    selected.ClearAll();
    m3.ElementFromFace(0, selected);
    int nm = selected.NumberSet();
    // debug("%d %d",nm,m3.numf);
    if (nm != m3.numf)
    {
      if (inst)
        to = m3;
      return false;
    }
    to = m3;
    return true;
  }
  void adjust_matrix(Matrix3 &m, Mesh &me)
  {
    if (m.Parity())
    {
      for (int i = 0; i < me.getNumFaces(); ++i)
      {
        int v = me.faces[i].v[2];
        me.faces[i].v[2] = me.faces[i].v[1];
        me.faces[i].v[1] = v;
      }
    }
  }
  /*bool intersects(MNMesh &from,MNMesh &to){
          to.PrepForBoolean();
          from.PrepForBoolean();
          MNMesh m3;
          if(!m3.MakeBoolean(to, from, MESHBOOL_UNION, NULL)) return false;
          m3.selLevel=MNM_SL_FACE;
          BitArray selected(m3.numf);
          selected.ClearAll();
          m3.ElementFromFace(0,selected);
          int nm=selected.NumberSet();
          debug("%d %d",nm,m3.numf);
          if(nm!=m3.numf) return false;
          return true;
  }*/
  void boolean()
  {
    if (nodes.Count() < 2)
      return;
    // sort_ip=ip;

    Object *obj = nodes[0]->EvalWorldState(ip->GetTime()).obj;
    assert(obj);
    TriObject *tri = (TriObject *)obj->ConvertToType(ip->GetTime(), Class_ID(TRIOBJ_CLASS_ID, 0));
    assert(tri);
    Mesh m1 = tri->GetMesh();
    MNMesh result(m1);
    Matrix3 tm = nodes[0]->GetObjTMAfterWSM(ip->GetTime());
    adjust_matrix(tm, m1);
    // sort_pos=tm.GetTrans();
    // base_sort=nodes[0];
    // nodes.Sort(CompTable);
    // Matrix3 tm=nodes[0]->GetObjectTM(ip->GetTime());
    result.Transform(tm);
    BitArray aligned(nodes.Count());
    aligned.ClearAll();
    aligned.Set(0);
    int crtm = ip->GetTime();
    bool flg = false;
    do
    {
      flg = false;
      for (int i = 1; i < nodes.Count(); ++i)
      {
        if (aligned[i])
          continue;
        Object *obj1 = nodes[i]->EvalWorldState(crtm).obj;
        assert(obj1);
        TriObject *tri1 = (TriObject *)obj1->ConvertToType(crtm, Class_ID(TRIOBJ_CLASS_ID, 0));
        assert(tri1);
        Mesh m2 = tri1->GetMesh();
        Matrix3 tm1 = nodes[i]->GetObjTMAfterWSM(crtm);
        adjust_matrix(tm1, m2);
        // Matrix3 tm1=nodes[i]->GetObjectTM(ip->GetTime());
        MNMesh ad(m2);
        ad.Transform(tm1);
        // if(!intersects(ad,result)) continue;
        if (!add(result, ad, 0))
          continue;
        if (tri1 != obj1)
          delete tri1;
        aligned.Set(i);
        flg = true;
      }
    } while (flg);
    {
      for (int i = 0; i < nodes.Count(); ++i)
        if (!aligned[i])
          ip->DeSelectNode(nodes[i]);
    }

    if (tri != obj)
      delete tri;
    Object *resobj = (Object *)ip->CreateInstance(GEOMOBJECT_CLASS_ID, triObjectClassID);
    assert(resobj);
    TSTR ss;
    resobj->GetClassName(ss);
    result.OutToTri(((TriObject *)resobj)->mesh);

    INode *resn = ip->CreateObjectNode(resobj);
    assert(resn);
    TSTR name(_T("MultiBoolResult"));
    ip->MakeNameUnique(name);
    resn->SetName(name);
    Matrix3 idm;
    idm.IdentityMatrix();
    resn->SetNodeTM(0, idm);
  }
  int proc(INode *n)
  {
    if (!n)
      return ECB_CONT;
    if (!n->Selected())
      return ECB_CONT;
    Object *obj = n->EvalWorldState(ip->GetTime()).obj;

    if (!obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
      return ECB_CONT;


    nodes.Append(1, &n);
    return ECB_CONT;
  }
};

// void DagUtil::perform_multiboolean(){
//         if(!ip) return;
//         MultiBooleanCB cb(ip);
//         enum_nodes(ip->GetRootNode(),&cb);
//         cb.boolean();
//         ip->RedrawViews(ip->GetTime());
// }

void DagUtil::get_ntrk()
{
  if (ip->GetSelNodeCount() != 1)
  {
    ip->DisplayTempPrompt(GetString(IDS_GET_NT_ERR));
    return;
  }
  INode *node = ip->GetSelNode(0);
  if (!node->HasNoteTracks())
  {
    ip->DisplayTempPrompt(GetString(IDS_GET_NT_ERR));
    return;
  }
  if (node->NumNoteTracks() != 1)
  {
    ip->DisplayTempPrompt(GetString(IDS_GET_NT_ERR));
    return;
  }
  NoteTrack *ntrack = node->GetNoteTrack(0);
  if (ntrack)
    if (ntrack->ClassID() == Class_ID(NOTETRACK_CLASS_ID, 0))
    {
      DefNoteTrack &nt = *(DefNoteTrack *)ntrack;
      ntrk_buf = nt.keys;
    }
    else
    {
      ip->DisplayTempPrompt(GetString(IDS_GET_NT_ERR));
      return;
    }
}

void DagUtil::put_ntrk()
{
  if (!ntrk_buf.Count())
  {
    ip->DisplayTempPrompt(GetString(IDS_NO_NTRK));
    return;
  }
  if (ip->GetSelNodeCount() != 1)
  {
    ip->DisplayTempPrompt(GetString(IDS_PUT_NT_ERR));
    return;
  }
  INode *node = ip->GetSelNode(0);
  if (node->HasNoteTracks())
  {
    for (int i = 0; i < node->NumNoteTracks(); ++i)
    {
      NoteTrack *nt = node->GetNoteTrack(i);
      if (nt)
      {
        node->DeleteNoteTrack(nt);
        i = -1;
      }
    }
  }
  DefNoteTrack *nt = (DefNoteTrack *)NewDefaultNoteTrack();
  if (!nt)
  {
    ip->DisplayTempPrompt(GetString(IDS_PUT_NT_ERR));
    return;
  }
  nt->keys = ntrk_buf;
  node->AddNoteTrack(nt);
}

///*
// void DagUtil::flatten_mesh(Mesh &m,Matrix3 wtm,INode *node) {
//         Point3 tnorm=Normalize(VectorTransform(Inverse(wtm),Point3(0,0,1)));
//         Tab<Point3> dv; dv.SetCount(m.numVerts);
//         Tab<int> dvn; dvn.SetCount(m.numVerts);
//         typedef float FaceEL[3];
//         Tab<FaceEL> fel; fel.SetCount(m.numFaces);
//         for(int i=0;i<m.numFaces;++i) {
//                 Point3 v0=m.verts[m.faces[i].v[0]];
//                 Point3 v1=m.verts[m.faces[i].v[1]];
//                 Point3 v2=m.verts[m.faces[i].v[2]];
//                 fel[i][0]=Length(v1-v0);
//                 fel[i][1]=Length(v2-v1);
//                 fel[i][2]=Length(v0-v2);
//         }
//         float ek=.1f;
//         float da=DegToRad(3);
//         float cosda=cosf(da);
//         for(int iter=0;;++iter) {
//                 for(int i=0;i<dv.Count();++i) {dv[i]=Point3(0,0,0);dvn[i]=0;}
//                 for(i=0;i<m.numFaces;++i) {
//                         Point3 v0=m.verts[m.faces[i].v[0]];
//                         Point3 v1=m.verts[m.faces[i].v[1]];
//                         Point3 v2=m.verts[m.faces[i].v[2]];
//                         Point3 n=Normalize(CrossProd(v1-v0,v2-v0));
//                         float dc=DotProd(n,tnorm);
//                         float a;
//                         if(dc>cosda) {
//                                 if(dc>=1) a=0; else a=acosf(dc);
//                         }else a=da;
//                         Point3 w=Normalize(CrossProd(n,tnorm))*a;
//                         Point3 c=(v0+v1+v2)/3;
//                         dv[m.faces[i].v[0]]+=CrossProd(w,v0-c); dvn[m.faces[i].v[0]]++;
//                         dv[m.faces[i].v[1]]+=CrossProd(w,v1-c); dvn[m.faces[i].v[1]]++;
//                         dv[m.faces[i].v[2]]+=CrossProd(w,v2-c); dvn[m.faces[i].v[2]]++;
//                 }
//                 for(i=0;i<dv.Count();++i) {
//                         if(dvn[i]) m.verts[i]+=dv[i]/dvn[i];
//                         dv[i]=Point3(0,0,0); dvn[i]=0;
//                 }
//                 for(i=0;i<m.numFaces;++i) {
//                         Point3 v0=m.verts[m.faces[i].v[0]];
//                         Point3 v1=m.verts[m.faces[i].v[1]];
//                         Point3 v2=m.verts[m.faces[i].v[2]];
//                         float l,dl,el; Point3 e;
//                         e=v1-v0; l=Length(e); el=fel[i][0]; dl=el-l;
//                                 e*=dl*ek/l;
//                                 dv[m.faces[i].v[1]]+=e; dv[m.faces[i].v[0]]-=e;
//                                 dvn[m.faces[i].v[1]]++; dvn[m.faces[i].v[0]]++;
//                         e=v2-v1; l=Length(e); el=fel[i][1]; dl=el-l;
//                                 e*=dl*ek/l;
//                                 dv[m.faces[i].v[2]]+=e; dv[m.faces[i].v[1]]-=e;
//                                 dvn[m.faces[i].v[2]]++; dvn[m.faces[i].v[1]]++;
//                         e=v0-v2; l=Length(e); el=fel[i][2]; dl=el-l;
//                                 e*=dl*ek/l;
//                                 dv[m.faces[i].v[0]]+=e; dv[m.faces[i].v[2]]-=e;
//                                 dvn[m.faces[i].v[0]]++; dvn[m.faces[i].v[2]]++;
//                 }
//                 for(i=0;i<dv.Count();++i) if(dvn[i]) m.verts[i]+=dv[i]/dvn[i];
//                 if((iter&31)==0) {
//                         m.InvalidateGeomCache();
//                         node->InvalidateWS();
//                         ip->RedrawViews(ip->GetTime());
//                         ip->ProgressUpdate(0);
//                 }
//                 if(ip->GetCancel()) break;
//         }
//         m.InvalidateGeomCache();
// }
//*/

/*
void DagUtil::flatten_mesh(Mesh &m,Matrix3 wtm,INode *node) {
        Point3 tnorm=Normalize(VectorTransform(Inverse(wtm),Point3(0,0,1)));
        AdjEdgeList ael(m);
        Tab<Point3> dv; dv.SetCount(m.numVerts);
        Tab<int> dvn; dvn.SetCount(m.numVerts);
        Tab<Point3> fn; fn.SetCount(m.numFaces);
        typedef int AdjVert[2];
        Tab<AdjVert> av; av.SetCount(ael.edges.Count());
        Tab<float> el; el.SetCount(ael.edges.Count());
        float ak=.1f,ek=.1f;
        float da=DegToRad(3);
        float cosda=cosf(da);
        float maxel=0;
        for(int i=0;i<ael.edges.Count();++i) {
                int v0=ael.edges[i].v[0],v1=ael.edges[i].v[1];
                el[i]=Length(m.verts[v0]-m.verts[v1]);
                if(el[i]>maxel) maxel=el[i];
                int f=ael.edges[i].f[0],j;
                if(f!=UNDEFINED) {
                        for(j=0;j<2;++j) if(m.faces[f].v[j]!=v0 && m.faces[f].v[j]!=v1) break;
                        av[i][0]=m.faces[f].v[j];
                }else av[i][0]=UNDEFINED;
                f=ael.edges[i].f[1];
                if(f!=UNDEFINED) {
                        for(j=0;j<2;++j) if(m.faces[f].v[j]!=v0 && m.faces[f].v[j]!=v1) break;
                        av[i][1]=m.faces[f].v[j];
                }else av[i][1]=UNDEFINED;
        }
        Point3 center;
        {
                Box3 box;
                for(int i=0;i<m.numVerts;++i) box+=m.verts[i];
                center=box.Center();
        }
//      float da=DegToRad(3);
//      float sinda=sinf(da);
        for(int iter=0;;++iter) {
                for(int i=0;i<dv.Count();++i) {dv[i]=Point3(0,0,0);dvn[i]=0;}
                for(i=0;i<m.numFaces;++i) {
                        Point3 v0=m.verts[m.faces[i].v[0]];
                        Point3 v1=m.verts[m.faces[i].v[1]];
                        Point3 v2=m.verts[m.faces[i].v[2]];
                        Point3 n=Normalize(CrossProd(v1-v0,v2-v0));
                        float dc=DotProd(n,tnorm);
                        float a;
                        if(dc>cosda) {
                                if(dc>=1) a=0; else a=acosf(dc);
                        }else a=da;
                        Point3 w=Normalize(CrossProd(n,tnorm))*a;
                        Point3 c=(v0+v1+v2)/3;
                        dv[m.faces[i].v[0]]+=CrossProd(w,v0-c); dvn[m.faces[i].v[0]]++;
                        dv[m.faces[i].v[1]]+=CrossProd(w,v1-c); dvn[m.faces[i].v[1]]++;
                        dv[m.faces[i].v[2]]+=CrossProd(w,v2-c); dvn[m.faces[i].v[2]]++;
                }
                for(i=0;i<dv.Count();++i) if(dvn[i]) m.verts[i]+=dv[i]/dvn[i];
                for(i=0;i<dv.Count();++i) {dv[i]=Point3(0,0,0);dvn[i]=0;}
                for(i=0;i<fn.Count();++i) {
                        Point3 v0=m.verts[m.faces[i].v[0]];
                        Point3 v1=m.verts[m.faces[i].v[1]];
                        Point3 v2=m.verts[m.faces[i].v[2]];
                        fn[i]=Normalize(CrossProd(v1-v0,v2-v0));
                };
                for(i=0;i<ael.edges.Count();++i) {
                        Point3 p0=m.verts[ael.edges[i].v[0]];
                        Point3 e=Normalize(m.verts[ael.edges[i].v[1]]-p0);
                        float ds=DotProd(e,CrossProd(fn[ael.edges[i].f[0]],fn[ael.edges[i].f[1]]));
                        float a;
//                      if(-sinda<ds && ds<sinda) a=asinf(ds);
//                      else if(ds>0) a=da; else a=-da;
                        if(ds>=1) a=HALFPI; else if(ds<=-1) a=-HALFPI; else a=asinf(ds);
                        Point3 w=e*(a*ak);
                        if(av[i][0]!=UNDEFINED)
                                {dv[av[i][0]]+=CrossProd(w,m.verts[av[i][0]]-p0); dvn[av[i][0]]++;}
                        if(av[i][1]!=UNDEFINED)
                                {dv[av[i][1]]-=CrossProd(w,m.verts[av[i][1]]-p0); dvn[av[i][1]]++;}
                }
//              for(i=0;i<3;++i) dvn[m.faces[0].v[i]]=0;
                for(i=0;i<dv.Count();++i) if(dvn[i]) m.verts[i]+=dv[i]/dvn[i];
                for(i=0;i<dv.Count();++i) {dv[i]=Point3(0,0,0);dvn[i]=0;}
                for(i=0;i<ael.edges.Count();++i) {
                        Point3 p0=m.verts[ael.edges[i].v[0]];
                        Point3 e=m.verts[ael.edges[i].v[1]]-p0;
//                      float l=Length(e),dl=el[i]-l;
                        float l=Length(e),dl=maxel-l;
                        if(dl<0) {
                                e*=dl*ek/l;
                                dv[ael.edges[i].v[1]]+=e; dv[ael.edges[i].v[0]]-=e;
                                dvn[ael.edges[i].v[1]]++; dvn[ael.edges[i].v[0]]++;
                        }
                }
//              for(i=0;i<3;++i) dvn[m.faces[0].v[i]]=0;
                for(i=0;i<dv.Count();++i) if(dvn[i]) m.verts[i]+=dv[i];///dvn[i];
                Box3 box;
                for(i=0;i<m.numVerts;++i) box+=m.verts[i];
                Point3 d=center-box.Center();
                for(i=0;i<m.numVerts;++i) m.verts[i]+=d;
                if((iter&31)==0) {
                        m.InvalidateGeomCache();
                        node->InvalidateWS();
                        ip->RedrawViews(ip->GetTime());
                        ip->ProgressUpdate(0);
                }
                if(ip->GetCancel()) break;
                //break;
        }
        m.InvalidateGeomCache();
}
*/

static DWORD WINAPI dummyprogressfn(LPVOID arg) { return 0; }


void DagUtil::maxtures2materials(Mesh &m, INode *node)
{
  if (!m.mapSupport(7) || !m.mapSupport(8))
    return;
  m.setMapSupport(1);
  Tab<UVVert> tverts;
  for (int i = 0; i < m.numFaces; ++i)
  {
    int mats[9] = {-1, -1, -1, -1, -1, -1, -1, -1, -1};
    int maps[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    int weights[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    int lastmat = 0;
    for (int v = 0; v < 3; ++v)
    {
      Point3 ta = m.mapVerts(7)[m.mapFaces(7)[i].t[v]];
      Point3 tb = m.mapVerts(8)[m.mapFaces(8)[i].t[v]];
      ta.y = 1 - ta.y;
      tb.y = 1 - tb.y;
      int cw = real2int(tb.y);
      int w[3], mat[3], map[3];
      w[0] = (cw >> 8) & 0xFF;
      w[1] = cw & 0xFF;
      w[2] = 255 - w[0] - w[1];

      mat[0] = real2int(ta.x) & 0xFF;
      mat[1] = real2int(ta.y) & 0xFF;
      mat[2] = real2int(tb.x) & 0xFF;
      map[0] = (real2int(ta.x) >> 8) & 0xFF;
      map[1] = (real2int(ta.y) >> 8) & 0xFF;
      map[2] = (real2int(tb.x) >> 8) & 0xFF;
      for (int wi = 0; wi < 3; ++wi)
        if (w[wi])
        {
          int mi;
          for (mi = 0; mi < lastmat; ++mi)
            if (mats[mi] == mat[wi])
              break;
          if (mi == lastmat)
            lastmat = mi + 1;
          mats[mi] = mat[wi];
          maps[mi] = map[wi];
          // if(weights[mi]<w[wi]) weights[mi]=w[wi];
          weights[mi] += w[wi];
        }
    }
    int bw = 0, bm = 0, bt = 1;
    for (int mi = 0; mi < lastmat; ++mi)
    {
      if (weights[mi] > bw)
      {
        bw = weights[mi];
        bm = mats[mi];
        bt = maps[mi] + 1;
      }
    }
    m.faces[i].setMatID(bm);
    if (m.mapSupport(bt))
    {
      UVVert tv[3];
      tv[0] = m.mapVerts(bt)[m.mapFaces(bt)[i].t[0]];
      tv[1] = m.mapVerts(bt)[m.mapFaces(bt)[i].t[1]];
      tv[2] = m.mapVerts(bt)[m.mapFaces(bt)[i].t[2]];
      int t = tverts.Append(3, tv);
      m.mapFaces(1)[i].t[0] = m.getNumMapVerts(1) + t;
      m.mapFaces(1)[i].t[1] = m.getNumMapVerts(1) + t + 1;
      m.mapFaces(1)[i].t[2] = m.getNumMapVerts(1) + t + 2;
    }
  }
  int oldsize = m.getNumMapVerts(1);
  m.setNumMapVerts(1, oldsize + tverts.Count(), 1);
  memcpy(m.mapVerts(1) + oldsize, &tverts[0], tverts.Count() * sizeof(UVVert));
}

void DagUtil::maxtures2materials()
{
  HCURSOR hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
  ip->ProgressStart(_T("Wait..."), FALSE, dummyprogressfn, NULL);
  TimeValue time = ip->GetTime();
  int n = ip->GetSelNodeCount();
  for (int i = 0; i < n; ++i)
  {
    INode *node = ip->GetSelNode(i);
    if (!node)
      continue;
    Object *o = node->GetObjectRef();
    if (!o)
      continue;
    if (!o->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID, 0)))
      continue;
    node->InvalidateWS();
    maxtures2materials(((TriObject *)o)->mesh, node);
    node->InvalidateWS();
    if (ip->GetCancel())
      break;
  }
  ip->ProgressEnd();
  ip->RedrawViews(time);
  SetCursor(hCur);
}

// void DagUtil::flatten() {
//         HCURSOR hCur=SetCursor(LoadCursor(NULL,IDC_WAIT));
//         ip->ProgressStart(_T("Wait..."),FALSE,dummyprogressfn,NULL);
//         TimeValue time=ip->GetTime();
//         int n=ip->GetSelNodeCount();
//         for(int i=0;i<n;++i) {
//                 INode *node=ip->GetSelNode(i);
//                 if(!node) continue;
//                 Object *o=node->GetObjectRef();
//                 if(!o) continue;
//                 if(!o->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID,0))) continue;
//                 node->InvalidateWS();
//                 flatten_mesh(((TriObject*)o)->mesh,node->GetObjectTM(time),node);
//                 node->InvalidateWS();
//                 if(ip->GetCancel()) break;
//         }
//         ip->ProgressEnd();
//         ip->RedrawViews(time);
//         SetCursor(hCur);
// }

/*
static void break_sel_edges(Mesh &m) {
        AdjEdgeList ael(m);
        Tab<int> bv;
        BitArray em(ael.edges.Count());
        em.ClearAll();
        for(int i=0;i<ael.edges.Count();++i) if(ael.edges[i].Selected(m.faces,m.edgeSel)) {
                for(int vi=0;vi<2;++vi) {
                        int v=ael.edges[i].v[vi];
                        for(int j=0;j<bv.Count();++j) if(bv[j]==v) break;
                        if(j>=bv.Count()) bv.Append(1,&v);
                }
        }
        for(i=0;i<bv.Count();++i) if(ael.list[bv[i]].Count()!=2) break;
        int e;
        int v1=bv[i],v2=ael.edges[e=ael.list[v1][0]].OtherVert(v1);
        for(;;) {
                em.Set(e);
                Face &f=m.faces[ael.edges[e].f[0]];
                int fi;
                if((f.v[0]==v1 && f.v[1]==v2)
                || (f.v[1]==v1 && f.v[2]==v2)
                || (f.v[2]==v1 && f.v[0]==v2) fi=ael.edges[e].f[1];
                        else fi=ael.edges[e].f[0];
                for(int j=0;j<3;++j)
                        if(m.faces[fi].v[j]==v1) m.faces[fi].v[j]=nv1;
                if(ael.list[v2].Count()!=2) break;
        }
}
*/

/*
void DagUtil::break_edges() {
        TimeValue time=ip->GetTime();
        int n=ip->GetSelNodeCount();
        for(int i=0;i<n;++i) {
                INode *node=ip->GetSelNode(i);
                if(!node) continue;
                Object *o=node->GetObjectRef();
                if(!o) continue;
                if(!o->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID,0))) continue;
                node->InvalidateWS();
//              break_sel_edges(((TriObject*)o)->mesh);
                node->InvalidateWS();
        }
        ip->RedrawViews(time);
}
*/

static void save_shape(Mesh &m, int ch)
{
  m.setMapSupport(ch);
  m.setNumMapVerts(ch, m.numVerts);
  m.setNumMapFaces(ch, m.numFaces);
  TVFace *tf = m.mapFaces(ch);
  for (int i = 0; i < m.numFaces; ++i)
    for (int j = 0; j < 3; ++j)
      tf[i].t[j] = m.faces[i].v[j];
  memcpy(m.mapVerts(ch), m.verts, m.numVerts * sizeof(Point3));
}

static void restore_shape(Mesh &m, int ch)
{
  if (!m.mapSupport(ch))
    return;
  TVFace *tf = m.mapFaces(ch);
  Point3 *tv = m.mapVerts(ch);
  for (int i = 0; i < m.numFaces; ++i)
    for (int j = 0; j < 3; ++j)
      m.verts[m.faces[i].v[j]] = tv[tf[i].t[j]];
  m.InvalidateGeomCache();
}

// void DagUtil::save_shp() {
//         TimeValue time=ip->GetTime();
//         int n=ip->GetSelNodeCount();
//         for(int i=0;i<n;++i) {
//                 INode *node=ip->GetSelNode(i);
//                 if(!node) continue;
//                 Object *o=node->GetObjectRef();
//                 if(!o) continue;
//                 if(!o->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID,0))) continue;
//                 node->InvalidateWS();
//                 save_shape(((TriObject*)o)->mesh,2);
//                 node->InvalidateWS();
//         }
//         ip->RedrawViews(time);
// }

// void DagUtil::restore_shp() {
//         TimeValue time=ip->GetTime();
//         int n=ip->GetSelNodeCount();
//         for(int i=0;i<n;++i) {
//                 INode *node=ip->GetSelNode(i);
//                 if(!node) continue;
//                 Object *o=node->GetObjectRef();
//                 if(!o) continue;
//                 if(!o->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID,0))) continue;
//                 node->InvalidateWS();
//                 restore_shape(((TriObject*)o)->mesh,2);
//                 node->InvalidateWS();
//         }
//         ip->RedrawViews(time);
// }

int apply_ltmap(Mesh &m, int mati, int mch, int w, int h, float psz, int &usage_percent);

// void DagUtil::map_ltmap(bool autostep) {
//         get_edint(elm_matid,lm_matid);
//         get_edint(elm_wd,lm_wd);
//         get_edint(elm_ht,lm_ht);
//         get_edfloat(elm_step,lm_step);
//         get_edint(elm_mapch,lm_mapch);
//         TimeValue time=ip->GetTime();
//         if(ip->GetSelNodeCount()!=1)
//                 {ip->DisplayTempPrompt(GetString(IDS_SEL_1_OBJ));return;}
//         INode *node=ip->GetSelNode(0);
//         if(!node) return;
//         Object *o=node->GetObjectRef();
//         if(!o)
//                 {ip->DisplayTempPrompt(GetString(IDS_MUST_BE_EDMESH));return;}
//         if(!o->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID,0)))
//                 {ip->DisplayTempPrompt(GetString(IDS_MUST_BE_EDMESH));return;}
//         float bstep=-1,astep;
//         int bperc=0;
//         if(autostep) {
//                 ip->ProgressStart(_T("Wait... or press ESC to stop"),FALSE,dummyprogressfn,NULL);
//                 ip->ProgressUpdate(0,FALSE,_T(""));
//         }else ip->DisplayTempPrompt(_T("Wait..."));
//         for(int iter=0;;) {
//                 if(bstep>0) {
//                         lm_step=(bstep+astep)*.5f;
//                 }
//                 int perc;
//                 if(!apply_ltmap(((TriObject*)o)->mesh,lm_matid-1,lm_mapch,lm_wd,lm_ht,lm_step,perc)) {
//                         if(autostep) {
//                                 TSTR s; s.printf(_T("%d: error (best %d%%)"),iter+1,bperc);
//                                 ip->ProgressUpdate(0,FALSE,s);
//                                 if(bstep<0) lm_step*=2;
//                                 else{
//                                         astep=lm_step;
//                                 }
//                         }else{
//                                 ip->DisplayTempPrompt(GetString(IDS_LTMAP_ERR));
//                         }
//                 }else{
//                         if(autostep) {
//                                 TSTR s; s.printf(_T("%d: %d%% used"),iter+1,perc);
//                                 ip->ProgressUpdate(0,FALSE,s);
//                                 bperc=perc;
//                                 if(bstep<0) {
//                                         bstep=lm_step;
//                                         astep=lm_step*.5f;
//                                 }else{
//                                         bstep=lm_step;
//                                 }
//                         }else{
//                                 TSTR s; s.printf(GetString(IDS_LTMAP_OK),perc);
//                                 ip->DisplayTempPrompt(s);
//                         }
//                 }
//                 if(!autostep) break;
//                 if(++iter>=50 || ip->GetCancel()) {
//                         if(lm_step!=bstep && bstep>0) {
//                                 lm_step=bstep;
//                                 if(!apply_ltmap(((TriObject*)o)->mesh,lm_matid-1,lm_mapch,lm_wd,lm_ht,lm_step,perc))
//                                         ip->DisplayTempPrompt(GetString(IDS_LTMAP_ERR));
//                                 else{
//                                         TSTR s; s.printf(GetString(IDS_LTMAP_OK),perc);
//                                         ip->DisplayTempPrompt(s);
//                                 }
//                         }
//                         break;
//                 }
//         }
//         if(autostep) {
//                 ip->ProgressEnd();
//                 update_ui();
//         }
//         node->InvalidateWS();
//         ip->RedrawViews(time);
// }

static class SuperLight : public ObjLightDesc
{
public:
  SuperLight() : ObjLightDesc(NULL)
  {
    affectDiffuse = TRUE;
    affectSpecular = FALSE;
    ambientOnly = FALSE;
  }
  void DeleteThis() {}
  BOOL Illuminate(ShadeContext &sc, Point3 &normal, Color &color, Point3 &dir, float &dot_nl, float &diffuseCoef)
  {
    color.White();
    dir = normal;
    dot_nl = 1;
    diffuseCoef = 1;
    return TRUE;
  }
} superlight;

static Point3 pabs(const Point3 &a) { return Point3(fabs(a.x), fabs(a.y), fabs(a.z)); }

float ComputeFaceCurvature(Point3 *n, Point3 *v)
{
  Point3 nc = (n[0] + n[1] + n[2]) / 3.0f;
  Point3 dn0 = n[0] - nc;
  Point3 dn1 = n[1] - nc;
  Point3 dn2 = n[2] - nc;
  Point3 c = (v[0] + v[1] + v[2]) / 3.0f;
  Point3 v0 = v[0] - c;
  Point3 v1 = v[1] - c;
  Point3 v2 = v[2] - c;
  float d0 = DotProd(dn0, v0) / LengthSquared(v0);
  float d1 = DotProd(dn1, v1) / LengthSquared(v1);
  float d2 = DotProd(dn2, v2) / LengthSquared(v2);
  float ad0 = (float)fabs(d0);
  float ad1 = (float)fabs(d1);
  float ad2 = (float)fabs(d2);
  return (ad0 > ad1) ? (ad0 > ad2 ? d0 : d2) : ad1 > ad2 ? d1 : d2;
}

void compute_bump_vectors(const Point3 tv[3], const Point3 v[3], Point3 bvec[3])
{
  float uva, uvb, uvc, uvd, uvk;
  Point3 v1, v2;
  uva = tv[1].x - tv[0].x;
  uvb = tv[2].x - tv[0].x;
  uvc = tv[1].y - tv[0].y;
  uvd = tv[2].y - tv[0].y;
  uvk = uvb * uvc - uva * uvd;
  v1 = v[1] - v[0];
  v2 = v[2] - v[0];
  if (uvk != 0)
  {
    bvec[0] = (uvc * v2 - uvd * v1) / uvk;
    bvec[1] = -(uva * v2 - uvb * v1) / uvk;
  }
  else
  {
    if (uva != 0)
      bvec[0] = v1 / uva;
    else if (uvb != 0)
      bvec[0] = v2 / uvb;
    else
      bvec[0] = Point3(0.0f, 0.0f, 0.0f);
    if (uvc != 0)
      bvec[1] = v1 / uvc;
    else if (uvd != 0)
      bvec[1] = v2 / uvd;
    else
      bvec[1] = Point3(0.0f, 0.0f, 0.0f);
  }
  bvec[2] = Point3(0, 0, 1);
}

class GetLightsCB : public ENodeCB
{
public:
  TimeValue time;
  Tab<ObjLightDesc *> &light;

  GetLightsCB(TimeValue t, Tab<ObjLightDesc *> &lt) : light(lt) { time = t; }
  int proc(INode *n)
  {
    if (!n)
      return ECB_CONT;
    if (n->IsNodeHidden())
      return ECB_CONT;
    Object *o = n->EvalWorldState(time).obj;
    if (o->SuperClassID() != LIGHT_CLASS_ID)
      return ECB_CONT;
    LightObject *lo = ((LightObject *)o);
    LightState lst;
    lo->EvalLightState(time, FOREVER, &lst);
    ObjLightDesc *l = lo->CreateLightDesc(n);
    if (!l)
      return ECB_CONT;
    n->SetRenderData(l);
    light.Append(1, &l);
    return ECB_CONT;
  }
};

static void get_lights(TimeValue time, Interface *ip, Tab<ObjLightDesc *> &light)
{
  GetLightsCB cb(time, light);
  enum_nodes(ip->GetRootNode(), &cb);
  class MyRC : public RendContext
  {
  public:
    Color gll;
    Color GlobalLightLevel() const { return gll; }
  } rc;
  rc.gll = ip->GetLightTint(time, FOREVER) * ip->GetLightLevel(time, FOREVER);
  for (int i = 0; i < light.Count(); ++i)
  {
    light[i]->Update(time, rc, NULL, TRUE, TRUE);
    light[i]->UpdateGlobalLightLevel(rc.gll);
    light[i]->UpdateViewDepParams(Matrix3(1));
  }
}

class RenderMapSC : public ShadeContext
{
public:
  TimeValue curtime;
  INode *node;
  Mesh *mesh;
  int face;
  Matrix3 cam2obj, obj2cam;
  Box3 objbox;
  Point3 bary, norm, normal, facenorm, view, orgview, pt, dpt, dpt_dx, dpt_dy, vp[3], bv[3], bo;
  Point3 tv[MAX_MESHMAPS][3];
  Color back_color;
  float curve, face_sz, sz_ratio, facecurve;
  Tab<Point3> fnorm;
  Tab<Point3[3]> vnorm;
  Tab<ObjLightDesc *> light;
  bool hastv[MAX_MESHMAPS];
  char lighting;

  RenderMapSC(TimeValue t, char lt, Interface *ip)
  {
    curtime = t;
    doMaps = TRUE;
    filterMaps = TRUE;
    backFace = FALSE;
    node = NULL;
    mesh = NULL;
    face = -1;
    lighting = lt;
    if (lighting)
    {
      ambientLight = ip->GetAmbient(t, FOREVER);
      get_lights(t, ip, light);
      nLights = light.Count();
    }
    else
    {
      ambientLight = Color(0, 0, 0);
      light.SetCount(1);
      light[0] = &superlight;
      nLights = 1;
    }
    back_color = ip->GetBackGround(t, FOREVER);
  }
  void setnode(INode *n)
  {
    node = n;
    if (n)
    {
      obj2cam = n->GetObjectTM(curtime);
      cam2obj = Inverse(obj2cam);
    }
  }
  void setmesh(Mesh &m)
  {
    mesh = &m;
    objbox = m.getBoundingBox();
    fnorm.SetCount(m.numFaces);
    int i;
    for (i = 0; i < m.numFaces; ++i)
      fnorm[i] = Normalize(VectorTransform(
        CrossProd(m.verts[m.faces[i].v[1]] - m.verts[m.faces[i].v[0]], m.verts[m.faces[i].v[2]] - m.verts[m.faces[i].v[0]]), obj2cam));
    Tab<int> *vfc = new Tab<int>[m.numVerts];
    assert(vfc);
    for (i = 0; i < m.numFaces; ++i)
      for (int j = 0; j < 3; ++j)
      {
        int v = m.faces[i].v[j];
        // assert(v>=0 && v<m.numVerts);
        vfc[v].Append(1, &i, 8);
      }
    vnorm.SetCount(m.numFaces);
    for (i = 0; i < m.numFaces; ++i)
    {
      DWORD sgr = m.faces[i].smGroup;
      for (int j = 0; j < 3; ++j)
      {
        Point3 n = fnorm[i];
        int v = m.faces[i].v[j];
        // assert(v>=0 && v<m.numVerts);
        for (int k = 0; k < vfc[v].Count(); ++k)
          if (vfc[v][k] != i)
            if (m.faces[vfc[v][k]].smGroup & sgr)
              n += fnorm[vfc[v][k]];
        vnorm[i][j] = Normalize(n);
      }
    }
    delete[] vfc;
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
      vp[i] = mesh->verts[f.v[i]] * obj2cam;
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
    facecurve = ComputeFaceCurvature(vnorm[fi], vp);
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
    normal = Normalize(vnorm[face][0] * bary[0] + vnorm[face][1] * bary[1] + vnorm[face][2] * bary[2]);
    orgview = -normal;
    norm = normal;
    view = orgview;
  }
  BOOL InMtlEditor() { return FALSE; }
  int Antialias() { return TRUE; }
  int ProjType() { return 0; }
  LightDesc *Light(int n) { return light[n]; }
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
  Point3 PObj() { return pt * cam2obj; }
  Point3 DPObj() { return VectorTransform(dpt, cam2obj); }
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
    co = back_color;
    tr.Black();
  }
  Point3 PointTo(const Point3 &p, RefFrame ito)
  {
    switch (ito)
    {
      case REF_WORLD:
      case REF_CAMERA: return p;
      case REF_OBJECT: return p * cam2obj;
    }
    return p;
  }
  Point3 PointFrom(const Point3 &p, RefFrame ito)
  {
    switch (ito)
    {
      case REF_WORLD:
      case REF_CAMERA: return p;
      case REF_OBJECT: return p * obj2cam;
    }
    return p;
  }
  Point3 VectorTo(const Point3 &p, RefFrame ito)
  {
    switch (ito)
    {
      case REF_WORLD:
      case REF_CAMERA: return p;
      case REF_OBJECT: return VectorTransform(p, cam2obj);
    }
    return p;
  }
  Point3 VectorFrom(const Point3 &p, RefFrame ito)
  {
    switch (ito)
    {
      case REF_WORLD:
      case REF_CAMERA: return p;
      case REF_OBJECT: return VectorTransform(p, obj2cam);
    }
    return p;
  }
};

// void DagUtil::render_map() {
//         if(!ip->GetSelNodeCount())
//                 {ip->DisplayTempPrompt(GetString(IDS_NO_SEL));return;}
//         get_edint(elm_wd,lm_wd);
//         get_edint(elm_ht,lm_ht);
//         get_edint(elm_mapch,lm_mapch);
//         if(!TheManager->SelectFileOutput(&rmap_bi,ip->GetMAXHWnd())) return;
//         rmap_bi.SetType(BMM_TRUE_32);
//         rmap_bi.SetWidth(lm_wd);
//         rmap_bi.SetHeight(lm_ht);
//         rmap_bi.SetFlags(MAP_HAS_ALPHA);
//         Bitmap *bm=TheManager->Create(&rmap_bi);
//         if(!bm) {ip->DisplayTempPrompt(GetString(IDS_CANT_SAVE_BITMAP));return;}
//         char *map=new char[lm_wd*lm_ht];
//         assert(map);
//         memset(map,0,lm_wd*lm_ht);
//         TimeValue time=ip->GetTime();
//         RenderMapSC sc(time,rmap_lighting,ip);
//         int nn=ip->GetSelNodeCount(),nr=0;
//         float du=1.f/lm_wd;
//         float dv=1.f/lm_ht;
//         int lastperc=0,perc;
//         ip->ProgressStart(_T("Rendering..."),TRUE,dummyprogressfn,NULL);
//         bm->Display();
//         for(int ni=0;ni<nn;++ni) {
//                 INode *node=ip->GetSelNode(ni);
//                 if(!node) continue;
//                 Mtl *mtl=node->GetMtl();
//                 if(!mtl) continue;
//                 mtl->Update(time,FOREVER);
//                 ip->ProgressUpdate(perc=ni*100/nn);
//                 sc.setnode(node);
//                 const ObjectState &os=node->EvalWorldState(time);
//                 if(!os.obj) continue;
//                 if(!os.obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID,0))) continue;
//                 TriObject *tri=(TriObject*)os.obj->ConvertToType(time,Class_ID(TRIOBJ_CLASS_ID,0));
//                 if(!tri) continue;
//                 Mesh &m=tri->mesh;
//                 if(m.mapSupport(lm_mapch)) {
//                         ++nr;
//                         sc.setmesh(m);
//                         Face *f=m.faces;
//                         TVFace *tf=m.mapFaces(lm_mapch);
//                         Point3 *tv=m.mapVerts(lm_mapch);
//                         for(int fi=0;fi<m.numFaces;++fi,++f,++tf) {
//                                 if((fi&31)==0) {
//                                         if(ip->GetCancel()) break;
//                                         ip->ProgressUpdate(perc=(ni*m.numFaces+fi)*100/(nn*m.numFaces));
//                                         if(perc-lastperc>=10) {
//                                                 bm->RefreshWindow();
//                                                 lastperc=perc;
//                                         }
//                                 }
//                                 sc.setface(fi,lm_mapch);
//                                 Point3 t0=tv[tf->t[0]]-Point3(du,dv,0.f)*.5f;
//                                 Point3 t1=tv[tf->t[1]]-Point3(du,dv,0.f)*.5f;
//                                 Point3 t2=tv[tf->t[2]]-Point3(du,dv,0.f)*.5f;
//                                 Box3 box;
//                                 box+=t0; box+=t1; box+=t2;
//                                 int u0=floorf(box.pmin.x*lm_wd);
//                                 int v0=floorf(box.pmin.y*lm_ht);
//                                 int u1=ceilf(box.pmax.x*lm_wd);
//                                 int v1=ceilf(box.pmax.y*lm_ht);
//                                 float A[3],B[3],C[3];
// #define  MAKELINE(p0,p1,A,B,C) { \
//        double a=-(p1.y-p0.y)*lm_ht; \
//        double b=(p1.x-p0.x)*lm_wd; \
//        double l=sqrt(a*a+b*b); \
//        if(l==0) A=B=0; else {A=a/l;B=b/l;} \
//        C=-(A*p0.x*lm_wd+B*p0.y*lm_ht); \
//}
//                                 MAKELINE(t0,t1,A[0],B[0],C[0]);
//                                 MAKELINE(t1,t2,A[1],B[1],C[1]);
//                                 MAKELINE(t2,t0,A[2],B[2],C[2]);
// #undef MAKELINE
//                                 if(A[0]*t2.x*lm_wd+B[0]*t2.y*lm_ht+C[0]<0)
//                                         for(int i=0;i<3;++i) {
//                                                 A[i]=-A[i];
//                                                 B[i]=-B[i];
//                                                 C[i]=-C[i];
//                                         }
//                                 int y=v0%lm_ht; if(y<0) y+=lm_ht;
//                                 int x0=u0%lm_wd; if(x0<0) x0+=lm_wd;
//                                 for(int v=v0;v<=v1;++v,y=(++y>=lm_ht?0:y)) {
//                                         float tv=v/float(lm_ht)+dv*.5f;
//                                         float de[3];
//                                         for(int i=0;i<3;++i) de[i]=B[i]*v+C[i];
//                                         for(int u=u0,x=x0;u<=u1;++u,x=(++x>=lm_wd?0:x)) {
//                                                 bool bord=false;
//                                                 for(i=0;i<3;++i) {
//                                                         float d=A[i]*u+de[i];
//                                                         if(d>=0) continue;
//                                                         if(d<=-1.5f) break;
//                                                         bord=true;
//                                                 }
//                                                 if(i<3) continue;
//                                                 if(map[y*lm_wd+x]==1) continue;
//                                                 float tu=u/float(lm_wd)+du*.5f;
//                                                 sc.ResetOutput();
//                                                 sc.setpt(tu,tv,du,dv);
//                                                 mtl->Shade(sc);
//                                                 sc.out.c.ClampMinMax();
//                                                 float a=1-(sc.out.t.r+sc.out.t.g+sc.out.t.b)/3;
//                                                 if(a<0) a=0; else if(a>1) a=1;
//                                                 BMM_Color_64 c;
//                                                 c.r=sc.out.c.r*0xFFFF;
//                                                 c.g=sc.out.c.g*0xFFFF;
//                                                 c.b=sc.out.c.b*0xFFFF;
//                                                 c.a=a*0xFFFF;
//                                                 verify(bm->PutPixels(x,lm_ht-1-y,1,&c));
//                                                 map[y*lm_wd+x]=(bord?2:1);
//                                         }
//                                 }
//                         }
//                 }
//                 if(tri!=os.obj) delete tri;
//                 if(ip->GetCancel()) break;
//                 bm->RefreshWindow(); lastperc=perc;
//         }
//         delete[] map;
//         bm->UnDisplay();
//         ip->ProgressUpdate(100,FALSE,_T("Saving bitmap..."));
//         if(nr && !ip->GetCancel()) {
//                 if(bm->OpenOutput(&rmap_bi)!=BMMRES_SUCCESS)
//                         {ip->DisplayTempPrompt(GetString(IDS_CANT_SAVE_BITMAP));return;}
//                 if(bm->Write(&rmap_bi)!=BMMRES_SUCCESS)
//                         {ip->DisplayTempPrompt(GetString(IDS_CANT_SAVE_BITMAP));return;}
//                 bm->Close(&rmap_bi);
//         }
//         bm->DeleteThis();
//         ip->ProgressEnd();
//         {
//         TSTR s; s.printf(GetString(IDS_OBJS_RENDERED),nr);
//         ip->DisplayTempPrompt(s);
//         }
// }

void extrude_spline(TimeValue time, INode &node, BezierShape &, Mesh &, float ht, float tile, float grav, int segs, int steps, bool up,
  bool usetang);

void DagUtil::make_grass()
{
  if (ip->GetSelNodeCount() != 1)
  {
    ip->DisplayTempPrompt(GetString(IDS_SEL_1_OBJ));
    return;
  }
  get_edfloat(egrassht, grass_ht);
  get_edfloat(egrasstile, grass_tile);
  get_edfloat(egrassg, grass_g);
  get_edint(egrasssegs, grass_segs);
  get_edint(egrasssteps, grass_steps);
  get_chkbool(IDC_GRASSUP, grass_up, splinesAndSmoothRoll);
  get_chkbool(IDC_GRASSUSETANG, grass_usetang, splinesAndSmoothRoll);
  TimeValue time = ip->GetTime();
  INode *node = ip->GetSelNode(0);
  Object *obj = node->EvalWorldState(time).obj;
  assert(obj);
  if (obj->SuperClassID() != SHAPE_CLASS_ID)
  {
    ip->DisplayTempPrompt(GetString(IDS_MUST_BE_SPLINE));
    return;
  }
  ShapeObject *shobj = (ShapeObject *)obj;
  if (!shobj->CanMakeBezier())
  {
    ip->DisplayTempPrompt(GetString(IDS_MUST_BE_SPLINE));
    return;
  }
  BezierShape shp;
  shobj->MakeBezier(time, shp);
  Mesh mesh;
  extrude_spline(time, *node, shp, mesh, grass_ht, grass_tile, grass_g, grass_segs, grass_steps, grass_up, grass_usetang);
  if (!mesh.numFaces)
    return;
  TriObject *tri = CreateNewTriObject();
  assert(tri);
  tri->mesh = mesh;
  Mtl *mtl = node->GetMtl();
  theHold.Begin();
  node = ip->CreateObjectNode(tri);
  assert(node);
  theHold.Accept(_T("Extrude spline"));
  TSTR name = _T("ExtrSpline");
  ip->MakeNameUnique(name);
  node->SetName(name);
  node->SetMtl(mtl);
  node->BackCull(FALSE);
  Matrix3 tm;
  tm.IdentityMatrix();
  node->SetNodeTM(time, tm);
  ip->RedrawViews(time);
}

void project_spline_on_scene(Interface *, INode &, BezierShape &shp, BezierShape &proj, float maxseglen, float optang,
  float optseglen);

void DagUtil::project_spline()
{
  if (ip->GetSelNodeCount() != 1)
  {
    ip->DisplayTempPrompt(GetString(IDS_SEL_1_OBJ));
    return;
  }
  get_edfloat(epjs_maxseglen, pjspl_maxseglen);
  get_edfloat(epjs_optseglen, pjspl_optseglen);
  get_edfloat(epjs_optang, pjspl_optang);
  TimeValue time = ip->GetTime();
  INode *node = ip->GetSelNode(0);
  Object *obj = node->EvalWorldState(time).obj;
  assert(obj);
  if (obj->SuperClassID() != SHAPE_CLASS_ID)
  {
    ip->DisplayTempPrompt(GetString(IDS_MUST_BE_SPLINE));
    return;
  }
  ShapeObject *shobj = (ShapeObject *)obj;
  if (!shobj->CanMakeBezier())
  {
    ip->DisplayTempPrompt(GetString(IDS_MUST_BE_SPLINE));
    return;
  }
  BezierShape shp;
  shobj->MakeBezier(time, shp);
  SplineShape *splshp = new SplineShape;
  assert(splshp);
  project_spline_on_scene(ip, *node, shp, splshp->shape, pjspl_maxseglen, DegToRad(pjspl_optang), pjspl_optseglen);
  if (splshp->shape.splineCount < 1)
  {
    delete splshp;
    return;
  }
  Mtl *mtl = node->GetMtl();
  theHold.Begin();
  node = ip->CreateObjectNode(splshp);
  assert(node);
  theHold.Accept(_T("Project spline"));
  TSTR name = _T("ProjSpline");
  ip->MakeNameUnique(name);
  node->SetName(name);
  node->SetMtl(mtl);
  Matrix3 tm;
  tm.IdentityMatrix();
  node->SetNodeTM(time, tm);
  ip->RedrawViews(time);
}
static inline Point3 get_pt(Point3 &v1, Point3 &v2, Point3 &nrm1, Point3 &nrm2, float pow, float wh)
{
  Point3 pr1 = (v2 - v1), pr2 = pr1; // Normalize
  pr1 = (pr1 - DotProd(nrm1, pr1) * nrm1);
  // pr1=nrm1;
  pr1 *= pow;
  pr2 = (pr2 - DotProd(nrm2, pr2) * nrm2);
  // pr2=nrm2;
  pr2 *= pow;
  float dx = v1.x, cx = pr1.x, ax = pr2.x - 2 * v2.x + 2 * dx + cx, bx = v2.x - ax - cx - dx;
  float dy = v1.y, cy = pr1.y, ay = pr2.y - 2 * v2.y + 2 * dy + cy, by = v2.y - ay - cy - dy;
  float dz = v1.z, cz = pr1.z, az = pr2.z - 2 * v2.z + 2 * dz + cz, bz = v2.z - az - cz - dz;
  return Point3(((ax * wh + bx) * wh + cx) * wh + dx, ((ay * wh + by) * wh + cy) * wh + dy, ((az * wh + bz) * wh + cz) * wh + dz);
}

static inline void neighbedge(Face &f, DWORD a, int &hh1, int &hh2)
{
  DWORD *v = f.getAllVerts();
  //      hh1=hh2=-1;
  if (a == v[0])
  {
    hh1 = 0;
    hh2 = 2;
  }
  else if (a == v[1])
  {
    hh1 = 0;
    hh2 = 1;
  }
  else
  {
    hh1 = 1;
    hh2 = 2;
  }
}

static int whichEdge(Face &f, DWORD a, DWORD b)
{
  DWORD *v = f.getAllVerts();
  if (a == v[0])
  {
    if (b == v[1])
      return 0;
    if (b == v[2])
      return 2;
  }
  else if (a == v[1])
  {
    if (b == v[0])
      return 0;
    if (b == v[2])
      return 1;
  }
  // a = v[2]
  if (b == v[0])
    return 2;
  if (b == v[1])
    return 1;
  return -1;
}

static inline int is_partof_selquad(Mesh &m, AdjEdgeList &ae, int vv1, int vv2, int fc1, int &fc2, int &v1, int &v2, int &v3, int &v4,
  int &he, int &he2, int &dved)
{
  for (he = 0; he < 3; ++he)
    if (!m.faces[fc1].getEdgeVis(he))
      break;
  if (he == 3)
    return 0;
  {
    int he1;
    for (he1 = he + 1; he1 < 3; ++he1)
      if (!m.faces[fc1].getEdgeVis(he1))
        break;
    if (he1 < 3)
    {
      mesh_face_sel(m).Clear(fc1);
      return 0;
    }
    he1 = whichEdge(m.faces[fc1], vv1, vv2);
    if (he1 == he)
    {
      mesh_face_sel(m).Clear(fc1);
      return 0;
    }
  }
  v1 = m.faces[fc1].v[(he == 0) ? 2 : (he - 1)];
  v2 = m.faces[fc1].v[he];
  v3 = m.faces[fc1].v[(he == 2) ? 0 : (he + 1)];
  dved = ae.FindEdge(v2, v3);
  fc2 = ae.edges[dved].f[0];
  if (fc2 == fc1)
    fc2 = ae.edges[dved].f[1];
  he2 = -1;
  if (fc2 == UNDEFINED)
  {
    mesh_face_sel(m).Clear(fc1);
    mesh_face_sel(m).Clear(fc2);
    return 0;
  }
  else
  {
    he2 = whichEdge(m.faces[fc2], v2, v3);
    if (he2 == -1)
    {
      mesh_face_sel(m).Clear(fc1);
      mesh_face_sel(m).Clear(fc2);
      return 0;
    }
    m.faces[fc2].setEdgeVisFlags(EDGE_VIS, EDGE_VIS, EDGE_VIS);
    m.faces[fc2].setEdgeVis(he2, EDGE_INVIS);
    mesh_face_sel(m).Set(fc2);
  }
  for (v4 = 0; v4 < 3; ++v4)
    if (m.faces[fc2].v[v4] != v2 && m.faces[fc2].v[v4] != v3)
    {
      v4 = m.faces[fc2].v[v4];
      break;
    }
  return 1;
}

static Tab<int> *recurse_fcsquads;
static Tab<int> *recurse_fcs;
static Tab<int> *recurse_dvEdges;
static Mesh *recurse_m;
static AdjEdgeList *recurse_ae;

static void _recurse(int eds[4], int vedb1, int vedb2)
{
  for (int k = 0; k < 4; ++k)
  {
    int ed1[2], ed2[2];
    {
      /*int vv1=m.faces[eds[k]/3].v[eds[k]%3];
      int vv2=m.faces[eds[k]/3].v[(eds[k]+)%3];
      int me=ae.FindEdge(vv1,vv2);*/
      int meee = eds[k];
      int vv1 = recurse_ae->edges[meee].v[0];
      int vv2 = recurse_ae->edges[meee].v[1];
      int fc1;
      {
        int f1 = recurse_ae->edges[meee].f[0];
        int f2 = recurse_ae->edges[meee].f[1];
        if (f2 == UNDEFINED || f1 == UNDEFINED)
          continue;
        if (!mesh_face_sel(*recurse_m)[f1] || !mesh_face_sel(*recurse_m)[f2])
          continue;
        // debug("fc %d %d",f1,f2);
        if ((*recurse_fcs)[f1] && (*recurse_fcs)[f2])
          continue;
        if ((*recurse_fcs)[f1])
          fc1 = f2;
        else
          fc1 = f1;
      }

      if ((recurse_m->faces[fc1].flags & VIS_MASK) == EDGE_ALL)
      {
        mesh_face_sel(*recurse_m).Clear(fc1);
        continue;
      }
      int he, he2, me;
      int fc2;
      int v1, v2, v3, v4;
      if (!is_partof_selquad(*recurse_m, *recurse_ae, vv1, vv2, fc1, fc2, v1, v2, v3, v4, he, he2, me))
        continue;
      int s = -1;
      {
        ed1[0] = (he == 2) ? 1 : ((he == 1) ? 0 : 2);
        int hh1, hh2;
        neighbedge(recurse_m->faces[fc2], v2, hh1, hh2);
        ed2[0] = (hh1 == he2) ? hh2 : hh1;
        {
          int k;
          for (k = 0; k < 3; ++k)
            if (k != he && k != ed1[0])
              break;
          ed1[1] = k;
        }
        {
          int k;
          for (k = 0; k < 3; ++k)
            if (k != he2 && k != ed2[0])
              break;
          ed2[1] = k;
        }
        // ed1[0]+=f1*3;ed1[1]+=f1*3;
        // ed2[0]+=f2*3;ed2[1]+=f2*3;
        int e;
        e = ed1[1];
        ed1[1] = ed2[1];
        ed2[1] = e;
        ed1[0] = recurse_ae->FindEdge(recurse_m->faces[fc1].v[ed1[0] % 3], recurse_m->faces[fc1].v[(ed1[0] + 1) % 3]);
        ed1[1] = recurse_ae->FindEdge(recurse_m->faces[fc2].v[ed1[1] % 3], recurse_m->faces[fc2].v[(ed1[1] + 1) % 3]);
        ed2[0] = recurse_ae->FindEdge(recurse_m->faces[fc2].v[ed2[0] % 3], recurse_m->faces[fc2].v[(ed2[0] + 1) % 3]);
        ed2[1] = recurse_ae->FindEdge(recurse_m->faces[fc1].v[ed2[1] % 3], recurse_m->faces[fc1].v[(ed2[1] + 1) % 3]);
        if ((*recurse_dvEdges)[ed1[0]] == 1)
          s = 0;
        else if ((*recurse_dvEdges)[ed1[0]] == -1)
          s = 1;
        else if ((*recurse_dvEdges)[ed2[1]] == 1)
          s = 1;
        else if ((*recurse_dvEdges)[ed2[1]] == -1)
          s = 0;
        else if ((*recurse_dvEdges)[ed1[1]] == 1)
          s = 0;
        else if ((*recurse_dvEdges)[ed1[1]] == -1)
          s = 1;
        else if ((*recurse_dvEdges)[ed2[0]] == 1)
          s = 1;
        else if ((*recurse_dvEdges)[ed2[0]] == -1)
          s = 0;
        else
          continue;
      }
      if (!s)
      {
        (*recurse_dvEdges)[ed1[0]] = 1;
        (*recurse_dvEdges)[ed1[1]] = 1;
        (*recurse_dvEdges)[ed2[0]] = -1;
        (*recurse_dvEdges)[ed2[1]] = -1;
      }
      else
      {
        (*recurse_dvEdges)[ed2[0]] = 1;
        (*recurse_dvEdges)[ed2[1]] = 1;
        (*recurse_dvEdges)[ed1[0]] = -1;
        (*recurse_dvEdges)[ed1[1]] = -1;
      }
      (*recurse_fcsquads)[fc1] = fc2;
      (*recurse_fcsquads)[fc2] = fc1;
      (*recurse_fcs)[fc1] = 1;
      (*recurse_fcs)[fc2] = 1;
    }
    {
      int eds[4];
      eds[0] = ed1[0];
      eds[1] = ed1[1];
      eds[2] = ed2[0];
      eds[3] = ed2[1];
      // debug("rec");
      _recurse(eds, vedb1, vedb2);
    }
  }
}

static void recurse(Tab<int> &fcsquads, Tab<int> &fcs, Tab<int> &dvEdges, Mesh &m, int eds[4], AdjEdgeList &ae, int vedb1, int vedb2,
  char trne)
{
  recurse_fcsquads = &fcsquads;
  recurse_fcs = &fcs;
  recurse_dvEdges = &dvEdges;
  recurse_m = &m;
  recurse_ae = &ae;
  _recurse(eds, vedb1, vedb2);
}

#define MESHSMOOTHMAPS_SUPPORT 8

struct DivideE
{
  int ed;
  int order[2];
  Tab<int> vrt; // in some array
  Tab<int[2]> tvrt[MESHSMOOTHMAPS_SUPPORT];
};
typedef Tab<int> Tabint;


int calc_sm_normals(Tab<Point3> &norms, Mesh &m, char use_sm, float pow)
{
  Tab<Point3> norm;
  norm.Resize(m.numFaces);
  norm.SetCount(m.numFaces);
  int i;
  for (i = 0; i < norm.Count(); ++i)
  {
    Point3 v1 = m.verts[m.faces[i].v[2]] - m.verts[m.faces[i].v[0]];
    Point3 v2 = m.verts[m.faces[i].v[1]] - m.verts[m.faces[i].v[0]];
    norm[i] = Normalize(CrossProd(v1, v2));
  }
  Tab<Tabint *> vrfs;
  vrfs.Resize(m.numVerts);
  vrfs.SetCount(m.numVerts);
  for (i = 0; i < m.numVerts; ++i)
    vrfs[i] = new Tabint;
  for (i = 0; i < m.numFaces; ++i)
  {
    vrfs[m.faces[i].v[0]]->Append(1, &i);
    vrfs[m.faces[i].v[1]]->Append(1, &i);
    vrfs[m.faces[i].v[2]]->Append(1, &i);
  }
  Tab<Point3[3]> nrms;
  nrms.Resize(m.numFaces);
  nrms.SetCount(m.numFaces);
  for (i = 0; i < m.numFaces; ++i)
  {
    Point3 ng[3];
    for (int vi = 0; vi < 3; ++vi)
    {
      Point3 n(0, 0, 0);
      int v = m.faces[i].v[vi];
      if (use_sm)
        if (m.faces[i].smGroup == 0)
          n += norm[i];
      for (int j = 0; j < vrfs[v]->Count(); ++j)
        if (!use_sm || (m.faces[(*vrfs[v])[j]].smGroup & m.faces[i].smGroup))
        {
          n += norm[(*vrfs[v])[j]];
        }
      ng[vi] = Normalize(n);
      // debug("nrm %g %g %g",ng[vi].x,ng[vi].y,ng[vi].z);
    }
    nrms[i][0] = ng[0];
    nrms[i][1] = ng[1];
    nrms[i][2] = ng[2];
  }
  norms.Resize(m.numVerts);
  norms.SetCount(m.numVerts);
  for (i = 0; i < norms.Count(); ++i)
  {
    Point3 n(0, 0, 0);
    // debug("fc %d",vrfs[i]->Count());
    for (int j = 0; j < vrfs[i]->Count(); ++j)
    {
      // if (!use_sm)if(!mesh_face_sel(m)[(*vrfs[i])[j]]) continue;
      int fc = (*vrfs[i])[j];
      int k;
      for (k = 0; k < 3; ++k)
        if (m.faces[fc].v[k] == i)
          break;
      if (k == 3)
        continue;
      if (!use_sm)
      {
        n = nrms[fc][k];
        break;
      }
      n += nrms[fc][k];
    }
    norms[i] = Normalize(n) * pow;
  }
  for (i = 0; i < vrfs.Count(); ++i)
    delete (vrfs[i]);
  vrfs.SetCount(0);
  return 1;
}

static inline void gettvrt(Mesh &m, float at, int v0, int v1, AdjEdgeList &ae, int &ed, Point3 &tvrt1, Point3 &tvrt2, int mi,
  int ord[2])
{
  int f[2];
  f[0] = ae.edges[ed].f[0];
  f[1] = ae.edges[ed].f[1];
  for (int i = 0; i < 2; ++i)
  {
    if (f[i] == UNDEFINED)
      continue;
    int wh = whichEdge(m.faces[f[i]], v0, v1);
    int t1 = m.mapFaces(mi)[f[i]].t[wh % 3], t2 = m.mapFaces(mi)[f[i]].t[(wh + 1) % 3];
    if (t1 < 0 || t2 < 0)
      continue;
    Point3 tvrt;
    if (!ord[i])
      tvrt = (m.mapVerts(mi)[t2] - m.mapVerts(mi)[t1]) * at + m.mapVerts(mi)[t1];
    else
      tvrt = (m.mapVerts(mi)[t1] - m.mapVerts(mi)[t2]) * at + m.mapVerts(mi)[t2];
    if (i == 0)
      tvrt1 = tvrt;
    else
      tvrt2 = tvrt;
  }
}

static int int_lcmp(void const *a, void const *b) { return *((int *)(b)) - *((int *)(a)); }

struct TQuad
{
  // int v[4];
  // int cred4;//0: 1-3 1-4 2-3 2-4
  // int nrm1;//0 1
  Face f[2];
  TVFace tf[MESHSMOOTHMAPS_SUPPORT][2];
  int v2v1[3], v2v2[3];
  // int t2t1[3],t2t2[3];
  TQuad(){};
  TQuad(int vr[4], int fc1, int fc2, Mesh &m)
  {
    //,AdjEdgeList &ae,int ed1,int ed2//int tr[4],
    /*for(int k=0;k<3;++k)
            if(!m.faces[fc1].getEdgeVis(k)) break;
    int vv[2];vv[0]=m.faces[fc1].v[k];vv[1]=m.faces[fc1].v[(k+1)%3];
    if(vv[0]==v[0]) {
            if(vv[1]==v[2]) cred4=0;
            else cred4=1;
    } else {
            if(vv[1]==v[2]) cred4=2;
            else cred4=3;
    }*/
    f[0] = m.faces[fc1];
    f[1] = m.faces[fc2];
    {
      for (int mi = 0; mi < m.getNumMaps(); ++mi)
        if (m.mapSupport(mi))
        {
          tf[mi][0] = m.mapFaces(mi)[fc1];
          tf[mi][1] = m.mapFaces(mi)[fc2];
        }
    }
    int k;
    for (k = 0; k < 3; ++k)
    {
      if (f[0].v[k] == vr[0])
        v2v1[k] = 0;
      else if (f[0].v[k] == vr[1])
        v2v1[k] = 1;
      else if (f[0].v[k] == vr[2])
        v2v1[k] = 2;
      else
        v2v1[k] = 3;
    }
    for (k = 0; k < 3; ++k)
    {
      if (f[1].v[k] == vr[0])
        v2v2[k] = 0;
      else if (f[1].v[k] == vr[1])
        v2v2[k] = 1;
      else if (f[1].v[k] == vr[2])
        v2v2[k] = 2;
      else
        v2v2[k] = 3;
    }
    /*              for(k=0;k<3;++k){
                            if(tf[0].t[k]==tr[0]) t2t1[k]=0;
                            else if(tf[0].t[k]==tr[1]) t2t1[k]=1;
                            else if(tf[0].t[k]==tr[2]) t2t1[k]=2;
                            else t2t1[k]=3;
                    }
                    for(k=0;k<3;++k){
                            if(tf[1].t[k]==tr[0]) t2t2[k]=0;
                            else if(tf[1].t[k]==tr[1]) t2t2[k]=1;
                            else if(tf[1].t[k]==tr[2]) t2t2[k]=2;
                            else t2t2[k]=3;
                    }*/
  }
  void GetAnalogueQuad(int fv[4], int ft[MESHSMOOTHMAPS_SUPPORT][4], Face &fc1, Face &fc2, TVFace tfc1[MESHSMOOTHMAPS_SUPPORT][2])
  {
    fc1 = f[0];
    fc2 = f[1];
    for (int mi = 0; mi < MESHSMOOTHMAPS_SUPPORT; ++mi)
    {
      tfc1[mi][0] = tf[mi][0];
      tfc1[mi][1] = tf[mi][1];
      {
        for (int k = 0; k < 3; ++k)
        {
          tfc1[mi][0].t[k] = ft[mi][v2v1[k]];
        }
      }
      {
        for (int k = 0; k < 3; ++k)
        {
          tfc1[mi][1].t[k] = ft[mi][v2v2[k]];
        }
      }
    }
    {
      for (int k = 0; k < 3; ++k)
      {
        fc1.v[k] = fv[v2v1[k]];
      }
    }
    {
      for (int k = 0; k < 3; ++k)
      {
        fc2.v[k] = fv[v2v2[k]];
      }
    }
  }
};


void DagUtil::smooth_mesh(INode *node, Mesh &m)
{
  Mesh &mesh = m;
  if (!m.getNumTVerts())
    return;

  if (m.getNumMaps() > MESHSMOOTHMAPS_SUPPORT)
    return;
  BitArray presel = mesh_face_sel(m);
  if (!smooth_selected)
    mesh_face_sel(m).SetAll();
  // debug("!!! %d",tmselectf);
  AdjEdgeList ae(mesh);
  Tab<int> divEdges;
  divEdges.Resize(ae.edges.Count());
  divEdges.SetCount(ae.edges.Count());
  memset(&divEdges[0], 0, ae.edges.Count() * 4);
  Tab<int> fcsPassed;
  fcsPassed.Resize(m.getNumFaces());
  fcsPassed.SetCount(m.getNumFaces());
  memset(&fcsPassed[0], 0, m.getNumFaces() * 4);
  Tab<int> fcsquads;
  fcsquads.Resize(m.getNumFaces());
  fcsquads.SetCount(m.getNumFaces());
  memset(&fcsquads[0], 0xFF, m.getNumFaces() * 4);
  {
    for (int f1 = 0; f1 < m.getNumFaces(); ++f1)
    {
      if (!mesh_face_sel(m)[f1])
        continue;
      if ((mesh.faces[f1].flags & VIS_MASK) == EDGE_ALL)
      {
        mesh_face_sel(m).Clear(f1);
        continue;
      }
      int he, he1;
      for (he = 0; he < 3; ++he)
        if (!mesh.faces[f1].getEdgeVis(he))
          break;
      for (he1 = he + 1; he1 < 3; ++he1)
        if (!mesh.faces[f1].getEdgeVis(he1))
          break;
      if (he1 < 3)
      {
        mesh_face_sel(m).Clear(f1);
        continue;
      }
      int v1, v2, v3, v4;
      v1 = mesh.faces[f1].v[(he == 0) ? 2 : (he - 1)];
      v2 = mesh.faces[f1].v[he];
      v3 = mesh.faces[f1].v[(he == 2) ? 0 : (he + 1)];
      int me = ae.FindEdge(v2, v3);
      int f2 = ae.edges[me].f[0];
      if (f2 == f1)
        f2 = ae.edges[me].f[1];
      int he2 = -1;
      if (f2 == UNDEFINED)
      {
        mesh_face_sel(m).Clear(f1);
        mesh_face_sel(m).Clear(f2);
        continue;
      }
      else
      {
        he2 = whichEdge(mesh.faces[f2], v2, v3);
        if (he2 == -1)
        {
          mesh_face_sel(m).Clear(f1);
          mesh_face_sel(m).Clear(f2);
          continue;
        }
        m.faces[f2].setEdgeVisFlags(EDGE_VIS, EDGE_VIS, EDGE_VIS);
        m.faces[f2].setEdgeVis(he2, EDGE_INVIS);
        mesh_face_sel(m).Set(f2);
      }
      for (v4 = 0; v4 < 3; ++v4)
        if (mesh.faces[f2].v[v4] != v2 && mesh.faces[f2].v[v4] != v3)
        {
          v4 = mesh.faces[f2].v[v4];
          break;
        }
      int ed1[2], ed2[2], s = 0;
      {
        ed1[0] = (he == 2) ? 1 : ((he == 1) ? 0 : 2);
        int hh1, hh2;
        neighbedge(mesh.faces[f2], v2, hh1, hh2);
        ed2[0] = (hh1 == he2) ? hh2 : hh1;
        {
          int k;
          for (k = 0; k < 3; ++k)
            if (k != he && k != ed1[0])
              break;
          ed1[1] = k;
        }
        {
          int k;
          for (k = 0; k < 3; ++k)
            if (k != he2 && k != ed2[0])
              break;
          ed2[1] = k;
        }
        {
          int tt1 = m.tvFace[f1].t[(ed1[0] + 1) % 3], tt2 = m.tvFace[f1].t[ed1[0]];
          int tt3 = m.tvFace[f1].t[(ed1[1] + 1) % 3], tt4 = m.tvFace[f1].t[ed1[1]];
          if (tt1 >= 0 && tt2 >= 0 && tt3 >= 0 && tt4 >= 0)
          {
            Point3 t1 = m.tVerts[tt1] - m.tVerts[tt2];
            Point3 t2 = m.tVerts[tt3] - m.tVerts[tt4];
            if (fabs(t1.x) > fabs(t2.x))
              s = 0;
            else
              s = 1;
          }
          else
            s = mesh_smoothtu;
        }
        // ed1[0]+=f1*3;ed1[1]+=f1*3;
        // ed2[0]+=f2*3;ed2[1]+=f2*3;
        int e;
        e = ed1[1];
        ed1[1] = ed2[1];
        ed2[1] = e;
        ed1[0] = ae.FindEdge(m.faces[f1].v[ed1[0] % 3], m.faces[f1].v[(ed1[0] + 1) % 3]);
        ed1[1] = ae.FindEdge(m.faces[f2].v[ed1[1] % 3], m.faces[f2].v[(ed1[1] + 1) % 3]);
        ed2[0] = ae.FindEdge(m.faces[f2].v[ed2[0] % 3], m.faces[f2].v[(ed2[0] + 1) % 3]);
        ed2[1] = ae.FindEdge(m.faces[f1].v[ed2[1] % 3], m.faces[f1].v[(ed2[1] + 1) % 3]);
      }
      if (mesh_smoothtu)
        s = !s;
      // debug("s %d %d",s,mesh_smoothtu);
      if (!s)
      {
        divEdges[ed1[0]] = 1;
        divEdges[ed1[1]] = 1;
        divEdges[ed2[0]] = -1;
        divEdges[ed2[1]] = -1;
      }
      else
      {
        divEdges[ed2[0]] = 1;
        divEdges[ed2[1]] = 1;
        divEdges[ed1[0]] = -1;
        divEdges[ed1[1]] = -1;
      }
      fcsPassed[f1] = 1;
      fcsPassed[f2] = 1;
      int eds[4];
      eds[0] = ed1[0];
      eds[1] = ed1[1];
      eds[2] = ed2[0];
      eds[3] = ed2[1];
      fcsquads[f1] = f2;
      fcsquads[f2] = f1;
      recurse(fcsquads, fcsPassed, divEdges, m, eds, ae, v2, v3, turn_edge);
      break;
    }
  }
  Tab<DivideE *> ded;
  Tab<Point3> normals;
  Tab<Point3> addverts, addtverts[MESHSMOOTHMAPS_SUPPORT];
  addverts.Append(m.numVerts, m.verts);
  {
    for (int mi = 0; mi < m.getNumMaps(); ++mi)
      if (m.mapSupport(mi))
        addtverts[mi].Append(m.getNumMapVerts(mi), m.mapVerts(mi));
  }
  // debug("calc smooth %g",sharpness);
  calc_sm_normals(normals, mesh, use_smooth_grps, 1);
  int i;
  for (i = 0; i < divEdges.Count(); ++i)
  {
    if (divEdges[i] != 1)
      continue;
    // debug("%d",i);
    DivideE *de = new DivideE;
    int rc = ded.Append(1, &de);
    ded[rc]->ed = i;
    int v0 = ae.edges[i].v[0], v1 = ae.edges[i].v[1];
    for (int uu = 0; uu < 2; ++uu)
    {
      if (ae.edges[i].f[uu] == UNDEFINED)
        ded[rc]->order[uu] = 0;
      else
      {
        int whe = whichEdge(m.faces[ae.edges[i].f[uu]], v0, v1);
        if (m.faces[ae.edges[i].f[uu]].v[whe] != v0)
          ded[rc]->order[uu] = 1;
        else
          ded[rc]->order[uu] = 0;
      }
    }
    // debug("%g",Length(normals[v0]));
    for (int ki = 0; ki < iterations; ++ki)
    {
      float at = float(ki + 1) / float(iterations + 1);
      Point3 vrt = get_pt(m.verts[v0], m.verts[v1], normals[v0], normals[v1], sharpness, at);
      int vv = addverts.Append(1, &vrt);
      ded[rc]->vrt.Append(1, &vv);
      {
        for (int mi = 0; mi < m.getNumMaps(); ++mi)
          if (m.mapSupport(mi))
          {
            Point3 tvrt[2];
            gettvrt(mesh, at, v0, v1, ae, i, tvrt[0], tvrt[1], mi, ded[rc]->order);
            int tt[2];
            tt[0] = addtverts[mi].Append(2, tvrt);
            tt[1] = tt[0] + 1;
            ded[rc]->tvrt[mi].Append(1, &tt);
          }
      }
    }
  }
  Tab<Face> newfcs;
  newfcs.Resize(m.numFaces);
  newfcs.SetCount(m.numFaces);
  memcpy(&newfcs[0], m.faces, m.numFaces * sizeof(Face));
  Tab<TVFace> newtvfcs[MESHSMOOTHMAPS_SUPPORT];
  {
    for (int mi = 0; mi < m.getNumMaps(); ++mi)
      if (m.mapSupport(mi))
      {
        newtvfcs[mi].Resize(m.numFaces);
        newtvfcs[mi].SetCount(m.numFaces);
        memcpy(&newtvfcs[mi][0], m.mapFaces(mi), m.numFaces * sizeof(TVFace));
      }
  }

  Tab<int> todel;
  Tab<int> fcsedg[2];
  fcsedg[0].Resize(m.getNumFaces());
  fcsedg[0].SetCount(m.getNumFaces());
  memset(&fcsedg[0][0], 0xff, m.getNumFaces() * 4);
  fcsedg[1].Resize(m.getNumFaces());
  fcsedg[1].SetCount(m.getNumFaces());
  memset(&fcsedg[1][0], 0xff, m.getNumFaces() * 4);
  for (i = 0; i < ded.Count(); ++i)
  {
    MEdge &me = ae.edges[ded[i]->ed];
    int fcc[2];
    fcc[0] = me.f[0];
    fcc[1] = me.f[1];
    for (int kf = 0; kf < 2; ++kf)
    {
      int fc1 = fcc[kf];
      if (fc1 == UNDEFINED)
        continue;
      if (fcsedg[0][fc1] == -1)
        fcsedg[0][fc1] = i;
      else
        fcsedg[1][fc1] = i;
    }
  }

  Tab<int> fcsdivd;
  fcsdivd.Resize(m.getNumFaces());
  fcsdivd.SetCount(m.getNumFaces());
  memset(&fcsdivd[0], 0, m.getNumFaces() * 4);
  mesh_face_sel(m) = presel;
  BitArray newfacesel;
  newfacesel = presel;

  for (i = 0; i < ded.Count(); ++i)
  {
    MEdge &me = ae.edges[ded[i]->ed];
    int fcc[2];
    fcc[0] = me.f[0];
    fcc[1] = me.f[1];
    for (int kf = 0; kf < 2; ++kf)
    {
      int fc1 = fcc[kf];
      if (fc1 == UNDEFINED)
        continue;
      if (fcsdivd[fc1])
        continue;
      if (fcsquads[fc1] != -1)
      {
        // debug("1212");
        int fc2 = fcsquads[fc1];
        // debug("fc2 %d",fc2);
        fcsdivd[fc1] = 1;
        fcsdivd[fc2] = 1;
        int ii2 = fcsedg[0][fc2];
        if (ii2 == i || ii2 < 0)
          ii2 = fcsedg[1][fc2];
        // debug("ed2 %d",ii2);
        if (ii2 < 0)
          continue;
        int fr2p, whe2, whe1;
        // int vsm1[3],vsm2[3];
        {
          MEdge &me = ae.edges[ded[ii2]->ed];
          fr2p = 0;
          if (me.f[fr2p] != fc2)
          {
            fr2p = 1;
            if (me.f[fr2p] != fc2)
              continue;
          }
          whe2 = whichEdge(m.faces[fc2], me.v[0], me.v[1]);
        }
        // debug("edg2 %d %d",whe2,fr2p);
        whe1 = whichEdge(m.faces[fc1], me.v[0], me.v[1]);
        // debug("edg1 %d",whe1);
        int vr[4];
        vr[0] = whe1;
        vr[1] = (vr[0] + 1) % 3;
        vr[2] = whe2;
        vr[3] = (vr[2] + 1) % 3;
        int curv[4], curt[MESHSMOOTHMAPS_SUPPORT][4], tr[MESHSMOOTHMAPS_SUPPORT][4];
        if (ded[i]->order[kf])
        {
          int cc = vr[0];
          vr[0] = vr[1];
          vr[1] = cc;
        }
        if (ded[ii2]->order[fr2p])
        {
          int cc = vr[2];
          vr[2] = vr[3];
          vr[3] = cc;
        }
        int bck = 0;
        if (ded[ii2]->order[fr2p] == ded[i]->order[kf])
        {
          bck = 1;
          int cc = vr[2];
          vr[2] = vr[3];
          vr[3] = cc;
        }
        // debug("order %d %d",ded[ii2]->order[fr2p],ded[i]->order[kf]);
        {
          for (int mi = 0; mi < m.getNumMaps(); ++mi)
            if (m.mapSupport(mi))
            {
              tr[mi][0] = m.mapFaces(mi)[fc1].t[vr[0]];
              tr[mi][1] = m.mapFaces(mi)[fc1].t[vr[1]];
              tr[mi][2] = m.mapFaces(mi)[fc2].t[vr[2]];
              tr[mi][3] = m.mapFaces(mi)[fc2].t[vr[3]];
            }
        }

        vr[0] = m.faces[fc1].v[vr[0]];
        vr[1] = m.faces[fc1].v[vr[1]];
        vr[2] = m.faces[fc2].v[vr[2]];
        vr[3] = m.faces[fc2].v[vr[3]];
        {
          for (int p = 0; p < 4; ++p)
          {
            {
              for (int mi = 0; mi < m.getNumMaps(); ++mi)
                if (m.mapSupport(mi))
                  curt[mi][p] = tr[mi][p];
            }
            curv[p] = vr[p];
          }
        }
        // debug("11");
        TQuad qd(vr, fc1, fc2, m);
        // debug("vr2 %d vr1 %d",ded[ii2]->vrt.Count(),ded[i]->vrt.Count());
        for (int r = 0; r <= ded[i]->vrt.Count(); ++r)
        {
          if (r < ded[i]->vrt.Count())
          {
            int r1 = r, r2 = r;
            if (bck)
            {
              r2 = ded[i]->vrt.Count() - 1 - r;
            }
            curv[1] = ded[i]->vrt[r1];
            curv[3] = ded[ii2]->vrt[r2];
            // int tr=r;
            // if(ded[i]->order[kf]) tr=ded[i]->tvrt.Count()-r-1;
            {
              for (int mi = 0; mi < m.getNumMaps(); ++mi)
                if (m.mapSupport(mi))
                {
                  curt[mi][1] = ded[i]->tvrt[mi][r1][kf];
                  // if(ded[ii2]->order[fr2p]) tr=ded[ii2]->tvrt.Count()-r-1;
                  curt[mi][3] = ded[ii2]->tvrt[mi][r2][fr2p];
                }
            }
          }
          else
          {
            curv[1] = vr[1];
            curv[3] = vr[3];
            {
              for (int mi = 0; mi < m.getNumMaps(); ++mi)
                if (m.mapSupport(mi))
                {
                  curt[mi][1] = tr[mi][1];
                  curt[mi][3] = tr[mi][3];
                }
            }
            // curt[1]=0;curt[3]=0;
          }
          Face fc[2];
          TVFace tfc[MESHSMOOTHMAPS_SUPPORT][2];
          qd.GetAnalogueQuad(curv, curt, fc[0], fc[1], tfc);
          // debug("frst tv1(%g %g %g) tv2(%g %g %g)",P3D(addtverts[curt[0]]),P3D(addtverts[curt[2]]));
          curv[0] = curv[1];
          curv[2] = curv[3];
          {
            for (int mi = 0; mi < m.getNumMaps(); ++mi)
              if (m.mapSupport(mi))
              {
                curt[mi][0] = curt[mi][1];
                curt[mi][2] = curt[mi][3];
              }
          }
          // debug("sec tv1(%g %g %g) tv2(%g %g %g)",P3D(addtverts[curt[0]]),P3D(addtverts[curt[2]]));
          if (r > 0)
          {
            int fad = newfcs.Append(2, fc);
            {
              for (int mi = 0; mi < m.getNumMaps(); ++mi)
                if (m.mapSupport(mi))
                {
                  newtvfcs[mi].Append(2, tfc[mi]);
                }
            }
            newfacesel.SetSize(newfcs.Count(), 1);
            newfacesel.Set(fad, presel[fc1]);
            newfacesel.Set(fad + 1, presel[fc1]);
          }
          else
          {
            // qd.GetAnalogueQuad(vr,tr,fc[0],fc[1],tfc[0],tfc[1]);
            newfcs[fc1] = fc[0];
            newfcs[fc2] = fc[1];
            {
              for (int mi = 0; mi < m.getNumMaps(); ++mi)
                if (m.mapSupport(mi))
                {
                  newtvfcs[mi][fc1] = tfc[mi][0];
                  newtvfcs[mi][fc2] = tfc[mi][1];
                }
            }
          }
        }
      }
      else
      {
        int curv = 0, curt[MESHSMOOTHMAPS_SUPPORT], frstv, secv;
        frstv = whichEdge(m.faces[fc1], me.v[0], me.v[1]);
        if (frstv < 0)
          continue;
        secv = (frstv + 1) % 3;
        Face fff = m.faces[fc1];
        TVFace ttt[MESHSMOOTHMAPS_SUPPORT];
        {
          for (int mi = 0; mi < m.getNumMaps(); ++mi)
            if (m.mapSupport(mi))
            {
              ttt[mi] = m.mapFaces(mi)[fc1];
            }
        }
        // if(LengthSquared(addvrts[ded[i]->vrt[0]]-addvrts[fff.v[frstv]])>LengthSquared(addvrts[ded[i]->vrt[0]]-addvrts[fff.v[secv]])){
        if (ded[i]->order[kf])
        {
          int cc = frstv;
          frstv = secv;
          secv = cc;
        }
        for (int l = 0; l <= ded[i]->vrt.Count(); ++l)
        {
          TVFace tface[MESHSMOOTHMAPS_SUPPORT];
          {
            for (int mi = 0; mi < MESHSMOOTHMAPS_SUPPORT; ++mi)
              tface[mi] = ttt[mi];
          }
          Face face = fff;
          if (l > 0)
            face.v[frstv] = curv;
          if (l < ded[i]->vrt.Count())
          {
            face.v[secv] = ded[i]->vrt[l];
            curv = ded[i]->vrt[l];
          }
          if (l > 0)
          {
            {
              for (int mi = 0; mi < m.getNumMaps(); ++mi)
                if (m.mapSupport(mi))
                {
                  tface[mi].t[frstv] = curt[mi];
                }
            }
          }
          if (l < ded[i]->vrt.Count())
          {
            {
              for (int mi = 0; mi < m.getNumMaps(); ++mi)
                if (m.mapSupport(mi))
                {
                  tface[mi].t[secv] = ded[i]->tvrt[mi][l][kf];
                  curt[mi] = ded[i]->tvrt[mi][l][kf];
                }
            }
          }
          if (l > 0)
          {
            int fad = newfcs.Append(1, &face);
            {
              for (int mi = 0; mi < m.getNumMaps(); ++mi)
                if (m.mapSupport(mi))
                {
                  newtvfcs[mi].Append(1, &tface[mi]);
                }
            }
            newfacesel.SetSize(newfcs.Count(), 1);
            newfacesel.Set(fad, presel[fc1]);
          }
          else
          {
            newfcs[fc1] = face;
            {
              for (int mi = 0; mi < m.getNumMaps(); ++mi)
                if (m.mapSupport(mi))
                {
                  newtvfcs[mi][fc1] = tface[mi];
                }
            }
          }
          // todel.Append(1,&fc1);
        }
        fcsdivd[fc1] = 1;
      }
    }
    delete ded[i];
  }
  ded.SetCount(0);
  /*todel.Sort(int_lcmp);
  {for(int p=0;p<todel.Count();++p) {newfcs.Delete(todel[p],1);newtvfcs.Delete(todel[p],1);}}*/
  // if(newtvfcs.Count()!=newfcs.Count())
  // theHold.Begin();
  // theHold.Put(new MeshSmoothRestore());
  m.setNumFaces(newfcs.Count());
  memcpy(m.faces, &newfcs[0], newfcs.Count() * sizeof(Face));
  {
    for (int mi = 0; mi < m.getNumMaps(); ++mi)
      if (m.mapSupport(mi))
      {
        memcpy(m.mapFaces(mi), &newtvfcs[mi][0], newtvfcs[mi].Count() * sizeof(TVFace));
      }
  }
  mesh_face_sel(m) = newfacesel;
  m.setNumVerts(addverts.Count());
  {
    for (int i = 0; i < addverts.Count(); ++i)
      m.setVert(i, addverts[i]);
  }
  {
    for (int mi = 0; mi < m.getNumMaps(); ++mi)
      if (m.mapSupport(mi))
      {
        m.setNumMapVerts(mi, addtverts[mi].Count());
        {
          for (int i = 0; i < addtverts[mi].Count(); ++i)
            m.setMapVert(mi, i, addtverts[mi][i]);
        }
      }
  }
  // theHold.Accept(GetString(IDS_MESHSMOOTH));
}

void DagUtil::mesh_smooth()
{
  update_vars();
  TimeValue time = ip->GetTime();
  int nc = ip->GetSelNodeCount();
  for (int nn = 0; nn < nc; ++nn)
  {
    INode *node = ip->GetSelNode(nn);
    if (!node)
      continue;
    // debug("!!!!cd");
    // if(!node->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID,0))) continue;
    Object *meshobj = node->EvalWorldState(time).obj;
    TriObject *to = (TriObject *)meshobj->ConvertToType(time, Class_ID(TRIOBJ_CLASS_ID, 0));
    if (!to)
      continue;
    if (to != meshobj)
    {
      to->DeleteMe();
      continue;
    }
    Mesh &mesh = to->mesh;
    // debug("!!!12!cd");
    smooth_mesh(node, mesh);
    mesh.InvalidateGeomCache();
    node->InvalidateWS();
  }
  ip->RedrawViews(time);
}

void put_meshes_on_mesh(Interface *ip, TCHAR *selsname, Tab<INode *> &snode, int objnum, int seed, bool set_to_norm, bool use_smgr,
  bool rotatez, float slope, bool selfaces, float xydiap[2], float zdiap[2], bool xys, char zs);

void DagUtil::genobjonsurf()
{
  update_vars();
  TimeValue time = ip->GetTime();
  int nc = ip->GetSelNodeCount();
  if (!nc)
    return;
  Tab<INode *> snode;
  snode.SetCount(nc);
  for (int i = 0; i < nc; ++i)
    snode[i] = ip->GetSelNode(i);
  put_meshes_on_mesh(ip, gs_sname, snode, gs_objnum, gs_seed, bool(gs_set2norm != 0), bool(gs_usesmgr != 0), bool(gs_rotatez != 0),
    DegToRad(gs_slope), bool(gs_selfaces != 0), gs_xydiap, gs_zdiap, bool(gs_scalexy != 0), gs_scalez);
}

Matrix3 get_scaled_stretch_node_tm(INode *node, TimeValue time)
{
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  float masterScale = (float)GetSystemUnitScale(UNITS_METERS);
#else
  float masterScale = (float)GetMasterScale(UNITS_METERS);
#endif
  Matrix3 tm = node->GetStretchTM(time) * node->GetNodeTM(time);
  tm.SetTrans(tm.GetTrans() * masterScale);
  return tm;
}


Matrix3 get_scaled_object_tm(INode *node, TimeValue time)
{
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  float masterScale = (float)GetSystemUnitScale(UNITS_METERS);
#else
  float masterScale = (float)GetMasterScale(UNITS_METERS);
#endif
  Matrix3 tm = node->GetObjectTM(time);
  tm.SetTrans(tm.GetTrans() * masterScale);
  return tm;
}

int enum_nodes(INode *node, ENodeCB *cb)
{
  if (!node)
    return 1;
  int cn = node->NumberOfChildren();
  for (int i = 0; i < cn; ++i)
  {
    INode *n = node->GetChildNode(i);
    switch (cb->proc(n))
    {
      case ECB_STOP: return 0;
      case ECB_CONT:
        if (!enum_nodes(n, cb))
          return 0;
    }
  }
  return 1;
}

void enum_layer_nodes(ILayer *layer, ENodeCB *cb)
{
  if (!layer)
    return;
  ILayerProperties *layerProp = (ILayerProperties *)layer->GetInterface(LAYERPROPERTIES_INTERFACE);
  Tab<INode *> nodes;
  layerProp->Nodes(nodes);
  for (int i = 0; i < nodes.Count(); ++i)
  {
    cb->proc(nodes[i]);
    enum_nodes(nodes[i], cb);
  }
}

void enum_nodes_by_node(INode *node, ENodeCB *cb)
{
  cb->proc(node);
  enum_nodes(node, cb);
}

bool is_default_layer(const ILayer &layer) { return _tcscmp(layer.GetName(), _T("0")) == 0; }
