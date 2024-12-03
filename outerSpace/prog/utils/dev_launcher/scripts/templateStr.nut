from "%darg/ui_imports.nut" import *

let {startswith, endswith} = require("string")
//let {kwpartial} = require("%sqstd/functools.nut")

function applyParams(func, params){
  let {parameters, defparams, varargs} = func.getfuncinfos()
  assert(varargs==0, "no varargs allowed")
  let arguments = array(parameters.len())
  let defparamsOffset = parameters.len() - defparams.len()
  foreach (idx, paramname in parameters){
    if (idx==0)
      continue
    if (paramname in params)
      arguments[idx] = params[paramname]
    else if (idx >= defparamsOffset){
      arguments[idx] = defparams[idx-defparamsOffset]
    }
    else {
      assert(false, @() $"no argument '{paramname}' is provided in params")
    }
  }
  return func.acall(arguments)
}


function buildStrByTemplates(str, params, formatters){
  let tokens = str.split(" ")
  local resStrings = []
  foreach (token in tokens){
    assert ((startswith(token, "<<") && endswith(token, ">>")) || (!startswith(token, "<<") && !endswith(token, ">>")))
    if (!startswith(token, "<<")) {
      resStrings.append(token)
      continue
    }
    let t = token.slice(">>".len()).slice(0, -"<<".len())
    assert(t in formatters, @() $"unknown template: {t}")
    if (type(formatters[t])=="string") {
      resStrings.append(buildStrByTemplates(formatters[t], params, formatters))
    }
    else {
      let funcArg = [null]
      if (t in params)
        funcArg.append(params[t]) //FIXME: add check for function call
      let func = formatters[t]
      resStrings.append(buildStrByTemplates(applyParams(func, params), params, formatters))
    }
  }
  let badstrings = resStrings.filter(@(v) v.indexof(" : 0x0000") != null) //for incorrect type conversion
  if (badstrings.len()>0)
    vlog("incorrect commandline argument", badstrings)

  resStrings = resStrings.filter(@(v) v!="")
  return " ".join(resStrings)
}

return buildStrByTemplates
