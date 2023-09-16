import sys
import typing
depsgraph_update_post = None
''' on depsgraph update (post)
'''

depsgraph_update_pre = None
''' on depsgraph update (pre)
'''

frame_change_post = None
''' Called after frame change for playback and rendering, after the data has been evaluated for the new frame.
'''

frame_change_pre = None
''' Called after frame change for playback and rendering, before any data is evaluated for the new frame. This makes it possible to change data and relations (for example swap an object to another mesh) for the new frame. Note that this handler is **not** to be used as 'before the frame changes' event. The dependency graph is not available in this handler, as data and relations may have been altered and the dependency graph has not yet been updated for that.
'''

load_factory_preferences_post = None
''' on loading factory preferences (after)
'''

load_factory_startup_post = None
''' on loading factory startup (after)
'''

load_post = None
''' on loading a new blend file (after)
'''

load_pre = None
''' on loading a new blend file (before)
'''

persistent = None
''' Function decorator for callback functions not to be removed when loading new files
'''

redo_post = None
''' on loading a redo step (after)
'''

redo_pre = None
''' on loading a redo step (before)
'''

render_cancel = None
''' on canceling a render job
'''

render_complete = None
''' on completion of render job
'''

render_init = None
''' on initialization of a render job
'''

render_post = None
''' on render (after)
'''

render_pre = None
''' on render (before)
'''

render_stats = None
''' on printing render statistics
'''

render_write = None
''' on writing a render frame (directly after the frame is written)
'''

save_post = None
''' on saving a blend file (after)
'''

save_pre = None
''' on saving a blend file (before)
'''

undo_post = None
''' on loading an undo step (after)
'''

undo_pre = None
''' on loading an undo step (before)
'''

version_update = None
''' on ending the versioning code
'''
