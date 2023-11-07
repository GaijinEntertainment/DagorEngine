#include <3d/dag_drv3d.h>
#include <libTools/staticGeom/geomObject.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/staticGeom/matFlags.h>
#include <generic/dag_sort.h>
#include <obsolete/dag_cfg.h>

#include <libTools/renderUtil/dynRenderBuf.h>
#include <libTools/shaderResBuilder/shaderMeshData.h>
#include <libTools/shaderResBuilder/globalVertexDataConnector.h>
#include <libTools/shaderResBuilder/matSubst.h>
#include <libTools/util/prepareBillboardMesh.h>
#include <libTools/util/progressInd.h>
#include <libTools/util/sHmathUtil.h>
#include <sceneBuilder/nsb_StdTonemapper.h>

#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>

#include <debug/dag_debug3d.h>
#include <debug/dag_debug.h>

#include <util/dag_bitArray.h>
#include <util/dag_hash.h>
#include <ioSys/dag_dataBlock.h>

#include <math/dag_mathAng.h>

#include <winGuiWrapper/wgw_dialogs.h>

#include <coolConsole/coolConsole.h>

#include <startup/dag_globalSettings.h>

#include <stdlib.h>

#include <3d/dag_render.h>

#include <libTools/dagFileRW/textureNameResolver.h>

#define ONE_LDR_STEP ((real)(1.0 / 255.0))

#define DEFAULT_SAMPLE_SIZE 0.0f

#define MINIMUM_COLOR_RED   (45.0 * ONE_LDR_STEP)
#define MINIMUM_COLOR_GREEN (45.0 * ONE_LDR_STEP)
#define MINIMUM_COLOR_BLUE  (60.0 * ONE_LDR_STEP)

#define LIGHTMAP_SCALE 1.0

#define DYN_LIGHTING_COEFF 0.2


// edged faces view parameters
bool GeomObject::isEdgedFaceView = false;
E3DCOLOR GeomObject::edgedFaceColor = E3DCOLOR(200, 200, 200, 255);
bool GeomObject::useDynLighting = false;
bool GeomObject::isRenderingReflection = false;
bool GeomObject::isRenderingShadows = false;
bool GeomObject::setNodeNameHashToShaders = false;

// lighting parameters
E3DCOLOR GeomObject::skyCol = E3DCOLOR(0, 10, 60, 255);
E3DCOLOR GeomObject::earthCol = E3DCOLOR(0, 0, 0, 255);
E3DCOLOR GeomObject::directCol = E3DCOLOR(255, 255, 230, 255);

Point3 GeomObject::skyColor = Point3(0.0, 0.1, 0.6);
Point3 GeomObject::earthColor = Point3(0.0, 0.0, 0.0);
Point3 GeomObject::directColor = Point3(2.55, 2.55, 2.3);

real GeomObject::skyBright = 0.1;
real GeomObject::earthBright = 0.1;
real GeomObject::directMult = 2.0;

Point2 GeomObject::lightingDirAngles = Point2(PI / 3, 0);
Point3 GeomObject::lightingDir = ::sph_ang_to_dir(Point2(lightingDirAngles.y, lightingDirAngles.x));

NameMap GeomObject::shadersMap;
static int shadersMapRefs = 0;

StaticSceneBuilder::StdTonemapper *GeomObject::toneMapper = NULL;

int GeomObject::hdrLightmapScaleId = -2;
int GeomObject::worldViewPosId = -2;

void (*stat_geom_on_remove)(GeomObject *p) = NULL;
void (*stat_geom_on_water_add)(GeomObject *p, const ShaderMesh *m) = NULL;

// all editor's objects
Tab<GeomObject *> *GeomObject::createdObjects = NULL;


Tab<String> GeomObject::fatalMessages(tmpmem_ptr());


//==============================================================================
GeomObject::GeomObject() :
  geom(NULL),
  shMesh(midmem),
  nodeVis(midmem),
  tm(TMatrix::IDENT),
  externalGeomContainer(0),
  sampleSize(DEFAULT_SAMPLE_SIZE),
  saveAllMats(0),
  allmats(tmpmem),
  shaderNameIds(midmem),
  useNodeVisRange(0),
  rayTracer(NULL),
  doNotChangeVertexColors(0),
  hasReflection(0),
  localBoundBox(0)
{
  initStaticMembers();

  geom = new (midmem) StaticGeometryContainer;

  createdObjects->push_back(this);
}


//==============================================================================
GeomObject::~GeomObject()
{
  if (hasReflection && stat_geom_on_remove)
    stat_geom_on_remove(this);
  delRayTracer();

  if (!externalGeomContainer)
    del_it(geom);

  for (int i = 0; i < createdObjects->size(); ++i)
    if (createdObjects->operator[](i) == this)
    {
      erase_items(*createdObjects, i, 1);
      break;
    }

  if (createdObjects->empty())
    del_it(createdObjects);

  shadersMapRefs--;
  if (!shadersMapRefs)
    shadersMap.clear();
}


//==============================================================================
void GeomObject::initStaticMembers()
{
  shadersMapRefs++;

  if (!createdObjects)
  {
    createdObjects = new (midmem) Tab<GeomObject *>(midmem);
    G_ASSERT(createdObjects);
  }

  if (hdrLightmapScaleId == -2)
    hdrLightmapScaleId = ::get_shader_variable_id("lightmap_scale", true);

  if (worldViewPosId == -2)
    worldViewPosId = get_shader_variable_id("world_view_pos", true);
}


