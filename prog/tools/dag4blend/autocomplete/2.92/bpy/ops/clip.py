import sys
import typing
import bpy.types


def add_marker(location: typing.List[float] = (0.0, 0.0)):
    ''' Place new marker at specified location

    :param location: Location, Location of marker on frame
    :type location: typing.List[float]
    '''

    pass


def add_marker_at_click():
    ''' Place new marker at the desired (clicked) position

    '''

    pass


def add_marker_move(CLIP_OT_add_marker=None, TRANSFORM_OT_translate=None):
    ''' Add new marker and move it on movie

    :param CLIP_OT_add_marker: Add Marker, Place new marker at specified location
    :param TRANSFORM_OT_translate: Move, Move selected items
    '''

    pass


def add_marker_slide(CLIP_OT_add_marker=None, TRANSFORM_OT_translate=None):
    ''' Add new marker and slide it with mouse until mouse button release

    :param CLIP_OT_add_marker: Add Marker, Place new marker at specified location
    :param TRANSFORM_OT_translate: Move, Move selected items
    '''

    pass


def apply_solution_scale(distance: float = 0.0):
    ''' Apply scale on solution itself to make distance between selected tracks equals to desired

    :param distance: Distance, Distance between selected tracks
    :type distance: float
    '''

    pass


def bundles_to_mesh():
    ''' Create vertex cloud using coordinates of reconstructed tracks :file: startup/bl_operators/clip.py\:299 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/clip.py$299> _

    '''

    pass


def camera_preset_add(name: str = "",
                      remove_name: bool = False,
                      remove_active: bool = False,
                      use_focal_length: bool = True):
    ''' Add or remove a Tracking Camera Intrinsics Preset

    :param name: Name, Name of the preset, used to make the path name
    :type name: str
    :param remove_name: remove_name
    :type remove_name: bool
    :param remove_active: remove_active
    :type remove_active: bool
    :param use_focal_length: Include Focal Length, Include focal length into the preset
    :type use_focal_length: bool
    '''

    pass


def change_frame(frame: int = 0):
    ''' Interactively change the current frame number

    :param frame: Frame
    :type frame: int
    '''

    pass


def clean_tracks(frames: int = 0,
                 error: float = 0.0,
                 action: typing.Union[int, str] = 'SELECT'):
    ''' Clean tracks with high error values or few frames

    :param frames: Tracked Frames, Effect on tracks which are tracked less than specified amount of frames
    :type frames: int
    :param error: Reprojection Error, Effect on tracks which have got larger reprojection error
    :type error: float
    :param action: Action, Cleanup action to execute * SELECT Select, Select unclean tracks. * DELETE_TRACK Delete Track, Delete unclean tracks. * DELETE_SEGMENTS Delete Segments, Delete unclean segments of tracks.
    :type action: typing.Union[int, str]
    '''

    pass


def clear_solution():
    ''' Clear all calculated data

    '''

    pass


def clear_track_path(action: typing.Union[int, str] = 'REMAINED',
                     clear_active: bool = False):
    ''' Clear tracks after/before current position or clear the whole track

    :param action: Action, Clear action to execute * UPTO Clear Up To, Clear path up to current frame. * REMAINED Clear Remained, Clear path at remaining frames (after current). * ALL Clear All, Clear the whole path.
    :type action: typing.Union[int, str]
    :param clear_active: Clear Active, Clear active track only instead of all selected tracks
    :type clear_active: bool
    '''

    pass


def constraint_to_fcurve():
    ''' Create F-Curves for object which will copy object's movement caused by this constraint :file: startup/bl_operators/clip.py\:543 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/clip.py$543> _

    '''

    pass


def copy_tracks():
    ''' Copy selected tracks to clipboard

    '''

    pass


def create_plane_track():
    ''' Create new plane track out of selected point tracks

    '''

    pass


def cursor_set(location: typing.List[float] = (0.0, 0.0)):
    ''' Set 2D cursor location

    :param location: Location, Cursor location in normalized clip coordinates
    :type location: typing.List[float]
    '''

    pass


def delete_marker():
    ''' Delete marker for current frame from selected tracks

    '''

    pass


