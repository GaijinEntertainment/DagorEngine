//expect:w255
//-file:undefined-global
//-file:declared-never-used
//-file:ident-hides-ident

//expect:w255

local class ClassName { //-declared-never-used
    function onTimer2(obj, dt, dt2) {
      local curOffs = obj.cur_slide_offs.tofloat()

  //    local pos = obj.getPos()
      local size = obj.getSize()
      local parentSize = obj.getParent().getSize()
      local speedCreditsScroll = (size[1] / parentSize[1] ) / ::timeToShowAll

      if (::pos[1] + :: size[1] < 0) {
        curOffs = -(0.9 * ::parentSize[1]).tointeger()
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

    function onTimer(obj, dt, dt) {
      local curOffs = obj.cur_slide_offs.tofloat()

  //    local pos = obj.getPos()
      local size = obj.getSize()
      local parentSize = obj.getParent().getSize()
      local speedCreditsScroll = (size[1] / parentSize[1] ) / ::timeToShowAll

      if (::pos[1] + :: size[1] < 0) {
        curOffs = -(0.9 * ::parentSize[1]).tointeger()
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