import sys
import typing
import bpy.types


def active_frame_delete():
    ''' Delete the active frame for the active Grease Pencil Layer

    '''

    pass


def active_frames_delete_all():
    ''' Delete the active frame(s) of all editable Grease Pencil layers

    '''

    pass


def annotate(mode: typing.Union[int, str] = 'DRAW',
             arrowstyle_start: typing.Union[int, str] = 'NONE',
             arrowstyle_end: typing.Union[int, str] = 'NONE',
             use_stabilizer: bool = False,
             stabilizer_factor: float = 0.75,
             stabilizer_radius: int = 35,
             stroke: typing.
             Union[typing.Dict[str, 'bpy.types.OperatorStrokeElement'], typing.
                   List['bpy.types.OperatorStrokeElement'],
                   'bpy_prop_collection'] = None,
             wait_for_input: bool = True):
    ''' Make annotations on the active data

    :param mode: Mode, Way to interpret mouse movements * DRAW Draw Freehand, Draw freehand stroke(s). * DRAW_STRAIGHT Draw Straight Lines, Draw straight line segment(s). * DRAW_POLY Draw Poly Line, Click to place endpoints of straight line segments (connected). * ERASER Eraser, Erase Annotation strokes.
    :type mode: typing.Union[int, str]
    :param arrowstyle_start: Start Arrow Style, Stroke start style * NONE None, Don't use any arrow/style in corner. * ARROW Arrow, Use closed arrow style. * ARROW_OPEN Open Arrow, Use open arrow style. * ARROW_OPEN_INVERTED Segment, Use perpendicular segment style. * DIAMOND Square, Use square style.
    :type arrowstyle_start: typing.Union[int, str]
    :param arrowstyle_end: End Arrow Style, Stroke end style * NONE None, Don't use any arrow/style in corner. * ARROW Arrow, Use closed arrow style. * ARROW_OPEN Open Arrow, Use open arrow style. * ARROW_OPEN_INVERTED Segment, Use perpendicular segment style. * DIAMOND Square, Use square style.
    :type arrowstyle_end: typing.Union[int, str]
    :param use_stabilizer: Stabilize Stroke, Helper to draw smooth and clean lines. Press Shift for an invert effect (even if this option is not active)
    :type use_stabilizer: bool
    :param stabilizer_factor: Stabilizer Stroke Factor, Higher values gives a smoother stroke
    :type stabilizer_factor: float
    :param stabilizer_radius: Stabilizer Stroke Radius, Minimum distance from last point before stroke continues
    :type stabilizer_radius: int
    :param stroke: Stroke
    :type stroke: typing.Union[typing.Dict[str, 'bpy.types.OperatorStrokeElement'], typing.List['bpy.types.OperatorStrokeElement'], 'bpy_prop_collection']
    :param wait_for_input: Wait for Input, Wait for first click instead of painting immediately
    :type wait_for_input: bool
    '''

    pass


def annotation_active_frame_delete():
    ''' Delete the active frame for the active Annotation Layer

    '''

    pass


def annotation_add():
    ''' Add new Annotation data-block

    '''

    pass


def bake_mesh_animation(target: typing.Union[int, str] = 'NEW',
                        frame_start: int = 1,
                        frame_end: int = 250,
                        step: int = 1,
                        thickness: int = 1,
                        angle: float = 1.22173,
                        offset: float = 0.001,
                        seams: bool = False,
                        faces: bool = True,
                        only_selected: bool = False,
                        frame_target: int = 1,
                        project_type: typing.Union[int, str] = 'VIEW'):
    ''' Bake mesh animation to grease pencil strokes

    :param target: Target Object, Target grease pencil
    :type target: typing.Union[int, str]
    :param frame_start: Start Frame, The start frame
    :type frame_start: int
    :param frame_end: End Frame, The end frame of animation
    :type frame_end: int
    :param step: Step, Step between generated frames
    :type step: int
    :param thickness: Thickness
    :type thickness: int
    :param angle: Threshold Angle, Threshold to determine ends of the strokes
    :type angle: float
    :param offset: Stroke Offset, Offset strokes from fill
    :type offset: float
    :param seams: Only Seam Edges, Convert only seam edges
    :type seams: bool
    :param faces: Export Faces, Export faces as filled strokes
    :type faces: bool
    :param only_selected: Only Selected Keyframes, Convert only selected keyframes
    :type only_selected: bool
    :param frame_target: Target Frame, Destination frame
    :type frame_target: int
    :param project_type: Projection Type * KEEP No Reproject. * FRONT Front, Reproject the strokes using the X-Z plane. * SIDE Side, Reproject the strokes using the Y-Z plane. * TOP Top, Reproject the strokes using the X-Y plane. * VIEW View, Reproject the strokes to end up on the same plane, as if drawn from the current viewpoint using 'Cursor' Stroke Placement. * CURSOR Cursor, Reproject the strokes using the orientation of 3D cursor.
    :type project_type: typing.Union[int, str]
    '''

    pass


def blank_frame_add(all_layers: bool = False):
    ''' Insert a blank frame on the current frame (all subsequently existing frames, if any, are shifted right by one frame)

    :param all_layers: All Layers, Create blank frame in all layers, not only active
    :type all_layers: bool
    '''

    pass


def brush_reset():
    ''' Reset brush to default parameters

    '''

    pass


def brush_reset_all():
    ''' Delete all mode brushes and recreate a default set

    '''

    pass


