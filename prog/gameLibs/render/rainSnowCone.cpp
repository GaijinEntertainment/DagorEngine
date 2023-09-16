#include <render/rainSnowCone.h>
#include <gameRes/dag_gameResources.h>
#include <ioSys/dag_dataBlock.h>
#include <3d/dag_materialData.h>
#include <3d/dag_render.h>
#include <math/dag_TMatrix.h>
#include <math/dag_Point4.h>
#include <3d/dag_drv3d.h>
#include <render/shaderBlockIds.h>


RainSnowCone::RainSnowCone(const DataBlock *blk, const Point3 &camera_pos) :
  xzMoveDir(1.f, 0.f, 0.f), coneDir(0.f, 1.f, 0.f), prevCameraPos(camera_pos), rainColorMult(1.f, 1.f, 1.f, 1.f)
{
  G_ASSERT(blk);

  memset(offset, 0, sizeof(offset));

  // Settings.

  length = blk->getReal("length", 20.f);
  radius = blk->getReal("radius", 10.f);
  cameraSmooth = blk->getReal("cameraSmooth", 0.5f);
  stretch = blk->getReal("stretch", 1.f);
  scroll = blk->getReal("scroll", 0.f);
  scrollGravitation = blk->getReal("scrollGravitation", 0.1f);
  stretchCenter = blk->getReal("stretchCenter", 0.3f);

  layerScale[0] = blk->getReal("layerScale0", 1.f);
  layerScale[1] = blk->getReal("layerScale1", 1.5f);
  layerScale[2] = blk->getReal("layerScale2", 2.f);
  layerScale[3] = blk->getReal("layerScale3", 3.f);

  maxVelCamera = blk->getReal("maxVelCamera", 100.f);
  velGravitation = blk->getReal("velGravitation", 100000.f);

  ShaderGlobal::set_real(get_shader_variable_id("cone_alpha"), blk->getReal("alpha", 4.f));

  color = color4(blk->getE3dcolor("color", 0xFFFFFFFF));
  ShaderGlobal::set_color4(get_shader_variable_id("cone_color"), color);

  coneTexId[0] = ::get_tex_gameres(blk->getStr("tex0", "cone0"));
  G_ASSERT(coneTexId[0] != BAD_TEXTUREID);

  coneTexId[1] = ::get_tex_gameres(blk->getStr("tex1", "cone1"));
  G_ASSERT(coneTexId[1] != BAD_TEXTUREID);

  coneTexId[2] = ::get_tex_gameres(blk->getStr("tex2", "cone2"));
  G_ASSERT(coneTexId[2] != BAD_TEXTUREID);

  coneTexId[3] = ::get_tex_gameres(blk->getStr("tex3", "cone3"));
  G_ASSERT(coneTexId[3] != BAD_TEXTUREID);


  // Shader vars.

  coneTexVarId[0] = get_shader_variable_id("cone_tex_0");
  coneTexVarId[1] = get_shader_variable_id("cone_tex_1");
  coneTexVarId[2] = get_shader_variable_id("cone_tex_2");
  coneTexVarId[3] = get_shader_variable_id("cone_tex_3");
  texcoordVarId[0] = get_shader_variable_id("cone_texcoord_0");
  texcoordVarId[1] = get_shader_variable_id("cone_texcoord_1");
  texcoordVarId[2] = get_shader_variable_id("cone_texcoord_2");
  texcoordVarId[3] = get_shader_variable_id("cone_texcoord_3");


  // Create material.

  Ptr<MaterialData> matNull = new MaterialData;
  static CompiledShaderChannelId chan[2] = {
    {SCTYPE_FLOAT3, SCUSAGE_POS, 0, 0},
    {SCTYPE_FLOAT4, SCUSAGE_TC, 0, 0},
  };

  matNull->className = "rain_snow_cone";
  material = new_shader_material(*matNull);
  G_ASSERT(material);
  material->addRef();
  matNull = NULL;

  G_ASSERT(material->checkChannels(chan, sizeof(chan) / sizeof(chan[0])));

  rendElem.shElem = material->make_elem();
  rendElem.vDecl = dynrender::addShaderVdecl(chan, sizeof(chan) / sizeof(chan[0]));
  G_ASSERT(rendElem.vDecl != BAD_VDECL);
  rendElem.stride = dynrender::getStride(chan, sizeof(chan) / sizeof(chan[0]));
  rendElem.minVert = 0;
  rendElem.startIndex = 0;


  // Create cone geometry.

  unsigned int numSegments = blk->getInt("numSegments", 20);
  rendElem.numVert = 3 * (numSegments + 1);
  rendElem.numPrim = 4 * numSegments;

  vb = d3d::create_vb(rendElem.numVert * rendElem.stride, 0, __FILE__);
  G_ASSERT(vb);

  struct Vertex
  {
    Point3 pos;
    Point4 texcoord;
  } *vertices;

  vb->lock(0, rendElem.numVert * rendElem.stride, (void **)&vertices, 0);

  float endRadius = 0.01f;
  float vPresp = 0.3f;
  for (unsigned int segmentNo = 0; segmentNo < numSegments + 1; segmentNo++)
  {
    float angleSin = sinf(TWOPI * segmentNo / numSegments);
    float angleCos = cosf(TWOPI * segmentNo / numSegments);
    float u = (float)segmentNo / numSegments;

    vertices[segmentNo].pos = Point3(endRadius * angleSin, 1.f, endRadius * angleCos);
    vertices[segmentNo].texcoord = Point4(u, 0.f, endRadius, vPresp);

    vertices[segmentNo + numSegments + 1].pos = Point3(angleSin, 0.f, angleCos);
    vertices[segmentNo + numSegments + 1].texcoord = Point4(u, 0.5f, 1.f, 1.f);

    vertices[segmentNo + 2 * (numSegments + 1)].pos = Point3(endRadius * angleSin, -1.f, endRadius * angleCos);
    vertices[segmentNo + 2 * (numSegments + 1)].texcoord = Point4(u, 1.f, endRadius, vPresp);
  }

  vb->unlock();

  ib = d3d::create_ib(rendElem.numPrim * 3 * sizeof(unsigned short int), 0);
  G_ASSERT(ib);

  unsigned short int *indices;
  ib->lock(0, rendElem.numPrim * 3 * sizeof(unsigned short int), &indices, 0);

  unsigned int indexNo = 0;
  for (unsigned int segmentNo = 0; segmentNo < numSegments; segmentNo++)
  {
    indices[indexNo++] = segmentNo;
    indices[indexNo++] = segmentNo + 1;
    indices[indexNo++] = numSegments + 1 + segmentNo;

    indices[indexNo++] = numSegments + 1 + segmentNo;
    indices[indexNo++] = segmentNo + 1;
    indices[indexNo++] = numSegments + 1 + segmentNo + 1;

    indices[indexNo++] = numSegments + 1 + segmentNo;
    indices[indexNo++] = numSegments + 1 + segmentNo + 1;
    indices[indexNo++] = 2 * (numSegments + 1) + segmentNo;

    indices[indexNo++] = 2 * (numSegments + 1) + segmentNo;
    indices[indexNo++] = numSegments + 1 + segmentNo + 1;
    indices[indexNo++] = 2 * (numSegments + 1) + segmentNo + 1;
  }

  ib->unlock();
}


