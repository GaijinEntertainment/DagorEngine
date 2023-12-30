#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/dagFileRW/dagFileFormat.h>
#include <libTools/dagFileRW/sceneImpIface.h>
#include <libTools/dagFileRW/dagFileShapeObj.h>
#include <libTools/dagFileRW/dagFileLightObj.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_materialData.h>
#include <math/dag_mesh.h>
#include <math/dag_meshBones.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include <libTools/dagFileRW/textureNameResolver.h>

static ITextureNameResolver *scene_tex_resolve_cb = NULL;

void set_global_tex_name_resolver(ITextureNameResolver *cb) { scene_tex_resolve_cb = cb; }
ITextureNameResolver *get_global_tex_name_resolver() { return scene_tex_resolve_cb; }

class MyLoadCB : public ImpSceneCB
{
public:
  Tab<TEXTUREID> mtex;
  PtrTab<MaterialData> mat;
  Tab<int> nstk;
  Node *root = nullptr, *cnode = nullptr;
  AScene *scene;
  int number = 0, number2 = 0;
  const char *fname;
  int flags;

  MyLoadCB(const char *fn, AScene *, int flg, bool fatal_on_error = true);
  ~MyLoadCB();

  void nomem_error(const char *fn);
  void open_error(const char *fn);
  void format_error(const char *fn);
  void read_error(const char *fn);

  int load_textures(int num);
  int load_material(const char *name);
  int load_node();
  int load_node_children();
  int load_node_data();
  int load_node_tm();
  int load_node_mat(int numsubs);
  int load_node_script();
  int load_node_obj();
  int load_node_bones(int);
  int load_node_bonesinf(int);

  int load_texture(int, const char *texfn);
  int load_material(ImpMat &);
  int load_node_data(char *name, uint16_t id, int numchild, int flg);
  int load_node_tm(TMatrix &);
  int load_node_submat(int, uint16_t id);
  int load_node_script(const char *);
  int load_node_mesh(Mesh *);
  int load_node_splines(SplineShape *);
  int load_node_light(D3dLight &);
  int load_node_bone(int, uint16_t nodeid, TMatrix &);
  int load_node_bonesinf(real *inf);

  virtual bool load_node_morph(int num);
  virtual bool load_node_morph_target(int index, uint16_t nodeid, const char *name);


private:
  bool fatalOnError_;
};


MyLoadCB::MyLoadCB(const char *fn, AScene *sc, int flg, bool fatal_on_error) :
  mtex(tmpmem), mat(tmpmem), nstk(tmpmem), fatalOnError_(fatal_on_error)
{
  fname = fn;
  scene = sc;
  flags = flg;
}

MyLoadCB::~MyLoadCB()
{
  if (root)
    delete root;
}

void MyLoadCB::nomem_error(const char *fn)
{
  if (fatalOnError_)
    DAG_FATAL("Not enough memory to load %s", fn);
  else
    debug("Not enough memory to load %s", fn);
}

void MyLoadCB::open_error(const char *fn)
{
  if (fatalOnError_)
    DAG_FATAL("Can't open %s", fn);
  else
    debug("Can't open %s", fn);
}

void MyLoadCB::format_error(const char *fn)
{
  if (fatalOnError_)
    DAG_FATAL("File %s has invalid format", fn);
  else
    debug("File %s has invalid format", fn);
}

void MyLoadCB::read_error(const char *fn)
{
  if (fatalOnError_)
    DAG_FATAL("Error reading %s", fn);
  else
    debug("Error reading %s", fn);
}

int MyLoadCB::load_textures(int num)
{
  if (flags & LASF_NOMATS)
    return 0;
  number = num;
  return 1;
}

int MyLoadCB::load_material(const char *name)
{
  if (flags & LASF_NOMATS)
    return 0;
  return 1;
}

int MyLoadCB::load_node_data() { return 1; }

int MyLoadCB::load_node_tm() { return 1; }

int MyLoadCB::load_node_script() { return 1; }

int MyLoadCB::load_node_mat(int num)
{
  if (flags & LASF_NOMATS)
    return 0;
  number = num;
  return 1;
}

int MyLoadCB::load_node_obj() { return 1; }

int MyLoadCB::load_texture(int i, const char *fn)
{
  if (i == 0)
  {
    mtex.resize(number);
    for (int j = 0; j < number; ++j)
      mtex[j] = BAD_TEXTUREID;
  }

  String realTexName;
  if (scene_tex_resolve_cb && scene_tex_resolve_cb->resolveTextureName(fn, realTexName))
    mtex[i] = add_managed_texture(realTexName);
  else
    mtex[i] = add_managed_texture(fn);
  return 1;
}