def convert(type: typing.Union[int, str] = 'PATH',
            bevel_depth: float = 0.0,
            bevel_resolution: int = 0,
            use_normalize_weights: bool = True,
            radius_multiplier: float = 1.0,
            use_link_strokes: bool = False,
            timing_mode: typing.Union[int, str] = 'FULL',
            frame_range: int = 100,
            start_frame: int = 1,
            use_realtime: bool = False,
            end_frame: int = 250,
            gap_duration: float = 0.0,
            gap_randomness: float = 0.0,
            seed: int = 0,
            use_timing_data: bool = False):
    ''' Convert the active Grease Pencil layer to a new Curve Object

    :param type: Type, Which type of curve to convert to * PATH Path, Animation path. * CURVE Bezier Curve, Smooth Bezier curve. * POLY Polygon Curve, Bezier curve with straight-line segments (vector handles).
    :type type: typing.Union[int, str]
    :param bevel_depth: Bevel Depth
    :type bevel_depth: float
    :param bevel_resolution: Bevel Resolution, Bevel resolution when depth is non-zero
    :type bevel_resolution: int
    :param use_normalize_weights: Normalize Weight, Normalize weight (set from stroke width)
    :type use_normalize_weights: bool
    :param radius_multiplier: Radius Factor, Multiplier for the points' radii (set from stroke width)
    :type radius_multiplier: float
    :param use_link_strokes: Link Strokes, Whether to link strokes with zero-radius sections of curves
    :type use_link_strokes: bool
    :param timing_mode: Timing Mode, How to use timing data stored in strokes * NONE No Timing, Ignore timing. * LINEAR Linear, Simple linear timing. * FULL Original, Use the original timing, gaps included. * CUSTOMGAP Custom Gaps, Use the original timing, but with custom gap lengths (in frames).
    :type timing_mode: typing.Union[int, str]
    :param frame_range: Frame Range, The duration of evaluation of the path control curve
    :type frame_range: int
    :param start_frame: Start Frame, The start frame of the path control curve
    :type start_frame: int
    :param use_realtime: Realtime, Whether the path control curve reproduces the drawing in realtime, starting from Start Frame
    :type use_realtime: bool
    :param end_frame: End Frame, The end frame of the path control curve (if Realtime is not set)
    :type end_frame: int
    :param gap_duration: Gap Duration, Custom Gap mode: (Average) length of gaps, in frames (Note: Realtime value, will be scaled if Realtime is not set)
    :type gap_duration: float
    :param gap_randomness: Gap Randomness, Custom Gap mode: Number of frames that gap lengths can vary
    :type gap_randomness: float
    :param seed: Random Seed, Custom Gap mode: Random generator seed
    :type seed: int
    :param use_timing_data: Has Valid Timing, Whether the converted Grease Pencil layer has valid timing data (internal use)
    :type use_timing_data: bool
    '''

    pass


def convert_old_files(annotation: bool = False):
    ''' Convert 2.7x grease pencil files to 2.80

    :param annotation: Annotation, Convert to Annotations
    :type annotation: bool
    '''

    pass


def copy():
    ''' Copy selected Grease Pencil points and strokes

    '''

    pass


def data_unlink():
    ''' Unlink active Annotation data-block

    '''

    pass


def delete(type: typing.Union[int, str] = 'POINTS'):
    ''' Delete selected Grease Pencil strokes, vertices, or frames

    :param type: Type, Method used for deleting Grease Pencil data * POINTS Points, Delete selected points and split strokes into segments. * STROKES Strokes, Delete selected strokes. * FRAME Frame, Delete active frame.
    :type type: typing.Union[int, str]
    '''

    pass


def dissolve(type: typing.Union[int, str] = 'POINTS'):
    ''' Delete selected points without splitting strokes

    :param type: Type, Method used for dissolving stroke points * POINTS Dissolve, Dissolve selected points. * BETWEEN Dissolve Between, Dissolve points between selected points. * UNSELECT Dissolve Unselect, Dissolve all unselected points.
    :type type: typing.Union[int, str]
    '''

    pass


def draw(mode: typing.Union[int, str] = 'DRAW',
         stroke: typing.
         Union[typing.Dict[str, 'bpy.types.OperatorStrokeElement'], typing.
               List['bpy.types.OperatorStrokeElement'],
               'bpy_prop_collection'] = None,
         wait_for_input: bool = True,
         disable_straight: bool = False,
         disable_fill: bool = False,
         disable_stabilizer: bool = False,
         guide_last_angle: float = 0.0):
    ''' Draw a new stroke in the active Grease Pencil object

    :param mode: Mode, Way to interpret mouse movements * DRAW Draw Freehand, Draw freehand stroke(s). * DRAW_STRAIGHT Draw Straight Lines, Draw straight line segment(s). * ERASER Eraser, Erase Grease Pencil strokes.
    :type mode: typing.Union[int, str]
    :param stroke: Stroke
    :type stroke: typing.Union[typing.Dict[str, 'bpy.types.OperatorStrokeElement'], typing.List['bpy.types.OperatorStrokeElement'], 'bpy_prop_collection']
    :param wait_for_input: Wait for Input, Wait for first click instead of painting immediately
    :type wait_for_input: bool
    :param disable_straight: No Straight lines, Disable key for straight lines
    :type disable_straight: bool
    :param disable_fill: No Fill Areas, Disable fill to use stroke as fill boundary
    :type disable_fill: bool
    :param disable_stabilizer: No Stabilizer
    :type disable_stabilizer: bool
    :param guide_last_angle: Angle, Speed guide angle
    :type guide_last_angle: float
    '''

    pass


def duplicate():
    ''' Duplicate the selected Grease Pencil strokes

    '''

    pass


