// getarity() gives the counts that getfuncinfos() gives, for each kind of callable below, without building the getfuncinfos() table.
from "math" import min, clamp, floor
from "string" import format, split_by_chars

function fromInfos(infos) {
  if (infos == null)
    return null
  let num_params = infos.parameters.len() - 1
  return { num_params, num_required = num_params - infos.defparams.len(), varargs = infos.varargs }
}

class Callable {
  function _call(_env, a, b = 2) { return a + b }
}
class NotCallable {}

let callables = {
  noParams = @() null
  one = @(a) a
  withDefault = @(a, _b = 1) a
  allDefaults = function(_a = 1, _b = 2) {}
  varargs = function(_a, ...) {}
  onlyVarargs = function(...) {}
  typed = function(_a: int, _b: string|null = null): int { return 0 }
  nativeMin = min
  nativeClamp = clamp
  nativeFloor = floor
  nativeFormat = format
  nativeSplit = split_by_chars
  nativePrint = print
  nativeArrayAppend = [].append
  nativeStringSlice = "".slice
  callableClass = Callable
  callableInstance = Callable()
  notCallableClass = NotCallable
  notCallableInstance = NotCallable()
  plainTable = {}
}

foreach (name in callables.keys().sort()) {
  let f = callables[name]
  let arity = f.getarity()
  let expected = fromInfos(f.getfuncinfos())
  let same = arity == null ? expected == null
    : arity.len() == 3 && arity.num_params == expected.num_params && arity.num_required == expected.num_required && arity.varargs == expected.varargs
  let text = arity == null ? "null" : $"num_params={arity.num_params} num_required={arity.num_required} varargs={arity.varargs}"
  println($"{name}: {text}{same ? "" : " DIFFERS from getfuncinfos"}")
}

// A getfuncinfos() method of the class does not change getarity(): the counts come from _call
class OwnInfos {
  function _call(_env, a, b = 1) { return a + b }
  function getfuncinfos() { return (@(_x) null).getfuncinfos() }
}
let callCounts = fromInfos(OwnInfos.getmetamethod("_call").getfuncinfos())
foreach (pair in [["ownInfosClass", OwnInfos], ["ownInfosInstance", OwnInfos()]]) {
  let arity = pair[1].getarity()
  let same = arity.num_params == callCounts.num_params && arity.num_required == callCounts.num_required
    && arity.varargs == callCounts.varargs
  let text = $"num_params={arity.num_params} num_required={arity.num_required} varargs={arity.varargs}"
  println($"{pair[0]}: {text}{same ? "" : " DIFFERS from _call"}, own getfuncinfos() num_params={fromInfos(pair[1].getfuncinfos()).num_params}")
}
