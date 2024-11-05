// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvViewAzimuth.h"
#include "ui/uiShared.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_renderObject.h>
#include <daRg/dag_transform.h>
#include <memory/dag_framemem.h>

#include <math/dag_mathBase.h>


using namespace darg;


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvViewAzimuth, bhv_view_azimuth, cstr);


BhvViewAzimuth::BhvViewAzimuth() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0) {}


void BhvViewAzimuth::onAttach(Element *elem) { discard_text_cache(elem->robjParams); }


int BhvViewAzimuth::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  Point3 lookDir = uishared::view_itm.getcol(2);
  float lookAngle = safe_atan2(lookDir.x, lookDir.z);

  if (elem->props.getBool(strings->setText))
  {
    String str(framemem_ptr());
    float angleDeg = floorf(lookAngle * RAD_TO_DEG + 0.5);
    if (angleDeg < 0.f)
      angleDeg += 360.f;

    str.printf(0, "%.0f", angleDeg);

    if (str != elem->props.text)
    {
      elem->props.text = str;
      discard_text_cache(elem->robjParams);
    }
  }

  if (elem->props.getBool(strings->setRotation))
  {
    if (elem->transform)
    {
      elem->transform->rotate = lookAngle;
    }
  }

  return 0;
}
