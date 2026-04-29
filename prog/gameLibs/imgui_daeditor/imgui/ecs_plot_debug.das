options no_aot
options persistent_heap

require app
require ecs
require strings
require daslib/strings_boost
require DagorImgui
require imgui/imgui_boost
require daslib/static_let
require %gameLibs.imgui_daeditor.imgui.imgui_common
require %gameLibs.imgui_daeditor.imgui.ecs_plot_common


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
      pw.plot_duration  = 10.0f
      pw.history_size   = history_size_for_duration(pw.plot_duration)
      pw.grid_x         = 10
      pw.grid_y         = 10
      pw.prev_n_entries = -1

def try_set_vec_component(eid : EntityId; name : string; hash : uint; comp_idx, ndim : int; pval; val)
  if pval == null || comp_idx < 0 || comp_idx >= ndim
    return
  var v = *pval
  v[comp_idx] = val
  set(eid, name, hash, v)

def try_sample_scalar(pval; var dim : int&; var val : float&) : bool
  if pval == null
    return false
  dim = 1
  val = float(*pval)
  return true

def try_sample_vec(pval; ndim : int; var dim : int&; var comp_idx : int&; var val : float&) : bool
  if pval == null
    return false
  dim = ndim
  comp_idx = min(comp_idx, ndim - 1)
  val = float((*pval)[comp_idx])
  return true


def plot_sample_entry(var e : PlotEntry&; eid : EntityId)
  if empty(e.name) || eid == INVALID_ENTITY_ID
    return
  let compHash = ecs_hash(e.name)
  if e.dim > 0
    // Fast path: type is known from a previous frame - single getter call
    if !e.is_int
      if e.dim == 1
        e.found = try_sample_scalar(get_float(eid, e.name, compHash), e.dim, e.committed_val)
      elif e.dim == 2
        e.found = try_sample_vec(get_Point2(eid, e.name, compHash), 2, e.dim, e.comp_idx, e.committed_val)
      elif e.dim == 3
        e.found = try_sample_vec(get_Point3(eid, e.name, compHash), 3, e.dim, e.comp_idx, e.committed_val)
      else
        e.found = try_sample_vec(get_Point4(eid, e.name, compHash), 4, e.dim, e.comp_idx, e.committed_val)
    else
      if e.dim == 1
        e.found = try_sample_scalar(get_int(eid, e.name, compHash), e.dim, e.committed_val)
      elif e.dim == 2
        e.found = try_sample_vec(get_IPoint2(eid, e.name, compHash), 2, e.dim, e.comp_idx, e.committed_val)
      elif e.dim == 3
        e.found = try_sample_vec(get_IPoint3(eid, e.name, compHash), 3, e.dim, e.comp_idx, e.committed_val)
      else
        e.found = try_sample_vec(get_IPoint4(eid, e.name, compHash), 4, e.dim, e.comp_idx, e.committed_val)
    if !e.found
      e.dim = 0  // component disappeared; re-discover next frame
      e.last_discovery_fail_time = 0.0f  // force an immediate retry (type may have changed)
  elif get_sync_time() - e.last_discovery_fail_time >= 1.0f
    // Discovery cascade (first frame, after name/eid change, or periodic retry every 1s).
    e.is_int = false
    e.found = try_sample_scalar(get_float(eid, e.name, compHash), e.dim, e.committed_val)
    if !e.found
      e.found = try_sample_vec(get_Point2(eid, e.name, compHash), 2, e.dim, e.comp_idx, e.committed_val)
    if !e.found
      e.found = try_sample_vec(get_Point3(eid, e.name, compHash), 3, e.dim, e.comp_idx, e.committed_val)
    if !e.found
      e.found = try_sample_vec(get_Point4(eid, e.name, compHash), 4, e.dim, e.comp_idx, e.committed_val)
    if !e.found && try_sample_scalar(get_int(eid, e.name, compHash), e.dim, e.committed_val)
      e.found = true; e.is_int = true
    if !e.found && try_sample_vec(get_IPoint2(eid, e.name, compHash), 2, e.dim, e.comp_idx, e.committed_val)
      e.found = true; e.is_int = true
    if !e.found && try_sample_vec(get_IPoint3(eid, e.name, compHash), 3, e.dim, e.comp_idx, e.committed_val)
      e.found = true; e.is_int = true
    if !e.found && try_sample_vec(get_IPoint4(eid, e.name, compHash), 4, e.dim, e.comp_idx, e.committed_val)
      e.found = true; e.is_int = true
    e.last_discovery_fail_time = e.found ? 0.0f : get_sync_time()


