//-file:plus-string
#disable-optimizer
let { get_arg_value_by_name = @(...) true, argv = freeze([]) } = require_optional("dagor.system")
let { rand, srand, min } = require("math")
let io = require("io")

function anyof(arr, probability_of_first = 0) {
  if (probability_of_first > 0) {
    if (rand() % 100 < probability_of_first)
      return arr[0]
  }
  return arr.len() == 0 ? null : arr[rand() % arr.len()]
}

class Scope {
  locals = null
  localsRvalue = null
  params = null
  paramsDefault = null
  loopVars = null
  allIdentifiers = null
  isFunction = false
  canContainLocals = true

  constructor() {
    this.locals = []
    this.localsRvalue = []
    this.params = []
    this.paramsDefault = []
    this.loopVars = []
    this.allIdentifiers = []
    this.isFunction = false
    this.canContainLocals = true
  }
}


function Fuzzer(allowLoops) {
  let scopes = []
  local topScope = -1
  local varCount = 0
  local ind = ""
  local treeDepth = 0
  local breakable = false
  local insideSwitch = 0
  local insideLoopCondition = 0
  local insideLoopBody = 0
  local logNum = 0

  let dump_scopes = function() {
    for (local i = 0; i < scopes.len(); i++) {
      print($"\nSCOPE {i}:")
      print("\nlocals: ")
      foreach(v in scopes[i].locals)
        print($"{v} ")
      print("\nallIdentifiers: ")
      foreach(v in scopes[i].allIdentifiers)
        print($"{v} ")
      print("\n\n")
    }
  }

  let logId = function() {
    logNum++
    return $"\"{logNum}\""
  }

  let get_function_scope = function() {
    for (local i = topScope; i >= 0; i--)
      if (scopes[i].isFunction)
        return scopes[i]
    return null
  }

  let get_local_scope = function() {
    for (local i = topScope; i >= 0; i--)
      if (scopes[i].canContainLocals)
        return scopes[i]
    return null
  }

  let inc_indent = function() {
    ind += "  "
    return ind
  }

  let dec_indent = function() {
    ind = ind.slice(0, ind.len() - 2)
    return ""
  }

  let e = function(fn) {
    treeDepth++
    // if (treeDepth <= 100)
    //   stop()

    let res = fn()

//    if (res == "v37")
//    {
//      dump_scopes()
//      stop()
//    }


    treeDepth--
    return res
  }

  let need_indent = @(s) s.len() > 1 && s[0] == '{'

  local int_expr, bool_bin_expr, bool_expr,
    int_field, int_delegate, int_array_value, int_add, int_sub, int_or, int_and,
    int_identifier, int_var, int_param, int_function_call,
    statement, statements,
    array_var, table_var, generator_var,
    void_function_call

  let int = function() {
    if (!allowLoops) {
      return anyof(["0", "-1", "1", "2", "-2", "127", "128", "-127", "-128", "255", "-255", "32767", "32768", "-32767", "-32768",
        "65535", "65536", "-65535", "-65536",
        "2147483646", "2147483647", "2147483648", "-2147483647", "-2147483648", "-2147483649",
        "9223372036854775806", "9223372036854775807", "-9223372036854775806", "-9223372036854775807", "(-9223372036854775807 - 1)"
        ])
    }
    else if (insideSwitch)
      return (rand() % 4 - 1).tostring()
    else if (rand() % 16 != 0)
      return (rand() % 20 - 5).tostring()
    else
      return (rand() & 0x1ff).tostring()
  }

  int_add = @() $"{e(int_expr)} + {e(int_expr)}"
  int_sub = @() $"{e(int_expr)} - {e(int_expr)}"
  int_or = @() $"(({e(int_expr)}) | ({e(int_expr)}))"
  int_and = @() $"(({e(int_expr)}) & ({e(int_expr)}))"

  let cmp_expr = @() $"({e(int_expr)} {anyof(["<", ">", "<=", ">=", "!=", "=="])} {e(int_expr)})"

  let not_expr = @() $"(!({e(bool_expr)}))"

  bool_bin_expr = @() $"(({e(bool_expr)}) {anyof(["&&", "||", "!=", "=="])} ({e(bool_expr)}))"
  bool_expr = @() e(anyof([cmp_expr, not_expr, bool_bin_expr], 20))

  let int_paren = @() $"({e(int_expr)})"
  let int_ternary = @() $"(({e(cmp_expr)}) ? ({e(int_expr)}) : ({e(int_expr)}))"

  let loop_cmp_op = @() anyof(["<", "<="])

  int_expr = @() e(anyof([int, int_field, int_delegate, int_array_value, int_var, int_add, int_sub, int_or, int_and,
                   int_ternary, int_identifier, int_var, int_paren, int_param,
                   int_function_call], treeDepth > 25 ? 90 : 32))
  let to_log = @() $"to_log({logId()}, {e(int_expr)})"

  let if_then = function() {
    local condition = e(anyof([bool_expr, int_paren]))
    inc_indent()
    local then_ = e(anyof([statement, statements]))
    dec_indent()
    return $"if {condition}\n{need_indent(then_) ? ind : ""}{then_}"
  }

  let if_then_else = function() {
    local condition = e(anyof([bool_expr, int_paren]))
    inc_indent()
    local then_ = e(anyof([statement, statements]))
    local else_ = e(anyof([statement, statements]))
    dec_indent()
    return $"if {condition}\n{need_indent(then_) ? ind : ""}{then_}\n{ind}else\n{need_indent(else_) ? ind : ""}{else_}"
  }

  let try_catch = function() {
    inc_indent()
    local code1 = statements(rand() % 3, rand() % 2 == 0)
    local code2 = statements(rand() % 3)
    dec_indent()
    local index = varCount++
    return $"try\n{ind}{code1}\n{ind}catch(err{index})\n{ind}{code2}"
  }

  let for_loop = function() {
    if (insideLoopBody >= 3 || !allowLoops)
      return e(to_log)

    scopes.append(Scope())
    topScope++

    scopes.top().isFunction = false
    scopes.top().canContainLocals = false

    let varInit = e(int_expr)

    let idx = varCount++
    let varName = $"loop{idx}"

    scopes.top().loopVars.append(varName)
    scopes.top().allIdentifiers.append(varName)

    insideLoopCondition++
    local s = $"for (local {varName} = {varInit}; {varName} {loop_cmp_op()} {e(int_expr)}; {varName}++)"
    insideLoopCondition--
    inc_indent()
    breakable = true
    insideLoopBody++
    local body = e(anyof([statement, statements]))
    insideLoopBody--
    breakable = false
    dec_indent()

    topScope--
    scopes.pop()

    return $"{s}\n{need_indent(body) ? ind : ""}{body}";
  }

  let foreach_loop = function() {
    if (insideLoopBody >= 3 || !allowLoops)
      return e(to_log)

    scopes.append(Scope())
    topScope++

    scopes.top().isFunction = false
    scopes.top().canContainLocals = false

    let idx1 = varCount++
    let varName1 = $"t{idx1}"
    let idx2 = varCount++
    let varName2 = $"t{idx2}"

    scopes.top().loopVars.append(varName1)
    scopes.top().loopVars.append(varName2)
    scopes.top().allIdentifiers.append(varName1)
    scopes.top().allIdentifiers.append(varName2)

    local s = $"foreach ({varName1}, {varName2} in {e(anyof([array_var, table_var, generator_var]))})"
    inc_indent()
    breakable = true
    insideLoopBody++
    local body = e(anyof([statement, statements]))
    insideLoopBody--
    breakable = false
    dec_indent()

    topScope--
    scopes.pop()

    return $"{s}\n{need_indent(body) ? ind : ""}{body}";
  }

  let while_loop = function() {
    if (insideLoopBody >= 3 || !allowLoops)
      return e(to_log)

    scopes.append(Scope())
    topScope++

    scopes.top().isFunction = false
    scopes.top().canContainLocals = false

    let varInit = e(int_expr)

    let idx = varCount++
    let varName = $"loop{idx}"

    scopes.top().loopVars.append(varName)
    scopes.top().allIdentifiers.append(varName)

    inc_indent()
    breakable = true
    insideLoopBody++
    local body = e(anyof([statement, statements]))
    insideLoopBody--
    breakable = false
    dec_indent()

    insideLoopCondition++
    let condition = $"{varName}++ {loop_cmp_op()} {e(int_expr)}"
    insideLoopCondition--


    local s = $"\{\n{ind}  local {varName} = {varInit}\n{ind}  while ({condition})\n  {need_indent(body) ? ind : ""}{body}\n{ind}\}"

    topScope--
    scopes.pop()

    return s;
  }

  let do_while_loop = function() {
    if (insideLoopBody >= 3 || !allowLoops)
      return e(to_log)

    scopes.append(Scope())
    topScope++

    scopes.top().isFunction = false
    scopes.top().canContainLocals = false

    let varInit = e(int_expr)

    let idx = varCount++
    let varName = $"loop{idx}"

    scopes.top().loopVars.append(varName)
    scopes.top().allIdentifiers.append(varName)

    inc_indent()
    breakable = true
    insideLoopBody++
    local body = e(anyof([statement, statements]))
    insideLoopBody--
    breakable = false
    dec_indent()

    insideLoopCondition++
    let condition = $"{varName}++ {loop_cmp_op()} {e(int_expr)}"
    insideLoopCondition--

    local s = $"\{\n{ind}  local {varName} = {varInit}\n{ind}  do\n  {need_indent(body) ? ind : ""}{body}\n{ind}  while ({condition})\n{ind}\}"

    topScope--
    scopes.pop()

    return s;
  }

  let break_continue = function() {
    if (!breakable || !allowLoops)
      return statement()

    return anyof(["break", "continue"])
  }

  let switch_case = function() {
    insideSwitch++

    local s = $"switch ({e(int_expr)}) \{\n"
    inc_indent()

    let caseCount = rand() % 3

    for (local k = 0; k < caseCount; k++) {
      s += $"{ind}case {e(int_expr)}:\n"
      let count = rand() % 3
      for (local i = 0; i < count; i++) {
        inc_indent()
        s += $"{e(statement)}\n"
        dec_indent()
      }

      if (rand() % 3 > 0)
        s += $"{ind}break\n"
    }

    if (rand() % 2 == 0) {

      s += $"{ind}default:\n"
      let count = rand() % 3
      for (local i = 0; i < count; i++) {
        inc_indent()
        s += $"{e(statement)}\n"
        dec_indent()
      }

      if (rand() % 3 > 0)
        s += $"{ind}break\n"
    }

    dec_indent()

    s += $"{ind}\}"

    insideSwitch--
    return s
  }

  statement = function() {
    let st = e(anyof([to_log, void_function_call, if_then, if_then_else,
                      foreach_loop, for_loop, while_loop, do_while_loop,
                      try_catch, break_continue], treeDepth > 25 ? 100 : 40))
    return $"{ind}{st}"
  }

  statements = function(statements_count = -1, should_throw = false) {
    scopes.append(Scope())
    topScope++

    let count = statements_count < 0 ? 1 + rand() % 5 : statements_count
    let throw_at = should_throw ? rand() % (count + 1) : -1

    local s = ""
    inc_indent()
    for (local i = 0; i < count; i++)
      if (i == throw_at)
        s += anyof([$"{ind}(\{\}).x\n", $"{ind}0/0\n", $"{ind}([0])[-1] *= 1\n"])
      else
        s += $"{e(statement)}\n"

    local v = ""
    local endval = ""
    let localCount = scopes.top().locals.len()
    for (local i = 0; i < localCount; i++) {
      v += $"{ind}let {scopes.top().locals[i]} = {scopes.top().localsRvalue[i]}\n"
      endval += $"{ind}to_log({logId()}, {scopes.top().locals[i]})\n"
    }
    dec_indent()

    if (rand() % 2 == 0)
      endval = "";

    topScope--
    scopes.pop()
    return $"\{\n{v}{s}{endval}{ind}\}"
  }

  int_identifier = function() {
    if (topScope < 0)
      return int()

    for (local i = rand() % (topScope + 1); i >= 0; i--) {
      if (scopes[i].allIdentifiers.len() > 0) {
        let s = anyof(scopes[i].allIdentifiers)
        if (insideLoopCondition && s.startswith("loop"))
          return int()
        return s
      }
    }

    return int()
  }

  int_var = function() {
    local scopeWithLocals = get_local_scope()
    if (!scopeWithLocals)
      return int()

    let idx = varCount++
    let varName = $"v{idx}"
    topScope--
    let rvalue = e(int_expr)
    topScope++
    scopeWithLocals.allIdentifiers.append(varName)
    scopeWithLocals.locals.append(varName)
    scopeWithLocals.localsRvalue.append(rvalue)
    return varName;
  }

  array_var = function() {
    local scopeWithLocals = get_local_scope()
    if (!scopeWithLocals)
      return int()

    let idx = varCount++
    let varName = $"v{idx}"
    topScope--
    local rvalue = "["
    local count = rand() % 8
    for (local i = 0; i < count; i++)
      rvalue += $"{int()}, "
    rvalue += "]"
    topScope++
    scopeWithLocals.allIdentifiers.append(varName)
    scopeWithLocals.locals.append(varName)
    scopeWithLocals.localsRvalue.append(rvalue)
    return varName;
  }

  table_var = function() {
    local scopeWithLocals = get_local_scope()
    if (!scopeWithLocals)
      return int()

    let idx = varCount++
    let varName = $"v{idx}"
    topScope--
    local rvalue = "{"
    local count = rand() % 8
    for (local i = 0; i < count; i++)
      rvalue += $"m{i} = {int()}, "
    rvalue += "}"
    topScope++
    scopeWithLocals.allIdentifiers.append(varName)
    scopeWithLocals.locals.append(varName)
    scopeWithLocals.localsRvalue.append(rvalue)
    return varName;
  }

  generator_var = function() {
    local scopeWithLocals = get_local_scope()
    if (!scopeWithLocals)
      return int()

    let idx = varCount++
    let varName = $"gen{idx}"
    topScope--
    local rvalue = "function () { "
    local count = rand() % 5
    for (local i = 0; i < count; i++)
      if (rand() % 6 == 0)
        rvalue += $"return {rand() % 20 - 5}; "
      else
        rvalue += $"yield {rand() % 20 - 5}; "
    rvalue += "}"
    topScope++
    scopeWithLocals.allIdentifiers.append(varName)
    scopeWithLocals.locals.append(varName)
    scopeWithLocals.localsRvalue.append(rvalue)
    return $"{varName}()"
  }

  int_field = function() {
    local scopeWithLocals = get_local_scope()
    if (!scopeWithLocals)
      return int()

    //topScope--

    let idx = varCount++
    let varName = $"t{idx}"
    local fields = rand() % 3
    local s = "{ "
    for (local i = 0; i < fields; i++)
      s += $"f{i}={e(int_expr)} "
    s += "}"

    //topScope++

    let rvalue = s
    scopeWithLocals.locals.append(varName)
    scopeWithLocals.localsRvalue.append(rvalue)
    if (rand() % 2 == 0 || fields == 0) {
      if (rand() % 2 == 0)
        return $"({varName}?[\"f{rand() % 2}\"] ?? {e(int_expr)})";
      else
        return $"({varName}?.f{rand() % 2} ?? {e(int_expr)})";
    }
    else {
      if (rand() % 2 == 0)
        return $"{varName}[\"f{fields / 2}\"]";
      else
        return $"{varName}.f{fields / 2}";
    }
  }

  int_delegate = function() {
    local scopeWithLocals = get_local_scope()
    if (!scopeWithLocals)
      return int()

    //topScope--

    let idx = varCount++
    let className = $"A{idx}"
    let classDecl = $"class \{ function _get(idx) \{ return idx + {e(int_expr)} \}\}"


    let varName = $"ci{idx}"
    let newInstance = $"A{idx}()"

    //topScope++

    scopeWithLocals.locals.append(className)
    scopeWithLocals.localsRvalue.append(classDecl)
    scopeWithLocals.locals.append(varName)
    scopeWithLocals.localsRvalue.append(newInstance)
    let qmark = rand() % 5 == 0 ? "?" : ""
    if (rand() % 50 != 0)
      return $"{varName}{qmark}[{e(int_expr)}]"
    else
      return $"{varName}{qmark}[\"s\"]";
  }

  int_array_value = function() {
    local scopeWithLocals = get_local_scope()
    if (!scopeWithLocals)
      return int()

    //topScope--

    let idx = varCount++
    let varName = $"a{idx}"
    local fields = rand() % 3
    local s = "[ "
    for (local i = 0; i < fields; i++)
      s += $"{e(int_expr)}, "
    s += "]"

    //topScope++

    let rvalue = s
    scopeWithLocals.locals.append(varName)
    scopeWithLocals.localsRvalue.append(rvalue)
    if (rand() % 2 == 0 || fields == 0) {
      return $"({varName}?[{rand() % 2}] ?? {e(int_expr)})";
    }
    else {
      return $"{varName}[{fields / 2}]";
    }
  }

  int_param = function() {
    local functionScope = get_function_scope()
    if (!functionScope)
      return int()
    let idx = varCount++
    let paramName = $"p{idx}"
    functionScope.params.append(paramName);
    functionScope.allIdentifiers.append(paramName);
    functionScope.paramsDefault.append("");
    return paramName
  }

  void_function_call = function() {
    local scopeWithLocals = get_local_scope()
    if (!scopeWithLocals)
      return to_log()

    let idx = varCount++
    let functionName = $"fn{idx}"
    local argNames = null;
    local args = []

    scopes.append(Scope())
    scopes.top().isFunction = true
    scopes.top().canContainLocals = false
    topScope++

    let fnCode = e(statements)
    argNames = clone scopes.top().params

    topScope--
    scopes.pop()

    foreach (_ in argNames)
      args.append(e(int_expr))

    scopeWithLocals.locals.append(functionName)
    scopeWithLocals.localsRvalue.append($"function {functionName}({", ".join(argNames)}) {fnCode}")

    return $"{functionName}({", ".join(args)})"
  }

  int_function_call = function() {
    local scopeWithLocals = get_local_scope()
    if (!scopeWithLocals)
      return int()

    let idx = varCount++
    let functionName = $"fni{idx}"
    local argNames = null;
    local args = []

    scopes.append(Scope())
    scopes.top().isFunction = true
    scopes.top().canContainLocals = false
    topScope++

    let fnCode = e(int_expr)
    argNames = clone scopes.top().params

    topScope--
    scopes.pop()

    foreach (_ in argNames)
      args.append(e(int_expr))

    scopeWithLocals.locals.append(functionName)
    scopeWithLocals.localsRvalue.append($"@({", ".join(argNames)}) {fnCode}")

    return $"{functionName}({", ".join(args)})"
  }

  return {
    statements
    dump_scopes
  }
}

