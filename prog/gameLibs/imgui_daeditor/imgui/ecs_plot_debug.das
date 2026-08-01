options no_aot
options persistent_heap

require app
require ecs
require strings
require daslib/strings_boost
require DagorImgui
require DagorMath
require imgui/imgui_boost
require daslib/static_let
require DaeditorPlotDebug
require gameLibs.imgui_daeditor.imgui.ecs_struct_walker_common
require %gameLibs.imgui_daeditor.imgui.imgui_common
require %gameLibs.imgui_daeditor.imgui.ecs_plot_common
require gameLibs.imgui_daeditor.imgui.ecs_plot_registry_common
require gameLibs.imgui_daeditor.imgui.ecs_plot_macro


def private get_plot_color(idx : int) : uint
  static_let <|
    let colors = fixed_array(
      IM_COL32(60,  60,  220, 255), IM_COL32(60,  220, 60,  255),
      IM_COL32(220, 100, 60,  255), IM_COL32(60,  220, 220, 255),
      IM_COL32(220, 220, 60,  255), IM_COL32(220, 60,  220, 255),
      IM_COL32(30,  30,  110, 255), IM_COL32(30,  110, 30,  255),
      IM_COL32(110, 50,  30,  255), IM_COL32(30,  110, 110, 255),
      IM_COL32(110, 110, 30,  255), IM_COL32(110, 30,  110, 255))
  return colors[idx % length(colors)]


def plot_ensure(var g_plot_windows : array<PlotWindow>&; win_idx : int)
  let old_len = length(g_plot_windows)
  if old_len <= win_idx
    g_plot_windows |> resize(win_idx + 1)
    for i in range(old_len, win_idx + 1)
      var pw & = unsafe(g_plot_windows[i])
      pw.plot_duration      = 10.0f
      pw.edit_plot_duration = 10.0f
      pw.history_size   = history_size_for_duration(pw.plot_duration)
      pw.grid_x         = 10
      pw.grid_y         = 10
      pw.prev_n_entries = -1
      pw.shared_fit       = true
      pw.shared_scale_min = -1.0f
      pw.shared_scale_max = 1.0f
      pw.fit_widget_x     = 720.0f  // reasonable default until per-entry rows are drawn and we capture the real X
      // Load here so pending "Add to Plot" requests append to the loaded set instead of the load
      // running after them and reordering the entries.
      plot_load_entries(pw, i)


def plot_sample_entry(var e : PlotEntry&; eid : EntityId)
  if empty(e.name) || eid == INVALID_ENTITY_ID
    return
  var data : void?
  var resolved_type = ""
  if !resolve_component_path(eid, e.name, data, resolved_type)
    e.found = false
    if e.dim > 0
      e.dim = 0
      e.rows = 0
      e.last_discovery_fail_time = 0.0f  // force an immediate retry (type may have changed)
    else
      e.last_discovery_fail_time = get_sync_time()
    return
  if e.dim > 0 && resolved_type != e.type_name
    // Component still exists but its type changed - drop into re-discovery.
    e.dim = 0
    e.rows = 0
  if !try_sample_typed(data, resolved_type, e.dim, e.comp_idx, e.rows, e.row_idx, e.committed_val)
    e.found = false
    e.dim = 0
    e.rows = 0
    e.last_discovery_fail_time = get_sync_time()
    return
  e.found = true
  e.type_name = clone_string(resolved_type)  // clone: resolved_type may point into ECS storage


def plot_set_component(eid : EntityId; full_name : string; comp_idx, row_idx : int; type_name : string; new_val : float)
  var data : void?
  var resolved_type = ""
  if !resolve_component_path(eid, full_name, data, resolved_type)
    return
  if resolved_type != type_name
    return  // type changed since last sample; skip rather than write to mismatched memory
  try_set_typed(data, type_name, comp_idx, row_idx, new_val)


def ring_idx(idx, capacity : int) : int  // requires capacity > 0 && idx >= -capacity
  return (idx + capacity) % capacity

def ring_oldest_idx(count, capacity, write_idx : int) : int  // requires count <= capacity && write_idx in [0, capacity)
  return ring_idx(write_idx - count, capacity)

def dq_reset(var dq : array<int>; sz : int)
  // No clear(): head/tail are reset by the caller and any stale slots are unreachable
  // until overwritten via dq_push_*.
  dq |> resize(sz)

def dq_evict(dq : array<int>; head, tail, evicted_idx : int) : int
  return (head < tail && dq[head % length(dq)] == evicted_idx) ? head + 1 : head

def dq_push_max(var dq : array<int>; head : int; var tail : int; values : array<float>; new_idx : int; new_val : float) : int
  while head < tail && values[dq[(tail - 1) % length(dq)]] < new_val
    tail--
  dq[tail % length(dq)] = new_idx
  return tail + 1

def dq_push_min(var dq : array<int>; head : int; var tail : int; values : array<float>; new_idx : int; new_val : float) : int
  while head < tail && values[dq[(tail - 1) % length(dq)]] > new_val
    tail--
  dq[tail % length(dq)] = new_idx
  return tail + 1

def history_size_for_duration(duration : float) : int
  return clamp(int(ceil(duration * float(PLOT_MAX_ASSUMED_FPS) / PLOT_MIN_ASSUMED_TIMESPEED)), PLOT_HISTORY_MIN, PLOT_HISTORY_MAX)

def entry_reset(var e : PlotEntry&)
  e.write_idx = 0
  e.count = 0
  e.max_head = 0
  e.max_tail = 0
  e.min_head = 0
  e.min_tail = 0

def entry_resize(var e : PlotEntry&; sz : int)
  // No clear(): write_idx/count are reset below; stale slots are unreachable until
  // overwritten by the ring writer in plot_sample_entries.
  e.values |> resize(sz)
  dq_reset(e.max_dq, sz)
  dq_reset(e.min_dq, sz)
  entry_reset(e)

def unroll_ring_to_head(var arr : array<float>; write_idx, count, old_cap, new_cap : int) : int
  // Snapshot the ring's valid range into linear order at the head of the resized array.
  // Returns the new write_idx.
  assert(count <= old_cap && count <= new_cap)
  if count == 0 || old_cap == 0
    arr |> resize(new_cap)
    return 0
  var snap : array<float>
  snap |> resize(new_cap)
  let oldest = ring_oldest_idx(count, old_cap, write_idx)
  for k in range(count)
    snap[k] = arr[ring_idx(oldest + k, old_cap)]
  arr <- snap
  return count == new_cap ? 0 : count

def rebuild_entry_deques(var e : PlotEntry&)
  // Assumes e.values[0..e.count-1] is the linear valid range (post-unroll).
  assert(e.count <= length(e.values))
  assert(length(e.max_dq) == length(e.values) && length(e.min_dq) == length(e.values))
  e.max_head = 0; e.max_tail = 0
  e.min_head = 0; e.min_tail = 0
  for k in range(e.count)
    e.max_tail = dq_push_max(e.max_dq, e.max_head, e.max_tail, e.values, k, e.values[k])
    e.min_tail = dq_push_min(e.min_dq, e.min_head, e.min_tail, e.values, k, e.values[k])


def plot_save_entries(w : PlotWindow&; win_idx : int)
  let blk = "ecs_plot_w{win_idx}"
  imgui_set_blk_val(blk, "grid_x",      w.grid_x)
  imgui_set_blk_val(blk, "grid_y",      w.grid_y)
  imgui_set_blk_val(blk, "y_scale_idx", w.y_scale_idx)
  imgui_set_blk_val(blk, "shared_y",         w.shared_y)
  imgui_set_blk_val(blk, "shared_fit",       w.shared_fit)
  imgui_set_blk_val(blk, "shared_scale_min", w.shared_scale_min)
  imgui_set_blk_val(blk, "shared_scale_max", w.shared_scale_max)
  imgui_set_blk_val(blk, "count", length(w.entries))
  for i in range(length(w.entries))
    let e & = unsafe(w.entries[i])
    imgui_set_blk_val(blk, "eid_{i}",       int(e.eid))
    imgui_set_blk_val(blk, "name_{i}",      e.name)
    imgui_set_blk_val(blk, "comp_idx_{i}",  e.comp_idx)
    imgui_set_blk_val(blk, "row_idx_{i}",   e.row_idx)
    imgui_set_blk_val(blk, "scale_min_{i}", e.scale_min)
    imgui_set_blk_val(blk, "scale_max_{i}", e.scale_max)
    imgui_set_blk_val(blk, "fit_{i}",       e.fit)
    imgui_set_blk_val(blk, "hidden_{i}",    e.hidden)
    imgui_set_blk_val(blk, "slot_{i}",      e.slot_mode)
    imgui_set_blk_val(blk, "slotidx_{i}",   int(e.slot))
    if empty(e.name)
      imgui_set_blk_val(blk, "val_{i}",     e.committed_val)

