#disable-optimizer
#allow-switch-statement
let _sv = @(_e) (type(_e) == "integer" || type(_e) == "float" || type(_e) == "null" || type(_e) == "bool" || type(_e) == "string") ? "" + _e : type(_e)
try {
// Code Bricks assembly - seed 1680
  global enum S1680_E1 { A, B, C }
  let v0 = S1680_E1.A + S1680_E1.B + S1680_E1.C  // [enum_basic]
  let v1_fn = function(x: int|float): int { return x.tointeger() }
  let v1 = v1_fn(v0)  // [typed_union_param]
  let v2_fn = function(x: int|null): int { return x != null ? x : -255 }
  let v2 = v2_fn(v0)  // [typed_nullable_param]
  let v3_fn = function(p0, p1) {
    local u0 = 3
    switch (v2 & 3) {
      case 0: u0 = -65538; break
      case 1: u0 = '9'; break
      case 2: u0 = -4; break
      default: u0 = 128
    }  // [switch_int]
    local u1_t = {arr = [2, null, {key = p1}]}
    local u1_inner = u1_t?.arr?[2]
    if (u1_inner != null) u1_inner.key += 9223372036854775807
    let u1 = u1_t?.arr?[2]?.key ?? 129  // [nested_table_array_modify]
    let u2 = [v1, p1]  // [array_of_ints]
    let u3 = [p1, v1]  // [array_of_ints]
    let {da = 0xFFFFFFFF, db = 127} = {da = u0}
    let u4 = da + db  // [destruct_defaults]
    let u5_fn = function([da, db]) { return da - db }
    let u5 = u5_fn([v1, u4])  // [destruct_param_array]
    let u6_Base = class {
      function compute(x) { return x + 0xFF }
    }
    let u6_Der = class(u6_Base) {
      function compute(x) { return base.compute(x) * -32770 }
    }
    let u6 = u6_Der().compute(u0)  // [class_method_override]
    let u7 = $"val={u1}"  // [string_interp]
    return u7
  }
  let v3 = v3_fn(v1, v0)  // [nested_func depth=1]
  let v4_fn = function() {
    local u0 = v1
    u0++  // [postfix_inc]
    let u1 = static({x = -2147483649, y = -32768})  // [static_table]
    let u2 = v3.len() > 2 ? v3.slice(-2) : v3  // [string_slice_neg]
    const u3 = -1  // [const_int]
    local u4_val = 0
    let u4_fn = function(x) { u4_val = u4_val + x }
    local u4_i = 0
    while (u4_i < 2) {
      u4_fn(v0)
      u4_i++
    }
    let u4 = u4_val  // [closure_in_loop]
    let u5 = (function() {
      let g = (function() {
        for (local i = 0; i < 0; i++) yield i
      })()
      local s = 0
      while (g.getstatus() == "suspended") {
        s += resume g
      }
      return s
    })()  // [coroutine_sum]
    return u5
  }
  let v4 = v4_fn()  // [nested_func depth=1]
  let v5_fn = function(...) {
    local m = 257
    foreach (v in vargv)
      if (typeof v == "integer" && v > m) m = v
    return m
  }
  let v5 = v5_fn(v2, v0)  // [vargv_max]
  local v6 = v1
  v6--  // [postfix_dec]
  let v7_fn = function(p0, p1, p2) {
    let {da = -65536, db = 0x8000} = {da = p2}
    let u0 = da + db  // [destruct_defaults]
    let u1 = array(p1 & 7, -2147483647)  // [array_function_init]
    enum S1680_E2 { LOW = 0, MID = 50, HIGH = 100 }
    local u2 = ""
    if (v4 < S1680_E2.MID) u2 = "low"
    else if (v4 < S1680_E2.HIGH) u2 = "mid"
    else u2 = "high"  // [enum_compare_chain]
    local u3 = p1
    u3--  // [postfix_dec]
    let u4_fn = function(q0, q1, q2) {
      let w0 = u1.filter(@(v) v != null).map(@(v) v)  // [array_filter_map]
      let w1_fn = function(x, ops = [@(v) v + -129, @(v) v * 2147483648, @(v) v - 0x7FFFFFFF]) {
        local r = x
        foreach (op in ops) r = op(r)
        return r
      }
      let w1_a = w1_fn(v1)
      let w1_b = w1_fn(v1, [@(v) v, @(v) v + -32770])
      let w1 = w1_a + w1_b  // [default_param_array_of_fns]
      let w2 = w0.indexof(p2) ?? 2147483648  // [array_indexof]
      let w3_fn = function(x, opts = {f = @(v) v + -65538, scale = -256}) {
        return opts.f(x) * opts.scale
      }
      let w3_a = w3_fn(w1)
      let w3_b = w3_fn(w1, {f = @(v) v * -256, scale = -32769})
      let w3 = w3_a + w3_b  // [default_param_fn_in_table]
      let w4_fn = function(r0, r1) {
        let z0 = v3 != r1  // [string_cmp_ne]
        let z1 = u3 ^ v5  // [int_xor]
        let z2 = q1 > v4  // [int_cmp_gt]
        local z3 = 0
        for (local z3_i = 8; z3_i > 0; z3_i--) {
          z3 += p0
        }  // [for_countdown]
        return z3
      }
      let w4 = w4_fn(v0, u2)  // [nested_func depth=3]
      return w4
    }
    let u4 = u4_fn(v6, p1, u2)  // [nested_func depth=2]
    return u4
  }
  let v7 = v7_fn(v4, v2, v0)  // [nested_func depth=1]
  let v8_fn = function(p0, p1, p2) {
    let u0 = p1
    assert(u0 >= 0 || u0 <= 0, "value check")  // [assert_msg]
    let u1_t = {x = v7}
    u1_t.x -= 65537
    let u1 = u1_t.x  // [table_field_minuseq]
    global enum S1680_E3 { A, B, C }
    let u2 = S1680_E3.A + S1680_E3.B + S1680_E3.C  // [enum_basic]
    let u3 = (((false ? ' ' : v7)).tofloat() != (((-129) >>> 1)).tofloat())  // [bool_literal]
    let u4_outer = function(x) {
      let inner = @(n) n * -256
      return inner(x) + 0x8000
    }
    let u4 = u4_outer(u1)  // [nested_function]
    local u5: int = p2  // [typed_int_var]
    return u5
  }
  let v8 = v8_fn(v6, v2, v7)  // [nested_func depth=1]
  let v9_C = class {
    xv = 0
    constructor(x) { this.xv = x }
  }
  let v9_arr = [v9_C(v5), v9_C(v5 + 1), v9_C(v5 + 2)]
  let v9 = v9_arr[0].xv + v9_arr[1].xv + v9_arr[2].xv  // [class_array_of]
  global enum S1680_E4 { A = "\u0100", B = "-1" }
  let v10 = $"{S1680_E4.A}{S1680_E4.B}"  // [enum_string]
  let v11_fn = function(p0, p1, p2) {
    let u0_a = [v9, v9 + 1]
    u0_a.append(v9 * 2)
    let u0 = u0_a.pop()  // [array_push_pop]
    local u1 = v5
    u1++  // [postfix_inc]
    let u2 = array(v6 & 7, 0x7FFFFFFF)  // [array_function_init]
    let u3 = v6 + p1  // [int_add]
    let u4 = u2.len() > 0 ? u2.reduce(@(a, _b) a) : 0x1  // [array_reduce_lambda]
    local u5_rows = [{v = v9}, null, {v = v9 + 1}, null, {v = '\\'}]
    local u5 = 0
    foreach (row in u5_rows) u5 += row?.v ?? 0  // [array_of_tables_nullable_sum]
    let [u6_a, u6_b] = [u4, u4 + 1]
    let [u6_c] = [u6_a + u6_b]
    let u6 = u6_c  // [destruct_nested]
    return u6
  }
  let v11 = v11_fn(v0, v2, v7)  // [nested_func depth=1]
  let v12_fn = function(p0, p1, p2) {
    let u0 = v9 + v7  // [int_add]
    local u1 = p2
    u1 /= p0  // [modify_div_int_unsafe]
    let u2_fn = function(q0, q1) {
      let w0 = v10.split("\u0001")  // [string_split]
      let w1_base = {extra = 256, x = -256}
      let w1_t = {x = v4}
      let w1 = w1_t.rawget("x") ?? w1_base.extra  // [table_setdelegate]
      let w2 = u1 != 0 ? w1 % u1 : 65537  // [int_mod_safe]
      local w3_ops = {
        inc  = @(x) x + -130,
        dec  = @(x) x - 257,
        dbl  = @(x) x * 2
      }
      let w3_op = w3_ops?[v10]
      let w3 = w3_op != null ? w3_op(v9) : v9  // [table_of_closures_dispatch]
      local w4 = v9
      w4 /= w2  // [modify_div_int_unsafe]
      return w4
    }
    let u2 = u2_fn(v2, v3)  // [nested_func depth=2]
    let u3_fn = function(q0, q1, q2) {
      let w0 = u0 | v7  // [int_bitor]
      let w1_Base = class { xv = 0; constructor(x) { this.xv = x } }
      let w1_Der = class(w1_Base) { constructor(x) { base.constructor(x) } }
      let w1_obj = w1_Der(q0)
      let w1 = w1_obj instanceof w1_Base  // [class_instanceof_base]
      let w2_t = {x = w0}
      w2_t.x -= 9223372036854775807
      let w2 = w2_t.x  // [table_field_minuseq]
      let w3_C = class {
        xv = 0
        constructor(x) { this.xv = x }
        function _unm() { return this.getclass()(-this.xv) }
      }
      let w3_r = -w3_C(v4)
      let w3 = w3_r.xv  // [meta_unm]
      let w4 = v0 < p0 ? v0 : p0  // [int_min_two]
      return w4
    }
    let u3 = u3_fn(v6, v3, u0)  // [nested_func depth=2]
    let u4_a = [u0]
    u4_a[0] *= 2147483648
    let u4 = u4_a[0]  // [array_elem_muleq]
    let u5_fn = function(q0, q1) {
      local w0_val = v2
      let w0_set = function(x) { w0_val = x }
      let w0_get = function() { return w0_val }
      w0_set(v2 + 1)
      let w0 = w0_get()  // [closure_getter_setter]
      local w1 = 128
      switch (v1 % 3) {
        case 0: w1 = -2147483649; break
        case 1: w1 = 65536; break
        default: w1 = 2147483647; break
      }  // [switch_default]
      let w2_fn = function(r0, r1, r2) {
        let z0_Base = class { xv = 0; constructor(x) { this.xv = x } }
        let z0_Der = class(z0_Base) { constructor(x) { base.constructor(x) } }
        let z0_obj = z0_Der(w1)
        let z0 = z0_obj instanceof z0_Base  // [class_instanceof_base]
        let z1_make = @(bv) @(x) x + bv
        let z1_add = z1_make(u2)
        let z1 = z1_add(-129)  // [func_returns_func]
        let z2_fn = function(x, ops = [@(v) v + 4294967297, @(v) v * -4294967296, @(v) v - -3]) {
          local r = x
          foreach (op in ops) r = op(r)
          return r
        }
        let z2_a = z2_fn(z1)
        let z2_b = z2_fn(z1, [@(v) v, @(v) v + 2])
        let z2 = z2_a + z2_b  // [default_param_array_of_fns]
        return z2
      }
      let w2 = w2_fn(v2, w1, v1)  // [nested_func depth=3]
      return w2
    }
    let u5 = u5_fn(v10, v4)  // [nested_func depth=2]
    let u6_C = class {
      xv = 0
      constructor(x) { this.xv = x }
      function _add(o) { return this.getclass()(this.xv + o.xv) }
    }
    let u6_r = u6_C(v5) + u6_C(u4)
    let u6 = u6_r.xv  // [meta_add]
    local u7 = v7
    ++u7  // [prefix_inc]
    return u7
  }
  let v12 = v12_fn(v6, v3, v2)  // [nested_func depth=1]
  local v13 = v2
  ++v13  // [prefix_inc]
  let v14 = v3.startswith(v10)  // [string_startswith]
  local v15_state = {scores = [v8, -2147483648, 4294967296], tag = "\u0001"}
  v15_state.scores[0] += 4294967295
  v15_state.scores[2] = v15_state.scores[0] + v15_state.scores[1]
  let v15 = v15_state.scores.reduce(@(a, v) a + v, 0)  // [table_slot_array_modify]
  let v16 = (function() {
    let g = (function() {
      for (local i = 0; i < 2; i++) yield i
    })()
    local s = 0
    while (g.getstatus() == "suspended") {
      s += resume g
    }
    return s
  })()  // [coroutine_sum]
  let v17 = v7 * v5  // [int_mul]
  let v18_fn = function() {
    const u0 = true  // [const_bool]
    let u1 = v10.len() > 2 ? v10.slice(-2) : v10  // [string_slice_neg]
    local u2_sum = 0
    let u2_add = function(x) { u2_sum = u2_sum + x }
    u2_add(v7)
    u2_add(v16)
    u2_add(v7 + v16)
    let u2 = u2_sum  // [closure_accumulator]
    let u3_fns = [@(x) x + 32769, @(x) x * 0, @(x) x - -65536]
    local u3 = v15
    foreach (f in u3_fns) u3 = f(u3)  // [array_of_closures_accumulate]
    let u4 = -v9  // [int_neg]
    return u4
  }
  let v18 = v18_fn()  // [nested_func depth=1]
  let v19_fn = function(p0, p1) {
    local u0 = -129
    try { u0 = v3.tointeger() } catch (_e) {}  // [string_tointeger]
    let u1_fn = @[pure, nodiscard] (a, b) a + b
    let u1 = u1_fn(p1, v1)  // [func_pure_nodiscard]
    let u2_fn = function(q0, q1, q2) {
      local w0_ops = {
        inc  = @(x) x + -65538,
        dec  = @(x) x - 2147483649,
        dbl  = @(x) x * 2
      }
      let w0_op = w0_ops?[q2]
      let w0 = w0_op != null ? w0_op(v11) : v11  // [table_of_closures_dispatch]
      let w1 = p0.replace(";", "\\")  // [string_replace]
      let w2_make = @(bv) @(x) x + bv
      let w2_add = w2_make(u0)
      let w2 = w2_add(-65537)  // [func_returns_func]
      local w3_store = {pages = [{lines = [0x8000, -32767]}, {lines = [-1, -9223372036854775807]}]}
      w3_store.pages[0].lines[1] += w0
      let w3 = w3_store?.pages?[0]?.lines?[1] ?? -129  // [nested_db_array_field]
      let w4_t = freeze({x = v1, y = -32769})
      let w4 = w4_t.x + w4_t.y  // [freeze_nested]
      return w4
    }
    let u2 = u2_fn(p1, v4, p0)  // [nested_func depth=2]
    let u3 = v18 / v17  // [int_div_unsafe]
    let u4 = [p0, v3, " "]  // [array_of_strings]
    let u5 = v3.tofloat()  // [string_tofloat]
    local u6 = v6
    u6 += u1
    u6 -= u0
    u6 *= -4  // [modify_chain]
    return u6
  }
  let v19 = v19_fn(v3, v15)  // [nested_func depth=1]
  let v20 = v15 > 0 ? 1 : (v15 < 0 ? -1 : 0)  // [int_sign]
  let v21 = v10.len() > 4 ? v10.slice(1, 3) : v10  // [string_slice_both]
  let v22_fn = function(x, opts = {f = @(v) v + 65537, scale = 257}) {
    return opts.f(x) * opts.scale
  }
  let v22_a = v22_fn(v2)
  let v22_b = v22_fn(v2, {f = @(v) v * 4294967297, scale = -65535})
  let v22 = v22_a + v22_b  // [default_param_fn_in_table]
  let v23_fn = @(a: int, b: int) a * b
  let v23 = v23_fn(v16, v5)  // [typed_lambda]
  let v24_fn = function() {
    local u0 = 0
    for (local u0_i = 0; u0_i < -1; u0_i++) {
      u0 = u0 + v12
    }  // [for_sum]
    let u1 = typeof v3 == "string"  // [typeof_string_check]
    let u2 = [v1, u0]  // [array_of_ints]
    local u3 = u0
    u3++  // [postfix_inc]
    let u4_fn = function(...) {
      local s = 0
      foreach (v in vargv) s += v
      return s
    }
    let u4 = u4_fn(v16, v7, 2)  // [vargv_sum]
    return u4
  }
  let v24 = v24_fn()  // [nested_func depth=1]
  let v25_fn = function(x, s, cfg = {
      fmt = @(n, t) $"{n}:{t}",
      limits = {lo = 0xFFFF, hi = -256}
    }) {
    local clamped = x < cfg.limits.lo ? cfg.limits.lo
                  : x > cfg.limits.hi ? cfg.limits.hi : x
    return cfg.fmt(clamped, s)
  }
  let v25 = v25_fn(v16, v3)  // [default_param_config_nested]
  let v26_fn = function(x: int|float): int { return x.tointeger() }
  let v26 = v26_fn(v12)  // [typed_union_param]
  let v27_fn = function(p0, p1, p2) {
    local u0_ops = {
      inc  = @(x) x + -65536,
      dec  = @(x) x - 257,
      dbl  = @(x) x * 2
    }
    let u0_op = u0_ops?[v25]
    let u0 = u0_op != null ? u0_op(v11) : v11  // [table_of_closures_dispatch]
    let u1 = ((0xFF).tofloat()).tointeger()  // [int_literal]
    let u2_C = class {
      xv = 0
      constructor(x) { this.xv = x }
      function _cmp(o) { return this.xv <=> o.xv }
    }
    let u2 = u2_C(v13) < u2_C(p1)  // [meta_cmp]
    let u3 = [-4294967295, -256, -2147483648]
    u3.insert(1, v6)
    u3.remove(0)  // [array_insert_remove]
    let u4_fn = function({da, db = "\\\\"}) {
      return $"{_sv(da)}{_sv(db)}"
    }
    let u4 = u4_fn({da = v19, db = v21})  // [destruct_param_nested_table]
    return u4
  }
  let v27 = v27_fn(v16, v20, v15)  // [nested_func depth=1]
  let v28 = v19 >= 0 ? v19 : -v19  // [int_abs_ternary]
  let v29 = $"flag={v14}"  // [string_interp_bool]
  let v30_C = class {
    _v = 0
    constructor(xv) { this._v = xv }
  }
  let v30_obj = v30_C(v9)
  let v30 = v30_obj instanceof v30_C  // [instanceof_check]
  enum S1680_E5 { A, B, C }
  let v31 = S1680_E5.A + S1680_E5.B + S1680_E5.C  // [enum_basic]
  let v32_a = [v6, v22, -4294967297]
  let v32 = v32_a.top()  // [array_top]
  local v33_val = v7
  let v33_set = function(x) { v33_val = x }
  let v33_get = function() { return v33_val }
  v33_set(v7 + 0xCAFE)
  let v33 = v33_get()  // [closure_getter_setter]
  let v34_fn = function([da, db = 0xFFFF]) { return da * db }
  let v34 = v34_fn([v19])  // [destruct_param_array_default]
  let v35 = v3.len()  // [string_len]
  let v36_C = class {
    xv = 0
    constructor(x) { this.xv = x }
    function _mul(o) { return this.getclass()(this.xv * o.xv) }
  }
  let v36_r = v36_C(v20) * v36_C(v6)
  let v36 = v36_r.xv  // [meta_mul]
  let v37 = [v25, v29, ""]  // [array_of_strings]
  let v38 = "{0}+{1}".subst(v29, "hello")  // [string_subst]
  let v39_fn = function(p0, p1, p2) {
    let u0 = v25.startswith(v3)  // [string_startswith]
    local u1 = v2
    if (v20 != 0) u1 %= v20  // [modify_mod_int_safe]
    let u2_C = class {
      _n = 0
      constructor(n) { this._n = n & 7 }
      function _nexti(idx) {
        local next = idx == null ? 0 : idx + 1
        return next < this._n ? next : null
      }
      function _get(idx) { return idx * 255 }
    }
    let u2_obj = u2_C(v1)
    local u2 = 0
    foreach (_i, v in u2_obj) u2 += v  // [meta_nexti]
    let u3 = v36 << 3  // [int_shl]
    local u4 = 4294967297
    foreach (v in v37) {
      if (typeof v == "integer") { u4 = v; break }
    }  // [foreach_array_break]
    return u4
  }
  let v39 = v39_fn(v18, v15, v25)  // [nested_func depth=1]
  let v40_fn = function(x,
      cls = class { function calc(v) { return v + 128 } }) {
    return cls().calc(x)
  }
  let v40_a = v40_fn(v32)
  let v40_other = class { function calc(v) { return v * 9223372036854775807 } }
  let v40_b = v40_fn(v32, v40_other)
  let v40 = v40_a + v40_b  // [default_param_class]
  let v41_Base = class {
    function compute(x) { return x + -127 }
  }
  let v41_Der = class(v41_Base) {
    function compute(x) { return base.compute(x) * 2 }
  }
  let v41 = v41_Der().compute(v23)  // [class_method_override]
  local v42_ops = {
    inc  = @(x) x + ' ',
    dec  = @(x) x - 42,
    dbl  = @(x) x * 2
  }
  let v42_op = v42_ops?[v29]
  let v42 = v42_op != null ? v42_op(v1) : v1  // [table_of_closures_dispatch]
  let v43 = v34 + v5  // [int_add]
  let v44 = v14 && v30  // [bool_and]
  let v45 = null  // [null_var]
  let v46_fn = function(p0, p1) {
    let u0 = v3.tolower()  // [string_tolower]
    local u1 = ""
    switch (v31 <=> -128) {
      case -1: u1 = "less"; break
      case 0: u1 = "equal"; break
      case 1: u1 = "greater"; break
      default: u1 = "other"
    }  // [switch_three_way]
    let u2_fn = function() {
      let w0_fn = function(x, pipe = {pre = @(v) v & 0xFF,
                                       run = @(v) v + 32767,
                                       post = @(v) v * -257}) {
        return pipe.post(pipe.run(pipe.pre(x)))
      }
      let w0 = w0_fn(p0)  // [default_param_pipeline]
      let w1 = [v3, v21, "\t\n\r"]  // [array_of_strings]
      let w2_fn = function(x, pipe = {pre = @(v) v & 0xFF,
                                       run = @(v) v + -257,
                                       post = @(v) v * 2}) {
        return pipe.post(pipe.run(pipe.pre(x)))
      }
      let w2 = w2_fn(v13)  // [default_param_pipeline]
      let w3 = v44 || v30  // [bool_or]
      local w4_items = [{k = ".", v = -65536},
                         {k = "\t\n\r", v = 0x8000},
                         {k = "\r\n", v = v41}]
      foreach (item in w4_items)
        if (item.v > -32770) item.v *= 2147483648
      local w4 = 0
      foreach (item in w4_items) w4 += item.v  // [foreach_array_tables_modify]
      return w4
    }
    let u2 = u2_fn()  // [nested_func depth=2]
    let u3_fn = function() {
      const w0 = false  // [const_bool]
      const function [pure] w1_fn(x) { return x + 0xFF }
      const w1_a = -2147483649
      const w1 = w1_fn(w1_a)  // [const_chain]
      let w2 = [-4, -4294967296, 128]
      w2.insert(1, v20)
      w2.remove(0)  // [array_insert_remove]
      let w3 = v39.tofloat()  // [int_to_float]
      let w4_inner = function(...) {
        local s = 0
        foreach (v in vargv) s += v
        return s
      }
      let w4_outer = function(a, ...) { return a + w4_inner(vargv.len(), a) }
      let w4 = w4_outer(p0, v2)  // [vargv_forward]
      let w5 = v45 && p0  // [logical_and_val]
      return w5
    }
    let u3 = u3_fn()  // [nested_func depth=2]
    return u3
  }
  let v46 = v46_fn(v35, v20)  // [nested_func depth=1]
  let v47_fn = function(a, b,
      combine = {op = @(x, y) x + y, identity = -65535}) {
    return combine.op(a, b) + combine.identity
  }
  let v47_add = v47_fn(v42, v20)
  let v47_mul = v47_fn(v42, v20,
    {op = @(x, y) x * y, identity = 1000})
  let v47 = v47_add + v47_mul  // [default_param_fn_field_call]
  local v48_cfg = {a = {val = v32 + -256}, b = {val = v32 - 257}}
  let v48 = v44 ? (v48_cfg?.a?.val ?? 2147483648) : (v48_cfg?.b?.val ?? 1000)  // [conditional_deep_path]
  let v49 = ",".join(v37.map(@(v) $"{_sv(v)}"))  // [string_join]
  let v50 = v3.toupper()  // [string_toupper]
  let v51_tbl = {a = v6, b = v26}
  let v51 = v51_tbl.a + v51_tbl.b  // [destruct_table]
  let v52_C = class {
    _v = 0
    constructor(xv) { this._v = xv }
    function _call(_env, arg) { return this._v + arg }
  }
  let v52_obj = v52_C(v43)
  let v52 = v52_obj(-65536)  // [meta_call]
  let v53 = v13 >= v23  // [int_cmp_ge]
  local v54 = 0
  local v54_i = 0
  while (v54_i < -1) {
    v54_i++
    if (v54_i % 2 == 0) continue
    v54 += v33
  }  // [while_continue_skip]
  local v55 = v4
  v55 /= v12  // [modify_div_int_unsafe]
  enum S1680_E6 { X = -32769, Y = 32767, Z = 2147483649 }
  let v56 = S1680_E6.X + S1680_E6.Y + S1680_E6.Z  // [enum_explicit]
  let v57 = v3.len() > 4 ? v3.slice(1, 3) : v3  // [string_slice_both]
  let v58_fn = function(p0, p1, p2) {
    local u0_a = (v56 & 0xFFFF) + 1
    local u0_b = (v31 & 0xFFFF) + 1
    while (u0_b != 0) {
      local u0_tmp = u0_b
      u0_b = u0_a % u0_b
      u0_a = u0_tmp
    }
    let u0 = u0_a  // [int_gcd]
    let u1 = v29.split_by_chars("\n")  // [string_split_by_chars]
    let u2 = clone v37
    u2.replace_with(v37.map(@(v) v))  // [array_replace_with]
    return u0
  }
  let v58 = v58_fn(v54, v47, v28)  // [nested_func depth=1]
  let v59 = v37?[-4294967298] ?? 0x7FFFFFFF  // [nullsafe_array_index]
  local v60 = v1
  ++v60  // [prefix_inc]
  local v61_sum = 0
  let v61_add = function(x) { v61_sum = v61_sum + x }
  v61_add(v28)
  v61_add(v12)
  v61_add(v28 + v12)
  let v61 = v61_sum  // [closure_accumulator]
  let v62_v = v33 & 0xFFFF
  let v62 = v62_v > 0 && (v62_v & (v62_v - 1)) == 0  // [int_pow2_check]
  let v63_fn = function(x, acc = []) {
    acc.append(x)
    return acc.len()
  }
  let v63_a = v63_fn(v22)
  let v63_b = v63_fn(v22 + 1)
  let v63 = v63_a + v63_b  // [default_param_mutable_shared]
  local v64 = -9223372036854775807
  for (local v64_i = 0; v64_i < 2; v64_i++) {
    v64 += v41
  }  // [modify_in_loop]
  let v65_fn = function(p0, p1, p2) {
    let u0_fn = function({da, db = "true"}) {
      return $"{_sv(da)}{_sv(db)}"
    }
    let u0 = u0_fn({da = v64, db = v49})  // [destruct_param_nested_table]
    let u1_f = @(x) x * -4
    let u1_g = @(x) x + 2147483648
    let u1 = u1_f(u1_g(v1))  // [func_compose]
    let u2_lo = -2
    let u2_hi = u2_lo + 256
    let u2 = v32 < u2_lo ? u2_lo : (v32 > u2_hi ? u2_hi : v32)  // [int_clamp]
    let u3_C = class {
      _v = 0
      constructor(xv) { this._v = xv }
      function _call(_env, arg) { return this._v + arg }
    }
    let u3_obj = u3_C(v26)
    let u3 = u3_obj(2147483647)  // [meta_call]
    local u4 = p2 & 7
    do {
      u4--
    } while (u4 > 0)  // [do_while_dec]
    let u5_fn = function(q0, q1) {
      let w0 = q0.len()  // [string_len]
      let w1_fn = function({da, db = " "}) {
        return $"{_sv(da)}{_sv(db)}"
      }
      let w1 = w1_fn({da = v55, db = v21})  // [destruct_param_nested_table]
      let w2_fn = function(r0, r1, r2) {
        let z0_fn = @[nodiscard] (x) x + -129
        let z0 = z0_fn(p2)  // [func_nodiscard]
        let z1_f = @(x) x * -258
        let z1_g = @(x) x + 0x80000000
        let z1 = z1_f(z1_g(v1))  // [func_compose]
        local z2 = 0
        foreach (idx, v in v37) {
          if (idx % 2 != 0) continue
          if (typeof v == "integer") z2 += v
        }  // [foreach_array_continue]
        return z2
      }
      let w2 = w2_fn(v40, v2, v33)  // [nested_func depth=3]
      let w3 = v64 <= v17  // [int_cmp_le]
      let w4_C = class {
        xv = 0
        constructor(x) { this.xv = x }
        function _typeof() { return "mytype" }
      }
      let w4 = typeof w4_C(v32)  // [meta_typeof]
      let w5 = q0.endswith(v3)  // [string_endswith]
      return w5
    }
    let u5 = u5_fn(v27, v26)  // [nested_func depth=2]
    let u6 = (1e-40 * (-2.5))  // [float_literal]
    let u7 = u6 + v1.tofloat()  // [float_add_int]
    return u7
  }
  let v65 = v65_fn(v24, v13, v9)  // [nested_func depth=1]
  let v66 = clone v37
  v66.replace_with(v37.map(@(v) v))  // [array_replace_with]
  let v67_C = class {
    _n = 0
    constructor(n) { this._n = n & 7 }
    function _nexti(idx) {
      local next = idx == null ? 0 : idx + 1
      return next < this._n ? next : null
    }
    function _get(idx) { return idx * 2147483647 }
  }
  let v67_obj = v67_C(v17)
  local v67 = 0
  foreach (_i, v in v67_obj) v67 += v  // [meta_nexti]
  let v68_fn = function(x, ops = [@(v) v + -65537, @(v) v * 2, @(v) v - -4294967296]) {
    local r = x
    foreach (op in ops) r = op(r)
    return r
  }
  let v68_a = v68_fn(v43)
  let v68_b = v68_fn(v43, [@(v) v, @(v) v + -257])
  let v68 = v68_a + v68_b  // [default_param_array_of_fns]
  let v69_fn = function({dx, dy = 0}, [da, db]) {
    return dx + dy + da + db
  }
  let v69 = v69_fn({dx = v11}, [v33, v55])  // [destruct_param_mixed]
  let v70_fn = function(x, ctx = {
      cls = class { v = 0; constructor(n) { this.v = n } },
      wrap = @(inst) inst.v + 0
    }) {
    return ctx.wrap(ctx.cls(x))
  }
  let v70 = v70_fn(v40)  // [default_param_table_class]
  let v71_inner = function(...) {
    local s = 0
    foreach (v in vargv) s += v
    return s
  }
  let v71_outer = function(a, ...) { return a + v71_inner(vargv.len(), a) }
  let v71 = v71_outer(v45, v58)  // [vargv_forward]
  let v72 = v2 != 0 ? v45 / v2 : -4294967298  // [int_div_safe]
  let v73 = null  // [null_var]
  let v74 = (@(x) -x)(v55)  // [lambda_negate]
  function [nodiscard] v75_fn(x) { return x + 32768 }
  let v75 = v75_fn(v7)  // [named_func_nodiscard]
  let v76_fn = function() {
    let u0_fn = function(x, ops = [@(v) v + 0, @(v) v * 32768, @(v) v - -1]) {
      local r = x
      foreach (op in ops) r = op(r)
      return r
    }
    let u0_a = u0_fn(v31)
    let u0_b = u0_fn(v31, [@(v) v, @(v) v + 1])
    let u0 = u0_a + u0_b  // [default_param_array_of_fns]
    let u1_fn = function(a: int, b: int): int { return a + b }
    let u1 = u1_fn(v17, v13)  // [typed_function_params]
    local u2 = -4294967295
    if (v62) {
      u2 = u2 + v51
      u2 = u2 + -2147483650
    }  // [if_accum]
    let u3_fn = function(x, opts = {f = @(v) v + 'Z', scale = 65536}) {
      return opts.f(x) * opts.scale
    }
    let u3_a = u3_fn(u2)
    let u3_b = u3_fn(u2, {f = @(v) v * -129, scale = 0})
    let u3 = u3_a + u3_b  // [default_param_fn_in_table]
    return u3
  }
  let v76 = v76_fn()  // [nested_func depth=1]
  local v77 = v29
  v77 += v50  // [modify_string_concat]
  let v78 = v30 ? v60 : v8  // [ternary_int]
  let v79_t = freeze({x = v42, y = 32768})
  let v79 = v79_t.x + v79_t.y  // [freeze_nested]
  let v80 = v10.lstrip()  // [string_lstrip]
  let v81_fn = function(p0) {
    local u0: float = v65  // [typed_float_var]
    local u1 = 0
    switch (v31 & 3) {
      case 0:
      case 1: u1 = -32770; break
      case 2: u1 = 255; break
      default: u1 = -255
    }  // [switch_fallthrough]
    let u2 = typeof v58 == "integer"  // [typeof_int_check]
    local u3 = 0
    foreach (_c in v21) u3++  // [foreach_string_chars]
    return u3
  }
  let v81 = v81_fn(v61)  // [nested_func depth=1]
  local v82 = v45
  v82 += v54
  v82 -= v78
  v82 *= -258  // [modify_chain]
  let v83 = v69 <= v54  // [int_cmp_le]
  let v84_C = class {
    _v = 0
    constructor(xv) { this._v = xv }
    function _call(_env, arg) { return this._v + arg }
  }
  let v84_obj = v84_C(v68)
  let v84 = v84_obj('9')  // [meta_call]
  local v85_items = [{k = "\r\n", v = 0xFF},
                     {k = @"raw_string", v = -128},
                     {k = "test", v = v73}]
  foreach (item in v85_items)
    if (item.v > 0x8000) item.v *= 2147483649
  local v85 = 0
  foreach (item in v85_items) v85 += item.v  // [foreach_array_tables_modify]
  let v86 = v82 | v39  // [int_bitor]
  local v87 = v0
  v87 += -2  // [modify_add_const]
  let v88_C = class {
    _bv = 0
    constructor(xb) { this._bv = xb }
    function _get(idx) { return this._bv + idx }
  }
  let v88_obj = v88_C(v64)
  let v88 = v88_obj[0xDEADBEEF]  // [meta_get]
  let v89_fn = function({da, db}) { return da + db }
  let v89 = v89_fn({da = v26, db = v28})  // [destruct_param_table]
  let v90 = v53 ? v50 : v77  // [ternary_str]
  let v91_fn = function([da, db = -1]) { return da * db }
  let v91 = v91_fn([v60])  // [destruct_param_array_default]
  let v92 = v37.findindex(@(val) val != null) ?? -2147483649  // [array_findindex]
  let v93_fn = function(a, b = -4294967297) { return a + b }
  let v93 = v93_fn(v84)  // [default_param_one]
  let v94_C = class {
    xv = 0
    constructor(x) { this.xv = x }
    function _cmp(o) { return this.xv <=> o.xv }
  }
  let v94 = v94_C(v32) < v94_C(v15)  // [meta_cmp]
  let v95 = v61 < v48  // [int_cmp_lt]
  local v96 = 0
  for (local v96_i = 0; v96_i < 0; v96_i++) {
    v96 = v96 + v47
  }  // [for_sum]
  local v97 = false
  try { v97 = v28 == v20 } catch (_e) {}  // [any_cmp_eq]
  local v98_items = [1000, null, {fn = @(x) x * -65535, tag = "\uffff"}, null]
  let v98_entry = v98_items?[2]
  let v98 = v98_entry != null ? v98_entry.fn(v26) : 0x100  // [mixed_array_fn_call]
  let v99_C = class {
    _items = []
    function add(x) { this._items.append(x) }
    function sum() {
      local s = 0
      foreach (v in this._items) s += v
      return s
    }
  }
  let v99_obj = v99_C()
  v99_obj.add(v6)
  v99_obj.add(v6 + 1)
  let v99 = v99_obj.sum()  // [class_with_array]
  let v100 = (function() {
    let g = (function() {
      for (local i = 0; i < 7; i++) yield i
    })()
    local s = 0
    while (g.getstatus() == "suspended") {
      s += resume g
    }
    return s
  })()  // [coroutine_sum]
  let v101 = v80.endswith(v10)  // [string_endswith]
  let v102 = v53.tostring()  // [bool_to_string]
  let v103_fn = function(x, s, cfg = {
      fmt = @(n, t) $"{n}:{t}",
      limits = {lo = 32769, hi = 32768}
    }) {
    local clamped = x < cfg.limits.lo ? cfg.limits.lo
                  : x > cfg.limits.hi ? cfg.limits.hi : x
    return cfg.fmt(clamped, s)
  }
  let v103 = v103_fn(v8, v50)  // [default_param_config_nested]
  let v104 = (((-129) + (-130))).tofloat()  // [float_literal]

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
  print("v37=["); _prn_a(v37); print("]\n")
  print($"v38={v38}\n")
  print($"v39={v39}\n")
  print($"v40={v40}\n")
  print($"v41={v41}\n")
  print($"v42={v42}\n")
  print($"v43={v43}\n")
  print($"v44={v44}\n")
  print($"v45={v45}\n")
  print($"v46={v46}\n")
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
  print($"v60={v60}\n")
  print($"v61={v61}\n")
  print($"v62={v62}\n")
  print($"v63={v63}\n")
  print($"v64={v64}\n")
  print($"v65={v65}\n")
  print("v66=["); _prn_a(v66); print("]\n")
  print($"v67={v67}\n")
  print($"v68={v68}\n")
  print($"v69={v69}\n")
  print($"v70={v70}\n")
  print($"v71={v71}\n")
  print($"v72={v72}\n")
  print($"v73={v73}\n")
  print($"v74={v74}\n")
  print($"v75={v75}\n")
  print($"v76={v76}\n")
  print($"v77={v77}\n")
  print($"v78={v78}\n")
  print($"v79={v79}\n")
  print($"v80={v80}\n")
  print($"v81={v81}\n")
  print($"v82={v82}\n")
  print($"v83={v83}\n")
  print($"v84={v84}\n")
  print($"v85={v85}\n")
  print($"v86={v86}\n")
  print($"v87={v87}\n")
  print($"v88={v88}\n")
  print($"v89={v89}\n")
  print($"v90={v90}\n")
  print($"v91={v91}\n")
  print($"v92={v92}\n")
  print($"v93={v93}\n")
  print($"v94={v94}\n")
  print($"v95={v95}\n")
  print($"v96={v96}\n")
  print($"v97={v97}\n")
  print($"v98={v98}\n")
  print($"v99={v99}\n")
  print($"v100={v100}\n")
  print($"v101={v101}\n")
  print($"v102={v102}\n")
  print($"v103={v103}\n")
  print($"v104={v104}\n")
} catch(e) { print(e) }
print("===\n")