if (__name__ == "__main__" && !get_arg_value_by_name("static-analysis")) {
  local seedSet = null
  local singleSeed = 0
  local filePrefix = null

  foreach (_, a in argv) {
    if (a.startswith("seed="))
      singleSeed = a.slice(5).tointeger()
    if (a.startswith("seedset="))
      seedSet = a.slice(8).split(",").map(@(v) v.tointeger())
    if (a.startswith("fileprefix="))
      filePrefix = a.slice(11)
  }

  if (seedSet == null)
    seedSet = [singleSeed]

  foreach (seed in seedSet) {
    local script = ""
    script += $"// seed = {seed}\n"
    srand(seed)
    script += "try{\n"
    script += "local count = 0\n"
    script += "let to_log = function(id, s) { let tp = typeof(s); print($\"{id}: \"); print(tp == \"integer\" ||" +
      " tp == \"float\" || tp == \"string\" ? s : tp); print(\"\\n\"); if (count++ > 1000) print[\"stop\"]; }\n"

    local allowLoops = (seed % 13 != 0)
    local fuzzer = Fuzzer(allowLoops)
    script += fuzzer.statements()
    script += "} catch (q) {print(q)}\nprint(\"===\\n\")"

    if (filePrefix == null) {
      for (local i = 0; i <= script.len() / 1024; i++)
        print(script.slice(i * 1024, min((i + 1) * 1024, script.len())))
    } else {
      local f = io.file(filePrefix + seed + ".opt.nut", "wt")
      f.writestring("//#disable-optimizer\n")
      f.writestring(script)
      f.close()
      f = io.file(filePrefix + seed + ".no-opt.nut", "wt")
      f.writestring("#disable-optimizer\n")
      f.writestring(script)
      f.close()
    }
  }
}
