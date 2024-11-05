import "math"

//let { log } = require("log.nut")()

class Env {
    parent = null
    symbols = null
    constructor(symbols, parent = null) {
        this.symbols = {};
        foreach (symbol in symbols) {
            this.symbols[symbol[0]] = symbol[1];
        }
        this.parent = parent;
    }

    get = function(sym) {
        local e = this;
        while (e != null) {
            if (e.symbols.rawin(sym))
              return e.symbols[sym];
            e = e.parent;
        }

        throw $"symbol {sym} not found in current environment";
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

let preamble = {
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
    apply = function(v, interp, env) {
      let [op, args] = v
      return interp([op].concat(args), env)
    },
    ["let"] = function(v, interp, env) {
        let [bindings, body] = v
        let newBindings = bindings.map(function(vv) {
          let [sym, expr] = vv
          return [sym, interp(expr, env)]; });
        return interp(body, Env(newBindings, env));
    },
    ["if"] = function(v, interp, env) {
        let [cnd, thn, els] = v
        return interp(interp(cnd, env) ? thn : els, env);
    },
    q = @(args, _, __) args[0]
    list = @(args) args

    "+": function plus(args) {
      return args.reduce(function(a, b) { return a + b; }, 0);
    },
    "-": function(args) { return args.len() ? args.reduce(function(a, b) { return a - b; }) : 0; },
    "*": function(args) { return args.reduce(function(a, b) { return a * b; }, 1); },
    "/": function(args) { return args.len() ? args.reduce(function(a, b) { return a / b; }) : 1; },
    "^": function(args) { return args.len() < 2 ? 1 : args.reduce(function(a, b) { return math.pow(a, b); }); },
    "%": function(args) { return args.len() < 2 ? args[0] : args.reduce(function(a, b) { return a % b; }); },
    ">": function(args) { return cmp_reduce(args, function(a, b) { return a <= b; }); },
    "<": function(args) { return cmp_reduce(args, function(a, b) { return a >= b; }); },
    ">=": function(args) { return cmp_reduce(args, function(a, b) { return a < b; }); },
    "<=": function(args) { return cmp_reduce(args, function(a, b) { return a > b; }); },
    "=": function(args) { return cmp_reduce(args, function(a, b) { return a != b; }); },
    "!": function(args) { return !args[0]; },
    ".": function(v) {
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
}
local apply

function interp(ast, env) {
    if (typeof ast == "array") {
        local stack = [];
        local expr = ast;
        do {
            stack.append(expr);
            expr = expr[0];
        } while (typeof expr == "array");

        if (expr not in env)
          throw $"Cannot apply {expr}";
        local op = env?[expr];
        if (typeof op != "function")
          throw $"Cannot apply {op}";

        for (local i = stack.len() - 1; i > 0; i--) {
            op = apply(op, env, stack[i].slice(1));
            if (typeof op != "function")
              throw $"Cannot apply {op}";
        }

        return apply(op, env, stack[0].slice(1));
    }

    if (ast in env) {
        return env[ast];
    }

    return ast;
}

apply = function(op, env, args) {
    let funcinfo  = op.getfuncinfos()
    return funcinfo.parameters.len() > 2 ?
        op(args, interp, env) :
        op(args.map(function(arg) { return interp(arg, env); }))
}

function interpreter(ast, bindings = null) {
    let env = bindings
      ? (typeof bindings == "instance" && bindings instanceof Env ? bindings : Env(bindings, preamble))
      : preamble
    return interp(ast, env)
}

if (__name__ == "__main__"){
  local supposed
  local res
  let pa = @(r, s, name = "") name!="" ? $"'{name}': {s} == {r}" : $"{s} == {r}"
  let check = @(r, s, name="") assert(r == s , @() pa(r, s, name))

  supposed = (1+2)+2
  res = interpreter(["+", ["+",1,2], 2])
  check(res, supposed, "+")

  res = interpreter(["if", [">",["+", 1, 3], 2], 1, 0])
  supposed = (1+3) > 2 ? 1 : 0
  check(res, supposed, "if")

  res = interpreter(["!", true])
  supposed = !true
  check(res, supposed, "!")
}

return {
  interpreter
  interp
}