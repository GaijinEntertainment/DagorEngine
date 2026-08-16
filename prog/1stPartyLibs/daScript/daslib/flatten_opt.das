options gen2
options indenting = 4
options strict_smart_pointers = true
options _comment_hygiene = true

module flatten_opt shared private

//! Internal optimization passes for daslib/flatten, split by theme. This aggregator
//! re-exports the common layer and each themed pass module so daslib/flatten (and
//! external backends/tests) require a single daslib/flatten_opt.

require daslib/flatten_opt_common public
require daslib/flatten_opt_straightline public
require daslib/flatten_opt_fold public
require daslib/flatten_opt_swizzle public
require daslib/flatten_opt_preshade public
require daslib/flatten_opt_fuse public
require daslib/flatten_opt_pack public
