
function findlast(str, substr, startidx=0){
    local ret = null
    for(local i=startidx; i<str.len(); i++) {
      local k = str.indexof(substr, i)
      if (k!=null) {
        i = k
        ret = k
      }
      else
        break;
    }
    return ret
  }