//==============================================================================
void GeomObject::clear()
{
  delRayTracer();

  if (externalGeomContainer)
  {
    geom = new StaticGeometryContainer;
    externalGeomContainer = 0;
  }
  else
    geom->clear();

  clear_and_shrink(shMesh);
  nodeVis.clear();

  eraseAllMats(true);
  localBoundBox = 0;
  bounds.setempty();
}


//==============================================================================
void GeomObject::eraseNode(int idx)
{
  del_it(geom->nodes[idx]);

  safe_erase_items(shMesh, idx, 1);
  safe_erase_items(geom->nodes, idx, 1);
}


//==============================================================================
void GeomObject::replaceNode(int idx, StaticGeometryNode *node, bool rebuild)
{
  if (!node || idx < 0 || idx >= geom->nodes.size())
    return;

  del_it(geom->nodes[idx]);

  geom->nodes[idx] = node;

  if (rebuild)
  {
    if (saveAllMats)
      recompileNode(idx);
    else
      recompile();
  }
}


//==============================================================================
void GeomObject::connectToGeomContainer(StaticGeometryContainer *new_geom)
{
  if (!new_geom)
    return;

  if (!externalGeomContainer)
  {
    clear();
    del_it(geom);
  }

  delRayTracer();

  geom = new_geom;
  externalGeomContainer = 1;

  setDefNodeVis();

  compileRenderData();
}


//==============================================================================
void GeomObject::createFromGeomContainer(const StaticGeometryContainer *cont, bool do_recompile)
{
  if (!cont)
    return;

  if (!externalGeomContainer)
    clear();
  else
  {
    geom = new (midmem) StaticGeometryContainer;
    externalGeomContainer = 0;
  }
  delRayTracer();

  eraseAllMats(false);

  for (int i = 0; i < cont->nodes.size(); ++i)
  {
    if (!cont->nodes[i])
      continue;

    StaticGeometryMesh *mesh = new (midmem) StaticGeometryMesh(midmem);

    mesh->mesh = cont->nodes[i]->mesh->mesh;
    mesh->mats.resize(cont->nodes[i]->mesh->mats.size());

    for (int j = 0; j < cont->nodes[i]->mesh->mats.size(); ++j)
    {
      StaticGeometryMaterial *mat = new (midmem) StaticGeometryMaterial(*cont->nodes[i]->mesh->mats[j]);

      mesh->mats[j] = mat;
    }

    StaticGeometryNode *node = new (midmem) StaticGeometryNode(*cont->nodes[i]);
    node->mesh = mesh;

    geom->addNode(node);
  }

  setDefNodeVis();

  if (do_recompile)
    recompile();
}


//==============================================================================
void GeomObject::setAmbientLight(E3DCOLOR sky_color, real sky_bright, E3DCOLOR earth_color, real earth_bright)
{
  skyCol = sky_color;
  earthCol = earth_color;

  skyBright = sky_bright;
  earthBright = earth_bright;

  skyColor = Point3(ONE_LDR_STEP * skyCol.r * skyBright, ONE_LDR_STEP * skyCol.g * skyBright, ONE_LDR_STEP * skyCol.b * skyBright);

  earthColor =
    Point3(ONE_LDR_STEP * earthCol.r * earthBright, ONE_LDR_STEP * earthCol.g * earthBright, ONE_LDR_STEP * earthCol.b * earthBright);
}


//==============================================================================
void GeomObject::setDirectLight(E3DCOLOR color, real mult)
{
  directCol = color;
  directMult = mult;

  directColor = Point3(ONE_LDR_STEP * color.r * mult, ONE_LDR_STEP * color.g * mult, ONE_LDR_STEP * color.b * mult);
}


//==============================================================================
void GeomObject::setLightAngles(real zenith, real azimuth)
{
  lightingDirAngles.x = zenith;
  lightingDirAngles.y = azimuth;

  lightingDir = ::sph_ang_to_dir(Point2(azimuth, zenith));
}


//==============================================================================
void GeomObject::setLightAnglesDeg(real alpha, real beta) { setLightAngles(DegToRad(alpha), DegToRad(beta)); }


//==============================================================================
void GeomObject::getSkyLight(E3DCOLOR &color, real &bright)
{
  color = skyCol;
  bright = skyBright;
}


//==============================================================================
void GeomObject::getEarthLight(E3DCOLOR &color, real &bright)
{
  color = earthCol;
  bright = earthBright;
}


//==============================================================================
void GeomObject::getDirectLight(E3DCOLOR &color, real &mult)
{
  color = directCol;
  mult = directMult;
}


//==============================================================================
void GeomObject::getLightAngles(real &alpha, real &beta)
{
  alpha = lightingDirAngles.x;
  beta = lightingDirAngles.y;
}


//==============================================================================
void GeomObject::recompileAllGeomObjects(IGenericProgressIndicator *progress)
{
  if (!createdObjects)
    return;

  if (progress)
  {
    progress->setActionDesc("Recompile geometry lighting...");
    progress->setTotal(createdObjects->size());
  }

  for (int i = 0; i < createdObjects->size(); ++i)
  {
    createdObjects->at(i)->recompile();

    if (progress)
      progress->incDone();
  }
}


//==============================================================================
void GeomObject::getAllObjects(GeomObjRequest req, Tab<GeomObject *> &objs, const void *condition)
{
  objs.clear();

  switch (req)
  {
    case REQUEST_CLASS: getObjectsByClass(objs, (const char *)condition); break;
  }
}


//==============================================================================
void GeomObject::getObjectsByClass(Tab<GeomObject *> &objs, const char *class_name)
{
  for (int i = 0; i < createdObjects->size(); ++i)
    for (int j = 0; j < createdObjects->operator[](i)->getShaderNamesCount(); ++j)
      if (!stricmp(class_name, createdObjects->operator[](i)->getShaderName(j)))
      {
        objs.push_back(createdObjects->operator[](i));
        break;
      }
}