def duplicate_move(GPENCIL_OT_duplicate=None, TRANSFORM_OT_translate=None):
    ''' Make copies of the selected Grease Pencil strokes and move them

    :param GPENCIL_OT_duplicate: Duplicate Strokes, Duplicate the selected Grease Pencil strokes
    :param TRANSFORM_OT_translate: Move, Move selected items
    '''

    pass


def editmode_toggle(back: bool = False):
    ''' Enter/Exit edit mode for Grease Pencil strokes

    :param back: Return to Previous Mode, Return to previous mode
    :type back: bool
    '''

    pass


def extract_palette_vertex(selected: bool = False, threshold: int = 1):
    ''' Extract all colors used in Grease Pencil Vertex and create a Palette

    :param selected: Only Selected, Convert only selected strokes
    :type selected: bool
    :param threshold: Threshold
    :type threshold: int
    '''

    pass


def extrude():
    ''' Extrude the selected Grease Pencil points

    '''

    pass


def extrude_move(GPENCIL_OT_extrude=None, TRANSFORM_OT_translate=None):
    ''' Extrude selected points and move them

    :param GPENCIL_OT_extrude: Extrude Stroke Points, Extrude the selected Grease Pencil points
    :param TRANSFORM_OT_translate: Move, Move selected items
    '''

    pass


def fill(on_back: bool = False):
    ''' Fill with color the shape formed by strokes

    :param on_back: Draw on Back, Send new stroke to back
    :type on_back: bool
    '''

    pass


def frame_clean_duplicate(type: typing.Union[int, str] = 'ALL'):
    ''' Remove any duplicated frame

    :param type: Type
    :type type: typing.Union[int, str]
    '''

    pass


def frame_clean_fill(mode: typing.Union[int, str] = 'ACTIVE'):
    ''' Remove 'no fill' boundary strokes

    :param mode: Mode * ACTIVE Active Frame Only, Clean active frame only. * ALL All Frames, Clean all frames in all layers.
    :type mode: typing.Union[int, str]
    '''

    pass


def frame_clean_loose(limit: int = 1):
    ''' Remove loose points

    :param limit: Limit, Number of points to consider stroke as loose
    :type limit: int
    '''

    pass


def frame_duplicate(mode: typing.Union[int, str] = 'ACTIVE'):
    ''' Make a copy of the active Grease Pencil Frame

    :param mode: Mode * ACTIVE Active, Duplicate frame in active layer only. * ALL All, Duplicate active frames in all layers.
    :type mode: typing.Union[int, str]
    '''

    pass


def generate_weights(mode: typing.Union[int, str] = 'NAME',
                     armature: typing.Union[int, str] = 'DEFAULT',
                     ratio: float = 0.1,
                     decay: float = 0.8):
    ''' Generate automatic weights for armatures (requires armature modifier)

    :param mode: Mode
    :type mode: typing.Union[int, str]
    :param armature: Armature, Armature to use
    :type armature: typing.Union[int, str]
    :param ratio: Ratio, Ratio between bone length and influence radius
    :type ratio: float
    :param decay: Decay, Factor to reduce influence depending of distance to bone axis
    :type decay: float
    '''

    pass


def guide_rotate(increment: bool = True, angle: float = 0.0):
    ''' Rotate guide angle

    :param increment: Increment, Increment angle
    :type increment: bool
    :param angle: Angle, Guide angle
    :type angle: float
    '''

    pass


def hide(unselected: bool = False):
    ''' Hide selected/unselected Grease Pencil layers

    :param unselected: Unselected, Hide unselected rather than selected layers
    :type unselected: bool
    '''

    pass


def image_to_grease_pencil(size: float = 0.005, mask: bool = False):
    ''' Generate a Grease Pencil Object using Image as source

    :param size: Point Size, Size used for grease pencil points
    :type size: float
    :param mask: Generate Mask, Create an inverted image for masking using alpha channel
    :type mask: bool
    '''

    pass


def interpolate(shift: float = 0.0):
    ''' Interpolate grease pencil strokes between frames

    :param shift: Shift, Bias factor for which frame has more influence on the interpolated strokes
    :type shift: float
    '''

    pass


def interpolate_reverse():
    ''' Remove breakdown frames generated by interpolating between two Grease Pencil frames

    '''

    pass


def interpolate_sequence():
    ''' Generate 'in-betweens' to smoothly interpolate between Grease Pencil frames

    '''

    pass


def layer_active(layer: int = 0):
    ''' Active Grease Pencil layer

    :param layer: Grease Pencil Layer
    :type layer: int
    '''

    pass


def layer_add():
    ''' Add new layer or note for the active data-block

    '''

    pass


def layer_annotation_add():
    ''' Add new Annotation layer or note for the active data-block

    '''

    pass


def layer_annotation_move(type: typing.Union[int, str] = 'UP'):
    ''' Move the active Annotation layer up/down in the list

    :param type: Type
    :type type: typing.Union[int, str]
    '''

    pass


def layer_annotation_remove():
    ''' Remove active Annotation layer

    '''

    pass


def layer_change(layer: typing.Union[int, str] = 'DEFAULT'):
    ''' Change active Grease Pencil layer

    :param layer: Grease Pencil Layer
    :type layer: typing.Union[int, str]
    '''

    pass


def layer_duplicate():
    ''' Make a copy of the active Grease Pencil layer

    '''

    pass


def layer_duplicate_object(object: str = "",
                           mode: typing.Union[int, str] = 'ALL'):
    ''' Make a copy of the active Grease Pencil layer to new object

    :param object: Object, Name of the destination object
    :type object: str
    :param mode: Mode
    :type mode: typing.Union[int, str]
    '''

    pass


