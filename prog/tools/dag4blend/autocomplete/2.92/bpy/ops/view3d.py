import sys
import typing
import bpy.types


def background_image_add(name: str = "Image",
                         filepath: str = "",
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
    ''' Add a new background image

    :param name: Name, Image name to assign
    :type name: str
    :param filepath: File Path, Path to file
    :type filepath: str
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


def background_image_remove(index: int = 0):
    ''' Remove a background image from the 3D view

    :param index: Index, Background image index to remove
    :type index: int
    '''

    pass


def camera_to_view():
    ''' Set camera view to active view

    '''

    pass


def camera_to_view_selected():
    ''' Move the camera so selected objects are framed

    '''

    pass


def clear_render_border():
    ''' Clear the boundaries of the border render and disable border render

    '''

    pass


def clip_border(xmin: int = 0,
                xmax: int = 0,
                ymin: int = 0,
                ymax: int = 0,
                wait_for_input: bool = True):
    ''' Set the view clipping region

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
    '''

    pass


def copybuffer():
    ''' Selected objects are copied to the clipboard

    '''

    pass


def cursor3d(use_depth: bool = True,
             orientation: typing.Union[int, str] = 'VIEW'):
    ''' Set the location of the 3D cursor

    :param use_depth: Surface Project, Project onto the surface
    :type use_depth: bool
    :param orientation: Orientation, Preset viewpoint to use * NONE None, Leave orientation unchanged. * VIEW View, Orient to the viewport. * XFORM Transform, Orient to the current transform setting. * GEOM Geometry, Match the surface normal.
    :type orientation: typing.Union[int, str]
    '''

    pass


def dolly(mx: int = 0,
          my: int = 0,
          delta: int = 0,
          use_cursor_init: bool = True):
    ''' Dolly in/out in the view

    :param mx: Region Position X
    :type mx: int
    :param my: Region Position Y
    :type my: int
    :param delta: Delta
    :type delta: int
    :param use_cursor_init: Use Mouse Position, Allow the initial mouse position to be used
    :type use_cursor_init: bool
    '''

    pass


def edit_mesh_extrude_individual_move():
    ''' Extrude each individual face separately along local normals :file: startup/bl_operators/view3d.py\:39 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/view3d.py$39> _

    '''

    pass


def edit_mesh_extrude_manifold_normal():
    ''' Extrude manifold region along normals :file: startup/bl_operators/view3d.py\:172 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/view3d.py$172> _

    '''

    pass


def edit_mesh_extrude_move_normal(dissolve_and_intersect: bool = False):
    ''' Extrude region together along the average normal

    :param dissolve_and_intersect: dissolve_and_intersect, Dissolves adjacent faces and intersects new geometry
    :type dissolve_and_intersect: bool
    '''

    pass


def edit_mesh_extrude_move_shrink_fatten():
    ''' Extrude region together along local normals :file: startup/bl_operators/view3d.py\:155 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/view3d.py$155> _

    '''

    pass


def fly():
    ''' Interactively fly around the scene

    '''

    pass


def interactive_add(primitive_type: typing.Union[int, str] = 'CUBE',
                    plane_axis: typing.Union[int, str] = 'Z',
                    plane_axis_auto: bool = False,
                    plane_depth: typing.Union[int, str] = 'SURFACE',
                    plane_orientation: typing.Union[int, str] = 'SURFACE',
                    snap_target: typing.Union[int, str] = 'GEOMETRY',
                    plane_origin_base: typing.Union[int, str] = 'EDGE',
                    plane_origin_depth: typing.Union[int, str] = 'EDGE',
                    plane_aspect_base: typing.Union[int, str] = 'FREE',
                    plane_aspect_depth: typing.Union[int, str] = 'FREE',
                    wait_for_input: bool = True):
    ''' Interactively add an object

    :param primitive_type: Primitive
    :type primitive_type: typing.Union[int, str]
    :param plane_axis: Plane Axis, The axis used for placing the base region
    :type plane_axis: typing.Union[int, str]
    :param plane_axis_auto: Auto Axis, Select the closest axis when placing objects (surface overrides)
    :type plane_axis_auto: bool
    :param plane_depth: Position, The initial depth used when placing the cursor * SURFACE Surface, Start placing on the surface, using the 3D cursor position as a fallback. * CURSOR_PLANE Cursor Plane, Start placement using a point projected onto the orientation axis at the 3D cursor position. * CURSOR_VIEW Cursor View, Start placement using a point projected onto the view plane at the 3D cursor position.
    :type plane_depth: typing.Union[int, str]
    :param plane_orientation: Orientation, The initial depth used when placing the cursor * SURFACE Surface, Use the surface normal (using the transform orientation as a fallback). * DEFAULT Default, Use the current transform orientation.
    :type plane_orientation: typing.Union[int, str]
    :param snap_target: Snap to, The target to use while snapping * GEOMETRY Geometry, Snap to all geometry. * DEFAULT Default, Use the current snap settings.
    :type snap_target: typing.Union[int, str]
    :param plane_origin_base: Origin, The initial position for placement * EDGE Edge, Start placing the edge position. * CENTER Center, Start placing the center position.
    :type plane_origin_base: typing.Union[int, str]
    :param plane_origin_depth: Origin, The initial position for placement * EDGE Edge, Start placing the edge position. * CENTER Center, Start placing the center position.
    :type plane_origin_depth: typing.Union[int, str]
    :param plane_aspect_base: Aspect, The initial aspect setting * FREE Free, Use an unconstrained aspect. * FIXED Fixed, Use a fixed 1:1 aspect.
    :type plane_aspect_base: typing.Union[int, str]
    :param plane_aspect_depth: Aspect, The initial aspect setting * FREE Free, Use an unconstrained aspect. * FIXED Fixed, Use a fixed 1:1 aspect.
    :type plane_aspect_depth: typing.Union[int, str]
    :param wait_for_input: Wait for Input
    :type wait_for_input: bool
    '''

    pass


def localview(frame_selected: bool = True):
    ''' Toggle display of selected object(s) separately and centered in view

    :param frame_selected: Frame Selected, Move the view to frame the selected objects
    :type frame_selected: bool
    '''

    pass


def localview_remove_from():
    ''' Move selected objects out of local view

    '''

    pass


def move(use_cursor_init: bool = True):
    ''' Move the view

    :param use_cursor_init: Use Mouse Position, Allow the initial mouse position to be used
    :type use_cursor_init: bool
    '''

    pass


def navigate():
    ''' Interactively navigate around the scene (uses the mode (walk/fly) preference)

    '''

    pass


def ndof_all():
    ''' Pan and rotate the view with the 3D mouse

    '''

    pass


def ndof_orbit():
    ''' Orbit the view using the 3D mouse

    '''

    pass


def ndof_orbit_zoom():
    ''' Orbit and zoom the view using the 3D mouse

    '''

    pass


def ndof_pan():
    ''' Pan the view with the 3D mouse

    '''

    pass


def object_as_camera():
    ''' Set the active object as the active camera for this view or scene

    '''

    pass


def object_mode_pie_or_toggle():
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.

    '''

    pass


def pastebuffer(autoselect: bool = True, active_collection: bool = True):
    ''' Objects from the clipboard are pasted

    :param autoselect: Select, Select pasted objects
    :type autoselect: bool
    :param active_collection: Active Collection, Put pasted objects in the active collection
    :type active_collection: bool
    '''

    pass


def render_border(xmin: int = 0,
                  xmax: int = 0,
                  ymin: int = 0,
                  ymax: int = 0,
                  wait_for_input: bool = True):
    ''' Set the boundaries of the border render and enable border render

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
    '''

    pass


def rotate(use_cursor_init: bool = True):
    ''' Rotate the view

    :param use_cursor_init: Use Mouse Position, Allow the initial mouse position to be used
    :type use_cursor_init: bool
    '''

    pass


def ruler_add():
    ''' Add ruler

    '''

    pass


def ruler_remove():
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.

    '''

    pass


def select(extend: bool = False,
           deselect: bool = False,
           toggle: bool = False,
           deselect_all: bool = False,
           center: bool = False,
           enumerate: bool = False,
           object: bool = False,
           location: typing.List[int] = (0, 0)):
    ''' Select and activate item(s)

    :param extend: Extend, Extend selection instead of deselecting everything first
    :type extend: bool
    :param deselect: Deselect, Remove from selection
    :type deselect: bool
    :param toggle: Toggle Selection, Toggle the selection
    :type toggle: bool
    :param deselect_all: Deselect On Nothing, Deselect all when nothing under the cursor
    :type deselect_all: bool
    :param center: Center, Use the object center when selecting, in edit mode used to extend object selection
    :type center: bool
    :param enumerate: Enumerate, List objects under the mouse (object mode only)
    :type enumerate: bool
    :param object: Object, Use object selection (edit mode only)
    :type object: bool
    :param location: Location, Mouse location
    :type location: typing.List[int]
    '''

    pass


def select_box(xmin: int = 0,
               xmax: int = 0,
               ymin: int = 0,
               ymax: int = 0,
               wait_for_input: bool = True,
               mode: typing.Union[int, str] = 'SET'):
    ''' Select items using box selection

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
    :param mode: Mode * SET Set, Set a new selection. * ADD Extend, Extend existing selection. * SUB Subtract, Subtract existing selection. * XOR Difference, Inverts existing selection. * AND Intersect, Intersect existing selection.
    :type mode: typing.Union[int, str]
    '''

    pass


def select_circle(x: int = 0,
                  y: int = 0,
                  radius: int = 25,
                  wait_for_input: bool = True,
                  mode: typing.Union[int, str] = 'SET'):
    ''' Select items using circle selection

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


def select_lasso(path: typing.Union[
        typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.
        List['bpy.types.OperatorMousePath'], 'bpy_prop_collection'] = None,
                 mode: typing.Union[int, str] = 'SET'):
    ''' Select items using lasso selection

    :param path: Path
    :type path: typing.Union[typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.List['bpy.types.OperatorMousePath'], 'bpy_prop_collection']
    :param mode: Mode * SET Set, Set a new selection. * ADD Extend, Extend existing selection. * SUB Subtract, Subtract existing selection. * XOR Difference, Inverts existing selection. * AND Intersect, Intersect existing selection.
    :type mode: typing.Union[int, str]
    '''

    pass


def select_menu(name: typing.Union[int, str] = '',
                extend: bool = False,
                deselect: bool = False,
                toggle: bool = False):
    ''' Menu object selection

    :param name: Object Name
    :type name: typing.Union[int, str]
    :param extend: Extend
    :type extend: bool
    :param deselect: Deselect
    :type deselect: bool
    :param toggle: Toggle
    :type toggle: bool
    '''

    pass


def smoothview():
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.

    '''

    pass


def snap_cursor_to_active():
    ''' Snap 3D cursor to the active item

    '''

    pass


def snap_cursor_to_center():
    ''' Snap 3D cursor to the world origin

    '''

    pass


def snap_cursor_to_grid():
    ''' Snap 3D cursor to the nearest grid division

    '''

    pass


def snap_cursor_to_selected():
    ''' Snap 3D cursor to the middle of the selected item(s)

    '''

    pass


def snap_selected_to_active():
    ''' Snap selected item(s) to the active item

    '''

    pass


def snap_selected_to_cursor(use_offset: bool = True):
    ''' Snap selected item(s) to the 3D cursor

    :param use_offset: Offset, If the selection should be snapped as a whole or by each object center
    :type use_offset: bool
    '''

    pass


def snap_selected_to_grid():
    ''' Snap selected item(s) to their nearest grid division

    '''

    pass


def toggle_matcap_flip():
    ''' Flip MatCap

    '''

    pass


def toggle_shading(type: typing.Union[int, str] = 'WIREFRAME'):
    ''' Toggle shading type in 3D viewport

    :param type: Type, Shading type to toggle * WIREFRAME Wireframe, Toggle wireframe shading. * SOLID Solid, Toggle solid shading. * MATERIAL LookDev, Toggle lookdev shading. * RENDERED Rendered, Toggle rendered shading.
    :type type: typing.Union[int, str]
    '''

    pass


def toggle_xray():
    ''' Transparent scene display. Allow selecting through items

    '''

    pass


def transform_gizmo_set(
        extend: bool = False,
        type: typing.Union[typing.Set[int], typing.Set[str]] = {}):
    ''' Set the current transform gizmo

    :param extend: Extend
    :type extend: bool
    :param type: Type
    :type type: typing.Union[typing.Set[int], typing.Set[str]]
    '''

    pass


def view_all(use_all_regions: bool = False, center: bool = False):
    ''' View all objects in scene

    :param use_all_regions: All Regions, View selected for all regions
    :type use_all_regions: bool
    :param center: Center
    :type center: bool
    '''

    pass


def view_axis(type: typing.Union[int, str] = 'LEFT',
              align_active: bool = False,
              relative: bool = False):
    ''' Use a preset viewpoint

    :param type: View, Preset viewpoint to use * LEFT Left, View from the left. * RIGHT Right, View from the right. * BOTTOM Bottom, View from the bottom. * TOP Top, View from the top. * FRONT Front, View from the front. * BACK Back, View from the back.
    :type type: typing.Union[int, str]
    :param align_active: Align Active, Align to the active object's axis
    :type align_active: bool
    :param relative: Relative, Rotate relative to the current orientation
    :type relative: bool
    '''

    pass


def view_camera():
    ''' Toggle the camera view

    '''

    pass


def view_center_camera():
    ''' Center the camera view, resizing the view to fit its bounds

    '''

    pass


def view_center_cursor():
    ''' Center the view so that the cursor is in the middle of the view

    '''

    pass


def view_center_lock():
    ''' Center the view lock offset

    '''

    pass


def view_center_pick():
    ''' Center the view to the Z-depth position under the mouse cursor

    '''

    pass


def view_lock_clear():
    ''' Clear all view locking

    '''

    pass


def view_lock_to_active():
    ''' Lock the view to the active object/bone

    '''

    pass


def view_orbit(angle: float = 0.0, type: typing.Union[int, str] = 'ORBITLEFT'):
    ''' Orbit the view

    :param angle: Roll
    :type angle: float
    :param type: Orbit, Direction of View Orbit * ORBITLEFT Orbit Left, Orbit the view around to the left. * ORBITRIGHT Orbit Right, Orbit the view around to the right. * ORBITUP Orbit Up, Orbit the view up. * ORBITDOWN Orbit Down, Orbit the view down.
    :type type: typing.Union[int, str]
    '''

    pass


def view_pan(type: typing.Union[int, str] = 'PANLEFT'):
    ''' Pan the view in a given direction

    :param type: Pan, Direction of View Pan * PANLEFT Pan Left, Pan the view to the left. * PANRIGHT Pan Right, Pan the view to the right. * PANUP Pan Up, Pan the view up. * PANDOWN Pan Down, Pan the view down.
    :type type: typing.Union[int, str]
    '''

    pass


def view_persportho():
    ''' Switch the current view from perspective/orthographic projection

    '''

    pass


def view_roll(angle: float = 0.0, type: typing.Union[int, str] = 'ANGLE'):
    ''' Roll the view

    :param angle: Roll
    :type angle: float
    :param type: Roll Angle Source, How roll angle is calculated * ANGLE Roll Angle, Roll the view using an angle value. * LEFT Roll Left, Roll the view around to the left. * RIGHT Roll Right, Roll the view around to the right.
    :type type: typing.Union[int, str]
    '''

    pass


def view_selected(use_all_regions: bool = False):
    ''' Move the view to the selection center

    :param use_all_regions: All Regions, View selected for all regions
    :type use_all_regions: bool
    '''

    pass


def walk():
    ''' Interactively walk around the scene

    '''

    pass


def zoom(mx: int = 0,
         my: int = 0,
         delta: int = 0,
         use_cursor_init: bool = True):
    ''' Zoom in/out in the view

    :param mx: Region Position X
    :type mx: int
    :param my: Region Position Y
    :type my: int
    :param delta: Delta
    :type delta: int
    :param use_cursor_init: Use Mouse Position, Allow the initial mouse position to be used
    :type use_cursor_init: bool
    '''

    pass


def zoom_border(xmin: int = 0,
                xmax: int = 0,
                ymin: int = 0,
                ymax: int = 0,
                wait_for_input: bool = True,
                zoom_out: bool = False):
    ''' Zoom in the view to the nearest object contained in the border

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
    :param zoom_out: Zoom Out
    :type zoom_out: bool
    '''

    pass


def zoom_camera_1_to_1():
    ''' Match the camera to 1:1 to the render output

    '''

    pass