def delete_proxy():
    ''' Delete movie clip proxy files from the hard drive :file: startup/bl_operators/clip.py\:369 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/clip.py$369> _

    '''

    pass


def delete_track():
    ''' Delete selected tracks

    '''

    pass


def detect_features(placement: typing.Union[int, str] = 'FRAME',
                    margin: int = 16,
                    threshold: float = 0.5,
                    min_distance: int = 120):
    ''' Automatically detect features and place markers to track

    :param placement: Placement, Placement for detected features * FRAME Whole Frame, Place markers across the whole frame. * INSIDE_GPENCIL Inside Annotated Area, Place markers only inside areas outlined with the Annotation tool. * OUTSIDE_GPENCIL Outside Annotated Area, Place markers only outside areas outlined with the Annotation tool.
    :type placement: typing.Union[int, str]
    :param margin: Margin, Only features further than margin pixels from the image edges are considered
    :type margin: int
    :param threshold: Threshold, Threshold level to consider feature good enough for tracking
    :type threshold: float
    :param min_distance: Distance, Minimal distance accepted between two features
    :type min_distance: int
    '''

    pass


def disable_markers(action: typing.Union[int, str] = 'DISABLE'):
    ''' Disable/enable selected markers

    :param action: Action, Disable action to execute * DISABLE Disable, Disable selected markers. * ENABLE Enable, Enable selected markers. * TOGGLE Toggle, Toggle disabled flag for selected markers.
    :type action: typing.Union[int, str]
    '''

    pass


def dopesheet_select_channel(location: typing.List[float] = (0.0, 0.0),
                             extend: bool = False):
    ''' Select movie tracking channel

    :param location: Location, Mouse location to select channel
    :type location: typing.List[float]
    :param extend: Extend, Extend selection rather than clearing the existing selection
    :type extend: bool
    '''

    pass


def dopesheet_view_all():
    ''' Reset viewable area to show full keyframe range

    '''

    pass


def filter_tracks(track_threshold: float = 5.0):
    ''' Filter tracks which has weirdly looking spikes in motion curves

    :param track_threshold: Track Threshold, Filter Threshold to select problematic tracks
    :type track_threshold: float
    '''

    pass


def frame_jump(position: typing.Union[int, str] = 'PATHSTART'):
    ''' Jump to special frame

    :param position: Position, Position to jump to * PATHSTART Path Start, Jump to start of current path. * PATHEND Path End, Jump to end of current path. * FAILEDPREV Previous Failed, Jump to previous failed frame. * FAILNEXT Next Failed, Jump to next failed frame.
    :type position: typing.Union[int, str]
    '''

    pass


def graph_center_current_frame():
    ''' Scroll view so current frame would be centered

    '''

    pass


def graph_delete_curve():
    ''' Delete track corresponding to the selected curve

    '''

    pass


def graph_delete_knot():
    ''' Delete curve knots

    '''

    pass


def graph_disable_markers(action: typing.Union[int, str] = 'DISABLE'):
    ''' Disable/enable selected markers

    :param action: Action, Disable action to execute * DISABLE Disable, Disable selected markers. * ENABLE Enable, Enable selected markers. * TOGGLE Toggle, Toggle disabled flag for selected markers.
    :type action: typing.Union[int, str]
    '''

    pass


def graph_select(location: typing.List[float] = (0.0, 0.0),
                 extend: bool = False):
    ''' Select graph curves

    :param location: Location, Mouse location to select nearest entity
    :type location: typing.List[float]
    :param extend: Extend, Extend selection rather than clearing the existing selection
    :type extend: bool
    '''

    pass


def graph_select_all_markers(action: typing.Union[int, str] = 'TOGGLE'):
    ''' Change selection of all markers of active track

    :param action: Action, Selection action to execute * TOGGLE Toggle, Toggle selection for all elements. * SELECT Select, Select all elements. * DESELECT Deselect, Deselect all elements. * INVERT Invert, Invert selection of all elements.
    :type action: typing.Union[int, str]
    '''

    pass


