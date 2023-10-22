#include "p_placeholder.h"

CPlaceholder::CPlaceholder(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  hdpi::Px h) :
  PropertyControlBase(id, event_handler, parent, x, y, w, h)
{}