//==============================================================================
void GeomObject::recompile(const Bitarray *node_vis)
{
  if (node_vis)
    setNodeVis(*node_vis);

  compileRenderData();
}


//==============================================================================
void GeomObject::setDefNodeVis()
{
  nodeVis.resize(geom->nodes.size());

  for (int i = 0; i < geom->nodes.size(); ++i)
    nodeVis.set(i, geom->nodes[i]->flags & StaticGeometryNode::FLG_RENDERABLE);
}


//==============================================================================
void GeomObject::setNodeVis(const Bitarray &from)
{
  for (int i = 0; i < __min(from.size(), nodeVis.size()); ++i)
    nodeVis.set(i, from[i]);
}


//==============================================================================
bool GeomObject::loadFromDag(const char *fname, ILogWriter *log, ITextureNameResolver *resolv_cb)
{
  clear();

  ITextureNameResolver *prev = get_global_tex_name_resolver();
  if (resolv_cb)
    set_global_tex_name_resolver(resolv_cb);

  if (geom->loadFromDag(fname, log))
  {
    setDefNodeVis();
    bounds.setempty();

    compileRenderData();
    set_global_tex_name_resolver(prev);

    return true;
  }

  set_global_tex_name_resolver(prev);

  return false;
}


//==============================================================================
void GeomObject::saveToDag(const char *fname) { geom->exportDag(fname); }


//==============================================================================
void GeomObject::saveToDagAndScale(const char *fname, real scale_coef)
{
  for (int i = 0; i < geom->nodes.size(); ++i)
    if (geom->nodes[i])
    {
      TMatrix tm = geom->nodes[i]->wtm;
      tm.setcol(0, tm.getcol(0) * scale_coef);
      tm.setcol(1, tm.getcol(1) * scale_coef);
      tm.setcol(2, tm.getcol(2) * scale_coef);
      geom->nodes[i]->wtm = tm;
    }

  geom->exportDag(fname);
}


//==================================================================================================
bool GeomObject::isNodeInVisRange(int idx) const
{
  class VisDeterminer
  {
  public:
    VisDeterminer(const TMatrix &m, const StaticGeometryNode &n) : tm(m), node(n) {}

    BBox3 getBoundingBox() { return tm * (node.wtm * node.boundBox); }
    BSphere3 getBoundingSphere() { return tm * (node.wtm * node.boundSphere); }
    real getVisibilityRange() { return -1; }

  private:
    const TMatrix &tm;
    const StaticGeometryNode &node;
  };

  if (idx >= 0 && idx < geom->nodes.size())
  {
    StaticGeometryNode *node = geom->nodes[idx];

    if (node)
    {
      if (node->flags & StaticGeometryNode::FLG_AUTOMATIC_VISRANGE || node->visRange == -1.0)
      {
        if (::visibility_finder)
        {
          VisDeterminer vd(tm, *node);
          return ::visibility_finder->isVisible(vd.getBoundingBox(), vd.getBoundingSphere(), -1);
        }
        else
        {
          ::debug("[GeomObject warning] VisibilityFinder class pointer is NULL");
          return false;
        }
      }
      else
      {
        const real dist = ::length((tm * (node->wtm * node->boundSphere.c)) - ::grs_cur_view.pos);
        return dist <= node->visRange + node->boundSphere.r;
      }
    }
  }

  return false;
}


//==============================================================================
void GeomObject::render()
{
  if (isRenderingReflection && hasReflection)
    return;

  // BBox3 bbox = getBoundBox();

  //    ShaderGlobal::set_color4(worldViewPosId,
  //      Color4(::grs_cur_view.pos.x, ::grs_cur_view.pos.y, ::grs_cur_view.pos.z, 1.f));

  for (int i = 0; i < shMesh.size(); ++i)
  {
    RenderObject &rendObj = shMesh[i];

    rendObj.beforeRenderCheck();

    int nodeId = rendObj.lods[rendObj.curLod].nodeID;
    int flags = geom->nodes[nodeId]->flags;
    if (flags & StaticGeometryNode::FLG_OPERATIVE_HIDE)
      continue;
    if (GeomObject::isRenderingShadows && !(flags & StaticGeometryNode::FLG_CASTSHADOWS))
      continue;

    if (useNodeVisRange && !isNodeInVisRange(nodeId))
      continue;
    d3d::settm(TM_WORLD, tm);
    if (GeomObject::setNodeNameHashToShaders)
    {
      static int geom_node_hash_gvid = ::get_shader_variable_id("geom_node_hash", true);
      int hash = str_hash_fnv1(geom->nodes[nodeId]->name) & 0xFFFF;
      ShaderGlobal::set_int(geom_node_hash_gvid, hash);
    }
    rendObj.render();
  }
}


//==============================================================================
void GeomObject::renderEdges(bool is_visible, const E3DCOLOR color)
{
  for (int i = 0; i < geom->nodes.size(); ++i)
    if (geom->nodes[i] && geom->nodes[i]->mesh && (bool)nodeVis[i] == is_visible)
    {
      if (useNodeVisRange && !isNodeInVisRange(i))
        continue;

      ::set_cached_debug_lines_wtm(tm * geom->nodes[i]->wtm);
      renderWireFrame(geom->nodes[i]->mesh->mesh, color);
    }
}