def graph_select_box(xmin: int = 0,
                     xmax: int = 0,
                     ymin: int = 0,
                     ymax: int = 0,
                     wait_for_input: bool = True,
                     deselect: bool = False,
                     extend: bool = True):
    ''' Select curve points using box selection

    :param xmin: X Min
    :type xmin: int
    :param xmax: X Max
    :type xmax: int
    :param ymin: Y Min
    :type ymin: int
    :param ymax: Y Max
    :type ymax: int
    :param wait_for_input: Wait for Input
    :type wait_for_input: bool
    :param deselect: Deselect, Deselect rather than select items
    :type deselect: bool
    :param extend: Extend, Extend selection instead of deselecting everything first
    :type extend: bool
    '''

    pass


def graph_view_all():
    ''' View all curves in editor

    '''

    pass


def hide_tracks(unselected: bool = False):
    ''' Hide selected tracks

    :param unselected: Unselected, Hide unselected tracks
    :type unselected: bool
    '''

    pass


def hide_tracks_clear():
    ''' Clear hide selected tracks

    '''

    pass


def join_tracks():
    ''' Join selected tracks

    '''

    pass


def keyframe_delete():
    ''' Delete a keyframe from selected tracks at current frame

    '''

    pass


def keyframe_insert():
    ''' Insert a keyframe to selected tracks at current frame

    '''

    pass


def lock_selection_toggle():
    ''' Toggle Lock Selection option of the current clip editor

    '''

    pass


def lock_tracks(action: typing.Union[int, str] = 'LOCK'):
    ''' Lock/unlock selected tracks

    :param action: Action, Lock action to execute * LOCK Lock, Lock selected tracks. * UNLOCK Unlock, Unlock selected tracks. * TOGGLE Toggle, Toggle locked flag for selected tracks.
    :type action: typing.Union[int, str]
    '''

    pass


def mode_set(mode: typing.Union[int, str] = 'TRACKING'):
    ''' Set the clip interaction mode

    :param mode: Mode * TRACKING Tracking, Show tracking and solving tools. * MASK Mask, Show mask editing tools.
    :type mode: typing.Union[int, str]
    '''

    pass


def open(directory: str = "",
         files: typing.
         Union[typing.Dict[str, 'bpy.types.OperatorFileListElement'], typing.
               List['bpy.types.OperatorFileListElement'],
               'bpy_prop_collection'] = None,
         hide_props_region: bool = True,
         filter_blender: bool = False,
         filter_backup: bool = False,
         filter_image: bool = True,
         filter_movie: bool = True,
         filter_python: bool = False,
         filter_font: bool = False,
         filter_sound: bool = False,
         filter_text: bool = False,
         filter_archive: bool = False,
         filter_btx: bool = False,
         filter_collada: bool = False,
         filter_alembic: bool = False,
         filter_usd: bool = False,
         filter_volume: bool = False,
         filter_folder: bool = True,
         filter_blenlib: bool = False,
         filemode: int = 9,
         relative_path: bool = True,
         show_multiview: bool = False,
         use_multiview: bool = False,
         display_type: typing.Union[int, str] = 'DEFAULT',
         sort_method: typing.Union[int, str] = ''):
    ''' Load a sequence of frames or a movie file

    :param directory: Directory, Directory of the file
    :type directory: str
    :param files: Files
    :type files: typing.Union[typing.Dict[str, 'bpy.types.OperatorFileListElement'], typing.List['bpy.types.OperatorFileListElement'], 'bpy_prop_collection']
    :param hide_props_region: Hide Operator Properties, Collapse the region displaying the operator settings
    :type hide_props_region: bool
    :param filter_blender: Filter .blend files
    :type filter_blender: bool
    :param filter_backup: Filter .blend files
    :type filter_backup: bool
    :param filter_image: Filter image files
    :type filter_image: bool
    :param filter_movie: Filter movie files
    :type filter_movie: bool
    :param filter_python: Filter python files
    :type filter_python: bool
    :param filter_font: Filter font files
    :type filter_font: bool
    :param filter_sound: Filter sound files
    :type filter_sound: bool
    :param filter_text: Filter text files
    :type filter_text: bool
    :param filter_archive: Filter archive files
    :type filter_archive: bool
    :param filter_btx: Filter btx files
    :type filter_btx: bool
    :param filter_collada: Filter COLLADA files
    :type filter_collada: bool
    :param filter_alembic: Filter Alembic files
    :type filter_alembic: bool
    :param filter_usd: Filter USD files
    :type filter_usd: bool
    :param filter_volume: Filter OpenVDB volume files
    :type filter_volume: bool
    :param filter_folder: Filter folders
    :type filter_folder: bool
    :param filter_blenlib: Filter Blender IDs
    :type filter_blenlib: bool
    :param filemode: File Browser Mode, The setting for the file browser mode to load a .blend file, a library or a special file
    :type filemode: int
    :param relative_path: Relative Path, Select the file relative to the blend file
    :type relative_path: bool
    :param show_multiview: Enable Multi-View
    :type show_multiview: bool
    :param use_multiview: Use Multi-View
    :type use_multiview: bool
    :param display_type: Display Type * DEFAULT Default, Automatically determine display type for files. * LIST_VERTICAL Short List, Display files as short list. * LIST_HORIZONTAL Long List, Display files as a detailed list. * THUMBNAIL Thumbnails, Display files as thumbnails.
    :type display_type: typing.Union[int, str]
    :param sort_method: File sorting mode
    :type sort_method: typing.Union[int, str]
    '''

    pass