def layer_isolate(affect_visibility: bool = False):
    ''' Toggle whether the active layer is the only one that can be edited and/or visible

    :param affect_visibility: Affect Visibility, In addition to toggling the editability, also affect the visibility
    :type affect_visibility: bool
    '''

    pass


def layer_mask_add(name: str = ""):
    ''' Add new layer as masking

    :param name: Layer, Name of the layer
    :type name: str
    '''

    pass


def layer_mask_remove():
    ''' Remove Layer Mask

    '''

    pass


def layer_merge():
    ''' Merge the current layer with the layer below

    '''

    pass


def layer_move(type: typing.Union[int, str] = 'UP'):
    ''' Move the active Grease Pencil layer up/down in the list

    :param type: Type
    :type type: typing.Union[int, str]
    '''

    pass


def layer_remove():
    ''' Remove active Grease Pencil layer

    '''

    pass


def lock_all():
    ''' Lock all Grease Pencil layers to prevent them from being accidentally modified

    '''

    pass


def lock_layer():
    ''' Lock and hide any color not used in any layer

    '''

    pass


def material_hide(unselected: bool = False):
    ''' Hide selected/unselected Grease Pencil materials

    :param unselected: Unselected, Hide unselected rather than selected colors
    :type unselected: bool
    '''

    pass


def material_isolate(affect_visibility: bool = False):
    ''' Toggle whether the active material is the only one that is editable and/or visible

    :param affect_visibility: Affect Visibility, In addition to toggling the editability, also affect the visibility
    :type affect_visibility: bool
    '''

    pass


def material_lock_all():
    ''' Lock all Grease Pencil materials to prevent them from being accidentally modified

    '''

    pass


def material_lock_unused():
    ''' Lock any material not used in any selected stroke

    '''

    pass


def material_reveal():
    ''' Unhide all hidden Grease Pencil materials

    '''

    pass


def material_select(deselect: bool = False):
    ''' Select/Deselect all Grease Pencil strokes using current material

    :param deselect: Deselect, Unselect strokes
    :type deselect: bool
    '''

    pass


def material_set(slot: typing.Union[int, str] = 'DEFAULT'):
    ''' Set active material

    :param slot: Material Slot
    :type slot: typing.Union[int, str]
    '''

    pass


def material_to_vertex_color(remove: bool = True,
                             palette: bool = True,
                             selected: bool = False,
                             threshold: int = 3):
    ''' Replace materials in strokes with Vertex Color

    :param remove: Remove Unused Materials, Remove any unused material after the conversion
    :type remove: bool
    :param palette: Create Palette, Create a new palette with colors
    :type palette: bool
    :param selected: Only Selected, Convert only selected strokes
    :type selected: bool
    :param threshold: Threshold
    :type threshold: int
    '''

    pass


def material_unlock_all():
    ''' Unlock all Grease Pencil materials so that they can be edited

    '''

    pass


def move_to_layer(layer: int = 0):
    ''' Move selected strokes to another layer

    :param layer: Grease Pencil Layer
    :type layer: int
    '''

    pass


def paintmode_toggle(back: bool = False):
    ''' Enter/Exit paint mode for Grease Pencil strokes

    :param back: Return to Previous Mode, Return to previous mode
    :type back: bool
    '''

    pass


def paste(type: typing.Union[int, str] = 'ACTIVE', paste_back: bool = False):
    ''' Paste previously copied strokes to active layer or to original layer

    :param type: Type
    :type type: typing.Union[int, str]
    :param paste_back: Paste on Back, Add pasted strokes behind all strokes
    :type paste_back: bool
    '''

    pass


def primitive_box(subdivision: int = 3,
                  edges: int = 2,
                  type: typing.Union[int, str] = 'BOX',
                  wait_for_input: bool = True):
    ''' Create predefined grease pencil stroke box shapes

    :param subdivision: Subdivisions, Number of subdivision by edges
    :type subdivision: int
    :param edges: Edges, Number of points by edge
    :type edges: int
    :param type: Type, Type of shape
    :type type: typing.Union[int, str]
    :param wait_for_input: Wait for Input
    :type wait_for_input: bool
    '''

    pass


def primitive_circle(subdivision: int = 94,
                     edges: int = 2,
                     type: typing.Union[int, str] = 'CIRCLE',
                     wait_for_input: bool = True):
    ''' Create predefined grease pencil stroke circle shapes

    :param subdivision: Subdivisions, Number of subdivision by edges
    :type subdivision: int
    :param edges: Edges, Number of points by edge
    :type edges: int
    :param type: Type, Type of shape
    :type type: typing.Union[int, str]
    :param wait_for_input: Wait for Input
    :type wait_for_input: bool
    '''

    pass


def primitive_curve(subdivision: int = 62,
                    edges: int = 2,
                    type: typing.Union[int, str] = 'CURVE',
                    wait_for_input: bool = True):
    ''' Create predefined grease pencil stroke curve shapes

    :param subdivision: Subdivisions, Number of subdivision by edges
    :type subdivision: int
    :param edges: Edges, Number of points by edge
    :type edges: int
    :param type: Type, Type of shape
    :type type: typing.Union[int, str]
    :param wait_for_input: Wait for Input
    :type wait_for_input: bool
    '''

    pass


def primitive_line(subdivision: int = 6,
                   edges: int = 2,
                   type: typing.Union[int, str] = 'LINE',
                   wait_for_input: bool = True):
    ''' Create predefined grease pencil stroke lines

    :param subdivision: Subdivisions, Number of subdivision by edges
    :type subdivision: int
    :param edges: Edges, Number of points by edge
    :type edges: int
    :param type: Type, Type of shape
    :type type: typing.Union[int, str]
    :param wait_for_input: Wait for Input
    :type wait_for_input: bool
    '''

    pass