def plot_load_entries(var w : PlotWindow&; win_idx : int)
  let blk = "ecs_plot_w{win_idx}"
  w.grid_x      = imgui_get_blk_val(blk, "grid_x",      w.grid_x)
  w.grid_y      = imgui_get_blk_val(blk, "grid_y",      w.grid_y)
  w.y_scale_idx = imgui_get_blk_val(blk, "y_scale_idx", w.y_scale_idx)
  w.shared_y         = imgui_get_blk_val(blk, "shared_y",         false)
  w.shared_fit       = imgui_get_blk_val(blk, "shared_fit",       true)
  w.shared_scale_min = imgui_get_blk_val(blk, "shared_scale_min", -1.0f)
  w.shared_scale_max = imgui_get_blk_val(blk, "shared_scale_max",  1.0f)
  let count = imgui_get_blk_val(blk, "count", 0)
  for i in range(count)
    if length(w.entries) >= PLOT_MAX_ENTRIES
      break
    w.entries |> resize(length(w.entries) + 1)
    var e & = unsafe(w.entries[length(w.entries) - 1])
    e.eid       = uint(imgui_get_blk_val(blk, "eid_{i}",  0))
    e.name      = clone_string(imgui_get_blk_val(blk, "name_{i}", ""))  // clone: BLK returns a pointer into its own storage
    e.comp_idx  = imgui_get_blk_val(blk, "comp_idx_{i}",  0)
    e.row_idx   = imgui_get_blk_val(blk, "row_idx_{i}",   0)
    e.scale_min = imgui_get_blk_val(blk, "scale_min_{i}", -1.0f)
    e.scale_max = imgui_get_blk_val(blk, "scale_max_{i}",  1.0f)
    e.fit       = imgui_get_blk_val(blk, "fit_{i}",       true)
    e.hidden    = imgui_get_blk_val(blk, "hidden_{i}",    false)
    e.slot_mode = imgui_get_blk_val(blk, "slot_{i}",      false)
    e.slot      = uint(imgui_get_blk_val(blk, "slotidx_{i}",   0))
    if empty(e.name)
      e.committed_val = imgui_get_blk_val(blk, "val_{i}", 0.0f)
      e.edit_val      = e.committed_val
    entry_resize(e, w.history_size)


def plot_process_requests(var g_plot_windows : array<PlotWindow>&)
  let req_count = imgui_get_blk_val("ecs_plot_req", "count", 0)
  if req_count > 0
    for ri in range(req_count)
      let req_win      = imgui_get_blk_val("ecs_plot_req", "win_{ri}",      -1)
      let req_eid      = imgui_get_blk_val("ecs_plot_req", "eid_{ri}",       0)
      let req_name     = imgui_get_blk_val("ecs_plot_req", "name_{ri}",     "")
      let req_comp_idx = imgui_get_blk_val("ecs_plot_req", "comp_idx_{ri}",  0)
      if req_win >= 0 && req_win < PLOT_WINDOW_COUNT && !empty(req_name)
        plot_ensure(g_plot_windows, req_win)
        var rw & = unsafe(g_plot_windows[req_win])
        if length(rw.entries) < PLOT_MAX_ENTRIES
          rw.entries |> resize(length(rw.entries) + 1)
          var re & = unsafe(rw.entries[length(rw.entries) - 1])
          re.eid = uint(req_eid)
          // clone: a later imgui_set_blk_val on the same name_{ri} slot mutates in place, so
          // any entry that stored the raw pointer would silently pick up the new name.
          re.name = clone_string(req_name)
          re.comp_idx = req_comp_idx
          re.scale_min = -1.0f
          re.scale_max = 1.0f
          re.fit = true
          entry_resize(re, rw.history_size)
          rw.dirty = true
    imgui_set_blk_val("ecs_plot_req", "count", 0)


def plot_build_csv(w : PlotWindow&) : string
  let n_entries = length(w.entries)
  let row_cnt = w.ts_count
  let ts_offset = ring_oldest_idx(w.ts_count, w.history_size, w.ts_write_idx)
  return build_string() <| $(sb)
    sb |> write("time")
    for i in range(n_entries)
      let e & = unsafe(w.entries[i])
      let axis_idx = clamp(e.comp_idx, 0, 3)
      let axis_char = slice("xyzw", axis_idx, axis_idx + 1)
      let row_suffix = e.rows > 1 ? "[{e.row_idx}]" : ""
      if e.dim == 1
        sb |> write(",{e.eid}/{e.name}{row_suffix}")
      else
        sb |> write(",{e.eid}/{e.name}{row_suffix}/{axis_char}")
    sb |> write("\n")
    for j in range(row_cnt)
      let ts_idx = ring_idx(ts_offset + j, w.history_size)
      sb |> write(fmt(":.4f", w.timestamps[ts_idx]))
      for i in range(n_entries)
        let e & = unsafe(w.entries[i])
        let entry_row_offset = row_cnt - e.count
        if j < entry_row_offset
          sb |> write(",")
        else
          let comp_offset = ring_oldest_idx(e.count, w.history_size, e.write_idx)
          let val_idx = ring_idx(comp_offset + j - entry_row_offset, w.history_size)
          sb |> write(",{fmt(":.4f", e.values[val_idx])}")
      sb |> write("\n")


def plot_settings(var w : PlotWindow&)
  if imgui::Selectable("Pause##plotpause", w.paused, ImGuiSelectableFlags.None, float2(imgui::CalcTextSize("Pause").x + 8.f, 0.f))
    w.paused = !w.paused
    if !w.paused
      // Clear all data on resume so we start fresh; recalculate history_size at current speed
      w.history_size = history_size_for_duration(w.plot_duration)
      w.ts_write_idx = 0
      w.ts_count = 0
      for k in range(length(w.entries))
        var ek & = unsafe(w.entries[k])
        entry_reset(ek)
  let n_entries = length(w.entries)
  imgui::SameLine()
  imgui::Text("duration:")
  imgui::SameLine()
  imgui::PushItemWidth(60.f)
  imgui::InputFloat("##plotdur", unsafe(addr(w.edit_plot_duration)), 0.0f)
  if imgui::IsItemDeactivatedAfterEdit()
    w.plot_duration = max(w.edit_plot_duration, 0.0f)
  if !imgui::IsItemActive()
    w.edit_plot_duration = w.plot_duration
  imgui::PopItemWidth()
  imgui::SameLine()
  imgui::Text("X grid:")
  imgui::SameLine()
  imgui::PushItemWidth(50.f)
  if imgui::InputInt("##gridx_step", unsafe(addr(w.grid_x)), 0)
    w.dirty = true
  imgui::PopItemWidth()
  w.grid_x = clamp(w.grid_x, 1, 100)
  imgui::SameLine()
  imgui::Text("Y grid:")
  imgui::SameLine()
  imgui::PushItemWidth(50.f)
  if imgui::InputInt("##gridy_step", unsafe(addr(w.grid_y)), 0)
    w.dirty = true
  imgui::PopItemWidth()
  w.grid_y = clamp(w.grid_y, 1, 100)
  imgui::SameLine()
  imgui::Text("Y scale from plot:")
  imgui::SameLine()
  imgui::PushItemWidth(80.f)
  if imgui::InputInt("##yscale", unsafe(addr(w.y_scale_idx)))
    w.dirty = true
  imgui::PopItemWidth()
  w.y_scale_idx = clamp(w.y_scale_idx, 0, max(n_entries - 1, 0))


