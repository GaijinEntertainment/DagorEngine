// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_color.h>
#include <generic/dag_carray.h>
#include <generic/dag_staticTab.h>
#include <math/dag_hlsl_floatx.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_shaderBlock.h>
#include <3d/dag_resPtr.h>
#include <projectors/shaders/projectors.hlsli>

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/updateStage.h>
#include <ecs/render/updateStageRender.h>

#include <math/dag_mathBase.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_driver.h>
#include <math/dag_hlsl_floatx.h>
#include <daECS/core/coreEvents.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>


constexpr const char *projectorsBufferName = "projectors_data";

class ProjectorsManager
{
public:
  struct ProjectorRec;

  ProjectorsManager()
  {
    if (!projRenderer.init("projectors", nullptr, 0, "projectors", true))
      return;

    projCbuf = dag::buffers::create_one_frame_cb(dag::buffers::cb_array_reg_count<Projector>(MAX_PROJECTORS), projectorsBufferName);

    atmosphereParamsVarId = ::get_shader_variable_id("atmosphere_params", true);
    atmosphereShiftVarId = ::get_shader_variable_id("atmosphere_shift", true);
  }
  ~ProjectorsManager()
  {
    projectors.clear();
    projRenderer.close();
  }

  int addProjector(Point3 pos, Point3 dir, Point3 color, float angle, float length, float sourceWidth)
  {
    Projector proj;
    proj.pos_angle = float4(pos.x, pos.y, pos.z, angle);
    Point3 normDir = normalize(dir);
    proj.dir_length = bitwise_cast<uint2>(Half4(normDir.x, normDir.y, normDir.z, length));
    proj.color_sourceWidth = bitwise_cast<uint2>(Half4(color.x, color.y, color.z, sourceWidth));
    return addProjector(proj);
  }

  int addProjector(const Projector &projLight)
  {
    if (projectors.size() >= MAX_PROJECTORS)
      return -1;

    projectors.push_back(projLight);
    needUpdate = true;
    return projectors.size() - 1;
  }

  void destroyAllProjectors()
  {
    projectors.clear();
    needUpdate = true;
  }

  void render()
  {
    if (!projectors.size() || !projRenderer.material)
      return;
    if (needUpdate)
      updateProjectors();
    projRenderer.shader->setStates(0, true);
    d3d::setvsrc(0, 0, 0);
    d3d::draw(PRIM_TRILIST, 0, projectors.size());
  }

  void setPos(uint32_t id, const Point3 &pos)
  {
    memcpy(&projectors[id].pos_angle, &pos, sizeof(pos)); //-V512 We really want to copy only 3 float
    needUpdate = true;
  }

  void setDir(uint32_t id, const Point3 &dir)
  {
    Point3 normDir = normalize(dir);
    uint16_t packed[3];
    packed[0] = float_to_half(normDir.x);
    packed[1] = float_to_half(normDir.y);
    packed[2] = float_to_half(normDir.z);
    memcpy(&projectors[id].dir_length, packed, sizeof(packed)); //-V512 We really want to copy only 3 uint16_t
    needUpdate = true;
  }

  void setLightColor(uint32_t id, const Color3 &color)
  {
    uint16_t packed[3];
    packed[0] = float_to_half(color.r);
    packed[1] = float_to_half(color.g);
    packed[2] = float_to_half(color.b);
    memcpy(&projectors[id].color_sourceWidth, packed, sizeof(packed)); //-V512 We really want to copy only 3 uint16_t
    needUpdate = true;
  }

  void setAngle(uint32_t id, float angle)
  {
    memcpy(&projectors[id].pos_angle.w, &angle, sizeof(angle));
    needUpdate = true;
  }

  void setLength(uint32_t id, float length)
  {
    uint16_t packed = float_to_half(length);
    memcpy(&projectors[id].dir_length.y + 2, &packed, sizeof(packed));
    needUpdate = true;
  }

