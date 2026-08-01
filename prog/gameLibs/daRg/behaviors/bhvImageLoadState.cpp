// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvImageLoadState.h"

#include <daRg/dag_element.h>
#include <daRg/dag_picture.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>

#include <quirrel/frp/dag_frp.h>


using namespace sqfrp;


namespace darg
{


BhvImageLoadState bhv_image_load_state;


BhvImageLoadState::BhvImageLoadState() : Behavior(STAGE_ACT, 0) {}


int BhvImageLoadState::update(UpdateStage /*stage*/, Element *elem, float /*dt*/)
{
  Sqrat::Object obs = elem->props.getObject(elem->csk->imageLoading);
  if (obs.GetType() != OT_INSTANCE)
    return 0;
  WatchedHandle *handle = obs.Cast<WatchedHandle *>();
  if (!handle)
    return 0;

  // Observe only: kicking a load here would race the render thread's getPic().
  // Writing an unchanged value is a no-op for the FRP graph.
  Picture *pic = elem->props.getPicture(elem->csk->image);
  bool isLoading = pic && !pic->isContentReady();
  handle->graph->setValue(handle->id, Sqrat::Object(isLoading, elem->getVM()));

  return 0;
}


} // namespace darg