def plot_update_timestamps(var w : PlotWindow&)
  let needed_history = history_size_for_duration(w.plot_duration)
  // Grows here; shrinks are handled in plot_maybe_shrink_history after eviction has
  // guaranteed ts_count fits.
  if needed_history > w.history_size || length(w.entries) == 0
    w.history_size = needed_history
  if length(w.timestamps) != w.history_size
    // Unroll the ring into a linear prefix so existing history is preserved across the resize.
    // Per-entry rings get the same treatment in plot_sample_entries.
    w.ts_write_idx = unroll_ring_to_head(w.timestamps, w.ts_write_idx, w.ts_count, length(w.timestamps), w.history_size)
  if !w.paused
    w.timestamps[w.ts_write_idx] = get_sync_time()
    w.ts_write_idx = ring_idx(w.ts_write_idx + 1, w.history_size)
    w.ts_count = min(w.ts_count + 1, w.history_size)


def plot_maybe_shrink_history(var w : PlotWindow&)
  // Shrinking is safe only after plot_evict_old_samples has trimmed ts_count (and each
  // entry's count, which is always <= ts_count) to fit the new duration. Physical buffer
  // resize happens next frame via plot_update_timestamps / plot_sample_entries length checks.
  let needed_history = history_size_for_duration(w.plot_duration)
  if needed_history < w.history_size && w.ts_count <= needed_history
    w.history_size = needed_history


def plot_sample_entries(var w : PlotWindow&)
  if !w.paused
    w.max_entry_count = 0
  for i in range(length(w.entries))
    var e & = unsafe(w.entries[i])

    // Resize ring buffer when history_size changes, preserving existing history.
    if length(e.values) != w.history_size
      e.write_idx = unroll_ring_to_head(e.values, e.write_idx, e.count, length(e.values), w.history_size)
      // Deques reference old ring indices; resize them and rebuild from linear values.
      dq_reset(e.max_dq, w.history_size)
      dq_reset(e.min_dq, w.history_size)
      rebuild_entry_deques(e)

    if w.paused
      continue

    if e.slot_mode
      e.dim = 1
      e.rows = 0
      e.type_name = "float"
      e.comp_idx = 0
      e.committed_val = daeditor_plot_debug_get_signal(int(e.slot))
      e.found = true
    else
      let entry_eid = EntityId(e.eid)
      if entry_eid != INVALID_ENTITY_ID
        plot_sample_entry(e, entry_eid)
    if !e.val_editing
      e.edit_val = e.committed_val
    let plot_val = e.committed_val
    let buffer_full = e.count == w.history_size
    let evicted_idx = e.write_idx
    if buffer_full
      e.max_head = dq_evict(e.max_dq, e.max_head, e.max_tail, evicted_idx)
      e.min_head = dq_evict(e.min_dq, e.min_head, e.min_tail, evicted_idx)
    e.values[e.write_idx] = plot_val
    let new_idx = e.write_idx
    e.write_idx = ring_idx(e.write_idx + 1, w.history_size)
    e.count = min(e.count + 1, w.history_size)
    w.max_entry_count = max(w.max_entry_count, e.count)
    e.max_tail = dq_push_max(e.max_dq, e.max_head, e.max_tail, e.values, new_idx, plot_val)
    e.min_tail = dq_push_min(e.min_dq, e.min_head, e.min_tail, e.values, new_idx, plot_val)
    // Skip per-entry auto-fit while Shared Y scale is on: it would silently mutate per-entry
    // scale_min/max, breaking the "toggle Shared Y scale off = restore prior per-entry state" contract.
    if !w.shared_y && e.fit && e.count > 0
      let fmax = e.values[e.max_dq[e.max_head % length(e.max_dq)]]
      let fmin = e.values[e.min_dq[e.min_head % length(e.min_dq)]]
      e.scale_min = fmin != fmax ? fmin : fmin - 1.0f
      e.scale_max = fmin != fmax ? fmax : fmax + 1.0f


def plot_compute_x_range(w : PlotWindow&) : float2
  let has_series_data = w.max_entry_count > 0
  let series_ts_newest = has_series_data ? w.timestamps[ring_idx(w.ts_write_idx - 1, w.history_size)] : 0.0f
  let now = (w.paused && has_series_data) ? series_ts_newest : get_sync_time()
  return float2(now - w.plot_duration, w.plot_duration)


def plot_evict_old_samples(var w : PlotWindow&)
  if w.paused || w.ts_count == 0
    return
  let cutoff = get_sync_time() - w.plot_duration
  let ts_base = ring_oldest_idx(w.ts_count, w.history_size, w.ts_write_idx)
  var evict_count = 0
  while evict_count < w.ts_count
    if w.timestamps[ring_idx(ts_base + evict_count, w.history_size)] >= cutoff
      break
    evict_count++
  if evict_count == 0
    return
  let old_ts_count = w.ts_count
  w.ts_count -= evict_count
  for i in range(length(w.entries))
    var e & = unsafe(w.entries[i])
    // Entry samples cover only the last e.count timestamps; only evict if the
    // evicted range overlaps with those (entries added mid-stream have e.count < old_ts_count)
    let entry_evict = max(0, evict_count - (old_ts_count - e.count))
    let val_base = ring_oldest_idx(e.count, w.history_size, e.write_idx)
    for k in range(entry_evict)
      let oldest_val_idx = ring_idx(val_base + k, w.history_size)
      e.max_head = dq_evict(e.max_dq, e.max_head, e.max_tail, oldest_val_idx)
      e.min_head = dq_evict(e.min_dq, e.min_head, e.min_tail, oldest_val_idx)
    e.count -= entry_evict


// Global auto-fit: min/max value across every entry's live ring-buffer window. Uses each
// entry's monotonic min/max deque head so the cost is O(entries), not O(entries * history).
def plot_global_data_bounds(w : PlotWindow&) : float2
  let n = length(w.entries)
  var g_min = 0.0f
  var g_max = 1.0f
  var seen = false
  for i in range(n)
    let e & = unsafe(w.entries[i])
    if e.count > 0 && !e.hidden
      let e_max = e.values[e.max_dq[e.max_head % length(e.max_dq)]]
      let e_min = e.values[e.min_dq[e.min_head % length(e.min_dq)]]
      if !seen
        g_min = e_min
        g_max = e_max
        seen = true
      else
        g_min = min(g_min, e_min)
        g_max = max(g_max, e_max)
  if seen && g_min == g_max
    g_min -= 1.0f
    g_max += 1.0f
  return float2(g_min, g_max)

// Selects the (min, max) that drives the plot's Y axis and every entry's line-drawing scale.
// Shared Y scale on + fit on : global auto-fit from data (per-entry scale_min/max is untouched).
// Shared Y scale on + fit off: user-set w.shared_scale_min/max.
// Shared Y scale off         : entries[y_scale_idx]'s scale, as before.
def plot_y_bounds(w : PlotWindow&) : float2
  let n = length(w.entries)
  if n == 0
    return float2(0.0f, 1.0f)
  if w.shared_y
    if w.shared_fit
      return plot_global_data_bounds(w)
    return float2(w.shared_scale_min, w.shared_scale_max)
  let idx = clamp(w.y_scale_idx, 0, n - 1)
  return float2(unsafe(w.entries[idx].scale_min), unsafe(w.entries[idx].scale_max))