  void setSourceWidth(uint32_t id, float width)
  {
    uint16_t packed = float_to_half(width);
    memcpy(&projectors[id].color_sourceWidth.y + 2, &packed, sizeof(packed));
    needUpdate = true;
  }

  void setAtmosphereParams(float atmosphereDensity, float noiseScale, float noiseStrength)
  {
    ShaderGlobal::set_color4_fast(atmosphereParamsVarId, Color4(atmosphereDensity, noiseScale, noiseStrength, 0.0f));
  }

  void setAtmosphereShift(const Point3 shift)
  {
    ShaderGlobal::set_color4_fast(atmosphereShiftVarId, Color4(shift.x, shift.y, shift.z, 0.0f));
  }

private:
  void updateProjectors()
  {
    if (projectors.size())
    {
      projCbuf.getBuf()->updateData(0, projectors.size() * sizeof(Projector), (const void *)projectors.data(), VBLOCK_DISCARD);
    }
    needUpdate = false;
  }

  StaticTab<Projector, MAX_PROJECTORS> projectors;
  UniqueBufHolder projCbuf;
  DynamicShaderHelper projRenderer;
  bool needUpdate = false;
  int atmosphereParamsVarId = -1;
  int atmosphereShiftVarId = -1;
  int globalFrameBlockId = ShaderGlobal::getBlockId("global_frame");
};

ECS_DECLARE_BOXED_TYPE(ProjectorsManager);
ECS_REGISTER_BOXED_TYPE(ProjectorsManager, nullptr);
ECS_AUTO_REGISTER_COMPONENT(ProjectorsManager, "projectors_manager", nullptr, 0);

template <typename Callable>
inline void projector_update_ecs_query(Callable fn);

template <typename Callable>
inline void add_projector_ecs_query(Callable fn);

static void add_projector(ecs::EntityId eid,
  ProjectorsManager &projectors_manager,
  int &projector__id,
  const Point3 &projector__color,
  float projector__angle,
  float projector__length,
  float projector__sourceWidth,
  float &projector__xAngleAmplitude,
  float &projector__zAngleAmplitude,
  TMatrix &transform)
{
  Point3 xAxisDir = transform.getcol(0);
  Point3 dir = transform.getcol(1);
  Point3 zAxisDir = normalize(cross(xAxisDir, dir));
  xAxisDir = normalize(cross(dir, zAxisDir));
  transform.setcol(0, xAxisDir);
  transform.setcol(1, normalize(cross(zAxisDir, xAxisDir)));
  transform.setcol(2, zAxisDir);

  if (abs(projector__xAngleAmplitude) >= 90.0f)
  {
    logwarn("Invalid value: projector.xAngleAmplitude = %f; changed to 0", projector__xAngleAmplitude);
    projector__xAngleAmplitude = 0.0;
  }
  if (abs(projector__zAngleAmplitude) >= 90.0f)
  {
    logwarn("Invalid value: projector.zAngleAmplitude = %f; changed to 0", projector__zAngleAmplitude);
    projector__zAngleAmplitude = 0.0;
  }

  projector__id = projectors_manager.addProjector(transform.getcol(3), dir, projector__color, DegToRad(projector__angle),
    projector__length, projector__sourceWidth);

  if (projector__id < 0)
    g_entity_mgr->destroyEntity(eid);
}