def primitive_polyline(subdivision: int = 6,
                       edges: int = 2,
                       type: typing.Union[int, str] = 'POLYLINE',
                       wait_for_input: bool = True):
    ''' Create predefined grease pencil stroke polylines

    :param subdivision: Subdivisions, Number of subdivision by edges
    :type subdivision: int
    :param edges: Edges, Number of points by edge
    :type edges: int
    :param type: Type, Type of shape
    :type type: typing.Union[int, str]
    :param wait_for_input: Wait for Input
    :type wait_for_input: bool
    '''

    pass


def recalc_geometry():
    ''' Update all internal geometry data

    '''

    pass


def reproject(type: typing.Union[int, str] = 'VIEW',
              keep_original: bool = False):
    ''' Reproject the selected strokes from the current viewpoint as if they had been newly drawn (e.g. to fix problems from accidental 3D cursor movement or accidental viewport changes, or for matching deforming geometry)

    :param type: Projection Type * FRONT Front, Reproject the strokes using the X-Z plane. * SIDE Side, Reproject the strokes using the Y-Z plane. * TOP Top, Reproject the strokes using the X-Y plane. * VIEW View, Reproject the strokes to end up on the same plane, as if drawn from the current viewpoint using 'Cursor' Stroke Placement. * SURFACE Surface, Reproject the strokes on to the scene geometry, as if drawn using 'Surface' placement. * CURSOR Cursor, Reproject the strokes using the orientation of 3D cursor.
    :type type: typing.Union[int, str]
    :param keep_original: Keep Original, Keep original strokes and create a copy before reprojecting instead of reproject them
    :type keep_original: bool
    '''

    pass


def reset_transform_fill(mode: typing.Union[int, str] = 'ALL'):
    ''' Reset any UV transformation and back to default values

    :param mode: Mode
    :type mode: typing.Union[int, str]
    '''

    pass


def reveal(select: bool = True):
    ''' Show all Grease Pencil layers

    :param select: Select
    :type select: bool
    '''

    pass


def sculpt_paint(stroke: typing.Union[
        typing.Dict[str, 'bpy.types.OperatorStrokeElement'], typing.
        List['bpy.types.OperatorStrokeElement'], 'bpy_prop_collection'] = None,
                 wait_for_input: bool = True):
    ''' Apply tweaks to strokes by painting over the strokes

    :param stroke: Stroke
    :type stroke: typing.Union[typing.Dict[str, 'bpy.types.OperatorStrokeElement'], typing.List['bpy.types.OperatorStrokeElement'], 'bpy_prop_collection']
    :param wait_for_input: Wait for Input, Enter a mini 'sculpt-mode' if enabled, otherwise, exit after drawing a single stroke
    :type wait_for_input: bool
    '''

    pass


def sculptmode_toggle(back: bool = False):
    ''' Enter/Exit sculpt mode for Grease Pencil strokes

    :param back: Return to Previous Mode, Return to previous mode
    :type back: bool
    '''

    pass


def select(extend: bool = False,
           deselect: bool = False,
           toggle: bool = False,
           deselect_all: bool = False,
           entire_strokes: bool = False,
           location: typing.List[int] = (0, 0),
           use_shift_extend: bool = False):
    ''' Select Grease Pencil strokes and/or stroke points

    :param extend: Extend, Extend selection instead of deselecting everything first
    :type extend: bool
    :param deselect: Deselect, Remove from selection
    :type deselect: bool
    :param toggle: Toggle Selection, Toggle the selection
    :type toggle: bool
    :param deselect_all: Deselect On Nothing, Deselect all when nothing under the cursor
    :type deselect_all: bool
    :param entire_strokes: Entire Strokes, Select entire strokes instead of just the nearest stroke vertex
    :type entire_strokes: bool
    :param location: Location, Mouse location
    :type location: typing.List[int]
    :param use_shift_extend: Extend
    :type use_shift_extend: bool
    '''

    pass


def select_all(action: typing.Union[int, str] = 'TOGGLE'):
    ''' Change selection of all Grease Pencil strokes currently visible

    :param action: Action, Selection action to execute * TOGGLE Toggle, Toggle selection for all elements. * SELECT Select, Select all elements. * DESELECT Deselect, Deselect all elements. * INVERT Invert, Invert selection of all elements.
    :type action: typing.Union[int, str]
    '''

    pass


def select_alternate(unselect_ends: bool = True):
    ''' Select alternative points in same strokes as already selected points

    :param unselect_ends: Unselect Ends, Do not select the first and last point of the stroke
    :type unselect_ends: bool
    '''

    pass


def select_box(xmin: int = 0,
               xmax: int = 0,
               ymin: int = 0,
               ymax: int = 0,
               wait_for_input: bool = True,
               mode: typing.Union[int, str] = 'SET'):
    ''' Select Grease Pencil strokes within a rectangular region

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
    ''' Select Grease Pencil strokes using brush selection

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


def select_first(only_selected_strokes: bool = False, extend: bool = False):
    ''' Select first point in Grease Pencil strokes

    :param only_selected_strokes: Selected Strokes Only, Only select the first point of strokes that already have points selected
    :type only_selected_strokes: bool
    :param extend: Extend, Extend selection instead of deselecting all other selected points
    :type extend: bool
    '''

    pass


def select_grouped(type: typing.Union[int, str] = 'LAYER'):
    ''' Select all strokes with similar characteristics

    :param type: Type * LAYER Layer, Shared layers. * MATERIAL Material, Shared materials.
    :type type: typing.Union[int, str]
    '''

    pass