def plot_set_component(eid : EntityId; name : string; comp_idx, dim : int; is_int : bool; new_val : float)
  if dim < 1 || dim > 4
    return
  let hash = ecs_hash(name)
  if is_int
    if dim == 1
      set(eid, name, hash, int(new_val))
    elif dim == 2
      try_set_vec_component(eid, name, hash, comp_idx, 2, get_IPoint2(eid, name, hash), int(new_val))
    elif dim == 3
      try_set_vec_component(eid, name, hash, comp_idx, 3, get_IPoint3(eid, name, hash), int(new_val))
    else
      try_set_vec_component(eid, name, hash, comp_idx, 4, get_IPoint4(eid, name, hash), int(new_val))
  else
    if dim == 1
      set(eid, name, hash, new_val)
    elif dim == 2
      try_set_vec_component(eid, name, hash, comp_idx, 2, get_Point2(eid, name, hash), new_val)
    elif dim == 3
      try_set_vec_component(eid, name, hash, comp_idx, 3, get_Point3(eid, name, hash), new_val)
    else
      try_set_vec_component(eid, name, hash, comp_idx, 4, get_Point4(eid, name, hash), new_val)


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
  imgui_set_blk_val(blk, "count", length(w.entries))
  for i in range(length(w.entries))
    let e & = unsafe(w.entries[i])
    imgui_set_blk_val(blk, "eid_{i}",       int(e.eid))
    imgui_set_blk_val(blk, "name_{i}",      e.name)
    imgui_set_blk_val(blk, "comp_idx_{i}",  e.comp_idx)
    imgui_set_blk_val(blk, "scale_min_{i}", e.scale_min)
    imgui_set_blk_val(blk, "scale_max_{i}", e.scale_max)
    imgui_set_blk_val(blk, "fit_{i}",       e.fit)
    imgui_set_blk_val(blk, "hidden_{i}",    e.hidden)
    if empty(e.name)
      imgui_set_blk_val(blk, "val_{i}",     e.committed_val)

def plot_load_entries(var w : PlotWindow&; win_idx : int)
  let blk = "ecs_plot_w{win_idx}"
  w.grid_x      = imgui_get_blk_val(blk, "grid_x",      w.grid_x)
  w.grid_y      = imgui_get_blk_val(blk, "grid_y",      w.grid_y)
  w.y_scale_idx = imgui_get_blk_val(blk, "y_scale_idx", w.y_scale_idx)
  let count = imgui_get_blk_val(blk, "count", 0)
  for i in range(count)
    if length(w.entries) >= PLOT_MAX_ENTRIES
      break
    w.entries |> resize(length(w.entries) + 1)
    var e & = unsafe(w.entries[length(w.entries) - 1])
    e.eid       = uint(imgui_get_blk_val(blk, "eid_{i}",  0))
    e.name      = imgui_get_blk_val(blk, "name_{i}",      "")
    e.comp_idx  = imgui_get_blk_val(blk, "comp_idx_{i}",  0)
    e.scale_min = imgui_get_blk_val(blk, "scale_min_{i}", -1.0f)
    e.scale_max = imgui_get_blk_val(blk, "scale_max_{i}",  1.0f)
    e.fit       = imgui_get_blk_val(blk, "fit_{i}",       true)
    e.hidden    = imgui_get_blk_val(blk, "hidden_{i}",    false)
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
          re.name = req_name
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
      if e.dim == 1
        sb |> write(",{e.eid}/{e.name}")
      else
        sb |> write(",{e.eid}/{e.name}/{axis_char}")
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
  imgui::InputFloat("##plotdur", unsafe(addr(w.plot_duration)), 0.0f)
  imgui::PopItemWidth()
  w.plot_duration = max(w.plot_duration, 0.0f)
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
    if e.fit && e.count > 0
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


def plot_draw_grid(w : PlotWindow&; plot_pos, plot_size : float2; x_left, x_range : float)
  let n = length(w.entries)
  let y_min = n > 0 ? w.entries[w.y_scale_idx].scale_min : 0.0f
  let y_max = n > 0 ? w.entries[w.y_scale_idx].scale_max : 1.0f
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
  for i in range(n)
    let e & = unsafe(w.entries[i])
    if e.count >= 1 && !e.hidden
      let cnt = e.count
      let offset = ring_oldest_idx(cnt, w.history_size, e.write_idx)
      let entry_color = get_plot_color(i)
      let smin = e.scale_min
      let smax = e.scale_max
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


def plot_draw_tooltip(w : PlotWindow&; plot_pos, plot_size : float2; x_left, x_range : float)
  let n = length(w.entries)
  if n == 0 || x_range <= 0.f
    return
  let mouse_pos = imgui::GetMousePos()
  let rel_x = (mouse_pos.x - plot_pos.x) / plot_size.x
  let rel_y = (mouse_pos.y - plot_pos.y) / plot_size.y
  if rel_x < 0.f || rel_x > 1.f || rel_y < 0.f || rel_y > 1.f
    return
  if imgui::BeginTooltip()
    let line1 = build_string() <| $(sb)
      sb |> write("t: {fmt(":.4f", x_left + rel_x * x_range)}")
      for i in range(min(n, 4))
        let e & = unsafe(w.entries[i])
        let v = e.scale_max + rel_y * (e.scale_min - e.scale_max)
        sb |> write(" | v{i}: {fmt(":.4f", v)}")
    imgui::TextUnformatted(line1)
    if n > 4
      let line2 = build_string() <| $(sb)
        for i in range(4, n)
          let e & = unsafe(w.entries[i])
          let v = e.scale_max + rel_y * (e.scale_min - e.scale_max)
          if i > 4
            sb |> write(" | ")
          sb |> write("v{i}: {fmt(":.4f", v)}")
      imgui::TextUnformatted(line2)
    imgui::EndTooltip()