def plot_draw_grid(w : PlotWindow&; plot_pos, plot_size : float2; x_left, x_range : float)
  let yb = plot_y_bounds(w)
  let y_min = yb.x
  let y_max = yb.y
  let grid_col  = IM_COL32(80, 80, 80, 160)
  let label_col = IM_COL32(200, 200, 200, 180)
  let font_size = imgui::GetFontSize()
  var dl = imgui::GetWindowDrawList()
  if w.grid_x > 0
    for gx in range(1, w.grid_x)
      let x = plot_pos.x + plot_size.x * float(gx) / float(w.grid_x)
      *dl |> AddLine(float2(x, plot_pos.y), float2(x, plot_pos.y + plot_size.y), grid_col, 1.0f)
      if x_range > 0.0f
        let lbl = fmt(":.3f", x_left + float(gx) / float(w.grid_x) * x_range)
        *dl |> AddText(float2(x + 2.f, plot_pos.y + plot_size.y - font_size - 2.f), label_col, lbl)
  if w.grid_y > 0
    for gy in range(1, w.grid_y)
      let y = plot_pos.y + plot_size.y * float(gy) / float(w.grid_y)
      *dl |> AddLine(float2(plot_pos.x, y), float2(plot_pos.x + plot_size.x, y), grid_col, 1.0f)
      let val = y_max + float(gy) / float(w.grid_y) * (y_min - y_max)
      *dl |> AddText(float2(plot_pos.x + 2.f, y - font_size - 2.f), label_col, fmt(":.3f", val))


def plot_draw_lines(w : PlotWindow&; plot_pos, plot_size : float2; x_left, x_range : float)
  if x_range <= 0.0f
    return
  let n = length(w.entries)
  var dl = imgui::GetWindowDrawList()
  *dl |> PushClipRect(plot_pos, plot_pos + plot_size, true)
  let yb = plot_y_bounds(w)
  for i in range(n)
    let e & = unsafe(w.entries[i])
    if e.count >= 1 && !e.hidden
      let cnt = e.count
      let offset = ring_oldest_idx(cnt, w.history_size, e.write_idx)
      let entry_color = get_plot_color(i)
      let smin = w.shared_y ? yb.x : e.scale_min
      let smax = w.shared_y ? yb.y : e.scale_max
      let scale_range = smax - smin
      if scale_range != 0.0f
        for j in range(cnt - 1)
          let idx0 = ring_idx(offset + j, w.history_size)
          let idx1 = ring_idx(offset + j + 1, w.history_size)
          let v0 = e.values[idx0]
          let v1 = e.values[idx1]
          let ts0 = w.timestamps[ring_idx(w.ts_write_idx - cnt + j, w.history_size)]
          let ts1 = w.timestamps[ring_idx(w.ts_write_idx - cnt + j + 1, w.history_size)]
          let x0 = plot_pos.x + plot_size.x * (ts0 - x_left) / x_range
          let x1 = plot_pos.x + plot_size.x * (ts1 - x_left) / x_range
          let y0 = plot_pos.y + plot_size.y * (1.0f - clamp((v0 - smin) / scale_range, 0.0f, 1.0f))
          let y1 = plot_pos.y + plot_size.y * (1.0f - clamp((v1 - smin) / scale_range, 0.0f, 1.0f))
          *dl |> AddLine(float2(x0, y0), float2(x1, y1), entry_color, 1.5f)
          *dl |> AddCircleFilled(float2(x0, y0), 2.0f, entry_color, 6)
        let idx_last = ring_idx(offset + cnt - 1, w.history_size)
        let v_last = e.values[idx_last]
        let ts_last = w.timestamps[ring_idx(w.ts_write_idx - 1, w.history_size)]
        let x_last = plot_pos.x + plot_size.x * (ts_last - x_left) / x_range
        let y_last = plot_pos.y + plot_size.y * (1.0f - clamp((v_last - smin) / scale_range, 0.0f, 1.0f))
        *dl |> AddCircleFilled(float2(x_last, y_last), 2.0f, entry_color, 6)
  *dl |> PopClipRect()


def plot_draw(var w : PlotWindow&; row_h : float)
  let n = length(w.entries)
  let avail = imgui::GetContentRegionAvail()
  let plot_size = float2(avail.x, max(avail.y - float(n + 1) * row_h, 50.f))
  let plot_pos = imgui::GetCursorScreenPos()
  imgui::Dummy(plot_size)
  let plot_hovered = imgui::IsItemHovered(ImGuiHoveredFlags.None)
  var dl = imgui::GetWindowDrawList()
  *dl |> AddRectFilled(plot_pos, plot_pos + plot_size, IM_COL32(20, 20, 20, 255), 0.0f, ImDrawFlags.None)
  *dl |> AddRect(plot_pos, plot_pos + plot_size, IM_COL32(100, 100, 100, 255), 0.0f, ImDrawFlags.None, 1.0f)
  let xr = plot_compute_x_range(w)
  let x_left  = xr.x
  let x_range = xr.y
  plot_draw_grid(w, plot_pos, plot_size, x_left, x_range)
  plot_draw_lines(w, plot_pos, plot_size, x_left, x_range)
  if plot_hovered
    plot_draw_tooltip(w, plot_pos, plot_size, x_left, x_range)


// Nearest-neighbor sample of one entry's ring buffer at target_ts.
// Returns (found, value); found=false when the entry has no samples or is hidden.
def plot_sample_at_ts(w : PlotWindow&; e : PlotEntry&; target_ts : float; var out_val : float&) : bool
  let cnt = e.count
  if cnt == 0 || e.hidden
    return false
  let val_offset = ring_oldest_idx(cnt, w.history_size, e.write_idx)
  let ts_offset  = w.ts_write_idx - cnt
  var best_j = 0
  var best_dist = 1.0e30f
  for j in range(cnt)
    let ts = w.timestamps[ring_idx(ts_offset + j, w.history_size)]
    let d = abs(ts - target_ts)
    if d < best_dist
      best_dist = d
      best_j = j
  let val_idx = ring_idx(val_offset + best_j, w.history_size)
  out_val = e.values[val_idx]
  return true


def plot_draw_tooltip(w : PlotWindow&; plot_pos, plot_size : float2; x_left, x_range : float)
  let n = length(w.entries)
  if n == 0 || x_range <= 0.f
    return
  let mouse_pos = imgui::GetMousePos()
  let rel_x = (mouse_pos.x - plot_pos.x) / plot_size.x
  let rel_y = (mouse_pos.y - plot_pos.y) / plot_size.y
  if rel_x < 0.f || rel_x > 1.f || rel_y < 0.f || rel_y > 1.f
    return
  let target_ts = x_left + rel_x * x_range
  imgui::SetNextWindowBgAlpha(0.5f)
  if imgui::BeginTooltip()
    imgui::TextUnformatted("t: {fmt(":.4f", target_ts)}")
    let row_count = (n + 3) / 4
    for row in range(row_count)
      let row_start = row * 4
      let row_end = min(row_start + 4, n)
      for i in range(row_start, row_end)
        // Row 0 (v0..v3) appends to the timestamp line; rows 1+ start on a fresh line.
        if row == 0 || i > row_start
          imgui::SameLine(0.f, 0.f)
          imgui::TextUnformatted(" | ")
          imgui::SameLine(0.f, 0.f)
        let e & = unsafe(w.entries[i])
        var v = 0.0f
        let has = plot_sample_at_ts(w, e, target_ts, v)
        let val_str = has ? fmt(":.4f", v) : "-"
        imgui::PushStyleColor(ImGuiCol.Text, get_plot_color(i))
        imgui::TextUnformatted("v{i}: {val_str}")
        imgui::PopStyleColor()
    imgui::EndTooltip()