def select_lasso(mode: typing.Union[int, str] = 'SET',
                 path: typing.
                 Union[typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.
                       List['bpy.types.OperatorMousePath'],
                       'bpy_prop_collection'] = None):
    ''' Select Grease Pencil strokes using lasso selection

    :param mode: Mode * SET Set, Set a new selection. * ADD Extend, Extend existing selection. * SUB Subtract, Subtract existing selection. * XOR Difference, Inverts existing selection. * AND Intersect, Intersect existing selection.
    :type mode: typing.Union[int, str]
    :param path: Path
    :type path: typing.Union[typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.List['bpy.types.OperatorMousePath'], 'bpy_prop_collection']
    '''

    pass


def select_last(only_selected_strokes: bool = False, extend: bool = False):
    ''' Select last point in Grease Pencil strokes

    :param only_selected_strokes: Selected Strokes Only, Only select the last point of strokes that already have points selected
    :type only_selected_strokes: bool
    :param extend: Extend, Extend selection instead of deselecting all other selected points
    :type extend: bool
    '''

    pass


def select_less():
    ''' Shrink sets of selected Grease Pencil points

    '''

    pass


def select_linked():
    ''' Select all points in same strokes as already selected points

    '''

    pass


def select_more():
    ''' Grow sets of selected Grease Pencil points

    '''

    pass


def select_vertex_color(threshold: int = 0):
    ''' Select all points with similar vertex color of current selected

    :param threshold: Threshold, Tolerance of the selection. Higher values select a wider range of similar colors
    :type threshold: int
    '''

    pass


def selection_opacity_toggle():
    ''' Hide/Unhide selected points for Grease Pencil strokes setting alpha factor

    '''

    pass


def selectmode_toggle(mode: int = 0):
    ''' Set selection mode for Grease Pencil strokes

    :param mode: Select Mode, Select mode
    :type mode: int
    '''

    pass


def set_active_material():
    ''' Set the selected stroke material as the active material

    '''

    pass


def snap_cursor_to_selected():
    ''' Snap cursor to center of selected points

    '''

    pass


def snap_to_cursor(use_offset: bool = True):
    ''' Snap selected points/strokes to the cursor

    :param use_offset: With Offset, Offset the entire stroke instead of selected points only
    :type use_offset: bool
    '''

    pass


def snap_to_grid():
    ''' Snap selected points to the nearest grid points

    '''

    pass


def stroke_apply_thickness():
    ''' Apply the thickness change of the layer to its strokes

    '''

    pass


def stroke_arrange(direction: typing.Union[int, str] = 'UP'):
    ''' Arrange selected strokes up/down in the drawing order of the active layer

    :param direction: Direction
    :type direction: typing.Union[int, str]
    '''

    pass


def stroke_caps_set(type: typing.Union[int, str] = 'TOGGLE'):
    ''' Change stroke caps mode (rounded or flat)

    :param type: Type * TOGGLE Both. * START Start. * END End. * TOGGLE Default, Set as default rounded.
    :type type: typing.Union[int, str]
    '''

    pass


def stroke_change_color(material: str = ""):
    ''' Move selected strokes to active material

    :param material: Material, Name of the material
    :type material: str
    '''

    pass


def stroke_cutter(path: typing.Union[
        typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.
        List['bpy.types.OperatorMousePath'], 'bpy_prop_collection'] = None,
                  flat_caps: bool = False):
    ''' Select section and cut

    :param path: Path
    :type path: typing.Union[typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.List['bpy.types.OperatorMousePath'], 'bpy_prop_collection']
    :param flat_caps: Flat Caps
    :type flat_caps: bool
    '''

    pass


def stroke_cyclical_set(type: typing.Union[int, str] = 'TOGGLE',
                        geometry: bool = False):
    ''' Close or open the selected stroke adding an edge from last to first point

    :param type: Type
    :type type: typing.Union[int, str]
    :param geometry: Create Geometry, Create new geometry for closing stroke
    :type geometry: bool
    '''

    pass


def stroke_editcurve_set_handle_type(
        type: typing.Union[int, str] = 'AUTOMATIC'):
    ''' Set the type of a edit curve handle

    :param type: Type, Spline type
    :type type: typing.Union[int, str]
    '''

    pass


def stroke_enter_editcurve_mode(error_threshold: float = 0.1):
    ''' Called to transform a stroke into a curve

    :param error_threshold: Error Threshold, Threshold on the maximum deviation from the actual stroke
    :type error_threshold: float
    '''

    pass


def stroke_flip():
    ''' Change direction of the points of the selected strokes

    '''

    pass


def stroke_join(type: typing.Union[int, str] = 'JOIN',
                leave_gaps: bool = False):
    ''' Join selected strokes (optionally as new stroke)

    :param type: Type
    :type type: typing.Union[int, str]
    :param leave_gaps: Leave Gaps, Leave gaps between joined strokes instead of linking them
    :type leave_gaps: bool
    '''

    pass


def stroke_merge(mode: typing.Union[int, str] = 'STROKE',
                 back: bool = False,
                 additive: bool = False,
                 cyclic: bool = False,
                 clear_point: bool = False,
                 clear_stroke: bool = False):
    ''' Create a new stroke with the selected stroke points

    :param mode: Mode
    :type mode: typing.Union[int, str]
    :param back: Draw on Back, Draw new stroke below all previous strokes
    :type back: bool
    :param additive: Additive Drawing, Add to previous drawing
    :type additive: bool
    :param cyclic: Cyclic, Close new stroke
    :type cyclic: bool
    :param clear_point: Dissolve Points, Dissolve old selected points
    :type clear_point: bool
    :param clear_stroke: Delete Strokes, Delete old selected strokes
    :type clear_stroke: bool
    '''

    pass