//==============================================================================
void GeomObject::renderTrans()
{
  if (isRenderingReflection && hasReflection)
    return;

  // BBox3 bbox = getBoundBox();

  //    ShaderGlobal::set_color4(worldViewPosId,
  //      Color4(::grs_cur_view.pos.x, ::grs_cur_view.pos.y, ::grs_cur_view.pos.z, 1.f));

  for (int i = 0; i < shMesh.size(); ++i)
  {
    int nodeId = shMesh[i].lods[shMesh[i].curLod].nodeID;
    int flags = geom->nodes[nodeId]->flags;
    if (flags & StaticGeometryNode::FLG_OPERATIVE_HIDE)
      continue;
    if (GeomObject::isRenderingShadows && !(flags & StaticGeometryNode::FLG_CASTSHADOWS))
      continue;

    if (useNodeVisRange && !isNodeInVisRange(nodeId))
      continue;

    d3d::settm(TM_WORLD, tm);
    if (GeomObject::setNodeNameHashToShaders)
    {
      static int geom_node_hash_gvid = ::get_shader_variable_id("geom_node_hash", true);
      int hash = str_hash_fnv1(geom->nodes[nodeId]->name) & 0xFFFF;
      ShaderGlobal::set_int(geom_node_hash_gvid, hash);
    }
    shMesh[i].renderTrans();
  }
}

//==============================================================================
void GeomObject::renderNodeColor(int idx, DynRenderBuffer *buffer, E3DCOLOR c, bool renderable_only)
{
  if (!buffer || idx < 0 || idx >= geom->nodes.size())
    return;

  const StaticGeometryNode *node = geom->nodes[idx];

  if (!node || !node->mesh)
    return;

  if (renderable_only && !(node->flags & StaticGeometryNode::FLG_RENDERABLE))
    return;

  // TODO: profile it
  //  if (useNodeVisRange && !isNodeInVisRange(idx))
  //    return;

  if (node->mesh->mesh.getVert().size() > 65536)
    return;

  Tab<DynRenderBuffer::Vertex> vertexTab(tmpmem);
  Tab<int> faceTab(tmpmem);
  DynRenderBuffer::Vertex v;
  const TMatrix tm = getTm();
  int i;

  buffer->clearBuf();

  vertexTab.resize(node->mesh->mesh.getVert().size());

  v.color = c;
  v.tc = Point2(0, 0);

  for (i = 0; i < node->mesh->mesh.getVert().size(); ++i)
  {
    v.pos = tm * (node->wtm * node->mesh->mesh.getVert()[i]);
    vertexTab[i] = v;
  }

  const int faceCnt = node->mesh->mesh.getFace().size() * 3;
  const bool doSplit = faceCnt > DynRenderBuffer::SAFETY_MAX_BUFFER_FACES;
  const int facesQuant = DynRenderBuffer::SAFETY_MAX_BUFFER_FACES / 3;
  const int iterCnt = faceCnt / facesQuant + 1;

  for (int iter = 0; iter < iterCnt; ++iter)
  {
    Mesh mesh;
    Mesh *processingMesh = &node->mesh->mesh;

    if (doSplit)
    {
      mesh = node->mesh->mesh;
      processingMesh = &mesh;

      if (iter)
        mesh.erasefaces(0, iter * facesQuant);

      mesh.erasefaces(facesQuant, mesh.getFace().size() - facesQuant);
    }

    faceTab.resize(processingMesh->getFace().size() * 3);

    for (i = 0; i < processingMesh->getFace().size(); ++i)
    {
      faceTab[i * 3 + 0] = processingMesh->getFace()[i].v[0];
      faceTab[i * 3 + 1] = processingMesh->getFace()[i].v[1];
      faceTab[i * 3 + 2] = processingMesh->getFace()[i].v[2];
    }

    buffer->drawCustom(vertexTab, faceTab);
    buffer->flushToBuffer(BAD_TEXTUREID);
    buffer->flush();
    buffer->clearBuf();
  }
}


//==============================================================================
void GeomObject::renderColor(DynRenderBuffer *buffer, E3DCOLOR c, bool renderable_only)
{
  if (buffer)
    for (int i = 0; i < geom->nodes.size(); ++i)
      renderNodeColor(i, buffer, c, renderable_only);
}


//==============================================================================
void GeomObject::renderWireFrame(const Mesh &mesh, E3DCOLOR color)
{
  for (int i = 0; i < mesh.getFace().size(); ++i)
  {
    ::draw_cached_debug_line(mesh.getVert()[mesh.getFace()[i].v[0]], mesh.getVert()[mesh.getFace()[i].v[1]], color);
    ::draw_cached_debug_line(mesh.getVert()[mesh.getFace()[i].v[1]], mesh.getVert()[mesh.getFace()[i].v[2]], color);
    ::draw_cached_debug_line(mesh.getVert()[mesh.getFace()[i].v[2]], mesh.getVert()[mesh.getFace()[i].v[0]], color);
  }
}


//==============================================================================
int GeomObject::lodCmp(const GeomObject::RenderObject::ObjectLod *a, const GeomObject::RenderObject::ObjectLod *b)
{
  if (a->lodRangeSq < b->lodRangeSq)
    return -1;

  if (a->lodRangeSq > b->lodRangeSq)
    return 1;

  return 0;
}


