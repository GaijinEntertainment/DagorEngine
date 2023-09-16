//expect:w210

let { contactsLists, kwarg } = require("sq3_sa_test")

local buildBtnParams = kwarg(function(_icon=null, option=null, count_list=null, counterFunc=null){
  local list = contactsLists[count_list ?? option].list
  counterFunc = counterFunc ?? function(_){ return list }
  return counterFunc
})

return buildBtnParams
