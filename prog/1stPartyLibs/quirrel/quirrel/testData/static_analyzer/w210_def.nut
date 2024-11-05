if (__name__ == "__analysis__")
  return

//expect:w210

local buildBtnParams = ::kwarg(function(icon=null, option=null, count_list=null, counterFunc=null){ //-declared-never-used
  local list = ::contactsLists[count_list ?? option].list
  counterFunc = counterFunc ?? function(_){ return list }
  return counterFunc
})

return buildBtnParams

//-file:undefined-global
