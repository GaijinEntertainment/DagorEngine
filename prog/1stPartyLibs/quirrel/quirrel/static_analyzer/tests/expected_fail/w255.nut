//expect:w255

::timeToShowAll <- require("sq3_sa_test").timeToShowAll
::pos <- require("sq3_sa_test").pos
::sizeG <- require("sq3_sa_test").sizeG
::parentSizeG <- require("sq3_sa_test").parentSizeG
::on_credits_finish <- require("sq3_sa_test").on_credits_finish
::format <- require("sq3_sa_test").format


::ClassName <- class {
  function onTimer2(obj, dt, dt2) { // -unused-func-param
    local curOffs = obj.cur_slide_offs.tofloat()

//    local pos = obj.getPos()
    local size = obj.getSize()
    local parentSize = obj.getParent().getSize()
    local speedCreditsScroll = (size[1] / parentSize[1] ) / ::timeToShowAll

    if (::pos[1] + :: sizeG[1] < 0) {
      curOffs = -(0.9 * ::parentSizeG[1]).tointeger()
      if (obj?.inited == "yes") {
        ::on_credits_finish()
        return
      }
      else
        obj.inited="yes"
    } else
      curOffs += dt * parentSize[1] * speedCreditsScroll //* 720 / parentSize[1] / 0.9
    obj.cur_slide_offs = ::format("%f", curOffs)
    obj.top = (-curOffs).tointeger().tostring()
  }

  function onTimer(obj, dt, dt3) { // -unused-func-param
    local curOffs = obj.cur_slide_offs.tofloat()

//    local pos = obj.getPos()
    local size = obj.getSize()
    local parentSize = obj.getParent().getSize()
    local speedCreditsScroll = (size[1] / parentSize[1] ) / ::timeToShowAll

    if (::pos[1] + :: sizeG[1] < 0) {
      curOffs = -(0.9 * ::parentSizeG[1]).tointeger()
      if (obj?.inited == "yes") {
        ::on_credits_finish()
        return
      }
      else
        obj.inited="yes"
    } else
      curOffs += dt * parentSize[1] * speedCreditsScroll //* 720 / parentSize[1] / 0.9
    obj.cur_slide_offs = ::format("%f", curOffs)
    obj.top = (-curOffs).tointeger().tostring()
  }
}