int MyLoadCB::load_material(ImpMat &m)
{
  TEXTUREID mt[MAXMATTEXNUM];
  for (int j = 0; j < MAXMATTEXNUM; ++j)
    if (m.texid[j] >= mtex.size() || (j >= 8 && !(m.flags & DAG_MF_16TEX)))
      mt[j] = BAD_TEXTUREID;
    else
      mt[j] = mtex[m.texid[j]];
  D3dMaterial dm;
  dm.emis = m.emis;
  dm.amb = m.amb;
  dm.diff = m.diff;
  dm.spec = m.spec;
  if (m.spec.isblack())
    dm.power = 0;
  else
    dm.power = m.power;

  MaterialData *ma = new MaterialData;

  memcpy(ma->mtex, mt, sizeof(ma->mtex));
  ma->mat = dm;
  ma->className = m.classname;

  if (m.flags & IMP_MF_2SIDED)
    ma->flags |= MATF_2SIDED;
  if (flags & LASF_MATNAMES)
    ma->matName = m.name;
  if (m.script)
    ma->matScript = m.script;

  if (m.script)
    memfree(m.script, strmem);

  mat.push_back(ma);
  return 1;
}

int MyLoadCB::load_node()
{
  if (!root)
  {
    root = new Node;
    cnode = root;
  }
  else
  {
    while (nstk.size())
    {
      int i = ++nstk.back();
      if (i == 0)
      {
        cnode = cnode->child[0];
        break;
      }
      else if (i < cnode->parent->child.size())
      {
        cnode = cnode->parent->child[i];
        break;
      }
      else
      {
        cnode = cnode->parent;
        nstk.pop_back();
      }
    }
  }
  return 1;
}

int MyLoadCB::load_node_children()
{
  int a = -1;
  nstk.push_back(a);
  for (int i = 0; i < cnode->child.size(); ++i)
  {
    cnode->child[i] = new Node;
    cnode->child[i]->parent = cnode;
  }
  return 1;
}

int MyLoadCB::load_node_data(char *name, uint16_t id, int cnum, int flg)
{
  if (cnum)
  {
    cnode->child.resize(cnum);
    memset(&cnode->child[0], 0, cnum * sizeof(Node *));
  }
  cnode->id = id;
  if (name)
    if (name[0])
      cnode->name = name;

  if (flg & IMP_NF_RENDERABLE)
    cnode->flags |= NODEFLG_RENDERABLE;
  if (flg & IMP_NF_CASTSHADOW)
    cnode->flags |= NODEFLG_CASTSHADOW;
  if (flg & IMP_NF_RCVSHADOW)
    cnode->flags |= NODEFLG_RCVSHADOW;
  return 1;
}

int MyLoadCB::load_node_tm(TMatrix &tm)
{
  cnode->tm = tm;
  return 1;
}

int MyLoadCB::load_node_submat(int i, uint16_t id)
{
  if (number > 1)
  {
    if (i == 0)
    {
      cnode->mat = new MaterialDataList;
      cnode->mat->list.resize(number);
    }

    MaterialDataList &mm = *cnode->mat;
    if (id < mat.size())
      mm.list[i] = mat[id];
  }
  else
  {
    if (id < mat.size())
    {
      cnode->mat = new MaterialDataList;
      cnode->mat->addSubMat(mat[id]);
    }
  }
  return 1;
}

int MyLoadCB::load_node_script(const char *s)
{
  cnode->script = s;
  memfree((void *)s, strmem);
  return 1;
}

int MyLoadCB::load_node_mesh(Mesh *m)
{
  if (flags & LASF_NOMESHES)
  {
    delete m;
    return 1;
  }
  if (m)
  {
    if (!m->check_mesh())
    {
      logerr("node <%s> from <%s> has invalid mesh", cnode->name.str(), fname);
      delete m;
      return 0;
    }
    m->kill_bad_faces();
    if (m->getFace().size() <= 0)
      logerr("warning: node <%s> from <%s> has mesh with 0 faces", cnode->name.str(), fname);
    if (!m->check_mesh())
    {
      logerr("node <%s> from <%s> mesh became invalid after kill_bad_faces()", cnode->name.str(), fname);
      delete m;
      return 0;
    }
  }
  MeshHolderObj *o = new MeshHolderObj(m);
  cnode->setobj(o);
  return 1;
}

int MyLoadCB::load_node_splines(SplineShape *s)
{
  if (flags & LASF_NOSPLINES)
  {
    delete s;
    return 1;
  }
  ShapeHolderObj *o = new ShapeHolderObj(s);
  cnode->setobj(o);
  return 1;
}

