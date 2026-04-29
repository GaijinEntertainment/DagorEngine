//#disable-optimizer
#allow-switch-statement
let _sv = @(_e) (type(_e) == "integer" || type(_e) == "float" || type(_e) == "null" || type(_e) == "bool" || type(_e) == "string") ? "" + _e : type(_e)
try {
// Code Bricks assembly - seed 126
  const function [pure] v0_fn(x) { return x + 0 }
  const v0_a = -65536
  const v0 = v0_fn(v0_a)  // [const_chain]
  local v1 = v0
  v1 *= 32769  // [modify_mul_const]
  let v2_C = class {
    _n = 0
    constructor(n) { this._n = n & 7 }
    function _nexti(idx) {
      local next = idx == null ? 0 : idx + 1
      return next < this._n ? next : null
    }
    function _get(idx) { return idx * -257 }
  }
  let v2_obj = v2_C(v0)
  local v2 = 0
  foreach (_i, v in v2_obj) v2 += v  // [meta_nexti]
  global enum S126_E1 { A = "\u0000", B = "world" }
  let v3 = $"{S126_E1.A}{S126_E1.B}"  // [enum_string]
  let v4_outer = {inner = {value = 42}}
  let v4 = v4_outer?.inner?.value ?? 4294967295  // [nullsafe_nested_table]
  let v5 = (function() {
    let g = (function() {
      for (local i = 0; i < 1; i++) yield i
    })()
    local s = 0
    while (g.getstatus() == "suspended") {
      s += resume g
    }
    return s
  })()  // [coroutine_sum]
  let v6_C = class {
    _v = 0
    constructor(xv) { this._v = xv }
  }
  let v6_obj = v6_C(v1)
  let v6 = v6_obj instanceof v6_C  // [instanceof_check]
  let v7 = ~v4  // [int_bitnot]
  local v8_root = {child = {data = {count = 32767, value = v5}}}
  local v8_data = v8_root.child.data
  v8_data.count += '9'
  v8_data.value *= 1000
  let v8 = v8_root.child.data.count + v8_root.child.data.value  // [modify_via_local_ref]
  let v9_fn = function(x, acc = []) {
    acc.append(x)
    return acc.len()
  }
  let v9_a = v9_fn(v7)
  let v9_b = v9_fn(v7 + 1)
  let v9 = v9_a + v9_b  // [default_param_mutable_shared]
  let v10_fn = function(x, pipe = {pre = @(v) v & 0xFF,
                                   run = @(v) v + 129,
                                   post = @(v) v * -255}) {
    return pipe.post(pipe.run(pipe.pre(x)))
  }
  let v10 = v10_fn(v1)  // [default_param_pipeline]
  let v11_C = class {
    _v = 0
    constructor(xv) { this._v = xv }
  }
  let v11_obj = v11_C(v10)
  let v11 = v11_obj instanceof v11_C  // [instanceof_check]
  let v12 = v0 < v8 ? v0 : v8  // [int_min_two]
  let v13 = v2 + v10  // [int_add]
  local v14 = 0
  local v14_v = v0 & 0xFF
  while (v14_v != 0) {
    v14 += v14_v & 1
    v14_v = v14_v >>> 1
  }  // [int_bit_count]
  let v15 = v3.len()  // [string_len]
  let v16 = v10
  assert(v16 == v16)  // [assert_tautology]
  let v17_fn = function(p0, p1, p2) {
    local u0 = 9223372036854775807
    try { u0 = p1.tointeger() } catch (_e) {}  // [string_tointeger]
    local u1_state = 0
    let u1_inc = function() { u1_state++; return u1_state }
    u1_inc()
    u1_inc()
    let u1 = u1_inc()  // [closure_counter]
    let u2 = p1.indexof(v3) ?? 129  // [string_indexof]
    local u3 = typeof p2
    switch (u3) {
      case "integer": u3 = "int"; break
      case "float": u3 = "flt"; break
      case "string": u3 = "str"; break
      default: u3 = "other"
    }  // [typeof_switch]
    let u4_fn = function(x, ctx = {
        cls = class { v = 0; constructor(n) { this.v = n } },
        wrap = @(inst) inst.v + -9223372036854775807
      }) {
      return ctx.wrap(ctx.cls(x))
    }
    let u4 = u4_fn(v14)  // [default_param_table_class]
    let u5_fns = [@(x) x + -130, @(x) x * 127, @(x) x - -257]
    local u5 = u1
    foreach (f in u5_fns) u5 = f(u5)  // [array_of_closures_accumulate]
    let u6 = typeof u4 == "integer"  // [typeof_int_check]
    let u7 = (@(a, b) a + b)(u3, p1)  // [lambda_str_concat]
    return u7
  }
  let v17 = v17_fn(v5, v3, v13)  // [nested_func depth=1]
  let v18_fn = function(n) {
    local i = 0
    while (i < 8) {
      if (i * n > -127) return i
      i++
    }
    return i
  }
  let v18 = v18_fn(v7)  // [while_early_return]
  let v19 = v0.tostring()  // [int_to_string]
  let v20 = v3.indexof(v17) ?? -65537  // [string_indexof]
  let v21 = v6 ? v13 : v5  // [ternary_int]
  let v22_outer = function(x) {
    let inner = @(n) n * -65535
    return inner(x) + 4294967297
  }
  let v22 = v22_outer(v0)  // [nested_function]
  let v23_fn = function(p0, p1, p2) {
    let u0 = __LINE__  // [magic_line]
    let u1 = -v0  // [int_neg]
    local u2 = 1
    local u2_i = 0
    while (u2_i < 1) {
      u2 = u2 * ((v0 % 4) + 1)
      u2_i++
    }  // [while_mul]
    local u3 = false
    try { u3 = u1 == v7 } catch (_e) {}  // [any_cmp_eq]
    local u4 = v13
    ++u4  // [prefix_inc]
    let u5 = v3.rstrip()  // [string_rstrip]
    return u5
  }
  let v23 = v23_fn(v12, v19, v6)  // [nested_func depth=1]
  local v24_iseven = null
  local v24_isodd = null
  v24_iseven = @(n) (n & 7) == 0 ? true : v24_isodd(n - 1)
  v24_isodd = @(n) (n & 7) == 0 ? false : v24_iseven(n - 1)
  let v24 = v24_iseven(v5 & 7) ? -3 : 255  // [mutual_recursion]
  let v25 = v19.len()  // [string_len]
  let v26_fn = function(...) {
    return vargv.len() > 0 ? vargv[0] : 256
  }
  let v26 = v26_fn(v8)  // [vargv_first]
  local v27 = 0
  switch (v24 & 3) {
    case 0:
    case 1: v27 = -2147483648; break
    case 2: v27 = -3; break
    default: v27 = '\t'
  }  // [switch_fallthrough]
  local v28 = -4294967295
  if (v26 > v13) {
    v28 = 65536
  } else if (v26 == v13) {
    v28 = 0
  } else {
    v28 = -32768
  }  // [if_chain]
  let v29 = !v11  // [bool_not]
  let v30_fn = @(a: int, b: int) a * b
  let v30 = v30_fn(v18, v21)  // [typed_lambda]
  local v31 = v0
  v31++  // [postfix_inc]
  local v32 = -127
  switch (v25 % 3) {
    case 0: v32 = -255; break
    case 1: v32 = 0xFF; break
    default: v32 = -32769; break
  }  // [switch_default]
  local v33 = v9
  v33 += v28
  v33 -= v22
  v33 *= -4294967298  // [modify_chain]
  let v34_fn = function({dx, dy = -9223372036854775807}, [da, db]) {
    return dx + dy + da + db
  }
  let v34 = v34_fn({dx = v24}, [v15, v18])  // [destruct_param_mixed]
  local v35_sum = 0
  let v35_add = function(x) { v35_sum = v35_sum + x }
  v35_add(v24)
  v35_add(v7)
  v35_add(v24 + v7)
  let v35 = v35_sum  // [closure_accumulator]
  let [v36_a, v36_b] = [v5, v5 + 1]
  let [v36_c] = [v36_a + v36_b]
  let v36 = v36_c  // [destruct_nested]
  let v37_fn = function({da, db}) { return da + db }
  let v37 = v37_fn({da = v30, db = v27})  // [destruct_param_table]
  let v38_fn = function(x,
      cls = class { function calc(v) { return v + 4294967295 } }) {
    return cls().calc(x)
  }
  let v38_a = v38_fn(v4)
  let v38_other = class { function calc(v) { return v * 0x100 } }
  let v38_b = v38_fn(v4, v38_other)
  let v38 = v38_a + v38_b  // [default_param_class]
  let v39_fn = function(p0, p1) {
    local u0 = -32770
    switch (v17) {
      case "a": u0 = 42; break
      case "b": u0 = -2147483647; break
      case "c": u0 = 1; break
      default: u0 = -257
    }  // [switch_string]
    let u1_fn = function(n) {
      local x = n & 7
      if (x <= 0) return 0
      return x + callee()(x - 1)
    }
    let u1 = u1_fn(v7)  // [func_recursive_sum]
    local u2 = v0
    u2 += 0xDEADBEEF  // [modify_add_const]
    let u3_fn = function() {
      let w0_t = {a = {b = {c = v36}}}
      let w0 = w0_t?.a?.b?.c ?? -1  // [table_deep]
      let w1_fns = [@(x) x + 256, @(x) x * 65536, @(x) x - -257]
      local w1 = v1
      foreach (f in w1_fns) w1 = f(w1)  // [array_of_closures_accumulate]
      let w2_idx = v17.indexof(v23)
      let w2 = w2_idx != null ? w2_idx : 0x80000000  // [string_find_method]
      local w3 = 0
      for (local w3_i = 0; w3_i < 2; w3_i++) {
        w3 += v30
        if (w3 >= (v13 & 0x7FFFFFFF)) break
      }  // [for_break_early]
      return w3
    }
    let u3 = u3_fn()  // [nested_func depth=2]
    let u4_fn = function() {
      let w0 = {a = null, b = null}  // [null_table_value]
      let w1_make = @(bv) @(x) x + bv
      let w1_add = w1_make(v12)
      let w1 = w1_add(32768)  // [func_returns_func]
      let w2_fn = function(x: int): string|null { return x > -32770 ? "[]{}()" : null }
      let w2 = w2_fn(v13) != null  // [typed_return_null]
      function [pure] w3_fn(x) { return x * 65535 }
      let w3 = w3_fn(v12)  // [named_func_pure]
      const function [pure] w4_fn(a, b) { return a + b * 65536 }
      const w4 = w4_fn(-4294967298, 3)  // [const_pure_call_multi]
      return w4
    }
    let u4 = u4_fn()  // [nested_func depth=2]
    let u5_fn = function() {
      local w0 = ""
      if (v11) {
        w0 = v19
      } else {
        w0 = v23
      }  // [if_else_str]
      let w1_t = {x = v25}
      w1_t.x *= -9223372036854775807
      let w1 = w1_t.x  // [table_field_muleq]
      local w2 = v30
      w2++
      w2++
      w2--  // [inc_dec_chain]
      return w2
    }
    let u5 = u5_fn()  // [nested_func depth=2]
    let u6_C = class {
      v = 0
      constructor(x) { this.v = x }
      function get() { return this.v }
    }
    local u6_holder = {obj = u6_C(v31), tag = "\uffff"}
    let u6 = u6_holder?.obj?.get() ?? 2147483647  // [table_with_instance_field]
    return u6
  }
  let v39 = v39_fn(v18, v16)  // [nested_func depth=1]
  local v40 = -4
  switch (v3) {
    case "a": v40 = 1000; break
    case "b": v40 = -127; break
    case "c": v40 = 256; break
    default: v40 = ' '
  }  // [switch_string]
  local v41 = v9 & 7
  do {
    v41--
  } while (v41 > 0)  // [do_while_dec]
  global enum S126_E2 { V1 = 3, V2 = -32769 }
  let v42_t = {a = S126_E2.V1, b = S126_E2.V2}
  let v42 = v42_t.a + v42_t.b  // [enum_in_table]
  let v43_fn = function(x: int|null): int { return x != null ? x : 0x80000000 }
  let v43 = v43_fn(v8)  // [typed_nullable_param]
  local v44 = v25 & 7
  do {
    v44--
  } while (v44 > 0)  // [do_while_dec]
  let v45_fn = function(p0, p1) {
    let u0_fn = function(n) {
      local i = 0
      while (i < -1) {
        if (i * n > 3) return i
        i++
      }
      return i
    }
    let u0 = u0_fn(v34)  // [while_early_return]
    let u1_fn = function(x: int): int { return x * 1000 }
    let u1 = u1_fn(v40)  // [typed_function_return]
    let u2_C = class {
      _a = 0
      _b = 0
      constructor(xa, xb) { this._a = xa; this._b = xb }
      function sum() { return this._a + this._b }
      function diff() { return this._a - this._b }
    }
    let u2_obj = u2_C(v15, v2)
    let u2 = u2_obj.sum() + u2_obj.diff()  // [class_method_call]
    let u3_fn = function(x: int|null): int { return x != null ? x : -2 }
    let u3 = u3_fn(v22)  // [typed_nullable_param]
    let u4_fn = function(a, b,
        combine = {op = @(x, y) x + y, identity = -3}) {
      return combine.op(a, b) + combine.identity
    }
    let u4_add = u4_fn(v4, v26)
    let u4_mul = u4_fn(v4, v26,
      {op = @(x, y) x * y, identity = -4294967295})
    let u4 = u4_add + u4_mul  // [default_param_fn_field_call]
    let u5 = (function() {
      let gen = (function() {
        yield 0
        yield -32769
        yield 32769
      })()
      let arr = []
      while (gen.getstatus() == "suspended") {
        arr.append(resume gen)
      }
      return arr
    })()  // [coroutine_collect_all]
    return u4
  }
  let v45 = v45_fn(v39, v25)  // [nested_func depth=1]
  let v46 = {}  // [empty_table]
  let v47_t = {arr = [0, null, {key = v39}]}
  let v47 = v47_t?.arr?[2]?.key ?? 42  // [nested_table_array_nullsafe_read]
  let v48_fn = function(x, s, cfg = {
      fmt = @(n, t) $"{n}:{t}",
      limits = {lo = -2147483648, hi = -65535}
    }) {
    local clamped = x < cfg.limits.lo ? cfg.limits.lo
                  : x > cfg.limits.hi ? cfg.limits.hi : x
    return cfg.fmt(clamped, s)
  }
  let v48 = v48_fn(v9, v17)  // [default_param_config_nested]
  local v49 = 32769
  if (let v49_v = v46?.x; v49_v != null) {
    v49 = v49_v
  }  // [if_let_nullcheck]
  local v50 = 0
  foreach (c in v48) {
    if (c >= '0' && c <= '9') v50++
  }  // [string_scan_digits]
  local v51 = 2147483648
  if (v29) {
    v51 = v2 + 128
  } else {
    v51 = v12 - 4294967297
  }  // [if_else_int]
  local v52 = ""
  foreach (c in v17) {
    if (c != ' ') v52 += c.tochar()
  }  // [string_build_filtered]
  const function [pure] v53_fn(x) { return x + 0xDEADBEEF }
  const v53_a = 0x1
  const v53 = v53_fn(v53_a)  // [const_chain]
  const v54 = -32769 + -255  // [const_expr]
  let v55 = typeof null == "null"  // [typeof_null_check]
  const function [pure] v56_fn(x) { return x * -1 }
  const v56 = v56_fn(4294967296)  // [const_pure_call]
  let v57_fn = function(p0, p1) {
    let u0 = []
    local u0_i = 0
    while (u0_i < 3) {
      u0.append(u0_i * u0_i)
      u0_i++
    }  // [while_array_fill]
    let u1_fn = function(x, ctx = {
        cls = class { v = 0; constructor(n) { this.v = n } },
        wrap = @(inst) inst.v + -4294967295
      }) {
      return ctx.wrap(ctx.cls(x))
    }
    let u1 = u1_fn(v9)  // [default_param_table_class]
    let u2_fn = function() {
      local w0 = 0
      for (local w0_i = 7; w0_i > 0; w0_i--) {
        w0 += v56
      }  // [for_countdown]
      local w1 = ""
      local w1_i = 0
      while (w1_i < 1) {
        w1 = w1 + v3
        w1_i++
      }  // [while_string_build]
      local w2 = 0
      u0.each(@(_v) w2++)  // [array_each_lambda]
      return w2
    }
    let u2 = u2_fn()  // [nested_func depth=2]
    let u3 = v9.tofloat()  // [int_to_float]
    let u4 = u0.findvalue(@(v) v != null) ?? 2147483648  // [array_findvalue]
    let u5 = array(v38 & 7, 128)  // [array_function_init]
    return u4
  }
  let v57 = v57_fn(v50, v7)  // [nested_func depth=1]
  local v58_state = 0
  let v58_inc = function() { v58_state++; return v58_state }
  v58_inc()
  v58_inc()
  let v58 = v58_inc()  // [closure_counter]
  enum S126_E3 { LOW = 0, MID = 50, HIGH = 100 }
  local v59 = ""
  if (v51 < S126_E3.MID) v59 = "low"
  else if (v51 < S126_E3.HIGH) v59 = "mid"
  else v59 = "high"  // [enum_compare_chain]
  let v60 = [v35, v17, true, null]  // [array_mixed_types]
  let v61 = v60.len()  // [array_len]
  local v62_state = {scores = [v51, -4294967296, 2], tag = "test"}
  v62_state.scores[0] += -65536
  v62_state.scores[2] = v62_state.scores[0] + v62_state.scores[1]
  let v62 = v62_state.scores.reduce(@(a, v) a + v, 0)  // [table_slot_array_modify]
  let v63_fn = @[pure, nodiscard] (a, b) a + b
  let v63 = v63_fn(v2, v36)  // [func_pure_nodiscard]
  let v64 = $"{v19}{v59}"  // [string_concat]
  let v65 = v29 ? v58 : v40  // [ternary_int]
  enum S126_E4 { LOW = 0, MID = 1, HIGH = 2 }
  let v66 = (v30 & 3) == S126_E4.MID  // [enum_compare]
  let v67_fn = function(p0, p1, p2) {
    if (v60.len() > 0) v60[0] = v32
    let u0 = v60?[0] ?? 4294967296  // [array_set_elem]
    let u1 = v60.hasvalue(v57)  // [array_hasvalue]
    let u2_fn = function() {
      let w0 = v48.len() > 4 ? v48.slice(1, 3) : v48  // [string_slice_both]
      let w1 = freeze(clone v46)  // [freeze_table]
      let w2 = v29 ? v34 : v57  // [ternary_int]
      return w2
    }
    let u2 = u2_fn()  // [nested_func depth=2]
    let u3 = array(v62 & 7, -65536)  // [array_function_init]
    return u2
  }
  let v67 = v67_fn(v4, v45, v53)  // [nested_func depth=1]
  let v68 = v46.len() > 0 ? v46.reduce(@(a, _v) a) : -2147483649  // [table_reduce]
  let v69 = typeof v60 == "array"  // [typeof_array_check]
  function [pure, nodiscard] v70_fn(x) { return x - 0x1 }
  let v70 = v70_fn(v26)  // [named_func_pure_nodiscard]
  local v71 = 0
  foreach (_k, _v in v46) {
    v71++
  }  // [foreach_table_keys]
  let v72 = v46.values()  // [table_values]
  local v73 = 0
  for (local v73_i = 0; v73_i < 1; v73_i++) {
    if (v73_i % 2 == 0) continue
    v73 = v73 + v2
  }  // [for_continue]
  let v74 = (@(x) -x)(v50)  // [lambda_negate]
  let v75_fn = function() {
    let u0 = [v1, v73]  // [array_of_ints]
    let u1_fn = function(...) {
      local s = 0
      foreach (v in vargv) s += v
      return s
    }
    let u1 = u1_fn(v44, v74, -4294967297)  // [vargv_sum]
    let u2_fn = function(q0) {
      local w0 = 0
      foreach (c in v3) {
        if (c >= 'A' && c <= 'Z') w0++
      }  // [string_scan_upper]
      let w1 = (true ? (v55 ? v23 : v23) : v59)  // [string_literal]
      let w2 = []
      local w2_i = 0
      while (w2_i < 0) {
        w2.append(w2_i * w2_i)
        w2_i++
      }  // [while_array_fill]
      let w3 = v63.tofloat()  // [int_to_float]
      local w4 = 0
      foreach (_c in v59) w4++  // [foreach_string_chars]
      let w5 = v46?.x ?? 256  // [nullsafe_field]
      return w5
    }
    let u2 = u2_fn(v56)  // [nested_func depth=2]
    let u3_t = {x = v35, y = 0xDEADBEEF}
    let u3_v = u3_t.rawget("x") ?? -65536
    u3_t.rawdelete("x")
    let u3 = u3_v  // [table_rawdelete]
    local u4 = ""
    if (let u4_v = v64; u4_v.len() > 0) {
      u4 = u4_v.toupper()
    }  // [if_let_string]
    const u5 = 1e-40  // [const_float]
    return u5
  }
  let v75 = v75_fn()  // [nested_func depth=1]
  let v76 = v32 < v30 ? v32 : v30  // [int_min_two]
  local v77: string = v59  // [typed_string_var]
  let v78_fn = function(p0, p1, p2) {
    let u0 = v45 > 0 ? 1 : (v45 < 0 ? -1 : 0)  // [int_sign]
    let u1_t = freeze({x = v24, y = -4})
    let u1 = u1_t.x + u1_t.y  // [freeze_nested]
    local u2_val = v34
    let u2_set = function(x) { u2_val = x }
    let u2_get = function() { return u2_val }
    u2_set(v34 + -255)
    let u2 = u2_get()  // [closure_getter_setter]
    let u3 = v41
    assert(u3 == u3)  // [assert_tautology]
    let u4 = v75.tointeger()  // [float_to_int]
    return u4
  }
  let v78 = v78_fn(v42, v10, v6)  // [nested_func depth=1]
  let v79 = v46.len()  // [table_len]
  let v80 = clone v72
  for (local v80_i = 0; v80_i < v80.len(); v80_i++)
    v80[v80_i] = v49  // [array_modify_elems]
  local v81 = -257
  if (let v81_v = v46?.x; v81_v != null) {
    v81 = v81_v
  }  // [if_let_nullcheck]
  let v82_fn = function(a, b,
      combine = {op = @(x, y) x + y, identity = 257}) {
    return combine.op(a, b) + combine.identity
  }
  let v82_add = v82_fn(v26, v63)
  let v82_mul = v82_fn(v26, v63,
    {op = @(x, y) x * y, identity = -4294967297})
  let v82 = v82_add + v82_mul  // [default_param_fn_field_call]
  local v83_cfg = {a = {val = v25 + 32769}, b = {val = v25 - 0x8000}}
  let v83 = v66 ? (v83_cfg?.a?.val ?? 0x100) : (v83_cfg?.b?.val ?? 0x8000)  // [conditional_deep_path]
  let v84_fn = function(p0) {
    local u0 = -256
    u0 = v40
    u0 = u0 + 2147483649
    u0 = u0 * 3  // [expr_stmt_assign_chain]
    let u1 = v4 || v71  // [logical_or_val]
    let u2 = v43 != 0 ? v58 % v43 : 0x7FFF  // [int_mod_safe]
    return u2
  }
  let v84 = v84_fn(v57)  // [nested_func depth=1]
  let v85_t = clone v46
  v85_t.tmp <- -2147483648
  let v85 = v85_t.tmp  // [table_delete_read]
  let v86 = typeof null == "null"  // [typeof_null_check]
  let v87_v = v39 & 0xFFFF
  let v87 = v87_v > 0 && (v87_v & (v87_v - 1)) == 0  // [int_pow2_check]
  let v88_fn = function([da, db = -4]) { return da * db }
  let v88 = v88_fn([v45])  // [destruct_param_array_default]
  let v89_arr = v46?.keys
  let v89 = v89_arr != null ? v46.keys().len() : -4294967298  // [nullsafe_method_on_result]
  let v90 = [v86, v66, true, false]  // [array_of_bools]
  let v91 = v3 == v48  // [str_cmp_eq]
  let v92_fn = function(p0, p1) {
    let u0 = v46.keys()  // [table_keys]
    local u1 = 3
    try {
      let g = (function() { yield v34; yield v34 + 1 })()
      u1 = resume g
    } catch (_e) {
      u1 = -2147483648
    }  // [coroutine_try_catch]
    local u2 = v9
    u2 *= v25  // [modify_mul_int]
    let u3 = []  // [empty_array]
    return u2
  }
  let v92 = v92_fn(v5, v35)  // [nested_func depth=1]
  let v93 = v90.len() >= 2
    ? v90[0] + v90[1]
    : v90.len() == 1 ? v90[0] : -65535  // [array_two_elems]
  let v94_fn = function(x: int|float): int { return x.tointeger() }
  let v94 = v94_fn(v38)  // [typed_union_param]
  let v95 = (function(x) { return x * x })(v39)  // [immediate_invoke]
  local v96 = v18
  v96 %= v13  // [modify_mod_int_unsafe]
  let v97 = [65535, 129, 4294967295]
  v97.insert(1, v38)
  v97.remove(0)  // [array_insert_remove]
  local v98 = v77
  v98 += v17  // [modify_string_concat]
  let v99_a = [v26, v26 + 1]
  v99_a.append(v26 * 2)
  let v99 = v99_a.pop()  // [array_push_pop]
  local v100 = ""
  local v100_i = 0
  while (v100_i < 1) {
    v100 = v100 + v52
    v100_i++
  }  // [while_string_build]
  let v101 = v87 ? v100 : v17  // [ternary_str]
  let v102_fn = function(p0, p1, p2) {
    let u0 = [v18, v34]  // [array_of_ints]
    let u1 = v19 < p0  // [string_cmp_lt]
    let u2_t = clone v46
    u2_t.clear()
    let u2 = u2_t.len()  // [table_clear]
    let u3_fn = function(a, b = "test") { return a + b }
    let u3 = u3_fn(p0)  // [default_param_string]
    let u4 = v46.map(@(v) v)  // [table_map]
    const u5 = "\n"  // [const_string]
    return u5
  }
  let v102 = v102_fn(v52, v57, v42)  // [nested_func depth=1]
  local v103 = 0
  foreach (c in v48) {
    if (c >= 'A' && c <= 'Z') v103++
  }  // [string_scan_upper]
  let v104_fn = function(p0) {
    let u0_Base = class {
      function compute(x) { return x + -129 }
    }
    let u0_Der = class(u0_Base) {
      function compute(x) { return base.compute(x) * 32767 }
    }
    let u0 = u0_Der().compute(v16)  // [class_method_override]
    let u1 = -v75  // [float_neg]
    let u2_fn = function(q0, q1) {
      local w0 = ""
      foreach (c in v98) {
        if (c != ' ') w0 += c.tochar()
      }  // [string_build_filtered]
      let w1_f = @(x) x * -4294967296
      let w1_g = @(x) x + 'A'
      let w1 = w1_f(w1_g(v68))  // [func_compose]
      let w2_fn = function(a, b = "0") { return a + b }
      let w2 = w2_fn(v101)  // [default_param_string]
      let w3_C = class {
        xv = 0
        constructor(x) { this.xv = x }
        function _sub(o) { return this.getclass()(this.xv - o.xv) }
      }
      let w3_r = w3_C(v51) - w3_C(v70)
      let w3 = w3_r.xv  // [meta_sub]
      let w4_fn = function(r0, r1, r2) {
        let z0 = v6 && v66  // [bool_and]
        let z1_Base = class { xv = 0 }
        let z1_Der = class(z1_Base) { yv = 0 }
        let z1 = z1_Der.getbase() == z1_Base  // [class_getbase]
        let z2 = [-2147483648, -4294967297, 0x7FFF]
        z2.insert(1, v8)
        z2.remove(0)  // [array_insert_remove]
        local z3: float = u1  // [typed_float_var]
        return z3
      }
      let w4 = w4_fn(v79, v42, u1)  // [nested_func depth=3]
      return w4
    }
    let u2 = u2_fn(v42, v15)  // [nested_func depth=2]
    let u3_fn = function(q0, q1) {
      const w0_BASE = 256
      const w0_SCALE = -65536
      let w0 = v24 * w0_SCALE + w0_BASE  // [const_in_expr]
      let w1 = v80.map(@(v) v)  // [array_map_lambda]
      let w2 = v80.filter(@(v) v != null)  // [array_filter_lambda]
      print($"array.len={w2.len()}\n")  // [print_array]
      local w3_t = {arr = [9223372036854775807, null, {key = v88}]}
      local w3_inner = w3_t?.arr?[2]
      if (w3_inner != null) w3_inner.key += 257
      let w3 = w3_t?.arr?[2]?.key ?? 1  // [nested_table_array_modify]
      let w4_fn = function() {
        let z0 = typeof u2 == "float"  // [typeof_float_check]
        let z1 = v17.lstrip()  // [string_lstrip]
        function [pure] z2_fn(x) { return x * 9223372036854775807 }
        let z2 = z2_fn(v84)  // [named_func_pure]
        return z2
      }
      let w4 = w4_fn()  // [nested_func depth=3]
      return w4
    }
    let u3 = u3_fn(v89, v37)  // [nested_func depth=2]
    return u3
  }
  let v104 = v104_fn(v34)  // [nested_func depth=1]
  let v105 = v101.replace("\u0000", "\"")  // [string_replace]
  let v106_inner = function(...) {
    local s = 0
    foreach (v in vargv) s += v
    return s
  }
  let v106_outer = function(a, ...) { return a + v106_inner(vargv.len(), a) }
  let v106 = v106_outer(v15, v32)  // [vargv_forward]
  let v107_C = class {
    _val = 0
    constructor(xv) { this._val = xv }
    function getVal() { return this._val }
  }
  let v107_obj = v107_C(v20)
  let v107 = v107_obj.getVal()  // [class_basic]
  let v108_fn = function({da, db = -130}) { return da + db }
  let v108 = v108_fn({da = v93})  // [destruct_param_table_default]
  let v109_fn = function(a, b = 3, c = '\'') { return a + b + c }
  let v109 = v109_fn(v67)  // [default_param_multi]
  let v110_fn = function(a, b = 2) { return a + b }
  let v110 = v110_fn(v44, v45)  // [default_param_call_with]
  let v111 = (@(a, b) a + b)(v10, v45)  // [immediate_lambda]
  let v112 = freeze(clone v80)  // [freeze_array]
  let v113 = v64.escape()  // [string_escape]
  print($"int={v110}\n")  // [print_int]
  const v114 = 3  // [const_int]
  let v115 = type(v106)  // [type_function]
  let [v116_a, v116_b] = [v18, v15]
  let [v116_x, v116_y] = [v116_b, v116_a]
  let v116 = v116_x + v116_y  // [destruct_swap]
  let v117 = v69 || v66  // [bool_or]
  let v118_fn = function(p0) {
    let u0 = v48.startswith(v98)  // [string_startswith]
    let u1_q = [v111, v2]
    u1_q.append(0xFFFF)
    let u1_val = u1_q[0]
    u1_q.remove(0)
    let u1 = u1_val  // [array_as_queue]
    let u2 = $"val={v75}"  // [string_interp_float]
    return u2
  }
  let v118 = v118_fn(v37)  // [nested_func depth=1]
  let v119 = {x = v57, s = v23}  // [table_create]
  let v120 = {[v115] = v31, ["y"] = -2147483648}  // [table_computed_key]
  let v121_C = class {
    xv = 0
    constructor(x) { this.xv = x }
  }
  let v121_arr = [v121_C(v70), v121_C(v70 + 1), v121_C(v70 + 2)]
  let v121 = v121_arr[0].xv + v121_arr[1].xv + v121_arr[2].xv  // [class_array_of]
  let v122 = v90.hasvalue(v37)  // [array_hasvalue]
  let v123_fn = function() {
    local u0 = 0
    foreach (c in v118) {
      if (c >= 'A' && c <= 'Z') u0++
    }  // [string_scan_upper]
    let u1 = ~v7  // [int_bitnot]
    let u2_fn = function(q0, q1, q2) {
      let w0 = {}  // [empty_table]
      let w1_fn = function(n) {
        local i = 0
        while (i < 7) {
          if (i * n > 0x1) return i
          i++
        }
        return i
      }
      let w1 = w1_fn(v107)  // [while_early_return]
      local w2 = v54 & 7
      do {
        w2--
      } while (w2 > 0)  // [do_while_dec]
      let w3_t = {[v48] = v4}
      let w3 = w3_t?[v48] ?? -2147483649  // [table_computed_key_read]
      let w4_fn = function() {
        local z0_val = v94
        let z0_set = function(x) { z0_val = x }
        let z0_get = function() { return z0_val }
        z0_set(v94 + 32767)
        let z0 = z0_get()  // [closure_getter_setter]
        local z1 = v109
        z1 -= v45  // [modify_sub_int]
        let z2 = null  // [null_var]
        let z3 = [v104, v103]  // [array_of_ints]
        return z2
      }
      let w4 = w4_fn()  // [nested_func depth=3]
      return w4
    }
    let u2 = u2_fn(v49, v29, v25)  // [nested_func depth=2]
    print($"float={v75}\n")  // [print_float]
    local u3 = 0
    local u3_v = v65 & 0xFF
    while (u3_v != 0) {
      u3 += u3_v & 1
      u3_v = u3_v >>> 1
    }  // [int_bit_count]
    return u3
  }
  let v123 = v123_fn()  // [nested_func depth=1]
  let v124_t = {arr = [4294967296, null, {key = v10}]}
  let v124 = v124_t?.arr?[2]?.key ?? -2147483647  // [nested_table_array_nullsafe_read]
  let v125 = (false ? (((-255) <= 65537) ? true : true) : v87)  // [bool_literal]
  local v126 = ""
  foreach (c in v115) {
    if (c != ' ') v126 += c.tochar()
  }  // [string_build_filtered]
  let v127_fn = function(p0, p1) {
    let u0_fn = @[nodiscard] (x) x + 4294967297
    let u0 = u0_fn(v82)  // [func_nodiscard]
    let u1 = v3.tolower()  // [string_tolower]
    let u2_C = class {
      xv = 0
      extra = 0
      constructor(x) { this.xv = x }
      function _cloned(_other) { this.extra = -4294967295 }
    }
    let u2_orig = u2_C(v47)
    let u2_cpy = clone u2_orig
    let u2 = u2_cpy.xv + u2_cpy.extra  // [meta_cloned]
    let u3_fn = function(q0) {
      local w0 = v76
      --w0  // [prefix_dec]
      local w1 = typeof v114
      switch (w1) {
        case "integer": w1 = "int"; break
        case "float": w1 = "flt"; break
        case "string": w1 = "str"; break
        default: w1 = "other"
      }  // [typeof_switch]
      let w2_C = class {
        v = 0
        constructor(x) { this.v = x }
        function get() { return this.v }
      }
      local w2_holder = {obj = w2_C(v45), tag = "\t\n\r"}
      let w2 = w2_holder?.obj?.get() ?? 0xFFFF  // [table_with_instance_field]
      let w3 = (1e-37).tointeger()  // [int_literal]
      let w4 = v90.len() >= 2
        ? v90[0] + v90[1]
        : v90.len() == 1 ? v90[0] : 32767  // [array_two_elems]
      return w4
    }
    let u3 = u3_fn(v79)  // [nested_func depth=2]
    let u4_fn = function() {
      let w0 = []
      v119.each(@(v, _k) w0.append(v))  // [table_each_collect]
      let w1_fn = @[nodiscard] (x) x + 32769
      let w1 = w1_fn(v116)  // [func_nodiscard]
      let w2_fn = function(n) {
        local i = 0
        while (i < 3) {
          if (i * n > -32767) return i
          i++
        }
        return i
      }
      let w2 = w2_fn(v54)  // [while_early_return]
      let w3_fn = @[pure, nodiscard] (a, b) a + b
      let w3 = w3_fn(v16, v28)  // [func_pure_nodiscard]
      let w4_fn = function(r0, r1) {
        let z0 = array(v111 & 7, -9223372036854775807)  // [array_function_init]
        let z1_a = [v79, v79 + 1]
        z1_a.append(v79 * 2)
        let z1 = z1_a.pop()  // [array_push_pop]
        local z2_db = {users = {alice = {score = 65535, level = 32769},
                                 bob   = {score = 4294967295, level = -257}}}
        z2_db.users.alice.score += v94
        z2_db.users.bob.score   -= v94
        let z2 = (z2_db?.users?.alice?.score ?? 128)
                  + (z2_db?.users?.bob?.score ?? -65535)  // [deep_nested_db_modify]
        let z3 = []
        local z3_i = 0
        while (z3_i < -1) {
          z3.append(z3_i * z3_i)
          z3_i++
        }  // [while_array_fill]
        return z2
      }
      let w4 = w4_fn(v32, v96)  // [nested_func depth=3]
      let w5 = v119?.x ?? -129  // [null_coalesce]
      return w5
    }
    let u4 = u4_fn()  // [nested_func depth=2]
    let u5 = v120.values()  // [table_values]
    return u4
  }
  let v127 = v127_fn(v28, v108)  // [nested_func depth=1]
  local v128_mat = [[2147483647, -32768, 1000],
                   [-127, 0xFFFF, -256]]
  v128_mat[0][1] += v24
  v128_mat[1][0] -= v24
  let v128 = (v128_mat?[0]?[1] ?? 0) + (v128_mat?[1]?[0] ?? -127)  // [matrix_modify_read]
  let v129_C = class {
    xv = 0
    constructor(x) { this.xv = x }
    function _mul(o) { return this.getclass()(this.xv * o.xv) }
  }
  let v129_r = v129_C(v88) * v129_C(v67)
  let v129 = v129_r.xv  // [meta_mul]
  let v130 = v14 >>> -1  // [int_ushr]
  let v131 = v72.len() > 0 ? v72.reduce(@(a, _b) a) : 2  // [array_reduce_lambda]
  let v132 = clone v60
  if (v132.len() >= 2) v132.swap(0, v132.len() - 1)  // [array_swap]
  let v133 = v97.hasvalue(v30)  // [array_hasvalue]
  let v134 = v64.tolower()  // [string_tolower]
  let v135_fn = function(p0, p1, p2) {
    let u0 = v93 != v81  // [int_cmp_ne]
    let u1 = (("\"" + v19)).toupper()  // [string_literal]
    const function [pure] u2_fn(a, b) { return a + b * -2147483647 }
    const u2 = u2_fn(32767, 32767)  // [const_pure_call_multi]
    let u3_fn = function(q0) {
      let w0 = ((v125 || q0) ? v126 : ("he said \"hi\"").toupper())  // [string_literal]
      enum S126_E5 { A, B, C }
      let w1 = S126_E5.A + S126_E5.B + S126_E5.C  // [enum_basic]
      let w2 = clone v120
      w2.__update(v119)  // [table_update]
      local w3_a = v107
      local w3_b = v5
      w3_a = w3_a ^ w3_b
      w3_b = w3_a ^ w3_b
      w3_a = w3_a ^ w3_b
      let w3 = w3_a  // [int_swap_via_xor]
      let w4_fn = function(r0, r1, r2) {
        local z0_maybe = v25 > -32770 ? v25 : null
        let z0 = z0_maybe ?? 4294967295  // [null_assign_fallback]
        let z1 = (w2.findindex(@(_v) true) ?? "\u0000").tostring()  // [table_findindex]
        let z2 = u2.tofloat() + v75  // [int_float_arith]
        let z3_C = class {
          xv = 0
          constructor(x) { this.xv = x }
          function _add(o) { return this.getclass()(this.xv + o.xv) }
        }
        let z3_r = z3_C(v73) + z3_C(v93)
        let z3 = z3_r.xv  // [meta_add]
        return z3
      }
      let w4 = w4_fn(v53, v96, v36)  // [nested_func depth=3]
      return w4
    }
    let u3 = u3_fn(v125)  // [nested_func depth=2]
    let u4 = ((v67 >>> 1)).tostring()  // [string_literal]
    let u5 = "{0}+{1}".subst(v59, "[]{}()")  // [string_subst]
    local u6_store = {pages = [{lines = [4294967295, 0xFFFFFFFF]}, {lines = [65535, 2147483647]}]}
    u6_store.pages[0].lines[1] += u2
    let u6 = u6_store?.pages?[0]?.lines?[1] ?? -32768  // [nested_db_array_field]
    return u6
  }
  let v135 = v135_fn(v62, v91, v16)  // [nested_func depth=1]
  local v136 = 0
  foreach (v in v90) {
    if (typeof v == "integer") v136 = v136 + v
  }  // [foreach_array_sum]
  let v137 = -v75  // [float_neg]
  const v138 = -1e+37  // [const_float]
  let v139 = v119.__merge(v120)  // [table_merge]
  local v140 = ""
  foreach (c in v105) {
    if (c != ' ') v140 += c.tochar()
  }  // [string_build_filtered]
  let v141_fn = function(x) {
    local cached = static({v = -2147483647 * 4294967296})
    return x + cached.v
  }
  let v141 = v141_fn(v114)  // [static_complex]
  let v142 = v97.totable()  // [array_totable]
  let v143_fn = function(x, s, cfg = {
      fmt = @(n, t) $"{n}:{t}",
      limits = {lo = -1, hi = -32770}
    }) {
    local clamped = x < cfg.limits.lo ? cfg.limits.lo
                  : x > cfg.limits.hi ? cfg.limits.hi : x
    return cfg.fmt(clamped, s)
  }
  let v143 = v143_fn(v85, v118)  // [default_param_config_nested]
  let v144_fn = function(p0) {
    let u0 = v97.totable()  // [array_totable]
    local u1 = -9223372036854775807
    switch (v77) {
      case "a": u1 = -4; break
      case "b": u1 = 257; break
      case "c": u1 = -2147483648; break
      default: u1 = 0
    }  // [switch_string]
    let u2 = (function() {
      let gen = (function() {
        yield -257
        yield -257
        yield -4
      })()
      let arr = []
      while (gen.getstatus() == "suspended") {
        arr.append(resume gen)
      }
      return arr
    })()  // [coroutine_collect_all]
    return u1
  }
  let v144 = v144_fn(v17)  // [nested_func depth=1]
  let v145 = $"{v19}{v59}"  // [string_concat]
  let v146 = v120.keys()  // [table_keys]
  let v147 = v119.rawget("x") ?? -32770  // [rawget_table]
  local v148 = v138
  v148 *= v75  // [modify_mul_float]
  let v149_stack = []
  v149_stack.append(v58)
  v149_stack.append(v54)
  let v149 = v149_stack.pop()  // [table_as_stack]
  let v150_t = {arr = [0x8000, null, {key = v76}]}
  let v150 = v150_t?.arr?[2]?.key ?? 65535  // [nested_table_array_nullsafe_read]
  let v151 = v72.hasindex(0)  // [array_hasindex]
  let v152 = v46.topairs()  // [table_topairs]
  let v153 = v117 && v11  // [bool_and]
  let v154_fn = @(a: int, b: int) a * b
  let v154 = v154_fn(v2, v67)  // [typed_lambda]
  let v155 = v97.totable()  // [array_totable]
  let v156 = v5 >= v99  // [int_cmp_ge]
  let v157_fn = function(x) {
    local cached = static({v = -4294967296 * 2147483648})
    return x + cached.v
  }
  let v157 = v157_fn(v9)  // [static_complex]
  enum S126_E6 { LOW = 0, MID = 1, HIGH = 2 }
  let v158 = (v79 & 3) == S126_E6.MID  // [enum_compare]
  let v159 = {}  // [empty_table]
  let v160 = freeze(clone v72)  // [freeze_array]
  let v161 = v155.map(@(v) v)  // [table_map]
  let v162 = v115.toupper()  // [string_toupper]
  let v163_fn = function(a, b = "true") { return a + b }
  let v163 = v163_fn(v145)  // [default_param_string]
  local v164 = -127
  if (let v164_v = v31; v164_v > 0xFFFF) {
    v164 = v164_v * -255
  }  // [if_let_positive]
  let v165_fn = function() {
    let u0 = v161.filter(@(v) v != null)  // [table_filter]
    let u1 = u0.len() > 0  // [table_getdelegate]
    let u2_fn = function(q0, q1, q2) {
      local w0 = 0
      for (local w0_i = 0; w0_i < 1; w0_i++) {
        for (local w0_j = 0; w0_j < -4; w0_j++) {
          w0 += w0_i * w0_j
        }
      }  // [nested_loops]
      const function [pure] w1_fn(a, b) { return a + b * 2147483648 }
      const w1 = w1_fn(-4294967298, 42)  // [const_pure_call_multi]
      let w2 = v34
      assert(w2 == w2)  // [assert_tautology]
      local w3 = v128
      w3 /= v38  // [modify_div_int_unsafe]
      let w4_t = {a = {b = {c = v45}}}
      let w4 = w4_t?.a?.b?.c ?? 0x8000  // [table_deep]
      return w4
    }
    let u2 = u2_fn(v117, v78, v89)  // [nested_func depth=2]
    return u2
  }
  let v165 = v165_fn()  // [nested_func depth=1]
  let v166 = v28 > 0 ? 1 : (v28 < 0 ? -1 : 0)  // [int_sign]
  let v167 = typeof v119 == "table"  // [typeof_table_check]
  let v168_fn = function(p0, p1, p2) {
    local u0 = 0
    local u0_i = 0
    while (u0_i < 100) {
      if (u0_i >= 5) break
      u0 = u0 + v37
      u0_i++
    }  // [while_break]
    let u1_prev = v161?.x ?? 2147483649
    v161.x <- v53
    let u1 = u1_prev + v161.x  // [table_modify_field]
    let u2 = v106 == v83  // [int_cmp_eq]
    return u2
  }
  let v168 = v168_fn(v42, v87, v106)  // [nested_func depth=1]
  let v169 = freeze(clone v112)  // [freeze_array]
  let v170 = v142.map(@(v) v)  // [table_map]
  let v171 = v170.len() > 0 ? v170.reduce(@(a, _v) a) : 129  // [table_reduce]
  let v172_A = class { v = 0; constructor(x) { this.v = x }; function get() { return this.v } }
  let v172_B = class(v172_A) { bonus = 0; constructor(x) { base.constructor(x); this.bonus = -4294967297 } }
  let v172_C = class(v172_B) { extra = 0; constructor(x) { base.constructor(x); this.extra = -3 } }
  let v172_obj = v172_C(v121)
  let v172 = v172_obj.get() + v172_obj.bonus + v172_obj.extra  // [class_three_level]
  local v173 = 0
  v46.each(@(_v, _k) v173++)  // [table_each]
  let v174 = []
  for (local v174_i = 0; v174_i < -4; v174_i++) {
    v174.append(v174_i)
  }  // [for_build_array]
  let v175 = v21 - v9  // [int_sub]
  let v176 = v90.len() >= 2 ? v90.slice(1) : clone v90  // [array_slice_mid]
  let v177 = [v77, v23, @"raw_string"]  // [array_of_strings]
  let v178_fn = function(p0, p1) {
    let u0 = v0 != 0 ? v135 / v0 : 42  // [int_div_safe]
    let u1 = $"flag={v133}"  // [string_interp_bool]
    local u2_items = [{k = "world", v = -2},
                       {k = "", v = 2147483648},
                       {k = "1e10", v = v14}]
    foreach (item in u2_items)
      if (item.v > -258) item.v *= -128
    local u2 = 0
    foreach (item in u2_items) u2 += item.v  // [foreach_array_tables_modify]
    return u2
  }
  let v178 = v178_fn(v77, v95)  // [nested_func depth=1]
  let v179 = (@(x) x + 4294967296)(v1)  // [lambda_inc]
  let v180 = v59.tolower()  // [string_tolower]
  let v181_C = class {
    _a = 0
    _b = 0
    constructor(xa, xb) { this._a = xa; this._b = xb }
    function sum() { return this._a + this._b }
    function diff() { return this._a - this._b }
  }
  let v181_obj = v181_C(v141, v22)
  let v181 = v181_obj.sum() + v181_obj.diff()  // [class_method_call]
  local v182 = 1
  local v182_i = 0
  while (v182_i < 5) {
    v182 = v182 * ((v54 % 4) + 1)
    v182_i++
  }  // [while_mul]
  local v183 = 0
  foreach (v in v132) {
    if (typeof v == "integer") v183 = v183 + v
  }  // [foreach_array_sum]
  let v184_fn = function(p0, p1) {
    local u0_mat = [[-3, -128, -1],
                     [42, 1000, '\t']]
    u0_mat[0][1] += v131
    u0_mat[1][0] -= v131
    let u0 = (u0_mat?[0]?[1] ?? 32767) + (u0_mat?[1]?[0] ?? -65535)  // [matrix_modify_read]
    let u1 = v102.len() > 2 ? v102.slice(-2) : v102  // [string_slice_neg]
    local u2 = -258
    if (local u2_v = v53 & 0xFF; u2_v > 0x80000000) {
      u2_v = u2_v - -129
      u2 = u2_v
    }  // [if_local_modify]
    let u3 = v91.tostring()  // [bool_to_string]
    let u4 = v85 != v12  // [int_cmp_ne]
    return u4
  }
  let v184 = v184_fn(v85, v63)  // [nested_func depth=1]
  let v185 = typeof v162 == "string"  // [typeof_string_check]
  let v186_C = class {
    xv = 0
    constructor(x) { this.xv = x }
    function _sub(o) { return this.getclass()(this.xv - o.xv) }
  }
  let v186_r = v186_C(v41) - v186_C(v4)
  let v186 = v186_r.xv  // [meta_sub]
  local v187 = v79
  v187 += v57  // [modify_add_int]
  let v188 = [v183, v150]  // [array_of_ints]
  local v189 = 1
  local v189_i = 0
  while (v189_i < 1) {
    v189 = v189 * ((v13 % 4) + 1)
    v189_i++
  }  // [while_mul]
  local v190 = 0
  foreach (_k, _v in v142) {
    v190++
  }  // [foreach_table_keys]
  let v191 = v5 - v71  // [int_sub]
  let v192 = type(v53)  // [type_function]
  let v193_t = {[v180] = v28}
  let v193 = v193_t?[v180] ?? 2147483648  // [table_computed_key_read]
  let v194_fn = function(...) {
    return vargv.len() > 0 ? vargv[0] : -2147483648
  }
  let v194 = v194_fn(v121)  // [vargv_first]
  local v195 = v75
  v195 += v148  // [modify_add_float]
  local v196 = -65535
  if (let v196_v = v96; v196_v > 257) {
    v196 = v196_v * 2
  }  // [if_let_positive]
  let v197_fn = function(p0) {
    let u0 = v27 >= 0 ? v27 : -v27  // [int_abs_ternary]
    local u1 = 0
    local u1_i = 0
    do {
      u1 += v193
      u1_i++
    } while (u1_i < 3)  // [do_while_count]
    let u2 = v152.len() > 0
      ? v152.reduce(@(acc, v) (typeof v == "integer" ? acc + v : acc), 0)
      : -130  // [array_reduce_sum]
    print($"int={v136}\n")  // [print_int]
    return u2
  }
  let v197 = v197_fn(v52)  // [nested_func depth=1]
  const v198_BASE = 0x100
  const v198_SCALE = 2
  let v198 = v21 * v198_SCALE + v198_BASE  // [const_in_expr]
  let v199 = typeof v89  // [typeof_expr]
  let v200_fn = function(p0, p1) {
    local u0 = 0
    foreach (c in v163) {
      if (c >= '0' && c <= '9') u0++
    }  // [string_scan_digits]
    v46.x <- v171
    let u1 = v46.x  // [table_set_field]
    let u2_fn = function(q0, q1) {
      let w0 = v169.hasvalue(v82)  // [array_hasvalue]
      let w1_fn = function(a, b = ";") { return a + b }
      let w1 = w1_fn(v23)  // [default_param_string]
      global enum S126_E7 { LO = -4294967298, HI = 3 }
      let w2 = v89 + S126_E7.LO + S126_E7.HI  // [enum_in_expr]
      local w3 = v96
      w3 /= v7  // [modify_div_int_unsafe]
      local w4 = 0
      local w4_i = 0
      while (w4_i < 1) {
        w4 = w4 + v38
        w4_i++
      }  // [while_sum]
      return w4
    }
    let u2 = u2_fn(v7, v122)  // [nested_func depth=2]
    let u3_fn = @(a, b = -2147483648, c = -65535) a * b + c
    let u3 = u3_fn(u1)  // [default_param_lambda]
    return u3
  }
  let v200 = v200_fn(v190, v8)  // [nested_func depth=1]
  local v201 = v24
  --v201  // [prefix_dec]
  let v202_C = class {
    _n = 0
    constructor(n) { this._n = n & 7 }
    function _nexti(idx) {
      local next = idx == null ? 0 : idx + 1
      return next < this._n ? next : null
    }
    function _get(idx) { return idx * -65536 }
  }
  let v202_obj = v202_C(v147)
  local v202 = 0
  foreach (_i, v in v202_obj) v202 += v  // [meta_nexti]
  let v203_Base = class { xv = 0; constructor(x) { this.xv = x } }
  let v203_Der = class(v203_Base) { constructor(x) { base.constructor(x) } }
  let v203_obj = v203_Der(v182)
  let v203 = v203_obj instanceof v203_Base  // [class_instanceof_base]

  // --- dump pool ---
  function _prn_t(_t) { let _a = []; _t.each(function(_k, _v) { _a.append(_sv(_k)); _a.append(_sv(_v)) }); _a.sort(); foreach (i, _v in _a) { if (i >= 10) { print("..."); break }; if (i > 0) print(", "); print(_v) }; }
  function _prn_a(_a) { foreach (i, _v in _a) { if (i >= 10) { print("..."); break }; if (i > 0) print(", "); print(_v) }; }
  print($"v0={v0}\n")
  print($"v1={v1}\n")
  print($"v2={v2}\n")
  print($"v3={v3}\n")
  print($"v4={v4}\n")
  print($"v5={v5}\n")
  print($"v6={v6}\n")
  print($"v7={v7}\n")
  print($"v8={v8}\n")
  print($"v9={v9}\n")
  print($"v10={v10}\n")
  print($"v11={v11}\n")
  print($"v12={v12}\n")
  print($"v13={v13}\n")
  print($"v14={v14}\n")
  print($"v15={v15}\n")
  print($"v16={v16}\n")
  print($"v17={v17}\n")
  print($"v18={v18}\n")
  print($"v19={v19}\n")
  print($"v20={v20}\n")
  print($"v21={v21}\n")
  print($"v22={v22}\n")
  print($"v23={v23}\n")
  print($"v24={v24}\n")
  print($"v25={v25}\n")
  print($"v26={v26}\n")
  print($"v27={v27}\n")
  print($"v28={v28}\n")
  print($"v29={v29}\n")
  print($"v30={v30}\n")
  print($"v31={v31}\n")
  print($"v32={v32}\n")
  print($"v33={v33}\n")
  print($"v34={v34}\n")
  print($"v35={v35}\n")
  print($"v36={v36}\n")
  print($"v37={v37}\n")
  print($"v38={v38}\n")
  print($"v39={v39}\n")
  print($"v40={v40}\n")
  print($"v41={v41}\n")
  print($"v42={v42}\n")
  print($"v43={v43}\n")
  print($"v44={v44}\n")
  print($"v45={v45}\n")
  print("v46={"); _prn_t(v46); print("}\n")
  print($"v47={v47}\n")
  print($"v48={v48}\n")
  print($"v49={v49}\n")
  print($"v50={v50}\n")
  print($"v51={v51}\n")
  print($"v52={v52}\n")
  print($"v53={v53}\n")
  print($"v54={v54}\n")
  print($"v55={v55}\n")
  print($"v56={v56}\n")
  print($"v57={v57}\n")
  print($"v58={v58}\n")
  print($"v59={v59}\n")
  print("v60=["); _prn_a(v60); print("]\n")
  print($"v61={v61}\n")
  print($"v62={v62}\n")
  print($"v63={v63}\n")
  print($"v64={v64}\n")
  print($"v65={v65}\n")
  print($"v66={v66}\n")
  print($"v67={v67}\n")
  print($"v68={v68}\n")
  print($"v69={v69}\n")
  print($"v70={v70}\n")
  print($"v71={v71}\n")
  print("v72=["); _prn_a(v72); print("]\n")
  print($"v73={v73}\n")
  print($"v74={v74}\n")
  print($"v75={v75}\n")
  print($"v76={v76}\n")
  print($"v77={v77}\n")
  print($"v78={v78}\n")
  print($"v79={v79}\n")
  print("v80=["); _prn_a(v80); print("]\n")
  print($"v81={v81}\n")
  print($"v82={v82}\n")
  print($"v83={v83}\n")
  print($"v84={v84}\n")
  print($"v85={v85}\n")
  print($"v86={v86}\n")
  print($"v87={v87}\n")
  print($"v88={v88}\n")
  print($"v89={v89}\n")
  print("v90=["); _prn_a(v90); print("]\n")
  print($"v91={v91}\n")
  print($"v92={v92}\n")
  print($"v93={v93}\n")
  print($"v94={v94}\n")
  print($"v95={v95}\n")
  print($"v96={v96}\n")
  print("v97=["); _prn_a(v97); print("]\n")
  print($"v98={v98}\n")
  print($"v99={v99}\n")
  print($"v100={v100}\n")
  print($"v101={v101}\n")
  print($"v102={v102}\n")
  print($"v103={v103}\n")
  print($"v104={v104}\n")
  print($"v105={v105}\n")
  print($"v106={v106}\n")
  print($"v107={v107}\n")
  print($"v108={v108}\n")
  print($"v109={v109}\n")
  print($"v110={v110}\n")
  print($"v111={v111}\n")
  print("v112=["); _prn_a(v112); print("]\n")
  print($"v113={v113}\n")
  print($"v114={v114}\n")
  print($"v115={v115}\n")
  print($"v116={v116}\n")
  print($"v117={v117}\n")
  print($"v118={v118}\n")
  print("v119={"); _prn_t(v119); print("}\n")
  print("v120={"); _prn_t(v120); print("}\n")
  print($"v121={v121}\n")
  print($"v122={v122}\n")
  print($"v123={v123}\n")
  print($"v124={v124}\n")
  print($"v125={v125}\n")
  print($"v126={v126}\n")
  print($"v127={v127}\n")
  print($"v128={v128}\n")
  print($"v129={v129}\n")
  print($"v130={v130}\n")
  print($"v131={v131}\n")
  print("v132=["); _prn_a(v132); print("]\n")
  print($"v133={v133}\n")
  print($"v134={v134}\n")
  print($"v135={v135}\n")
  print($"v136={v136}\n")
  print($"v137={v137}\n")
  print($"v138={v138}\n")
  print("v139={"); _prn_t(v139); print("}\n")
  print($"v140={v140}\n")
  print($"v141={v141}\n")
  print("v142={"); _prn_t(v142); print("}\n")
  print($"v143={v143}\n")
  print($"v144={v144}\n")
  print($"v145={v145}\n")
  print("v146=["); _prn_a(v146); print("]\n")
  print($"v147={v147}\n")
  print($"v148={v148}\n")
  print($"v149={v149}\n")
  print($"v150={v150}\n")
  print($"v151={v151}\n")
  print("v152=["); _prn_a(v152); print("]\n")
  print($"v153={v153}\n")
  print($"v154={v154}\n")
  print("v155={"); _prn_t(v155); print("}\n")
  print($"v156={v156}\n")
  print($"v157={v157}\n")
  print($"v158={v158}\n")
  print("v159={"); _prn_t(v159); print("}\n")
  print("v160=["); _prn_a(v160); print("]\n")
  print("v161={"); _prn_t(v161); print("}\n")
  print($"v162={v162}\n")
  print($"v163={v163}\n")
  print($"v164={v164}\n")
  print($"v165={v165}\n")
  print($"v166={v166}\n")
  print($"v167={v167}\n")
  print($"v168={v168}\n")
  print("v169=["); _prn_a(v169); print("]\n")
  print("v170={"); _prn_t(v170); print("}\n")
  print($"v171={v171}\n")
  print($"v172={v172}\n")
  print($"v173={v173}\n")
  print("v174=["); _prn_a(v174); print("]\n")
  print($"v175={v175}\n")
  print("v176=["); _prn_a(v176); print("]\n")
  print("v177=["); _prn_a(v177); print("]\n")
  print($"v178={v178}\n")
  print($"v179={v179}\n")
  print($"v180={v180}\n")
  print($"v181={v181}\n")
  print($"v182={v182}\n")
  print($"v183={v183}\n")
  print($"v184={v184}\n")
  print($"v185={v185}\n")
  print($"v186={v186}\n")
  print($"v187={v187}\n")
  print("v188=["); _prn_a(v188); print("]\n")
  print($"v189={v189}\n")
  print($"v190={v190}\n")
  print($"v191={v191}\n")
  print($"v192={v192}\n")
  print($"v193={v193}\n")
  print($"v194={v194}\n")
  print($"v195={v195}\n")
  print($"v196={v196}\n")
  print($"v197={v197}\n")
  print($"v198={v198}\n")
  print($"v199={v199}\n")
  print($"v200={v200}\n")
  print($"v201={v201}\n")
  print($"v202={v202}\n")
  print($"v203={v203}\n")
} catch(e) { print(e) }
print("===\n")
