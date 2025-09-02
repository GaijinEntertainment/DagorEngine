import "math"
from "string.nut" import tostring_r
//rewritten https://github.com/gliese1337/json-lisp/tree/master/src

let log =  @(...) println(tostring_r(vargv, {compact=false, maxdeeplevel=12}))

class Env {
    parent = null
    symbols = null
    constructor(symbols, parent = null) {
        if (type(symbols)=="table")
          this.symbols = clone symbols
        else {
          this.symbols = {}
          foreach (symbol in symbols) {
            this.symbols[symbol[0]] <- symbol[1]
          }
        }
        this.parent = parent;
    }
    tostring = @() tostring_r(this.symbols)
    get = function(sym) {
        local e = this;
        while (e != null) {
            if (e.symbols.rawin(sym))
              return e.symbols[sym];
            e = e.parent;
        }
        //log("get!", sym, e, e?.symbols)
        throw $"symbol '{sym}' not found in current environment";
    }
}

function cmp_reduce(args, cmp) {
    let l = args.len() - 1;
    for (local i = 0; i < l; i++) {
        if (cmp(args[i], args[i+1]))
          return false;
    }
    return true;
}

let preamble = Env({
    function lambda(v, interp, env) {
        let [params, body] = v
        return function(args) {
            return interp(body, Env(params.map(function(p, i) { return [p, args[i]]; }), env));
        };
    }
    function macro(v, _, stcEnv) {
        let [params, body] = v
        return function(args, interp, dynEnv) {
            return interp(interp(body, Env(params.map(function(p, i) { return [p, args[i]]; }), stcEnv)), dynEnv);
        };
    },
    function rec(v, interp, env) {
        let [name, params, body] = v
        let f = function(args) {
            return interp(body, Env([[name, f]].concat(params.map(function(p, i) { return [p, args[i]]; })), env));
        }
        return f
    },
    function recmacro(v, _, stcEnv) {
        let [name, params, body] = v
        let m = function(args, interp, dynEnv) {
            return interp(interp(body, Env([[name, m]].concat(params.map(function(p, i) { return [p, args[i]]; })), stcEnv)), dynEnv);
        };
        return m;
    },
    function apply(v, interp, env) {
      let [op, args] = v
      return interp([op].concat(args), env)
    },
    ["let"] = function bind(v, interp, env) {
        let [bindings, body] = v
        let newBindings = bindings.map(function(vv) {
          let [sym, expr] = vv
          return [sym, interp(expr, env)] }
        )
        //log(Env(newBindings, env))
        return interp(body, Env(newBindings, env))
    },
    ["if"] = function cond(v, interp, env) {
        let [cnd, thn, els] = v
        return interp(interp(cnd, env) ? thn : els, env);
    },
    q = @(args, _, __) args[0]
    list = @(args) args

    function and(v, interp, env) {
      local result = true
      // Evaluate arguments one-by-one, short-circuit if any is falsy
      foreach (expr in v) {
        result = interp(expr, env)
        if (!result)
          return result
      }
      return result
    }
    // Added support for 'or' operator
    function or(v, interp, env) {
      local result = false
      // Evaluate arguments one-by-one, short-circuit if any is truthy
      foreach (expr in v) {
        result = interp(expr, env)
        if (result)
          return result
      }
      return result
    },
    "+": function plus(args) {
      return args.reduce(function(a, b) { return a + b; }, 0);
    },
    "-": function(args) { return args.len() ? args.reduce(function minus(a, b) { return a - b; }) : 0; },
    "*": function(args) { return args.reduce(function mul(a, b) { return a * b; }, 1); },
    "/": function(args) { return args.len() ? args.reduce(function div(a, b) { return a / b; }) : 1; },
    "^": function(args) { return args.len() < 2 ? 1 : args.reduce(function pow(a, b) { return math.pow(a, b); }); },
    "%": function(args) { return args.len() < 2 ? args[0] : args.reduce(function mod(a, b) { return a % b; }); },
    ">": function(args) { return cmp_reduce(args, function greater(a, b) { return a <= b; }); },
    "<": function(args) { return cmp_reduce(args, function less(a, b) { return a >= b; }); },
    ">=": function(args) { return cmp_reduce(args, function greater_or_equal(a, b) { return a < b; }); },
    "<=": function(args) { return cmp_reduce(args, function less_or_equal(a, b) { return a > b; }); },
    "=": function(args) { return cmp_reduce(args, function equal(a, b) { return a != b; }); },
    "!": function(args) { return !args[0]; },
    ".": function get(v) {
        local obj = v[0]
        let path = v.slice(1)
        foreach (p in path) {
            if (obj != null) {
                obj = obj?[p];
            }
            else
              return null
        }
        return obj
    }
})
local apply

function interp(ast, env) {
  if (typeof ast == "array") {
    let stack = []
    local expr = ast
    do {
        stack.append(expr);
        expr = expr[0];
    } while (typeof expr == "array");

    if (typeof expr != "string")
      throw $"Cannot apply expr {expr}"
    local op = env.get(expr)
    if (typeof op != "function")
      throw $"Cannot apply op `{ op }`"

    for (local i = stack.len() - 1; i > 0; i--) {
      op = apply(op, env, stack[i].slice(1));
      if (typeof op != "function") {
        throw $"Cannot apply op '{op}'"
      }
    }
    return apply(op, env, stack[0].slice(1))
  }

  if (typeof ast == "string") {
    return env.get(ast)
  }
  return ast
}

apply = function(op, env, args) {
    let funcinfo  = op.getfuncinfos()
    return funcinfo.parameters.len() > 2 ?
        op(args, interp, env) :
        op(args.map(function(arg) { return interp(arg, env); }))
}

function interpreter(ast, bindings = null, safe=false) {
    let env = bindings
      ? (typeof bindings == "instance" && bindings instanceof Env ? bindings : Env(bindings, preamble))
      : preamble
    if (!safe)
      return interp(ast, env)
    try {
      return interp(ast, env)
    }
    catch(e){
      log($"error: '{e}' while interperting")
      throw e
    }
}


if (__name__ == "__main__"){
  let pa = @(r, s, name = "") name!="" ? $"'{name}': {s} == {r}" : $"{s} == {r}"
  let check = function(e, s, name="") {
    log("interpert:", e)
    let res = interpreter(e)
    log($"result: `{res}`, supposed: `{s}`")
    assert(res == s ,@() pa(res, s, name))
  }

  check(["let", [["foo", 1]], "foo"], 1)
  check(["q", 1], 1)
  check(1, 1)
  check(["let", [["foo", false]], "foo"], false)
  check(["+", ["+",1,2], 2], (1+2)+2, "+")
  check(["if", [">",["+", 1, 3], 2], 1, 0], (1+3) > 2 ? 1 : 0, "if")

  check(["!", true], !true, "!")
  check(["and", true, false], true && false, false)
  check(["let", [["foo", 2]], ["or", [">", "foo", 1], false]], (2>1) || false, true)
}

return freeze({
  interpreter
  interp
})