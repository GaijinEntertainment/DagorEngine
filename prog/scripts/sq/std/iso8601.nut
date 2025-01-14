
let numbers = "012345678"

let marks = {
  P = {
    Y = "years"
    M = "months"
    W = "weeks"
    D = "days"
  }
  T = {
    H = "hours"
    M = "minutes"
    S = "seconds"
  }
}

function parse_duration(str) {
  let res = {}
  if (str == "")
    return res

  local numberList = []
  local activeMarks = null
  foreach (cInt in str) {
    let c = cInt.tochar()
    let config = marks?[c]
    if (config != null) {
      numberList.clear()
      activeMarks = config
      continue
    }

    let mark = activeMarks?[c]
    if (mark != null) {
      if (mark != "" && numberList.len() != 0)
        res[mark] <- "".join(numberList).tointeger()
      numberList.clear()
      continue
    }

    if (numbers.indexof(c) == null)
      return res
    numberList.append(c)
  }
  return res
}

return {
  parse_duration
}