def stroke_merge_by_distance(threshold: float = 0.001,
                             use_unselected: bool = False):
    ''' Merge points by distance

    :param threshold: Threshold
    :type threshold: float
    :param use_unselected: Unselected, Use whole stroke, not only selected points
    :type use_unselected: bool
    '''

    pass


def stroke_merge_material(hue_threshold: float = 0.001,
                          sat_threshold: float = 0.001,
                          val_threshold: float = 0.001):
    ''' Replace materials in strokes merging similar

    :param hue_threshold: Hue Threshold
    :type hue_threshold: float
    :param sat_threshold: Saturation Threshold
    :type sat_threshold: float
    :param val_threshold: Value Threshold
    :type val_threshold: float
    '''

    pass


def stroke_reset_vertex_color(mode: typing.Union[int, str] = 'BOTH'):
    ''' Reset vertex color for all or selected strokes

    :param mode: Mode * STROKE Stroke, Reset Vertex Color to Stroke only. * FILL Fill, Reset Vertex Color to Fill only. * BOTH Stroke and Fill, Reset Vertex Color to Stroke and Fill.
    :type mode: typing.Union[int, str]
    '''

    pass


def stroke_sample(length: float = 0.1):
    ''' Sample stroke points to predefined segment length

    :param length: Length
    :type length: float
    '''

    pass


def stroke_separate(mode: typing.Union[int, str] = 'POINT'):
    ''' Separate the selected strokes or layer in a new grease pencil object

    :param mode: Mode * POINT Selected Points, Separate the selected points. * STROKE Selected Strokes, Separate the selected strokes. * LAYER Active Layer, Separate the strokes of the current layer.
    :type mode: typing.Union[int, str]
    '''

    pass


def stroke_simplify(factor: float = 0.0):
    ''' Simplify selected stroked reducing number of points

    :param factor: Factor
    :type factor: float
    '''

    pass


def stroke_simplify_fixed(step: int = 1):
    ''' Simplify selected stroked reducing number of points using fixed algorithm

    :param step: Steps, Number of simplify steps
    :type step: int
    '''

    pass


def stroke_smooth(repeat: int = 1,
                  factor: float = 0.5,
                  only_selected: bool = True,
                  smooth_position: bool = True,
                  smooth_thickness: bool = True,
                  smooth_strength: bool = False,
                  smooth_uv: bool = False):
    ''' Smooth selected strokes

    :param repeat: Repeat
    :type repeat: int
    :param factor: Factor
    :type factor: float
    :param only_selected: Selected Points, Smooth only selected points in the stroke
    :type only_selected: bool
    :param smooth_position: Position
    :type smooth_position: bool
    :param smooth_thickness: Thickness
    :type smooth_thickness: bool
    :param smooth_strength: Strength
    :type smooth_strength: bool
    :param smooth_uv: UV
    :type smooth_uv: bool
    '''

    pass


def stroke_split():
    ''' Split selected points as new stroke on same frame

    '''

    pass


def stroke_subdivide(number_cuts: int = 1,
                     factor: float = 0.0,
                     repeat: int = 1,
                     only_selected: bool = True,
                     smooth_position: bool = True,
                     smooth_thickness: bool = True,
                     smooth_strength: bool = False,
                     smooth_uv: bool = False):
    ''' Subdivide between continuous selected points of the stroke adding a point half way between them

    :param number_cuts: Number of Cuts
    :type number_cuts: int
    :param factor: Smooth
    :type factor: float
    :param repeat: Repeat
    :type repeat: int
    :param only_selected: Selected Points, Smooth only selected points in the stroke
    :type only_selected: bool
    :param smooth_position: Position
    :type smooth_position: bool
    :param smooth_thickness: Thickness
    :type smooth_thickness: bool
    :param smooth_strength: Strength
    :type smooth_strength: bool
    :param smooth_uv: UV
    :type smooth_uv: bool
    '''

    pass


def stroke_trim():
    ''' Trim selected stroke to first loop or intersection

    '''

    pass


def tint_flip():
    ''' Switch tint colors :file: startup/bl_ui/properties_grease_pencil_common.py\:872 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_ui/properties_grease_pencil_common.py$872> _

    '''

    pass


def trace_image(target: typing.Union[int, str] = 'NEW',
                thickness: int = 10,
                resolution: int = 5,
                scale: float = 1.0,
                sample: float = 0.0,
                threshold: float = 0.5,
                turnpolicy: typing.Union[int, str] = 'MINORITY',
                mode: typing.Union[int, str] = 'SINGLE'):
    ''' Extract Grease Pencil strokes from image

    :param target: Target Object, Target grease pencil
    :type target: typing.Union[int, str]
    :param thickness: Thickness
    :type thickness: int
    :param resolution: Resolution, Resolution of the generated curves
    :type resolution: int
    :param scale: Scale, Scale of the final stroke
    :type scale: float
    :param sample: Sample, Distance to sample points, zero to disable
    :type sample: float
    :param threshold: Color Threshold, Determine the lightness threshold above which strokes are generated
    :type threshold: float
    :param turnpolicy: Turn Policy, Determines how to resolve ambiguities during decomposition of bitmaps into paths * BLACK Black, Prefers to connect black (foreground) components. * WHITE White, Prefers to connect white (background) components. * LEFT Left, Always take a left turn. * RIGHT Right, Always take a right turn. * MINORITY Minority, Prefers to connect the color (black or white) that occurs least frequently in the local neighborhood of the current position. * MAJORITY Majority, Prefers to connect the color (black or white) that occurs most frequently in the local neighborhood of the current position. * RANDOM Random, Choose pseudo-randomly.
    :type turnpolicy: typing.Union[int, str]
    :param mode: Mode, Determines if trace simple image or full sequence * SINGLE Single, Trace the current frame of the image. * SEQUENCE Sequence, Trace full sequence.
    :type mode: typing.Union[int, str]
    '''

    pass


