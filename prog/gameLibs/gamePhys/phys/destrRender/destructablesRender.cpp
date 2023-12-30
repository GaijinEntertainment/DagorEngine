#include <gamePhys/phys/destructableRendObject.h>
#include <render/dynmodelRenderer.h>
#include <shaders/dag_dynSceneRes.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_render.h>
#include <phys/dag_physDecl.h>
#include <phys/dag_physObject.h>
#include <gamePhys/phys/destructableObject.h>
#include <math/dag_frustum.h>

void destructables::before_render(const Point3 &view_pos, bool has_motion_vectors)
{
  for (const auto destr : destructables::getDestructableObjects())
  {
    if (destr->isAlive())
    {
      destr->physObj->beforeRender(view_pos);
      if (has_motion_vectors)
      {
        for (const DestrRendData::RendData &rdata : destr->rendData->rendData)
        {
          if (rdata.inst)
            rdata.inst->savePrevNodeWtm();
        }
      }
    }
  }
}

void destructables::render(dynrend::ContextId inst_ctx, const Frustum &frustum, float min_bbox_radius)
{
  static int instanceInitPosVarId = get_shader_variable_id("instance_init_pos_const_no", true);
  static int instanceInitPosConstNo = instanceInitPosVarId >= 0 ? ShaderGlobal::get_int_fast(instanceInitPosVarId) : -1;

  float min_bbox_r2 = min_bbox_radius * min_bbox_radius;

  for (const auto destr : destructables::getDestructableObjects())
  {
    if (!destr->isAlive())
      continue;
    const Point4 *itm = destr->intialTmAndHash;
    Point4 initialPos(itm[0].w, itm[1].w, itm[2].w, 0.f); // mat43f
    if (dynrend::is_initialized())
    {
      if (destr->rendData)
      {
        for (const DestrRendData::RendData &rdata : destr->rendData->rendData)
        {
          if (!rdata.inst)
            continue;

          const BSphere3 &boundingSphere = rdata.inst->getBoundingSphere();

          if (
            !frustum.testSphere(boundingSphere) || (min_bbox_r2 > 0 && rdata.inst->getBoundingBox().width().lengthSq() < min_bbox_r2))
            continue;

          dynrend::PerInstanceRenderData renderData;
          renderData.params.push_back(ZERO<Point4>());
          renderData.params.push_back(initialPos);
          dynrend::add(inst_ctx, rdata.inst, rdata.initialNodes, &renderData);
        }
      }
    }
    else
    {
      if (instanceInitPosConstNo >= 0)
        d3d::set_vs_const(instanceInitPosConstNo, &initialPos.x, 1);
      destr->physObj->render();
    }
  }
}

destructables::DestrRendData *destructables::init_rend_data(DynamicPhysObjectClass<PhysWorld> *phys_obj)
{
  DestrRendData *rdata = new DestrRendData();
  clear_and_resize(rdata->rendData, phys_obj->getModelCount());
  for (int i = 0; i < phys_obj->getModelCount(); ++i)
  {
    rdata->rendData[i].inst = phys_obj->getModel(i);
    rdata->rendData[i].initialNodes = (dynrend::is_initialized() && rdata->rendData[i].inst && phys_obj->getData()->skeleton)
                                        ? new dynrend::InitialNodes(rdata->rendData[i].inst, phys_obj->getData()->skeleton)
                                        : NULL;
  }
  return rdata;
}

void destructables::clear_rend_data(destructables::DestrRendData *data)
{
  if (!data)
    return;
  for (int i = 0; i < data->rendData.size(); ++i)
    delete data->rendData[i].initialNodes;
  del_it(data);
}
