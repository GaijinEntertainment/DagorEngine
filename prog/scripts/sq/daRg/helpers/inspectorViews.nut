from "%darg/ui_imports.nut" import *

let fieldsMapHorAlign = {
  [ALIGN_LEFT] = "ALIGN_LEFT",
  [ALIGN_CENTER] = "ALIGN_CENTER",
  [ALIGN_RIGHT] = "ALIGN_RIGHT",
}

let fieldsMapVerAlign = {
  [ALIGN_TOP] = "ALIGN_TOP",
  [ALIGN_CENTER] = "ALIGN_CENTER",
  [ALIGN_BOTTOM] = "ALIGN_BOTTOM",
}

let fieldsMap = {
  rendObj = {
    [ROBJ_IMAGE] = "ROBJ_IMAGE",
    [ROBJ_INSCRIPTION] = "ROBJ_INSCRIPTION",
    [ROBJ_TEXT] = "ROBJ_TEXT",
    [ROBJ_TEXTAREA] = "ROBJ_TEXTAREA",
    [ROBJ_9RECT] = "ROBJ_9RECT",
    [ROBJ_BOX] = "ROBJ_BOX",
    [ROBJ_SOLID] = "ROBJ_SOLID",
    [ROBJ_FRAME] = "ROBJ_FRAME",
    [ROBJ_PROGRESS_CIRCULAR] = "ROBJ_PROGRESS_CIRCULAR",
    [ROBJ_VECTOR_CANVAS] = "ROBJ_VECTOR_CANVAS",
    [ROBJ_MASK] = "ROBJ_MASK",
    [ROBJ_PROGRESS_LINEAR] = "ROBJ_PROGRESS_LINEAR",
  }
  flow = {
    [FLOW_PARENT_RELATIVE] = "FLOW_PARENT_RELATIVE",
    [FLOW_HORIZONTAL] = "FLOW_HORIZONTAL",
    [FLOW_VERTICAL] = "FLOW_VERTICAL",
  }
  keepAspect = {
    [KEEP_ASPECT_NONE] = "KEEP_ASPECT_NONE",
    [KEEP_ASPECT_FILL] = "KEEP_ASPECT_FILL",
    [KEEP_ASPECT_FIT] = "KEEP_ASPECT_FIT",
    [false] = "KEEP_ASPECT_NONE",
    [true] = "KEEP_ASPECT_FIT",
  }
  valign = fieldsMapVerAlign
  halign = fieldsMapHorAlign
  vplace = fieldsMapVerAlign
  hplace = fieldsMapHorAlign
  imageValign = fieldsMapVerAlign
  imageHalign = fieldsMapHorAlign
}

return fieldsMap