def plot_toolbar(var w : PlotWindow&; daeditor_eid : EntityId)
  if imgui::Button("Add##plotadd") && length(w.entries) < PLOT_MAX_ENTRIES
    let n = length(w.entries)
    w.entries |> resize(n + 1)
    var last & = unsafe(w.entries[n])
    if w.toolbar_slot_mode
      last.slot_mode = true
      last.slot = w.toolbar_slot
    else
      last.eid = w.toolbar_eid
    last.scale_min = -1.0f
    last.scale_max = 1.0f
    last.fit = true
    entry_resize(last, w.history_size)
    w.dirty = true
  imgui::SameLine()
  let toolbar_mode_label = w.toolbar_slot_mode ? "slt:" : "eid:"
  if imgui::SmallButton("{toolbar_mode_label}##toolbar_mode_toggle")
    w.toolbar_slot_mode = !w.toolbar_slot_mode
  imgui::SameLine()
  imgui::PushItemWidth(80.f)
  if w.toolbar_slot_mode
    imgui::InputScalar("##toolbar_slot", ImGuiDataType.U32, unsafe(addr(w.toolbar_slot)), null, null, "%u", ImGuiInputTextFlags.None)
  else
    imgui::InputScalar("##toolbar_eid", ImGuiDataType.U32, unsafe(addr(w.toolbar_eid)), null, null, "%u", ImGuiInputTextFlags.None)
  imgui::PopItemWidth()
  imgui::SameLine()
  // "Use selected" only makes sense in eid mode; show a same-size dummy in slot mode to keep downstream buttons aligned.
  if w.toolbar_slot_mode
    let use_sel_w = imgui::CalcTextSize("Use selected").x + imgui::GetStyle().FramePadding.x * 2.f
    imgui::Dummy(float2(use_sel_w, imgui::GetFrameHeight()))
  else
    if imgui::Button("Use selected##toolbar_use_sel") && daeditor_eid != INVALID_ENTITY_ID
      w.toolbar_eid = uint(daeditor_eid)
  imgui::SameLine()
  if imgui::Button("Delete all##plotdeleteall") && !empty(w.entries)
    w.entries |> clear()
    w.dirty = true
  imgui::SameLine()
  if imgui::Button("Copy CSV##copycsv") && !empty(w.entries)
    imgui::SetClipboardText(plot_build_csv(w))
  plot_toolbar_shared_y(w)


def plot_toolbar_shared_y(var w : PlotWindow&)
  // Mirror global data range into the InputFloat-backed shared_scale_min/max while fit is on.
  if w.shared_y && w.shared_fit && !empty(w.entries)
    let gb = plot_global_data_bounds(w)
    w.shared_scale_min = gb.x
    w.shared_scale_max = gb.y
  let spacing = imgui::GetStyle().ItemSpacing.x
  let cluster_w = imgui::CalcTextSize("Shared Y scale").x + spacing + imgui::GetFrameHeight()
  imgui::SameLine(w.fit_widget_x - cluster_w - spacing * 2.0f)
  imgui::Text("Shared Y scale")
  imgui::SameLine()
  if imgui::Checkbox("##sharedy", unsafe(addr(w.shared_y)))
    if w.shared_y && !empty(w.entries)
      // Seed from current data range so the axis doesn't jump on toggle.
      let gb = plot_global_data_bounds(w)
      w.shared_scale_min = gb.x
      w.shared_scale_max = gb.y
      var any_fit = false
      for i in range(length(w.entries))
        if unsafe(w.entries[i]).fit
          any_fit = true
      w.shared_fit = any_fit
    w.dirty = true
  if !w.shared_y
    return
  imgui::SameLine(w.fit_widget_x)
  imgui::Text("fit:")
  imgui::SameLine()
  if imgui::Checkbox("##sharedfit", unsafe(addr(w.shared_fit)))
    w.dirty = true
  imgui::SameLine()
  imgui::Text("min:")
  imgui::SameLine()
  imgui::PushItemWidth(70.f)
  if imgui::InputFloat("##sharedmin", unsafe(addr(w.shared_scale_min)))
    w.shared_fit = false
    w.dirty = true
  imgui::PopItemWidth()
  imgui::SameLine()
  imgui::Text("max:")
  imgui::SameLine()
  imgui::PushItemWidth(70.f)
  if imgui::InputFloat("##sharedmax", unsafe(addr(w.shared_scale_max)))
    w.shared_fit = false
    w.dirty = true
  imgui::PopItemWidth()