def paste_tracks():
    ''' Paste tracks from clipboard

    '''

    pass


def prefetch():
    ''' Prefetch frames from disk for faster playback/tracking

    '''

    pass


def rebuild_proxy():
    ''' Rebuild all selected proxies and timecode indices in the background

    '''

    pass


def refine_markers(backwards: bool = False):
    ''' Refine selected markers positions by running the tracker from track's reference to current frame

    :param backwards: Backwards, Do backwards tracking
    :type backwards: bool
    '''

    pass


def reload():
    ''' Reload clip

    '''

    pass


def select(extend: bool = False,
           deselect_all: bool = False,
           location: typing.List[float] = (0.0, 0.0)):
    ''' Select tracking markers

    :param extend: Extend, Extend selection rather than clearing the existing selection
    :type extend: bool
    :param deselect_all: Deselect On Nothing, Deselect all when nothing under the cursor
    :type deselect_all: bool
    :param location: Location, Mouse location in normalized coordinates, 0.0 to 1.0 is within the image bounds
    :type location: typing.List[float]
    '''

    pass


def select_all(action: typing.Union[int, str] = 'TOGGLE'):
    ''' Change selection of all tracking markers

    :param action: Action, Selection action to execute * TOGGLE Toggle, Toggle selection for all elements. * SELECT Select, Select all elements. * DESELECT Deselect, Deselect all elements. * INVERT Invert, Invert selection of all elements.
    :type action: typing.Union[int, str]
    '''

    pass


def select_box(xmin: int = 0,
               xmax: int = 0,
               ymin: int = 0,
               ymax: int = 0,
               wait_for_input: bool = True,
               mode: typing.Union[int, str] = 'SET'):
    ''' Select markers using box selection

    :param xmin: X Min
    :type xmin: int
    :param xmax: X Max
    :type xmax: int
    :param ymin: Y Min
    :type ymin: int
    :param ymax: Y Max
    :type ymax: int
    :param wait_for_input: Wait for Input
    :type wait_for_input: bool
    :param mode: Mode * SET Set, Set a new selection. * ADD Extend, Extend existing selection. * SUB Subtract, Subtract existing selection.
    :type mode: typing.Union[int, str]
    '''

    pass


def select_circle(x: int = 0,
                  y: int = 0,
                  radius: int = 25,
                  wait_for_input: bool = True,
                  mode: typing.Union[int, str] = 'SET'):
    ''' Select markers using circle selection

    :param x: X
    :type x: int
    :param y: Y
    :type y: int
    :param radius: Radius
    :type radius: int
    :param wait_for_input: Wait for Input
    :type wait_for_input: bool
    :param mode: Mode * SET Set, Set a new selection. * ADD Extend, Extend existing selection. * SUB Subtract, Subtract existing selection.
    :type mode: typing.Union[int, str]
    '''

    pass


