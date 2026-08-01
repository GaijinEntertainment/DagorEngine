module ecs_plot_common shared

let PLOT_HISTORY_MIN = 16
let PLOT_HISTORY_MAX = 90000  // 5 minutes at 300 FPS and 1x timeSpeed
let PLOT_MAX_ASSUMED_FPS = 300
let PLOT_MIN_ASSUMED_TIMESPEED = 0.1f
let PLOT_MAX_ENTRIES = 12
let PLOT_WINDOW_COUNT = 4

struct PlotEntry
  name        : string
  comp_idx    : int
  dim         : int
  row_idx     : int
  rows        : int  // 0 or 1 means no row selector (scalars, vectors); >1 for TMatrix / bbox3f / BBox3
  eid         : uint
  slot        : uint  // signal slot index used when slot_mode is true; preserved across mode toggles
  scale_min   : float
  scale_max   : float
  fit         : bool
  hidden      : bool
  type_name   : string  // empty until first successful resolve_component_path
  edit_val      : float
  committed_val : float
  values      : array<float>
  write_idx   : int
  count       : int
  found       : bool
  val_editing : bool
  slot_mode   : bool  // true: eid field is a signal slot index sourced from daeditor_plot_debug__signals
  last_discovery_fail_time : float  // 0 = never tried; positive = get_sync_time() of last failed attempt
  max_dq      : array<int>
  min_dq      : array<int>
  max_head    : int
  max_tail    : int
  min_head    : int
  min_tail    : int

struct PlotWindow
  entries          : array<PlotEntry>
  history_size     : int
  paused           : bool
  grid_x           : int
  grid_y           : int
  y_scale_idx      : int
  ts_write_idx     : int
  ts_count         : int
  plot_duration    : float
  edit_plot_duration : float  // toolbar-input buffer; committed to plot_duration on Enter/defocus
  prev_n_entries   : int
  win_size_init    : bool
  timestamps       : array<float>
  max_entry_count  : int
  toolbar_eid       : uint
  toolbar_slot      : uint
  toolbar_slot_mode : bool  // false: Add creates an ECS entry; true: Add creates a debug-signal slot entry
  shared_y          : bool  // "Shared Y scale" toolbar toggle: when true, the plot's Y axis is driven by w.shared_fit / w.shared_scale_min/max instead of per-entry state
  shared_fit        : bool
  shared_scale_min  : float
  shared_scale_max  : float
  fit_widget_x      : float // X of per-entry "fit:" text captured last frame; used to align toolbar's shared controls
  dirty             : bool