def plot_entry_rows(var w : PlotWindow&; var to_delete : int&)
  var signal_count = 0
  find_query() <| $ [es(REQUIRE=daeditor_plot_debug__signalCount)] (daeditor_plot_debug__signalCount : int)
    signal_count = daeditor_plot_debug__signalCount
    return true
  for i in range(length(w.entries))
    imgui::PushID(i)
    var e & = unsafe(w.entries[i])

    // Delete button
    if imgui::Button("Del##del")
      to_delete = i
    imgui::SameLine()
    // Hide toggle (Ctrl+click = solo: hide all others, show only this one; Ctrl+click again = restore all)
    if imgui::Selectable("Hide##hide", e.hidden, ImGuiSelectableFlags.None, float2(imgui::CalcTextSize("Hide").x + 8.f, 0.f))
      if imgui::IsKeyDown(ImGuiKey.ImGuiMod_Ctrl)
        var all_others_hidden = true
        for j in range(length(w.entries))
          if j != i && !w.entries[j].hidden
            all_others_hidden = false
            break
        if all_others_hidden && !e.hidden
          for j in range(length(w.entries))
            unsafe(w.entries[j]).hidden = false
        else
          for j in range(length(w.entries))
            unsafe(w.entries[j]).hidden = j != i
      else
        e.hidden = !e.hidden
      w.dirty = true
    imgui::SameLine()

    // Color swatch
    let entry_color = get_plot_color(i)
    let swatch_pos = imgui::GetCursorScreenPos()
    imgui::Dummy(float2(14.f, 14.f))
    var dl_row = imgui::GetWindowDrawList()
    *dl_row |> AddRectFilled(swatch_pos, swatch_pos + float2(14.f, 14.f), entry_color, 0.0f, ImDrawFlags.None)
    imgui::SameLine()

    // "eid:" / "slt:" mode toggle button. In slot mode the eid field is a signal slot
    // index (0..signal_count-1) sourced from the daeditor_plot_debug singleton's signals list.
    let mode_label = e.slot_mode ? "slt:" : "eid:"
    if imgui::SmallButton("{mode_label}##mode_toggle")
      e.slot_mode = !e.slot_mode
      e.dim = 0
      e.rows = 0
      e.type_name = ""
      e.last_discovery_fail_time = 0.0f
      w.dirty = true
    imgui::SameLine()

    // EID / slot field - backed by separate uint variables so toggling modes preserves
    // both values (switching back to eid shows the previously typed eid).
    let entry_eid_ui = EntityId(e.eid)
    let eid_exists = doesEntityExist(entry_eid_ui)
    let eid_invalid_as_entity = !e.slot_mode && entry_eid_ui != INVALID_ENTITY_ID && !eid_exists
    let slot_invalid = e.slot_mode && int(e.slot) >= signal_count
    let field_invalid = eid_invalid_as_entity || slot_invalid
    imgui::PushItemWidth(90.f)
    if field_invalid
      imgui::PushStyleColor(ImGuiCol.FrameBg, IM_COL32(50, 50, 160, 160))
    if e.slot_mode
      if imgui::InputScalar("##slot", ImGuiDataType.U32, unsafe(addr(e.slot)), null, null, "%u", ImGuiInputTextFlags.None)
        w.dirty = true
    else
      if imgui::InputScalar("##eid", ImGuiDataType.U32, unsafe(addr(e.eid)), null, null, "%u", ImGuiInputTextFlags.None)
        w.dirty = true
        e.last_discovery_fail_time = 0.0f
    if field_invalid
      imgui::PopStyleColor()
    imgui::PopItemWidth()
    imgui::SameLine()

    // Component name - hidden in slot mode (replaced by a Dummy of the same footprint so
    // downstream widgets keep their column position). The backing e.name is preserved.
    if e.slot_mode
      let name_label_w = imgui::CalcTextSize("name:").x
      let name_input_w = 200.f
      let spacing = imgui::GetStyle().ItemSpacing.x
      imgui::Dummy(float2(name_label_w + spacing + name_input_w, imgui::GetFrameHeight()))
    else
      let name_not_found = !empty(e.name) && entry_eid_ui != INVALID_ENTITY_ID && !e.found && !w.paused
      imgui::Text("name:")
      imgui::SameLine()
      imgui::PushItemWidth(200.f)
      if name_not_found
        imgui::PushStyleColor(ImGuiCol.FrameBg, IM_COL32(50, 50, 160, 160))
      let buf <- ImGuiInputTextBuffer(e.name, 256)
      if InputText("##name", buf, ImGuiInputTextFlags.None)
        e.name = clone_string(to_string(buf))
        e.dim = 0
        e.rows = 0
        e.type_name = ""
        e.last_discovery_fail_time = 0.0f
        w.dirty = true
      if name_not_found
        imgui::PopStyleColor()
      imgui::PopItemWidth()
      if imgui::IsItemHovered(ImGuiHoveredFlags.None) && !empty(e.name)
        if imgui::BeginTooltip()
          imgui::TextUnformatted(e.name)
          imgui::EndTooltip()

    let max_row = e.rows > 0 ? e.rows - 1 : 3
    e.row_idx = clamp(e.row_idx, 0, max_row)
    imgui::SameLine()
    let row_btn_w = imgui::CalcTextSize("[0]").x + imgui::GetStyle().FramePadding.x * 2.f
    if e.rows > 1
      if imgui::Button("[{e.row_idx}]##row")
        e.row_idx = (e.row_idx + 1) % (max_row + 1)
        w.dirty = true
    else
      imgui::Dummy(float2(row_btn_w, imgui::GetFrameHeight()))

    // xyzw index button - click to cycle x->y->z->w->x; hidden for scalars but space is preserved
    let max_idx = e.dim > 0 ? e.dim - 1 : 3
    e.comp_idx = clamp(e.comp_idx, 0, max_idx)
    imgui::SameLine()
    let axis_names = "xyzw"
    let btn_w = imgui::CalcTextSize(".x").x + imgui::GetStyle().FramePadding.x * 2.f
    if e.dim > 1
      if imgui::Button(".{slice(axis_names, e.comp_idx, e.comp_idx + 1)}##idx")
        e.comp_idx = (e.comp_idx + 1) % (max_idx + 1)
        w.dirty = true
    else
      imgui::Dummy(float2(btn_w, imgui::GetFrameHeight()))

    // Current value (editable; writing commits to ECS)
    imgui::SameLine()
    imgui::Text("value:")
    imgui::SameLine()
    imgui::PushItemWidth(80.f)
    let val_editing = e.val_editing
    // When not editing: render InputFloat with invisible text so the widget still accepts input,
    // then overdraw with right-aligned text via the draw list (avoids left-aligned cursor artifact).
    if !val_editing
      imgui::PushStyleColor(ImGuiCol.Text, IM_COL32(0, 0, 0, 0))
    let val_field_pos = imgui::GetCursorScreenPos()
    imgui::InputFloat("##lastval", unsafe(addr(e.edit_val)), 0.0f, 0.0f, "%.4f", ImGuiInputTextFlags.None)
    e.val_editing = imgui::IsItemActive()
    if !val_editing
      imgui::PopStyleColor()
      let val_text = fmt(":.4f", e.edit_val)
      let val_text_w = imgui::CalcTextSize(val_text).x
      let val_frame_h = imgui::GetFrameHeight()
      let val_font_h = imgui::GetFontSize()
      let tx = val_field_pos.x + 80.f - val_text_w - 4.f
      let ty = val_field_pos.y + (val_frame_h - val_font_h) * 0.5f
      var dl_val = imgui::GetWindowDrawList()
      *dl_val |> AddText(float2(tx, ty), IM_COL32(255, 255, 255, 220), val_text)
    imgui::PopItemWidth()
    if imgui::IsItemDeactivatedAfterEdit()
      let new_val = e.edit_val
      e.committed_val = new_val
      if e.slot_mode
        daeditor_plot_debug_signal(new_val, int(e.slot))
      else
        if empty(e.name)
          w.dirty = true
        let set_eid = EntityId(e.eid)
        if set_eid != INVALID_ENTITY_ID
          plot_set_component(set_eid, e.name, e.comp_idx, e.row_idx, e.type_name, new_val)

    // Y-scale: per-entry auto-fit toggle and manual min/max bounds. Always visible; when
    // Shared Y scale is on the plot ignores these (uses w.shared_* instead), but the values are
    // preserved so toggling Shared Y scale off restores the per-entry state exactly as it was.
    imgui::SameLine()
    w.fit_widget_x = imgui::GetCursorPosX()  // captured for toolbar's shared-controls alignment
    imgui::Text("fit:")
    imgui::SameLine()
    if imgui::Checkbox("##fit", unsafe(addr(e.fit)))
      w.dirty = true
    imgui::SameLine()
    imgui::Text("min:")
    imgui::SameLine()
    imgui::PushItemWidth(70.f)
    if imgui::InputFloat("##smin", unsafe(addr(e.scale_min)))
      e.fit = false  // manual bound wins; otherwise auto-fit clobbers it next frame
      w.dirty = true
    imgui::PopItemWidth()
    imgui::SameLine()
    imgui::Text("max:")
    imgui::SameLine()
    imgui::PushItemWidth(70.f)
    if imgui::InputFloat("##smax", unsafe(addr(e.scale_max)))
      e.fit = false
      w.dirty = true
    imgui::PopItemWidth()

    imgui::PopID()


def aux_ecs_component_plot(win_idx : int)
  static_let <|
    var g_plot_windows : array<PlotWindow>

  plot_process_requests(g_plot_windows)

  plot_ensure(g_plot_windows, win_idx)
  var w & = unsafe(g_plot_windows[win_idx])

  // Set default window size on first open. (Saved entries are loaded inside plot_ensure.)
  if !w.win_size_init
    imgui::SetWindowSize(float2(995.f, 400.f), ImGuiCond FirstUseEver)
    w.win_size_init = true

  // Query daeditor selection - needed for defaults and "cur selected" buttons
  var daeditor_eid = INVALID_ENTITY_ID
  query() <| $ [es(REQUIRE=daeditor__selected)] (eid : EntityId)
    daeditor_eid = eid

  // UI input: top bar (captures duration/grid/y-scale/pause changes)
  plot_settings(w)
  imgui::Separator()

  // Data: timestamps, sample, evict, maybe-shrink (uses just-captured settings)
  plot_update_timestamps(w)
  plot_sample_entries(w)
  plot_evict_old_samples(w)
  plot_maybe_shrink_history(w)

  // UI render: plot body
  let row_h = imgui::GetFrameHeightWithSpacing()
  plot_draw(w, row_h)

  // UI input + render: bottom controls (add/delete, per-entry edits)
  plot_toolbar(w, daeditor_eid)
  var to_delete = -1
  plot_entry_rows(w, to_delete)

  // Post: apply deferred mutations, adjust window size, persist on dirty
  if to_delete >= 0
    w.entries |> erase(to_delete)
    w.dirty = true

  // Resize window vertically so plot area stays the same height as entries are added/deleted
  let n_current = length(w.entries)
  if w.prev_n_entries >= 0 && n_current != w.prev_n_entries
    let cur_size = imgui::GetWindowSize()
    imgui::SetWindowSize(float2(cur_size.x, cur_size.y + float(n_current - w.prev_n_entries) * row_h))
  w.prev_n_entries = n_current

  // Save only on real user mutations so idle windows don't hit BLK every frame
  if w.dirty
    plot_save_entries(w, win_idx)
    w.dirty = false


[imgui_window(name="Plot", group="ECS")]
def ecs_component_plot_1()
  if imgui::IsWindowCollapsed()
    return
  aux_ecs_component_plot(0)

[imgui_window(name="Plot 2", group="ECS")]
def ecs_component_plot_2()
  if imgui::IsWindowCollapsed()
    return
  aux_ecs_component_plot(1)

[imgui_window(name="Plot 3", group="ECS")]
def ecs_component_plot_3()
  if imgui::IsWindowCollapsed()
    return
  aux_ecs_component_plot(2)

[imgui_window(name="Plot 4", group="ECS")]
def ecs_component_plot_4()
  if imgui::IsWindowCollapsed()
    return
  aux_ecs_component_plot(3)


