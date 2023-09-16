#include "bhvMoveToArea.h"

#include <daRg/dag_element.h>
#include <daRg/dag_transform.h>
#include <daRg/dag_stringKeys.h>

#include <perfMon/dag_cpuFreq.h>

#include "guiScene.h"


namespace darg
{


BhvMoveToArea bhv_move_to_area;


BhvMoveToArea::BhvMoveToArea() : Behavior(STAGE_BEFORE_RENDER, 0) {}


void BhvMoveToArea::onAttach(Element *elem) { elem->setHidden(true); }


int BhvMoveToArea::update(UpdateStage /*stage*/, darg::Element *elem, float dt)
{
  G_ASSERT_RETURN(elem->parent, 0);

  BhvMoveToAreaTarget *target = elem->props.scriptDesc.RawGetSlotValue<BhvMoveToAreaTarget *>(elem->csk->target, nullptr);

  bool wasHidden = elem->isHidden();
  if (!target || target->targetRect.isempty())
  {
    elem->setHidden(true);
    return wasHidden ? 0 : R_REBUILD_RENDER_AND_INPUT_LISTS;
  }

  ScreenCoord &sc = elem->screenCoord;
  ScreenCoord &psc = elem->parent->screenCoord;
  Layout &layout = elem->layout;
  Layout &playout = elem->parent->layout;
  Point2 offs = ::max(layout.margin.lt(), playout.padding.lt());

  elem->setHidden(false);
  if (wasHidden)
  {
    elem->layout.hPlacement = ALIGN_LEFT;
    elem->layout.vPlacement = ALIGN_TOP;
    elem->layout.pos[0].mode = SizeSpec::PIXELS;
    elem->layout.pos[0].value = target->targetRect.left() - psc.screenPos.x - offs.x;
    elem->layout.pos[1].mode = SizeSpec::PIXELS;
    elem->layout.pos[1].value = target->targetRect.top() - psc.screenPos.y - offs.y;
    elem->layout.size[0].mode = SizeSpec::PIXELS;
    elem->layout.size[0].value = target->targetRect.width().x;
    elem->layout.size[1].mode = SizeSpec::PIXELS;
    elem->layout.size[1].value = target->targetRect.width().y;
    elem->recalcLayout();
  }
  else
  {
    Point2 dLt = target->targetRect.leftTop() - sc.screenPos;
    Point2 dRb = target->targetRect.rightBottom() - (sc.screenPos + sc.size);
    float distLt = length(dLt);
    float distRb = length(dRb);
    if (distLt > 0.1f || distRb > 0.1f)
    {
      Point2 newLt, newRb;

      if (elem->props.scriptDesc.RawHasKey(elem->csk->speed))
      {
        // allow to use single parameter for speed
        float baseSpeed = ::max(1.0f, elem->props.getFloat(elem->csk->speed, 100.0f));
        float speedLt = baseSpeed, speedRb = baseSpeed;
        float safetyEps = 1.0f;
        if (distLt > distRb)
          speedLt = baseSpeed * (distLt + safetyEps) / (distRb + safetyEps);
        else if (distRb > distLt)
          speedRb = baseSpeed * (distRb + safetyEps) / (distLt + safetyEps);

        float maxDeltaLt = dt * speedLt;
        float maxDeltaRb = dt * speedRb;

        if (distLt > maxDeltaLt)
          dLt *= maxDeltaLt / distLt;
        if (distRb > maxDeltaRb)
          dRb *= maxDeltaRb / distRb;

        newLt = sc.relPos - offs + dLt;
        newRb = sc.relPos - offs + sc.size + dRb;
      }
      else
      {
        float viscosity = ::max(0.001f, elem->props.getFloat(elem->csk->viscosity, 0.1f));
        newLt = ::approach(sc.screenPos, target->targetRect.leftTop(), dt, viscosity) - psc.screenPos - offs;
        newRb = ::approach(sc.screenPos + sc.size, target->targetRect.rightBottom(), dt, viscosity) - psc.screenPos - offs;
      }

      elem->layout.hPlacement = ALIGN_LEFT;
      elem->layout.vPlacement = ALIGN_TOP;
      elem->layout.pos[0].mode = SizeSpec::PIXELS;
      elem->layout.pos[0].value = newLt.x;
      elem->layout.pos[1].mode = SizeSpec::PIXELS;
      elem->layout.pos[1].value = newLt.y;
      elem->layout.size[0].mode = SizeSpec::PIXELS;
      elem->layout.size[0].value = newRb.x - newLt.x;
      elem->layout.size[1].mode = SizeSpec::PIXELS;
      elem->layout.size[1].value = newRb.y - newLt.y;
      elem->recalcLayout();
    }
  }

  return wasHidden ? R_REBUILD_RENDER_AND_INPUT_LISTS : 0;
}


} // namespace darg
