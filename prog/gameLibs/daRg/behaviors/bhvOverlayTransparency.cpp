#include "bhvOverlayTransparency.h"

#include <perfMon/dag_statDrv.h>
#include <daRg/dag_element.h>
#include <daRg/dag_stringKeys.h>
#include "guiScene.h"

#include <workCycle/dag_workCycle.h>

#define MIN_OPACITY          0.15f
#define OPACITY_CHANGE_TIME  0.35f
#define OPACITY_CHANGE_SPEED (1.f / OPACITY_CHANGE_TIME)

namespace darg
{

struct TransparentOnOverlay
{
  float priority;
  Element *elem;
  BBox2 bbox;
};

typedef dag::Vector<TransparentOnOverlay> TransparentOnOverlayVec;
static TransparentOnOverlayVec tr_info[2];
static int cur_idx = 0;
static int past_idx = 1 - cur_idx;
static int cur_frame = -1;


static void on_new_frame()
{
  past_idx = cur_idx;
  cur_idx = 1 - cur_idx;
  tr_info[cur_idx].clear();
}


static bool check_overlayed_past_frame(const TransparentOnOverlay &new_tr)
{
  const TransparentOnOverlay *pastMe = nullptr;
  for (const TransparentOnOverlay &past : tr_info[past_idx])
    if (past.elem == new_tr.elem)
    {
      pastMe = &past;
      break;
    }

  if (!pastMe)
    return false;

  for (const TransparentOnOverlay &past : tr_info[past_idx])
    if (past.priority > pastMe->priority && past.elem != new_tr.elem && !non_empty_boxes_not_intersect(past.bbox, pastMe->bbox))
    {
      return true;
    }

  return false;
}


BhvOverlayTransparency bhv_overlay_transparency;


BhvOverlayTransparency::BhvOverlayTransparency() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0) {}


int BhvOverlayTransparency::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  TIME_PROFILE(bhv_overlay_transparency_update);

  Sqrat::Table &pprops = elem->props.storage;

  GuiScene *guiScene = GuiScene::get_from_sqvm(pprops.GetVM());
  G_ASSERT(guiScene);
  int frame = guiScene->getFramesCount();

  if (frame != cur_frame)
  {
    cur_frame = frame;
    on_new_frame();
  }

  if (elem->isHidden())
  {
    elem->props.setCurrentOpacity(1.f);
  }
  else
  {
    TransparentOnOverlay tr;
    tr.priority = pprops.RawGetSlotValue<float>(elem->csk->priority, VERY_BIG_NUMBER);

    Sqrat::Object data = elem->props.scriptDesc.RawGetSlot(elem->csk->data);
    if (!data.IsNull())
      tr.priority += data.RawGetSlotValue<float>(elem->csk->priorityOffset, 0);

    tr.elem = elem;
    tr.bbox = elem->transformedBbox;

    float opacity = elem->props.getCurrentOpacity();

    bool underlayed = check_overlayed_past_frame(tr);
    tr_info[cur_idx].push_back(tr);

    if (underlayed && opacity > MIN_OPACITY)
    {
      opacity = max(opacity - ::dagor_game_act_time * OPACITY_CHANGE_SPEED, MIN_OPACITY);
      elem->props.setCurrentOpacity(opacity);
    }
    else if (!underlayed && opacity < 1.f)
    {
      opacity = min(opacity + ::dagor_game_act_time * OPACITY_CHANGE_SPEED, 1.f);
      elem->props.setCurrentOpacity(opacity);
    }
    float opacityCenterMinMult = data.RawGetSlotValue<float>(elem->csk->opacityCenterMinMult, -1.f);
    float opacityCenterRelativeDist = data.RawGetSlotValue<float>(elem->csk->opacityCenterRelativeDist, -1.f);
    if (opacityCenterMinMult > 0.f && opacityCenterRelativeDist > 0.f && opacity > MIN_OPACITY)
    {
      Point2 screenSize = StdGuiRender::screen_size();
      Point2 screenCenter = screenSize * 0.5f;
      Point2 elemPos = elem->transformedBbox.center();
      Point2 centerRelativeDist = elemPos - screenCenter;
      centerRelativeDist.x = safediv(centerRelativeDist.x, screenSize.x);
      centerRelativeDist.y = safediv(centerRelativeDist.y, screenSize.y);
      float elemCenterRelativeDist = length(centerRelativeDist);
      if (elemCenterRelativeDist > opacityCenterRelativeDist)
        return 0;
      float opacityValue = max(safediv(elemCenterRelativeDist, opacityCenterRelativeDist), opacityCenterMinMult);
      opacity = min(opacityValue, opacity);
      elem->props.setCurrentOpacity(opacity);
    }
  }

  return 0;
}

} // namespace darg