def plot_toolbar(var w : PlotWindow&; daeditor_eid : EntityId)
  if imgui::Button("Add##plotadd") && length(w.entries) < PLOT_MAX_ENTRIES
    let n = length(w.entries)
    w.entries |> resize(n + 1)
    var last & = unsafe(w.entries[n])
    last.eid = w.toolbar_eid
    last.scale_min = -1.0f
    last.scale_max = 1.0f
    last.fit = true
    entry_resize(last, w.history_size)
    w.dirty = true
  imgui::SameLine()
  imgui::Text("eid:")
  imgui::SameLine()
  imgui::PushItemWidth(80.f)
  imgui::InputScalar("##toolbar_eid", ImGuiDataType.U32, unsafe(addr(w.toolbar_eid)), null, null, "%u", ImGuiInputTextFlags.None)
  imgui::PopItemWidth()
  imgui::SameLine()
  if imgui::Button("Use selected##toolbar_use_sel") && daeditor_eid != INVALID_ENTITY_ID
    w.toolbar_eid = uint(daeditor_eid)
  imgui::SameLine()
  if imgui::Button("Delete all##plotdeleteall") && !empty(w.entries)
    w.entries |> clear()
    w.dirty = true
  imgui::SameLine()
  if imgui::Button("Copy CSV##copycsv") && !empty(w.entries)
    imgui::SetClipboardText(plot_build_csv(w))


def plot_entry_rows(var w : PlotWindow&; var to_delete : int&)
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

    // EID field
    let entry_eid_ui = EntityId(e.eid)
    let eid_exists = doesEntityExist(entry_eid_ui)
    let eid_invalid = entry_eid_ui != INVALID_ENTITY_ID && !eid_exists
    imgui::Text("eid:")
    imgui::SameLine()
    imgui::PushItemWidth(90.f)
    if eid_invalid
      imgui::PushStyleColor(ImGuiCol.FrameBg, IM_COL32(50, 50, 160, 160))
    if imgui::InputScalar("##eid", ImGuiDataType.U32, unsafe(addr(e.eid)), null, null, "%u", ImGuiInputTextFlags.None)
      w.dirty = true
      e.last_discovery_fail_time = 0.0f
    if eid_invalid
      imgui::PopStyleColor()
    imgui::PopItemWidth()
    imgui::SameLine()

    // Component name
    let name_not_found = !empty(e.name) && entry_eid_ui != INVALID_ENTITY_ID && !e.found
    imgui::Text("name:")
    imgui::SameLine()
    imgui::PushItemWidth(200.f)
    if name_not_found
      imgui::PushStyleColor(ImGuiCol.FrameBg, IM_COL32(50, 50, 160, 160))
    let buf <- ImGuiInputTextBuffer(e.name, 256)
    if InputText("##name", buf, ImGuiInputTextFlags.None)
      e.name = to_string(buf)
      e.dim = 0
      e.is_int = false
      e.last_discovery_fail_time = 0.0f
      w.dirty = true
    if name_not_found
      imgui::PopStyleColor()
    imgui::PopItemWidth()
    if imgui::IsItemHovered(ImGuiHoveredFlags.None) && !empty(e.name)
      if imgui::BeginTooltip()
        imgui::TextUnformatted(e.name)
        imgui::EndTooltip()

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
      if empty(e.name)
        w.dirty = true
      let set_eid = EntityId(e.eid)
      if set_eid != INVALID_ENTITY_ID
        plot_set_component(set_eid, e.name, e.comp_idx, e.dim, e.is_int, new_val)

    // Y-scale: auto-fit toggle and manual min/max bounds
    imgui::SameLine()
    imgui::Text("fit:")
    imgui::SameLine()
    if imgui::Checkbox("##fit", unsafe(addr(e.fit)))
      w.dirty = true
    imgui::SameLine()
    imgui::Text("min:")
    imgui::SameLine()
    imgui::PushItemWidth(70.f)
    if imgui::InputFloat("##smin", unsafe(addr(e.scale_min)))
      w.dirty = true
    imgui::PopItemWidth()
    imgui::SameLine()
    imgui::Text("max:")
    imgui::SameLine()
    imgui::PushItemWidth(70.f)
    if imgui::InputFloat("##smax", unsafe(addr(e.scale_max)))
      w.dirty = true
    imgui::PopItemWidth()

    imgui::PopID()


def aux_ecs_component_plot(win_idx : int)
  static_let <|
    var g_plot_windows : array<PlotWindow>

  plot_process_requests(g_plot_windows)

  plot_ensure(g_plot_windows, win_idx)
  var w & = unsafe(g_plot_windows[win_idx])

  // Set default window size and load persisted entries on first open
  if !w.win_size_init
    imgui::SetWindowSize(float2(960.f, 400.f), ImGuiCond FirstUseEver)
    w.win_size_init = true
    plot_load_entries(w, win_idx)

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