def transform_fill(mode: typing.Union[int, str] = 'ROTATE',
                   location: typing.List[float] = (0.0, 0.0),
                   rotation: float = 0.0,
                   scale: float = 0.0,
                   release_confirm: bool = False):
    ''' Transform grease pencil stroke fill

    :param mode: Mode
    :type mode: typing.Union[int, str]
    :param location: Location
    :type location: typing.List[float]
    :param rotation: Rotation
    :type rotation: float
    :param scale: Scale
    :type scale: float
    :param release_confirm: Confirm on Release
    :type release_confirm: bool
    '''

    pass


def unlock_all():
    ''' Unlock all Grease Pencil layers so that they can be edited

    '''

    pass


def vertex_color_brightness_contrast(mode: typing.Union[int, str] = 'BOTH',
                                     brightness: float = 0.0,
                                     contrast: float = 0.0):
    ''' Adjust vertex color brightness/contrast

    :param mode: Mode
    :type mode: typing.Union[int, str]
    :param brightness: Brightness
    :type brightness: float
    :param contrast: Contrast
    :type contrast: float
    '''

    pass


def vertex_color_hsv(mode: typing.Union[int, str] = 'BOTH',
                     h: float = 0.5,
                     s: float = 1.0,
                     v: float = 1.0):
    ''' Adjust vertex color HSV values

    :param mode: Mode
    :type mode: typing.Union[int, str]
    :param h: Hue
    :type h: float
    :param s: Saturation
    :type s: float
    :param v: Value
    :type v: float
    '''

    pass


def vertex_color_invert(mode: typing.Union[int, str] = 'BOTH'):
    ''' Invert RGB values

    :param mode: Mode
    :type mode: typing.Union[int, str]
    '''

    pass


def vertex_color_levels(mode: typing.Union[int, str] = 'BOTH',
                        offset: float = 0.0,
                        gain: float = 1.0):
    ''' Adjust levels of vertex colors

    :param mode: Mode
    :type mode: typing.Union[int, str]
    :param offset: Offset, Value to add to colors
    :type offset: float
    :param gain: Gain, Value to multiply colors by
    :type gain: float
    '''

    pass


def vertex_color_set(mode: typing.Union[int, str] = 'BOTH',
                     factor: float = 1.0):
    ''' Set active color to all selected vertex

    :param mode: Mode
    :type mode: typing.Union[int, str]
    :param factor: Factor, Mix Factor
    :type factor: float
    '''

    pass


def vertex_group_assign():
    ''' Assign the selected vertices to the active vertex group

    '''

    pass


def vertex_group_deselect():
    ''' Deselect all selected vertices assigned to the active vertex group

    '''

    pass


def vertex_group_invert():
    ''' Invert weights to the active vertex group

    '''

    pass


def vertex_group_normalize():
    ''' Normalize weights to the active vertex group

    '''

    pass


def vertex_group_normalize_all(lock_active: bool = True):
    ''' Normalize all weights of all vertex groups, so that for each vertex, the sum of all weights is 1.0

    :param lock_active: Lock Active, Keep the values of the active group while normalizing others
    :type lock_active: bool
    '''

    pass


def vertex_group_remove_from():
    ''' Remove the selected vertices from active or all vertex group(s)

    '''

    pass


def vertex_group_select():
    ''' Select all the vertices assigned to the active vertex group

    '''

    pass


def vertex_group_smooth(factor: float = 0.5, repeat: int = 1):
    ''' Smooth weights to the active vertex group

    :param factor: Factor
    :type factor: float
    :param repeat: Iterations
    :type repeat: int
    '''

    pass


def vertex_paint(stroke: typing.Union[
        typing.Dict[str, 'bpy.types.OperatorStrokeElement'], typing.
        List['bpy.types.OperatorStrokeElement'], 'bpy_prop_collection'] = None,
                 wait_for_input: bool = True):
    ''' Paint stroke points with a color

    :param stroke: Stroke
    :type stroke: typing.Union[typing.Dict[str, 'bpy.types.OperatorStrokeElement'], typing.List['bpy.types.OperatorStrokeElement'], 'bpy_prop_collection']
    :param wait_for_input: Wait for Input
    :type wait_for_input: bool
    '''

    pass


def vertexmode_toggle(back: bool = False):
    ''' Enter/Exit vertex paint mode for Grease Pencil strokes

    :param back: Return to Previous Mode, Return to previous mode
    :type back: bool
    '''

    pass


def weight_paint(stroke: typing.Union[
        typing.Dict[str, 'bpy.types.OperatorStrokeElement'], typing.
        List['bpy.types.OperatorStrokeElement'], 'bpy_prop_collection'] = None,
                 wait_for_input: bool = True):
    ''' Paint stroke points with a color

    :param stroke: Stroke
    :type stroke: typing.Union[typing.Dict[str, 'bpy.types.OperatorStrokeElement'], typing.List['bpy.types.OperatorStrokeElement'], 'bpy_prop_collection']
    :param wait_for_input: Wait for Input
    :type wait_for_input: bool
    '''

    pass


def weightmode_toggle(back: bool = False):
    ''' Enter/Exit weight paint mode for Grease Pencil strokes

    :param back: Return to Previous Mode, Return to previous mode
    :type back: bool
    '''

    pass