//==============================================================================
void GeomObject::compileRenderData()
{
  if (!geom)
    return;

  if (hasReflection && stat_geom_on_remove)
    stat_geom_on_remove(this);

  fatalMessages.clear();
  bool (*prev_handler)(const char *, const char *, const char *, int) = ::dgs_fatal_handler;
  ::dgs_fatal_handler = &fatalHandler;

  eraseAllMats(false);
  shMesh.clear();
  int i;

  for (i = 0; i < geom->nodes.size(); i++)
  {
    RenderObject *o = NULL;
    SimpleString lodName(geom->nodes[i]->topLodName);

    if (lodName.length())
      for (int objNo = 0; objNo < shMesh.size(); objNo++)
      {
        if (!shMesh[objNo].lods.size())
          continue;

        SimpleString objLodName(geom->nodes[shMesh[objNo].lods[0].nodeID]->topLodName);

        if (objLodName.length() && !strcmp(objLodName, lodName))
          o = &shMesh[objNo];
      }

    if (!o)
    {
      o = &shMesh.emplace_back();
      o->geom = this;
    }

    RenderObject::ObjectLod l;
    l.mesh = new ShaderMesh;
    l.lodRangeSq = geom->nodes[i]->lodRange * geom->nodes[i]->lodRange;
    l.nodeID = i;

    if (!strcmp(lodName, geom->nodes[i]->name))
      insert_item_at(o->lods, 0, l);
    else
      o->lods.push_back(l);
  }

  for (i = 0; i < geom->nodes.size(); ++i)
    recompileNode(i);

  for (i = 0; i < shMesh.size(); ++i)
    sort(shMesh[i].lods, 1, shMesh[i].lods.size() - 1, &lodCmp);

  if (!saveAllMats)
    eraseAllMats(true);

  const int reflectionTextureVarId = ::get_shader_variable_id("reflectionTex", true);
  const int refractionTextureVarId = ::get_shader_variable_id("refractionTex", true);

  hasReflection = 0;
  for (i = 0; i < shMesh.size(); ++i)
  {
    const ShaderMesh *mesh = shMesh[i].lods[0].mesh;

    shMesh[i].hasReflectionRefraction = !mesh->getAllElems().empty() && mesh->getAllElems()[0].mat &&
                                        (mesh->getAllElems()[0].mat->hasVariable(reflectionTextureVarId) ||
                                          mesh->getAllElems()[0].mat->hasVariable(refractionTextureVarId));
    if (shMesh[i].hasReflectionRefraction && stat_geom_on_water_add)
    {
      stat_geom_on_water_add(this, mesh);
      hasReflection = 1;
    }
  }

  if (fatalMessages.size())
  {
    String msg;
    for (int i = 0; i < fatalMessages.size(); ++i)
    {
      msg += "\n";
      msg += fatalMessages[i];
    }

    wingw::message_box(wingw::MBS_EXCL, "Fatal errors", "Geometry compilation has fatal error(s):\n%s", msg.str());
  }

  ::dgs_fatal_handler = prev_handler;
}

static void prepare_extra_for_landmesh(Mesh &mesh, Point3 &pos_to_world_ofs, Point3 &pos_to_world_scale, bool has_transparency)
{
  BBox3 box;
  for (int vi = 0; vi < mesh.getVert().size(); ++vi)
    box += mesh.getVert()[vi];
  Point3 cellScale(safediv(2.0f, box.width().x), safediv(2.0f, box.width().y), safediv(2.0f, box.width().z));
  Point3 cellOfs = box.center();
  compress_land_position(cellOfs, cellScale, mesh, has_transparency);
  pos_to_world_ofs = box.center();
  pos_to_world_scale = box.width() * 0.5f;
}