int MyLoadCB::load_node_light(D3dLight &l)
{
  if (flags & LASF_NOLIGHTS)
    return 1;
  LightObject *o = new LightObject(&l);
  cnode->setobj(o);
  return 1;
}

int MyLoadCB::load_node_bones(int n)
{
  number = n;
  return 1;
}

int MyLoadCB::load_node_bonesinf(int n)
{
  number2 = n;
  return 1;
}


int MyLoadCB::load_node_bone(int i, uint16_t id, TMatrix &tm)
{
  if (flags & LASF_NOMESHES)
  {
    return 1;
  }

  if (i == 0)
  {
    if (!cnode->obj)
    {
      format_error(fname);
      return 0;
    }
    if (!cnode->obj->isSubOf(OCID_MESHHOLDER))
    {
      format_error(fname);
      return 0;
    }
    MeshHolderObj *h = (MeshHolderObj *)cnode->obj;
    MeshBones *b = new MeshBones;
    b->setnumbones(number);
    h->setbones(b);
  }
  MeshBones *b = ((MeshHolderObj *)cnode->obj)->bones;
  b->boneNames[i] = "XXXX";
  *(uint16_t *)(char *)b->boneNames[i] = id;
  b->orgtm[i] = inverse(tm);
  return 1;
}


int MyLoadCB::load_node_bonesinf(real *inf)
{
  if (flags & LASF_NOMESHES)
  {
    memfree(inf, tmpmem);
    return 1;
  }
  MeshBones *b = ((MeshHolderObj *)cnode->obj)->bones;
  b->boneinf.resize(number * number2);
  memcpy(&b->boneinf[0], inf, number * number2 * sizeof(real));

  memfree(inf, tmpmem);
  return 1;
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


bool MyLoadCB::load_node_morph(int num)
{
  number = num;
  return true;
}


bool MyLoadCB::load_node_morph_target(int i, uint16_t nodeid, const char *name)
{
  if (flags & LASF_NOMESHES)
    return true;

  if (i == 0)
  {
    if (!cnode->obj)
    {
      format_error(fname);
      return false;
    }
    if (!cnode->obj->isSubOf(OCID_MESHHOLDER))
    {
      format_error(fname);
      return false;
    }
    MeshHolderObj *h = (MeshHolderObj *)cnode->obj;
    h->morphTargets.resize(number);
  }

  MeshHolderObj *h = (MeshHolderObj *)cnode->obj;
  h->morphTargets[i].name = name;
  h->morphTargets[i].node = (Node *)(void *)(intptr_t)nodeid;

  return true;
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static void set_bones(Node *n, Node *root)
{
  if (n->obj && n->obj->isSubOf(OCID_MESHHOLDER))
  {
    MeshHolderObj *h = (MeshHolderObj *)n->obj;

    for (int i = 0; i < h->morphTargets.size(); ++i)
      h->morphTargets[i].node = root->find_node(intptr_t(h->morphTargets[i].node));

    MeshBones *b = h->bones;
    if (b)
      for (int i = 0; i < b->boneNames.size(); ++i)
      {
        Node *n = root->find_node(int(*(uint16_t *)(char *)b->boneNames[i]));
        b->boneNames[i] = n ? n->name.str() : NULL;
      }
  }

  for (int i = 0; i < n->child.size(); ++i)
    set_bones(n->child[i], root);
}


int load_ascene(const char *fn, AScene &sc, int flg, bool fatal_on_error, PtrTab<MaterialData> *mat_list)
{
  MyLoadCB cb(fn, &sc, flg, fatal_on_error);
  if (!load_scene(fn, cb))
  {
    if (fatal_on_error)
      DAG_FATAL("invalid scene <%s>: unable to read", fn);
    else
      logerr("invalid scene <%s>: unable to read", fn);
    return 0;
  }

  if (mat_list)
  {
    mat_list->resize(cb.mat.size());
    for (int i = 0; i < mat_list->size(); ++i)
      (*mat_list)[i] = cb.mat[i];
  }

  if (!cb.root)
  {
    if (fatal_on_error)
      DAG_FATAL("invalid scene <%s>: no root node", fn);
    else
      logerr("invalid scene <%s>: no root node", fn);
    return 0;
  }
  sc.root = cb.root;
  cb.root = NULL;
  set_bones(sc.root, sc.root);
  return 1;
}


Object::~Object() {}
int Object::classid() { return OCID_OBJECT; }
bool Object::isSubOf(unsigned c) { return c == OCID_OBJECT; }


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


MeshHolderObj::MeshHolderObj(Mesh *m) : morphTargets(tmpmem)
{
  mesh = m;
  bones = NULL;
}


MeshHolderObj::~MeshHolderObj()
{
  if (mesh)
    delete mesh;
  if (bones)
    delete bones;
}

void MeshHolderObj::setmesh(Mesh *m)
{
  if (mesh)
    delete mesh;
  mesh = m;
}

void MeshHolderObj::setbones(MeshBones *b)
{
  if (bones)
    delete bones;
  bones = b;
}

int MeshHolderObj::classid() { return OCID_MESHHOLDER; }

bool MeshHolderObj::isSubOf(unsigned c) { return c == OCID_MESHHOLDER || Object::isSubOf(c); }

ShapeHolderObj::ShapeHolderObj(SplineShape *s) { shp = s; }

ShapeHolderObj::~ShapeHolderObj()
{
  if (shp)
    delete shp;
}

void ShapeHolderObj::setshape(SplineShape *s)
{
  if (shp)
    delete shp;
  shp = s;
}

int ShapeHolderObj::classid() { return OCID_SHAPEHOLDER; }

bool ShapeHolderObj::isSubOf(unsigned c) { return c == OCID_SHAPEHOLDER || Object::isSubOf(c); }

LightObject::LightObject(D3dLight *l)
{
  if (l)
    lt = *l;
  flags = 0;
}

LightObject::~LightObject() {}

int LightObject::classid() { return OCID_LIGHT; }

bool LightObject::isSubOf(unsigned c) { return c == OCID_LIGHT || Object::isSubOf(c); }

Node::Node() : child(midmem)
{
  parent = NULL;
  obj = NULL;
  mat = NULL;
  flags = 0;
  id = -1;
  tm.identity();
  tmpval = -1;
}

Node::~Node()
{
  for (int i = 0; i < child.size(); ++i)
    if (child[i])
      delete (child[i]);
  if (obj)
    delete obj;
  del_it(mat);
}

void Node::setobj(Object *o)
{
  if (obj)
    delete obj;
  obj = o;
}

Node *Node::find_node(int fid)
{
  if (id == fid)
    return this;
  for (int i = 0; i < child.size(); ++i)
  {
    Node *n = child[i]->find_node(fid);
    if (n)
      return n;
  }
  return NULL;
}

void Node::calc_wtm()
{
  if (!(flags & NODEFLG_WTMOK))
  {
    if (parent)
      wtm = parent->wtm * tm;
    else
      wtm = tm;
    flags |= NODEFLG_WTMOK;
  }
  for (int i = 0; i < child.size(); ++i)
    child[i]->calc_wtm();
}

void Node::calc_wtm(NodeTMChangeCB &cb)
{
  if (!(flags & NODEFLG_WTMOK))
  {
    if (parent)
      wtm = parent->wtm * tm;
    else
      wtm = tm;
    flags |= NODEFLG_WTMOK;
    cb.node_tm_changed(this);
  }
  for (int i = 0; i < child.size(); ++i)
    child[i]->calc_wtm(cb);
}

void Node::invalidate_anim()
{
  for (int i = 0; i < child.size(); ++i)
    child[i]->invalidate_anim();
}

void Node::invalidate_wtm()
{
  flags &= ~NODEFLG_WTMOK;
  for (int i = 0; i < child.size(); ++i)
    child[i]->invalidate_wtm();
}

Node *Node::find_node(const char *nm)
{
  if (name && strcmp(name, nm) == 0)
    return this;
  for (int i = 0; i < child.size(); ++i)
  {
    Node *n = child[i]->find_node(nm);
    if (n)
      return n;
  }
  return NULL;
}

Node *Node::find_inode(const char *nm)
{
  if (name && stricmp(name, nm) == 0)
    return this;
  for (int i = 0; i < child.size(); ++i)
  {
    Node *n = child[i]->find_inode(nm);
    if (n)
      return n;
  }
  return NULL;
}

bool Node::enum_nodes(NodeEnumCB &cb, Node **res)
{
  int rs = cb.node(*this);
  if (res)
    *res = NULL;
  if (rs == -1)
  {
    if (res)
      *res = this;
    return true;
  }
  else if (rs == 1)
  {
    if (res)
      *res = this;
  }
  for (int i = 0; i < child.size(); ++i)
    if (child[i])
    {
      Node *n;
      if (child[i]->enum_nodes(cb, &n))
      {
        if (res)
          *res = n;
        return true;
      }
    }
  return false;
}

AScene::AScene() { root = NULL; }

AScene::~AScene()
{
  if (root)
  {
    delete root;
    root = NULL;
  }
}