RainSnowCone::~RainSnowCone()
{
  material->delRef();
  del_d3dres(vb);
  del_d3dres(ib);
  for (unsigned int layerNo = 0; layerNo < NUM_LAYERS; layerNo++)
    release_managed_tex(coneTexId[layerNo]);
}


void RainSnowCone::update(float dt, const Point3 &camera_pos)
{
  Point3 velCamera = camera_pos - prevCameraPos;
  prevCameraPos = camera_pos;

  Point3 xzVelCamera = velCamera;
  xzVelCamera.y = 0.f;
  if (xzVelCamera.lengthSq() > 4.f * dt * dt)
    xzMoveDir = normalize(xzVelCamera);

  float velLength = velCamera.length();
  if (velLength > maxVelCamera)
    velCamera *= maxVelCamera / velLength;

  coneDir = velCamera + Point3(0.f, velGravitation, 0.f);
  if (coneDir.lengthSq() < 0.00001f)
    coneDir = Point3(0.f, 1.f, 0.f);

  ShaderGlobal::set_color4(get_shader_variable_id("cone_color"), color * rainColorMult);

  float cameraSpeed = velCamera.length();
  for (unsigned int layerNo = 0; layerNo < NUM_LAYERS; layerNo++)
  {
    float scale = layerScale[layerNo] / (stretch * cameraSpeed + 1.f);
    float speed = scrollGravitation + scroll * cameraSpeed * (normalize(velCamera) * normalize(coneDir));

    offset[layerNo] -= speed * dt;
    offset[layerNo] = fmodf(offset[layerNo], 1.f);

    ShaderGlobal::set_color4(texcoordVarId[layerNo], layerScale[layerNo], scale, 0.f, offset[layerNo] - stretchCenter * scale);
  }
}


void RainSnowCone::render()
{
  if (rainColorMult.a < 0.001)
    return;

  TMatrix scale(TMatrix::IDENT);
  scale.setcol(0, radius, 0.f, 0.f);
  scale.setcol(1, 0.f, length, 0.f);
  scale.setcol(2, 0.f, 0.f, radius);

  TMatrix rotate(TMatrix::IDENT);
  rotate.setcol(1, normalize(coneDir));
  rotate.setcol(0, normalize(xzMoveDir % Point3(0.f, 1.f, 0.f)));
  rotate.setcol(2, normalize(rotate.getcol(0) % rotate.getcol(1)));
  rotate.setcol(3, ::grs_cur_view.itm.getcol(3));
  d3d::settm(TM_WORLD, rotate * scale);

  ShaderGlobal::setBlock(shaderblocks::forwardSceneBlkId, ShaderGlobal::LAYER_SCENE);

  for (unsigned int layerNo = 0; layerNo < NUM_LAYERS; layerNo++)
    ShaderGlobal::set_texture(coneTexVarId[layerNo], coneTexId[layerNo]);

  d3d::setvdecl(rendElem.vDecl);
  d3d::setvsrc(0, vb, rendElem.stride);
  d3d::setind(ib);

  rendElem.shElem->render(0, rendElem.numVert, 0, rendElem.numPrim);

  d3d::settm(TM_WORLD, TMatrix::IDENT);
  ShaderGlobal::setBlock(shaderblocks::forwardSceneBlkId, ShaderGlobal::LAYER_SCENE);
}
