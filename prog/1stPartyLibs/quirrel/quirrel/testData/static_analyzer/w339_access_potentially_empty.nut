//-file:declared-never-used
//-file:mutating-shared-default

// ---- expect warnings ----

function param_table_field(x = {}) {
  return x.y // warn
}

function param_array_index(a = []) {
  return a[0] // warn
}

function nullc_index(v) {
  return (v ?? [])[0] // warn
}

function nullc_field(v) {
  return (v ?? {}).name // warn
}

function ternary_index(b, t) {
  return (b ? [] : t)[1] // warn
}

function local_ternary(b, h, x) {
  let v = (b ? h : [])
  let unrelated = b + 1
  return v[x] + unrelated // warn
}

function local_nullc(t) {
  let v = t ?? {}
  return v.field // warn
}

// ---- expect no warnings ----

function param_filled_by_append(a = []) {
  a.append(1)
  return a[0]
}

function param_len_checked(x = {}) {
  if (x.len() > 0)
    return x.y
  return null
}

function param_in_checked(x = {}) {
  if ("y" in x)
    return x.y
  return null
}

function param_passed_to_filler(x = {}) {
  let fill = function(t) { t.y <- 1 }
  fill(x)
  return x.y
}

function param_newslot_fill(x = {}) {
  x.y <- 1
  return x.y
}

function param_safe_access(x = {}) {
  return x?.y
}

function param_nonempty_default(x = { y = 1 }) {
  return x.y
}

function nullc_nonempty(v) {
  return (v ?? [1])[0]
}

function method_on_temp(v) {
  return (v ?? []).len()
}

function foreach_over_temp(v) {
  local s = 0
  foreach (x in (v ?? []))
    s += x
  return s
}

function guarded_local(b, h) {
  let v = (b ? h : [])
  if (v.len() > 0)
    return v[0]
  return null
}

function param_reassigned(x = {}) {
  x = { y = 1 }
  return x.y
}

function safe_access_elsewhere(p = {}) {
  if (p?.itemId != null)
    return p.itemId
  return null
}

function safe_slot_elsewhere(mods, k) {
  let m = mods ?? {}
  if (m?[k] != null)
    return m[k]
  return null
}

function destructured_with_defaults(params = {}) {
  let { isEnabled = true } = params
  return [params.txtStyle, isEnabled]
}

let hotkeysActive = {}
let hotkeysInactive = {}
foreach (key in ["a", "b"])
  hotkeysActive[key] <- key

function named_stub_tables(active, key) {
  return (active ? hotkeysActive : hotkeysInactive)[key]
}

function parallel_arrays_foreach(comp) {
  let seatEids = comp?.seatEids ?? []
  let remap = comp?.remap ?? []
  let res = []
  foreach (_i, idx in remap)
    res.append(seatEids[idx])
  return res
}

function parallel_arrays_for(comp) {
  let names = comp?.names ?? []
  let icons = comp?.icons ?? []
  let res = []
  for (local idx = 0; idx < names.len(); idx++)
    res.append(icons[idx])
  return res
}