//==============================================================================
void GeomObject::recompileNode(int idx)
{
  StaticGeometryNode *node = geom->nodes[idx];
  if (!node || !node->mesh)
    return;

  if (nodeVis.size() != geom->nodes.size())
    setDefNodeVis();

  if (!nodeVis.get(idx))
    return;

  RenderObject::ObjectLod *o = NULL;
  int objNo;
  for (objNo = 0; objNo < shMesh.size() && !o; objNo++)
  {
    for (int lodNo = 0; lodNo < shMesh[objNo].lods.size(); lodNo++)
      if (shMesh[objNo].lods[lodNo].nodeID == idx)
      {
        o = &shMesh[objNo].lods[lodNo];
        break;
      }
  }

  if (!o)
    return;


  node->checkBillboard();

  int j;

  // create materials
  const int reducedMatMapStep = 2;
  Tab<int> reducedMatMap(tmpmem);
  reducedMatMap.reserve(reducedMatMapStep);

  bool has_lmesh_transparency = false;
  for (j = 0; j < node->mesh->mats.size(); ++j)
  {
    StaticGeometryMaterial *gmPtr = node->mesh->mats[j];
    if (!gmPtr)
      return;
    StaticGeometryMaterial &gm = *gmPtr;
    if (strstr(gm.className.str(), "land_mesh"))
    {
      if (!strstr(gm.className.str(), "decal") && strstr(gm.scriptText.str(), "render_landmesh_combined"))
      {
        CfgReader c;
        c.getdiv_text(String(128, "[q]\r\n%s\r\n", gm.scriptText.str()), "q");
        if (!c.getbool("render_landmesh_combined", 1))
        {
          String name(128, "%s_decal", gm.className.str());
          gm.className = name;
          logwarn("node <%s> has material<%s> with obsolete decal material."
                  "Instead of render_landmesh_combined = 0, use *_decal in classname",
            node->name.str(), gm.name.str());
        }
      }
      if (!has_lmesh_transparency && strstr(gm.scriptText.str(), "vertex_opacity"))
      {
        CfgReader c;
        c.getdiv_text(String(128, "[q]\r\n%s\r\n", gm.scriptText.str()), "q");
        if (c.getbool("vertex_opacity", 0))
          has_lmesh_transparency = true;
      }
    }


    const int shNameId = shadersMap.addNameId(gm.className);
    bool found = false;

    for (int i = 0; i < shaderNameIds.size(); ++i)
      if (shaderNameIds[i] == shNameId)
      {
        found = true;
        break;
      }

    if (!found)
      shaderNameIds.push_back(shNameId);

    Ptr<MaterialData> matNull = new MaterialData;
    matNull->matName = gm.name;
    matNull->matScript = gm.scriptText;
    matNull->className = gm.className;

    matNull->flags = (gm.flags & MatFlags::FLG_2SIDED) ? MATF_2SIDED : 0;

    matNull->mat.amb = gm.amb;
    matNull->mat.diff = gm.diff;
    matNull->mat.spec = gm.spec;
    matNull->mat.emis = gm.emis;
    matNull->mat.power = gm.power;

    static_geom_mat_subst.substMatClass(*matNull);
    compileLight(*matNull, node->lighting);

    for (int ti = 0; ti < StaticGeometryMaterial::NUM_TEXTURES && ti < MAXMATTEXNUM; ++ti) //-V560
      if (gm.textures[ti])
        matNull->mtex[ti] = ::add_managed_texture(gm.textures[ti]->fileName);

    int k;
    for (k = 0; k < allmats.size(); ++k)
      if (matNull->flags == allmats[k]->flags && !strcmp(matNull->matScript, allmats[k]->matScript) &&
          !strcmp(matNull->className, allmats[k]->className) && !memcmp(&matNull->mat, &allmats[k]->mat, sizeof(matNull->mat)))
      {
        bool equal = true;

        for (int ti = 0; ti < StaticGeometryMaterial::NUM_TEXTURES && ti < MAXMATTEXNUM; ++ti) //-V560
          if (matNull->mtex[ti] != allmats[k]->mtex[ti])
          {
            equal = false;
            break;
          }

        if (equal)
          break;
      }

    if (k < allmats.size())
    {
      reducedMatMap.push_back(k);
      matNull = NULL;
    }
    else
    {
      reducedMatMap.push_back(allmats.size());
      allmats.push_back(matNull);
    }
  }

  // compile meshes
  int id = 0;
  PtrTab<ShaderMaterial> mat(tmpmem);
  for (j = 0; j < node->mesh->mats.size(); ++j)
  {
    int i = reducedMatMap[id++];
    ShaderMaterial *sm;
    sm = new_shader_material(*allmats[i]);
    mat.push_back(sm);
  }

  Mesh mesh = node->mesh->mesh;

  // process real2sided
  ::split_two_sided(mesh, node->mesh->mats);

  SmallTab<Point3, TmpmemAlloc> orgVerts;
  if (node->flags & StaticGeometryNode::FLG_BILLBOARD_MESH)
    orgVerts = mesh.getVert();
  if (!(node->flags & StaticGeometryNode::FLG_NO_RECOMPUTE_NORMALS))
  {
    // fixme:
    //==
    mesh.calc_ngr();
    mesh.calc_vertnorms();
    //==
  }
  mesh.transform(node->wtm);
  if (node->flags & StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS)
  {
    Point3 center(node->wtm.getcol(3));
    for (int f = 0; f < mesh.face.size(); ++f)
      for (unsigned v = 0; v < 3; ++v)
        mesh.vertnorm[mesh.facengr[f][v]] = ::normalize(mesh.vert[mesh.face[f].v[v]] - center);
  }
  else if (node->flags & (StaticGeometryNode::FLG_FORCE_WORLD_NORMALS | StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS))
  {
    Point3 normal = node->normalsDir;

    if (node->flags & StaticGeometryNode::FLG_FORCE_WORLD_NORMALS)
      normal = ::normalize(normal);
    else if (node->flags & StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS)
      normal = ::normalize(node->wtm % normal);

    for (int i = 0; i < mesh.vertnorm.size(); ++i)
      mesh.vertnorm[i] = normal;
  }

  prepare_extra_for_landmesh(mesh, o->pos_to_world_ofs, o->pos_to_world_scale, has_lmesh_transparency);

  mesh.createTextureSpaceData(SCUSAGE_EXTRA, 50, 51);

  if (node->flags & StaticGeometryNode::FLG_BILLBOARD_MESH)
    ::prepare_billboard_mesh(mesh, node->wtm, orgVerts);

  if (!doNotChangeVertexColors)
    createVHarmLightmap(mesh, idx, node->lighting == StaticGeometryNode::LIGHT_NONE);

  // delete old shader mesh
  del_it(o->mesh);
  o->delVdata();

  // build shader mesh

  ShaderMeshData meshData;
  meshData.build(mesh, (ShaderMaterial **)&mat[0], mat.size(), IdentColorConvert::object);
  mat.clear();

  GlobalVertexDataConnector gvd;
  gvd.addMeshData(&meshData, Point3(0, 0, 0), 0);
  gvd.connectData(false, NULL);

  PtrTab<GlobalVertexDataSrc> vdata(tmpmem);
  vdata = gvd.getResult();

  memfree(o->vdata.data(), midmem);
  o->vdata.set((GlobalVertexData *)memalloc(sizeof(GlobalVertexData) * vdata.size(), midmem), vdata.size());
  for (int i = 0; i < vdata.size(); ++i)
    o->vdata[i].initGvdMem(vdata[i]->numv, vdata[i]->stride,
      vdata[i]->numv <= 65536 ? data_size(vdata[i]->iData) : data_size(vdata[i]->iData32), 0, vdata[i]->vData.data(),
      vdata[i]->numv <= 65536 ? (void *)vdata[i]->iData.data() : (void *)vdata[i]->iData32.data());


  o->mesh = meshData.makeShaderMesh(vdata, make_span(o->vdata));
  o->hasLandmesh = false;
  for (int mi = 0; mi < o->mesh->getAllElems().size() && !o->hasLandmesh; ++mi)
  {
    const ShaderMesh::RElem &re = o->mesh->getAllElems()[mi];
    dag::ConstSpan<ShaderChannelId> channels = re.e->getChannels();
    for (int ci = 0; ci < channels.size(); ++ci)
      if (channels[ci].u == SCUSAGE_EXTRA && channels[ci].ui == 7)
      {
        o->hasLandmesh = true;
        break;
      }
  }
}