def select_grouped(group: typing.Union[int, str] = 'ESTIMATED'):
    ''' Select all tracks from specified group

    :param group: Action, Clear action to execute * KEYFRAMED Keyframed Tracks, Select all keyframed tracks. * ESTIMATED Estimated Tracks, Select all estimated tracks. * TRACKED Tracked Tracks, Select all tracked tracks. * LOCKED Locked Tracks, Select all locked tracks. * DISABLED Disabled Tracks, Select all disabled tracks. * COLOR Tracks with Same Color, Select all tracks with same color as active track. * FAILED Failed Tracks, Select all tracks which failed to be reconstructed.
    :type group: typing.Union[int, str]
    '''

    pass


def select_lasso(path: typing.Union[
        typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.
        List['bpy.types.OperatorMousePath'], 'bpy_prop_collection'] = None,
                 mode: typing.Union[int, str] = 'SET'):
    ''' Select markers using lasso selection

    :param path: Path
    :type path: typing.Union[typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.List['bpy.types.OperatorMousePath'], 'bpy_prop_collection']
    :param mode: Mode * SET Set, Set a new selection. * ADD Extend, Extend existing selection. * SUB Subtract, Subtract existing selection.
    :type mode: typing.Union[int, str]
    '''

    pass


def set_active_clip():
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __. :file: startup/bl_operators/clip.py\:228 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/clip.py$228> _

    '''

    pass


def set_axis(axis: typing.Union[int, str] = 'X'):
    ''' Set direction of scene axis rotating camera (or its parent if present) and assume selected track lies on real axis, joining it with the origin

    :param axis: Axis, Axis to use to align bundle along * X X, Align bundle align X axis. * Y Y, Align bundle align Y axis.
    :type axis: typing.Union[int, str]
    '''

    pass


def set_center_principal():
    ''' Set optical center to center of footage

    '''

    pass


def set_origin(use_median: bool = False):
    ''' Set active marker as origin by moving camera (or its parent if present) in 3D space

    :param use_median: Use Median, Set origin to median point of selected bundles
    :type use_median: bool
    '''

    pass


def set_plane(plane: typing.Union[int, str] = 'FLOOR'):
    ''' Set plane based on 3 selected bundles by moving camera (or its parent if present) in 3D space

    :param plane: Plane, Plane to be used for orientation * FLOOR Floor, Set floor plane. * WALL Wall, Set wall plane.
    :type plane: typing.Union[int, str]
    '''

    pass


def set_scale(distance: float = 0.0):
    ''' Set scale of scene by scaling camera (or its parent if present)

    :param distance: Distance, Distance between selected tracks
    :type distance: float
    '''

    pass


def set_scene_frames():
    ''' Set scene's start and end frame to match clip's start frame and length

    '''

    pass


def set_solution_scale(distance: float = 0.0):
    ''' Set object solution scale using distance between two selected tracks

    :param distance: Distance, Distance between selected tracks
    :type distance: float
    '''

    pass


def set_solver_keyframe(keyframe: typing.Union[int, str] = 'KEYFRAME_A'):
    ''' Set keyframe used by solver

    :param keyframe: Keyframe, Keyframe to set
    :type keyframe: typing.Union[int, str]
    '''

    pass


def set_viewport_background():
    ''' Set current movie clip as a camera background in 3D Viewport (works only when a 3D Viewport is visible) :file: startup/bl_operators/clip.py\:433 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/clip.py$433> _

    '''

    pass


def setup_tracking_scene():
    ''' Prepare scene for compositing 3D objects into this footage :file: startup/bl_operators/clip.py\:997 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/clip.py$997> _

    '''

    pass


def slide_marker(offset: typing.List[float] = (0.0, 0.0)):
    ''' Slide marker areas

    :param offset: Offset, Offset in floating-point units, 1.0 is the width and height of the image
    :type offset: typing.List[float]
    '''

    pass


def slide_plane_marker():
    ''' Slide plane marker areas

    '''

    pass


def solve_camera():
    ''' Solve camera motion from tracks

    '''

    pass


def stabilize_2d_add():
    ''' Add selected tracks to 2D translation stabilization

    '''

    pass


def stabilize_2d_remove():
    ''' Remove selected track from translation stabilization

    '''

    pass


def stabilize_2d_rotation_add():
    ''' Add selected tracks to 2D rotation stabilization

    '''

    pass


def stabilize_2d_rotation_remove():
    ''' Remove selected track from rotation stabilization

    '''

    pass


def stabilize_2d_rotation_select():
    ''' Select tracks which are used for rotation stabilization

    '''

    pass


def stabilize_2d_select():
    ''' Select tracks which are used for translation stabilization

    '''

    pass


def track_color_preset_add(name: str = "",
                           remove_name: bool = False,
                           remove_active: bool = False):
    ''' Add or remove a Clip Track Color Preset

    :param name: Name, Name of the preset, used to make the path name
    :type name: str
    :param remove_name: remove_name
    :type remove_name: bool
    :param remove_active: remove_active
    :type remove_active: bool
    '''

    pass


def track_copy_color():
    ''' Copy color to all selected tracks

    '''

    pass


def track_markers(backwards: bool = False, sequence: bool = False):
    ''' Track selected markers

    :param backwards: Backwards, Do backwards tracking
    :type backwards: bool
    :param sequence: Track Sequence, Track marker during image sequence rather than single image
    :type sequence: bool
    '''

    pass


def track_settings_as_default():
    ''' Copy tracking settings from active track to default settings :file: startup/bl_operators/clip.py\:1028 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/clip.py$1028> _

    '''

    pass


def track_settings_to_track():
    ''' Copy tracking settings from active track to selected tracks :file: startup/bl_operators/clip.py\:1076 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/clip.py$1076> _

    '''

    pass


def track_to_empty():
    ''' Create an Empty object which will be copying movement of active track :file: startup/bl_operators/clip.py\:275 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/clip.py$275> _

    '''

    pass


def tracking_object_new():
    ''' Add new object for tracking

    '''

    pass


def tracking_object_remove():
    ''' Remove object for tracking

    '''

    pass


def tracking_settings_preset_add(name: str = "",
                                 remove_name: bool = False,
                                 remove_active: bool = False):
    ''' Add or remove a motion tracking settings preset

    :param name: Name, Name of the preset, used to make the path name
    :type name: str
    :param remove_name: remove_name
    :type remove_name: bool
    :param remove_active: remove_active
    :type remove_active: bool
    '''

    pass


def view_all(fit_view: bool = False):
    ''' View whole image with markers

    :param fit_view: Fit View, Fit frame to the viewport
    :type fit_view: bool
    '''

    pass


def view_center_cursor():
    ''' Center the view so that the cursor is in the middle of the view

    '''

    pass


def view_ndof():
    ''' Use a 3D mouse device to pan/zoom the view

    '''

    pass


def view_pan(offset: typing.List[float] = (0.0, 0.0)):
    ''' Pan the view

    :param offset: Offset, Offset in floating-point units, 1.0 is the width and height of the image
    :type offset: typing.List[float]
    '''

    pass


def view_selected():
    ''' View all selected elements

    '''

    pass


def view_zoom(factor: float = 0.0, use_cursor_init: bool = True):
    ''' Zoom in/out the view

    :param factor: Factor, Zoom factor, values higher than 1.0 zoom in, lower values zoom out
    :type factor: float
    :param use_cursor_init: Use Mouse Position, Allow the initial mouse position to be used
    :type use_cursor_init: bool
    '''

    pass


def view_zoom_in(location: typing.List[float] = (0.0, 0.0)):
    ''' Zoom in the view

    :param location: Location, Cursor location in screen coordinates
    :type location: typing.List[float]
    '''

    pass


def view_zoom_out(location: typing.List[float] = (0.0, 0.0)):
    ''' Zoom out the view

    :param location: Location, Cursor location in normalized (0.0 to 1.0) coordinates
    :type location: typing.List[float]
    '''

    pass


def view_zoom_ratio(ratio: float = 0.0):
    ''' Set the zoom ratio (based on clip size)

    :param ratio: Ratio, Zoom ratio, 1.0 is 1:1, higher is zoomed in, lower is zoomed out
    :type ratio: float
    '''

    pass