[plot_sample(type_name="float")]
def sample_float(data : void?;
                 var dim : int&; var comp_idx : int&;
                 var rows : int&; var row_idx : int&;
                 var val : float&) : void
  dim = 1
  unsafe
    val = *reinterpret<float?> data

[plot_set(type_name="float")]
def set_float(data : void?; comp_idx, row_idx : int; new_val : float) : void
  unsafe
    *reinterpret<float?> data = new_val

[plot_sample(type_name="double")]
def sample_double(data : void?;
                  var dim : int&; var comp_idx : int&;
                  var rows : int&; var row_idx : int&;
                  var val : float&) : void
  dim = 1
  unsafe
    val = float(*reinterpret<double?> data)

[plot_set(type_name="double")]
def set_double(data : void?; comp_idx, row_idx : int; new_val : float) : void
  unsafe
    *reinterpret<double?> data = double(new_val)

[plot_sample(type_name="int")]
def sample_int(data : void?;
               var dim : int&; var comp_idx : int&;
               var rows : int&; var row_idx : int&;
               var val : float&) : void
  dim = 1
  unsafe
    val = float(*reinterpret<int?> data)

[plot_set(type_name="int")]
def set_int(data : void?; comp_idx, row_idx : int; new_val : float) : void
  unsafe
    *reinterpret<int?> data = int(new_val)

[plot_sample(type_name="int8_t")]
def sample_int8(data : void?;
                var dim : int&; var comp_idx : int&;
                var rows : int&; var row_idx : int&;
                var val : float&) : void
  dim = 1
  unsafe
    val = float(*reinterpret<int8?> data)

[plot_set(type_name="int8_t")]
def set_int8(data : void?; comp_idx, row_idx : int; new_val : float) : void
  unsafe
    *reinterpret<int8?> data = int8(clamp(int(new_val), -128, 127))

[plot_sample(type_name="uint8_t")]
def sample_uint8(data : void?;
                 var dim : int&; var comp_idx : int&;
                 var rows : int&; var row_idx : int&;
                 var val : float&) : void
  dim = 1
  unsafe
    val = float(*reinterpret<uint8?> data)

[plot_set(type_name="uint8_t")]
def set_uint8(data : void?; comp_idx, row_idx : int; new_val : float) : void
  unsafe
    *reinterpret<uint8?> data = uint8(clamp(int(new_val), 0, 255))

[plot_sample(type_name="int16_t")]
def sample_int16(data : void?;
                 var dim : int&; var comp_idx : int&;
                 var rows : int&; var row_idx : int&;
                 var val : float&) : void
  dim = 1
  unsafe
    val = float(*reinterpret<int16?> data)

[plot_set(type_name="int16_t")]
def set_int16(data : void?; comp_idx, row_idx : int; new_val : float) : void
  unsafe
    *reinterpret<int16?> data = int16(clamp(int(new_val), -32768, 32767))

[plot_sample(type_name="uint16_t")]
def sample_uint16(data : void?;
                  var dim : int&; var comp_idx : int&;
                  var rows : int&; var row_idx : int&;
                  var val : float&) : void
  dim = 1
  unsafe
    val = float(*reinterpret<uint16?> data)

[plot_set(type_name="uint16_t")]
def set_uint16(data : void?; comp_idx, row_idx : int; new_val : float) : void
  unsafe
    *reinterpret<uint16?> data = uint16(clamp(int(new_val), 0, 65535))

[plot_sample(type_name="uint32_t")]
def sample_uint32(data : void?;
                  var dim : int&; var comp_idx : int&;
                  var rows : int&; var row_idx : int&;
                  var val : float&) : void
  dim = 1
  unsafe
    val = float(*reinterpret<uint?> data)

[plot_set(type_name="uint32_t")]
def set_uint32(data : void?; comp_idx, row_idx : int; new_val : float) : void
  unsafe
    *reinterpret<uint?> data = uint(max(int(new_val), 0))

[plot_sample(type_name="int64_t")]
def sample_int64(data : void?;
                 var dim : int&; var comp_idx : int&;
                 var rows : int&; var row_idx : int&;
                 var val : float&) : void
  dim = 1
  unsafe
    val = float(*reinterpret<int64?> data)

[plot_set(type_name="int64_t")]
def set_int64(data : void?; comp_idx, row_idx : int; new_val : float) : void
  unsafe
    *reinterpret<int64?> data = int64(new_val)

[plot_sample(type_name="uint64_t")]
def sample_uint64(data : void?;
                  var dim : int&; var comp_idx : int&;
                  var rows : int&; var row_idx : int&;
                  var val : float&) : void
  dim = 1
  unsafe
    val = float(*reinterpret<uint64?> data)

[plot_set(type_name="uint64_t")]
def set_uint64(data : void?; comp_idx, row_idx : int; new_val : float) : void
  unsafe
    *reinterpret<uint64?> data = uint64(max(new_val, 0.0f))

[plot_sample(type_name="ecs::EntityId")]
def sample_EntityId(data : void?;
                    var dim : int&; var comp_idx : int&;
                    var rows : int&; var row_idx : int&;
                    var val : float&) : void
  dim = 1
  unsafe
    val = float(*reinterpret<uint?> data)

[plot_set(type_name="ecs::EntityId")]
def set_EntityId(data : void?; comp_idx, row_idx : int; new_val : float) : void
  unsafe
    *reinterpret<uint?> data = uint(max(int(new_val), 0))

[plot_sample(type_name="bool")]
def sample_bool(data : void?;
                var dim : int&; var comp_idx : int&;
                var rows : int&; var row_idx : int&;
                var val : float&) : void
  dim = 1
  unsafe
    val = *reinterpret<bool?> data ? 1.0f : 0.0f

[plot_set(type_name="bool")]
def set_bool(data : void?; comp_idx, row_idx : int; new_val : float) : void
  unsafe
    *reinterpret<bool?> data = new_val != 0.0f


[plot_sample(type_name="Point2")]
def sample_Point2(data : void?;
                  var dim : int&; var comp_idx : int&;
                  var rows : int&; var row_idx : int&;
                  var val : float&) : void
  dim = 2
  comp_idx = clamp(comp_idx, 0, 1)
  unsafe
    val = (*reinterpret<float2?> data)[comp_idx]

[plot_set(type_name="Point2")]
def set_Point2(data : void?; comp_idx, row_idx : int; new_val : float) : void
  if comp_idx < 0 || comp_idx >= 2
    return
  unsafe
    (*reinterpret<float2?> data)[comp_idx] = new_val

[plot_sample(type_name="Point3")]
def sample_Point3(data : void?;
                  var dim : int&; var comp_idx : int&;
                  var rows : int&; var row_idx : int&;
                  var val : float&) : void
  dim = 3
  comp_idx = clamp(comp_idx, 0, 2)
  unsafe
    val = (*reinterpret<float3?> data)[comp_idx]

[plot_set(type_name="Point3")]
def set_Point3(data : void?; comp_idx, row_idx : int; new_val : float) : void
  if comp_idx < 0 || comp_idx >= 3
    return
  unsafe
    (*reinterpret<float3?> data)[comp_idx] = new_val

[plot_sample(type_name="Point4")]
def sample_Point4(data : void?;
                  var dim : int&; var comp_idx : int&;
                  var rows : int&; var row_idx : int&;
                  var val : float&) : void
  dim = 4
  comp_idx = clamp(comp_idx, 0, 3)
  unsafe
    val = (*reinterpret<float4?> data)[comp_idx]

[plot_set(type_name="Point4")]
def set_Point4(data : void?; comp_idx, row_idx : int; new_val : float) : void
  if comp_idx < 0 || comp_idx >= 4
    return
  unsafe
    (*reinterpret<float4?> data)[comp_idx] = new_val


[plot_sample(type_name="IPoint2")]
def sample_IPoint2(data : void?;
                   var dim : int&; var comp_idx : int&;
                   var rows : int&; var row_idx : int&;
                   var val : float&) : void
  dim = 2
  comp_idx = clamp(comp_idx, 0, 1)
  unsafe
    val = float((*reinterpret<int2?> data)[comp_idx])