void GeomObject::RenderObject::ObjectLod::delVdata()
{
  for (int i = 0; i < vdata.size(); i++)
    vdata[i].free();
  memfree(vdata.data(), midmem);
  vdata.reset();
}


//==============================================================================
void GeomObject::compileLight(MaterialData &mat_null, StaticGeometryNode::Lighting light)
{
  String script(mat_null.matScript);

  switch (light)
  {
    case StaticGeometryNode::LIGHT_NONE: StaticGeometryNode::setScriptLighting(script, "none"); break;

    case StaticGeometryNode::LIGHT_DEFAULT:
    {
      String lt;
      StaticGeometryNode::getScriptLighting(script, lt);
      if (stricmp(lt, "none"))
        StaticGeometryNode::setScriptLighting(script, "vltmap");
      break;
    }

    default: StaticGeometryNode::setScriptLighting(script, "vltmap");
  }

  mat_null.matScript = script;
}


//==============================================================================
void GeomObject::createVHarmLightmap(Mesh &m, int idx, bool none_lighting)
{
  MeshData &mesh = m.getMeshData();
  const StaticGeometryNode *node = geom->nodes[idx];
  const Point3 vertical(0.0, 1.0, 0.0);
  const Point3 minColor(toneMapper ? MINIMUM_COLOR_RED * toneMapper->getPostScale().x : MINIMUM_COLOR_RED,
    toneMapper ? MINIMUM_COLOR_GREEN * toneMapper->getPostScale().y : MINIMUM_COLOR_GREEN,
    toneMapper ? MINIMUM_COLOR_BLUE * toneMapper->getPostScale().z : MINIMUM_COLOR_BLUE);

  mesh.cvert.resize(mesh.face.size() * 3);
  mesh.cface.resize(mesh.face.size());

  for (int f = 0; f < mesh.face.size(); ++f)
  {
    for (unsigned v = 0; v < 3; ++v)
    {
      Point3 normal = mesh.vertnorm[mesh.facengr[f][v]];
      const int vi = f * 3 + v;

      Color4 color(toneMapper ? toneMapper->getPostScale().x : 1, toneMapper ? toneMapper->getPostScale().y : 1,
        toneMapper ? toneMapper->getPostScale().z : 1);

      if (!none_lighting)
      {
        Point3 ambCol = skyColor * (1 + normal * vertical) / 2 + earthColor * (1 - normal * vertical) / 2;
        Point3 p3col = (normal * lightingDir) * directColor + ambCol;

        color = Color4(p3col.x > 0.0 ? p3col.x : 0.0, p3col.y > 0.0 ? p3col.y : 0.0, p3col.z > 0.0 ? p3col.z : 0.0);

        if (toneMapper)
          color = toneMapper->mapColor(color);
      }

      if (color.r < minColor.x)
        color.r = minColor.x;

      if (color.g < minColor.y)
        color.g = minColor.y;

      if (color.b < minColor.z)
        color.b = minColor.z;

      mesh.cvert[vi] = color;
      mesh.cface[f].t[v] = vi;
    }
  }
}


//==============================================================================
Point3 GeomObject::getFarestPoint() const
{
  Point3 rez(0, 0, 0);
  if (geom)
  {
    for (int i = 0; i < geom->nodes.size(); ++i)
    {
      if (!geom->nodes[i] || !geom->nodes[i]->mesh)
        continue;

      const TMatrix locTm = tm * geom->nodes[i]->wtm;

      for (int j = 0; j < geom->nodes[i]->mesh->mesh.getVert().size(); ++j)
      {
        const Point3 vert = locTm * geom->nodes[i]->mesh->mesh.getVert()[j] - locTm * Point3(0, 0, 0);

        if (vert.length() > rez.length())
          rez = vert;
      }
    }
  }

  return rez;
}


//==============================================================================
Point3 GeomObject::getNearestPoint() const
{
  Point3 rez(MAX_REAL, MAX_REAL, MAX_REAL);

  if (geom)
  {
    for (int i = 0; i < geom->nodes.size(); ++i)
    {
      if (!geom->nodes[i] || !geom->nodes[i]->mesh)
        continue;

      const TMatrix locTm = tm * geom->nodes[i]->wtm;

      for (int j = 0; j < geom->nodes[i]->mesh->mesh.getVert().size(); ++j)
      {
        const Point3 vert = locTm * geom->nodes[i]->mesh->mesh.getVert()[j] - locTm * Point3(0, 0, 0);

        if (vert.length() < rez.length())
          rez = vert;
      }
    }
  }

  return rez;
}


//==============================================================================
BBox3 GeomObject::getNodeBoundBox(int idx, bool local_coord) const
{
  BBox3 ret;

  if (idx < 0 || idx >= geom->nodes.size())
    return ret;

  const StaticGeometryNode *node = geom->nodes[idx];

  if (!node || !node->mesh)
    return ret;

  TMatrix locTm;
  if (!local_coord)
    locTm = tm * node->wtm;
  else
    locTm = node->wtm;

  for (int i = 0; i < node->mesh->mesh.getVert().size(); ++i)
  {
    const Point3 vert = locTm * node->mesh->mesh.getVert()[i];
    ret += vert;
  }

  return ret;
}