#define TRANSPARENCY_NODE_PRIORITY_PROJECTORS 0
ECS_TAG(render)
ECS_ON_EVENT(on_appear)
void init_manager_es_event_handler(const ecs::Event &,
  ProjectorsManager &projectors_manager,
  dafg::NodeHandle &projectors_node,
  Point3 &projectors_manager__atmosphereMoveDir,
  float projectors_manager__atmosphereDensity = 1.0f,
  float projectors_manager__noiseScale = 1.0f,
  float projectors_manager__noiseStrength = 0.0f)
{
  float len = length(projectors_manager__atmosphereMoveDir);
  if (len > 0.0001f)
    projectors_manager__atmosphereMoveDir /= len;
  else
    projectors_manager__atmosphereMoveDir = Point3(0.0f, 0.0f, 0.0f);
  projectors_manager.setAtmosphereParams(projectors_manager__atmosphereDensity, projectors_manager__noiseScale,
    projectors_manager__noiseStrength);

  add_projector_ecs_query([&projectors_manager](ecs::EntityId eid, int &projector__id, const Point3 &projector__color,
                            float projector__angle, float projector__length, float projector__sourceWidth,
                            float &projector__xAngleAmplitude, float &projector__zAngleAmplitude, TMatrix &transform) {
    add_projector(eid, projectors_manager, projector__id, projector__color, projector__angle, projector__length,
      projector__sourceWidth, projector__xAngleAmplitude, projector__zAngleAmplitude, transform);
  });
  auto nodeNs = dafg::root() / "transparent" / "far";
  projectors_node = nodeNs.registerNode("projectors", DAFG_PP_NODE_SRC, [&projectors_manager](dafg::Registry registry) {
    registry.requestRenderPass().color({"color_target"}).depthRoAndBindToShaderVars("depth", {"depth_gbuf"});
    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_PROJECTORS);

    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    use_current_camera(registry);

    registry.requestState().setFrameBlock("global_frame");

    return [&projectors_manager] {
      projectors_manager.render(); // TODO: Don't store this logic in manager
    };
  });
}

ECS_TAG(render)
ECS_AFTER(update_projector_es)
void update_projectors_atmosphere_es(const ecs::UpdateStageInfoAct &act,
  ProjectorsManager &projectors_manager,
  const Point3 &projectors_manager__atmosphereMoveDir,
  float projectors_manager__atmosphereMoveSpeed)
{
  projectors_manager.setAtmosphereShift(projectors_manager__atmosphereMoveDir * act.curTime * projectors_manager__atmosphereMoveSpeed);
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
void create_projector_es_event_handler(const ecs::Event &,
  ecs::EntityId eid,
  int &projector__id,
  const Point3 &projector__color,
  float projector__angle,
  float projector__length,
  float projector__sourceWidth,
  float &projector__xAngleAmplitude,
  float &projector__zAngleAmplitude,
  TMatrix &transform)
{
  projector_update_ecs_query([&](ProjectorsManager &projectors_manager) {
    add_projector(eid, projectors_manager, projector__id, projector__color, projector__angle, projector__length,
      projector__sourceWidth, projector__xAngleAmplitude, projector__zAngleAmplitude, transform);
  });
}

ECS_TAG(render)
ECS_TRACK(projector__color)
void update_projector_color_changed_es(const ecs::Event &, int projector__id, const Point3 &projector__color)
{
  projector_update_ecs_query([&](ProjectorsManager &projectors_manager) {
    projectors_manager.setLightColor(projector__id, Color3(projector__color.x, projector__color.y, projector__color.z));
  });
}

ECS_TAG(render)
ECS_NO_ORDER
void update_projector_es(const ecs::UpdateStageInfoAct &act,
  int projector__id,
  float &projector__phase,
  float projector__period,
  float projector__xAngleAmplitude,
  float projector__zAngleAmplitude,
  const TMatrix &transform)
{
  projector_update_ecs_query([&](ProjectorsManager &projectors_manager) {
    float xAmplitude = tanf(DegToRad(projector__xAngleAmplitude));
    float zAmplitude = tanf(DegToRad(projector__zAngleAmplitude));

    Point3 dir = transform.getcol(0) * xAmplitude * cos(projector__phase) + transform.getcol(1) +
                 transform.getcol(2) * zAmplitude * sin(projector__phase);

    projectors_manager.setDir(projector__id, dir);
    projectors_manager.setPos(projector__id, transform.getcol(3));

    projector__phase += act.dt * PI * 2.0f / projector__period;
  });
}