[plot_set(type_name="IPoint2")]
def set_IPoint2(data : void?; comp_idx, row_idx : int; new_val : float) : void
  if comp_idx < 0 || comp_idx >= 2
    return
  unsafe
    (*reinterpret<int2?> data)[comp_idx] = int(new_val)

[plot_sample(type_name="IPoint3")]
def sample_IPoint3(data : void?;
                   var dim : int&; var comp_idx : int&;
                   var rows : int&; var row_idx : int&;
                   var val : float&) : void
  dim = 3
  comp_idx = clamp(comp_idx, 0, 2)
  unsafe
    val = float((*reinterpret<int3?> data)[comp_idx])

[plot_set(type_name="IPoint3")]
def set_IPoint3(data : void?; comp_idx, row_idx : int; new_val : float) : void
  if comp_idx < 0 || comp_idx >= 3
    return
  unsafe
    (*reinterpret<int3?> data)[comp_idx] = int(new_val)

[plot_sample(type_name="IPoint4")]
def sample_IPoint4(data : void?;
                   var dim : int&; var comp_idx : int&;
                   var rows : int&; var row_idx : int&;
                   var val : float&) : void
  dim = 4
  comp_idx = clamp(comp_idx, 0, 3)
  unsafe
    val = float((*reinterpret<int4?> data)[comp_idx])

[plot_set(type_name="IPoint4")]
def set_IPoint4(data : void?; comp_idx, row_idx : int; new_val : float) : void
  if comp_idx < 0 || comp_idx >= 4
    return
  unsafe
    (*reinterpret<int4?> data)[comp_idx] = int(new_val)


// DPoint2 is not a das-registered handle; use raw double access instead of `.x/.y`.
[plot_sample(type_name="DPoint2")]
def sample_DPoint2(data : void?;
                   var dim : int&; var comp_idx : int&;
                   var rows : int&; var row_idx : int&;
                   var val : float&) : void
  dim = 2
  comp_idx = clamp(comp_idx, 0, 1)
  unsafe
    let dp = reinterpret<double const?> data
    val = float(*(dp + comp_idx))

[plot_set(type_name="DPoint2")]
def set_DPoint2(data : void?; comp_idx, row_idx : int; new_val : float) : void
  if comp_idx < 0 || comp_idx >= 2
    return
  unsafe
    let dp = reinterpret<double const?>(data)
    *reinterpret<double? -const>(dp + comp_idx) = double(new_val)

[plot_sample(type_name="DPoint3")]
def sample_DPoint3(data : void?;
                   var dim : int&; var comp_idx : int&;
                   var rows : int&; var row_idx : int&;
                   var val : float&) : void
  dim = 3
  comp_idx = clamp(comp_idx, 0, 2)
  unsafe
    let p & = *reinterpret<DPoint3?> data
    val = float(comp_idx == 0 ? p.x : (comp_idx == 1 ? p.y : p.z))

[plot_set(type_name="DPoint3")]
def set_DPoint3(data : void?; comp_idx, row_idx : int; new_val : float) : void
  unsafe
    var p & = *reinterpret<DPoint3?> data
    if comp_idx == 0
      p.x = double(new_val)
    elif comp_idx == 1
      p.y = double(new_val)
    elif comp_idx == 2
      p.z = double(new_val)


[plot_sample(type_name="vec4f")]
def sample_vec4f(data : void?;
                 var dim : int&; var comp_idx : int&;
                 var rows : int&; var row_idx : int&;
                 var val : float&) : void
  dim = 4
  comp_idx = clamp(comp_idx, 0, 3)
  unsafe
    val = (*reinterpret<float4?> data)[comp_idx]

[plot_set(type_name="vec4f")]
def set_vec4f(data : void?; comp_idx, row_idx : int; new_val : float) : void
  if comp_idx < 0 || comp_idx >= 4
    return
  unsafe
    (*reinterpret<float4?> data)[comp_idx] = new_val

// E3DCOLOR is BGRA in memory; comp_idx 0..3 is remapped to R, G, B, A for the plot's axis buttons.
[plot_sample(type_name="E3DCOLOR")]
def sample_E3DCOLOR(data : void?;
                    var dim : int&; var comp_idx : int&;
                    var rows : int&; var row_idx : int&;
                    var val : float&) : void
  dim = 4
  comp_idx = clamp(comp_idx, 0, 3)
  let byte_off = comp_idx == 0 ? 2 : (comp_idx == 1 ? 1 : (comp_idx == 2 ? 0 : 3))
  unsafe
    let bp = reinterpret<uint8 const?> data
    val = float(*(bp + byte_off))

[plot_set(type_name="E3DCOLOR")]
def set_E3DCOLOR(data : void?; comp_idx, row_idx : int; new_val : float) : void
  if comp_idx < 0 || comp_idx >= 4
    return
  let byte_off = comp_idx == 0 ? 2 : (comp_idx == 1 ? 1 : (comp_idx == 2 ? 0 : 3))
  unsafe
    let bp = reinterpret<uint8 const?>(data)
    *reinterpret<uint8? -const>(bp + byte_off) = uint8(clamp(int(new_val), 0, 255))


// float3x4 / TMatrix: 4 Point3 columns; UI presents each column as a "row" in the row selector.
[plot_sample(type_name="TMatrix")]
def sample_TMatrix(data : void?;
                   var dim : int&; var comp_idx : int&;
                   var rows : int&; var row_idx : int&;
                   var val : float&) : void
  rows = 4
  dim = 3
  row_idx = clamp(row_idx, 0, 3)
  comp_idx = clamp(comp_idx, 0, 2)
  unsafe
    let m & = *reinterpret<float3x4?> data
    val = m[row_idx][comp_idx]

[plot_set(type_name="TMatrix")]
def set_TMatrix(data : void?; comp_idx, row_idx : int; new_val : float) : void
  if row_idx < 0 || row_idx >= 4 || comp_idx < 0 || comp_idx >= 3
    return
  unsafe
    var m & = *reinterpret<float3x4?> data
    m[row_idx][comp_idx] = new_val

// bbox3f: bmin/bmax are vec4f in storage but conceptually 3D; w is SIMD padding, so dim=3.
[plot_sample(type_name="bbox3f")]
def sample_bbox3f(data : void?;
                  var dim : int&; var comp_idx : int&;
                  var rows : int&; var row_idx : int&;
                  var val : float&) : void
  rows = 2
  dim = 3
  row_idx = clamp(row_idx, 0, 1)
  comp_idx = clamp(comp_idx, 0, 2)
  unsafe
    let b & = *reinterpret<bbox3f?> data
    let v = row_idx == 0 ? b.bmin : b.bmax
    val = v[comp_idx]

[plot_set(type_name="bbox3f")]
def set_bbox3f(data : void?; comp_idx, row_idx : int; new_val : float) : void
  if row_idx < 0 || row_idx >= 2 || comp_idx < 0 || comp_idx >= 3
    return
  unsafe
    var b & = *reinterpret<bbox3f?> data
    if row_idx == 0
      b.bmin[comp_idx] = new_val
    else
      b.bmax[comp_idx] = new_val

// BBox3 is Point3 lim[2] laid out as 6 contiguous floats (no SIMD padding).
[plot_sample(type_name="BBox3")]
def sample_BBox3(data : void?;
                 var dim : int&; var comp_idx : int&;
                 var rows : int&; var row_idx : int&;
                 var val : float&) : void
  rows = 2
  dim = 3
  row_idx = clamp(row_idx, 0, 1)
  comp_idx = clamp(comp_idx, 0, 2)
  unsafe
    let fp = reinterpret<float const?>(data)
    val = *(fp + row_idx * 3 + comp_idx)

[plot_set(type_name="BBox3")]
def set_BBox3(data : void?; comp_idx, row_idx : int; new_val : float) : void
  if row_idx < 0 || row_idx >= 2 || comp_idx < 0 || comp_idx >= 3
    return
  unsafe
    let fp = reinterpret<float const?>(data)
    *reinterpret<float? -const>(fp + row_idx * 3 + comp_idx) = new_val