//==============================================================================
BBox3 GeomObject::getBoundBox(bool local_coord)
{
  if (bounds.isempty() || local_coord != (bool)localBoundBox)
  {
    bounds.setempty();

    for (int i = 0; i < geom->nodes.size(); ++i)
      bounds += getNodeBoundBox(i, local_coord);

    localBoundBox = local_coord;
  }

  return bounds;
}


//==============================================================================
void GeomObject::eraseAllMats(bool clear) { clear ? clear_and_shrink(allmats) : allmats.clear(); }


//==============================================================================
bool GeomObject::fatalHandler(const char *msg, const char *stack_msg, const char *file, int line)
{
  String message(256, "%s,%d:\n    %s\n%s\n", file, line, msg, stack_msg);

  for (int i = 0; i < fatalMessages.size(); ++i)
    if (!stricmp(message, fatalMessages[i]))
      return false;

  fatalMessages.push_back(message);

  return false;
}


void GeomObject::RenderObject::beforeRenderCheck()
{
  if (lods.size() > 1)
  {
    real distSq = lengthSq(geom->getTm().getcol(3) - ::grs_cur_view.pos);

    int i;
    for (i = lods.size() - 1; i > 0; --i)
      if (lods[i].lodRangeSq < distSq)
        break;

    curLod = i;
  }
}

const char *filter_class_name = NULL;

void GeomObject::RenderObject::render()
{
  ShaderGlobal::set_real(hdrLightmapScaleId, LIGHTMAP_SCALE);

  beforeRenderCheck();
  static int lmesh_vs_const__pos_to_worldVarId = ::get_shader_variable_id("lmesh_vs_const__pos_to_world", true);
  static int lmesh_vs_const__pos_to_world = clamp(ShaderGlobal::get_int_fast(lmesh_vs_const__pos_to_worldVarId), 0, 512);
  if (lmesh_vs_const__pos_to_world >= 0 && lods[curLod].hasLandmesh)
  {
    d3d::set_vs_const1(lmesh_vs_const__pos_to_world, lods[curLod].pos_to_world_scale.x, lods[curLod].pos_to_world_scale.y,
      lods[curLod].pos_to_world_scale.z, 0);
    d3d::set_vs_const1(lmesh_vs_const__pos_to_world + 1, lods[curLod].pos_to_world_ofs.x, lods[curLod].pos_to_world_ofs.y,
      lods[curLod].pos_to_world_ofs.z, 1);
  }
  if (filter_class_name)
  {
    for (int mi = 0; mi < lods[curLod].mesh->getAllElems().size(); ++mi)
      if (strstr(lods[curLod].mesh->getAllElems()[mi].mat->getShaderClassName(), filter_class_name))
      {
        const ShaderMesh::RElem &re = lods[curLod].mesh->getAllElems()[mi];
        re.vertexData->setToDriver();
        re.e->render(re.sv, re.numv, re.si, re.numf);
      }
  }
  else
  {
    if (lmesh_vs_const__pos_to_world >= 0 && lods[curLod].hasLandmesh)
    {
      d3d::settm(TM_WORLD, geom->getTm());
      for (int mi = 0; mi < lods[curLod].mesh->getAllElems().size(); ++mi)
      {
        const ShaderMesh::RElem &re = lods[curLod].mesh->getAllElems()[mi];
        re.vertexData->setToDriver();
        re.e->render(re.sv, re.numv, re.si, re.numf);
      }
    }
    else
      lods[curLod].mesh->render();
  }
  if (lmesh_vs_const__pos_to_world >= 0 && lods[curLod].hasLandmesh)
  {
    d3d::set_vs_const1(lmesh_vs_const__pos_to_world, 1, 1, 1, 0);
    d3d::set_vs_const1(lmesh_vs_const__pos_to_world + 1, 0, 0, 0, 0);
    ShaderElement::invalidate_cached_state_block();
  }
}


void GeomObject::RenderObject::renderTrans()
{
  static int lmesh_vs_const__pos_to_worldVarId = ::get_shader_variable_id("lmesh_vs_const__pos_to_world", true);
  static int lmesh_vs_const__pos_to_world = clamp(ShaderGlobal::get_int_fast(lmesh_vs_const__pos_to_worldVarId), 0, 512);
  if (lmesh_vs_const__pos_to_world >= 0 && lods[curLod].hasLandmesh)
  {
    d3d::set_vs_const1(lmesh_vs_const__pos_to_world, lods[curLod].pos_to_world_scale.x, lods[curLod].pos_to_world_scale.y,
      lods[curLod].pos_to_world_scale.z, 0);
    d3d::set_vs_const1(lmesh_vs_const__pos_to_world + 1, lods[curLod].pos_to_world_ofs.x, lods[curLod].pos_to_world_ofs.y,
      lods[curLod].pos_to_world_ofs.z, 1);
  }
  ShaderGlobal::set_real(hdrLightmapScaleId, LIGHTMAP_SCALE);

  lods[curLod].mesh->render_trans();
  if (lmesh_vs_const__pos_to_world >= 0 && lods[curLod].hasLandmesh)
  {
    d3d::set_vs_const1(lmesh_vs_const__pos_to_world, 1, 1, 1, 0);
    d3d::set_vs_const1(lmesh_vs_const__pos_to_world + 1, 0, 0, 0, 0);
    ShaderElement::invalidate_cached_state_block();
  }
}


ShaderMesh *GeomObject::getMesh(int node_id) const
{
  for (int objNo = 0; objNo < shMesh.size(); objNo++)
  {
    for (int lodNo = 0; lodNo < shMesh[objNo].lods.size(); lodNo++)
      if (shMesh[objNo].lods[lodNo].nodeID == node_id)
        return shMesh[objNo].lods[lodNo].mesh;
  }
  return NULL;
}
