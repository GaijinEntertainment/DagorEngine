import sys
import typing
import bpy.types


def brush_stroke(stroke: typing.Union[
        typing.Dict[str, 'bpy.types.OperatorStrokeElement'], typing.
        List['bpy.types.OperatorStrokeElement'], 'bpy_prop_collection'] = None,
                 mode: typing.Union[int, str] = 'NORMAL',
                 ignore_background_click: bool = False):
    ''' Sculpt a stroke into the geometry

    :param stroke: Stroke
    :type stroke: typing.Union[typing.Dict[str, 'bpy.types.OperatorStrokeElement'], typing.List['bpy.types.OperatorStrokeElement'], 'bpy_prop_collection']
    :param mode: Stroke Mode, Action taken when a paint stroke is made * NORMAL Regular, Apply brush normally. * INVERT Invert, Invert action of brush for duration of stroke. * SMOOTH Smooth, Switch brush to smooth mode for duration of stroke.
    :type mode: typing.Union[int, str]
    :param ignore_background_click: Ignore Background Click, Clicks on the background do not start the stroke
    :type ignore_background_click: bool
    '''

    pass


def cloth_filter(type: typing.Union[int, str] = 'GRAVITY',
                 strength: float = 1.0,
                 force_axis: typing.Union[typing.Set[int], typing.Set[str]] = {
                     'X', 'Y', 'Z'
                 },
                 orientation: typing.Union[int, str] = 'LOCAL',
                 cloth_mass: float = 1.0,
                 cloth_damping: float = 0.0,
                 use_face_sets: bool = False,
                 use_collisions: bool = False):
    ''' Applies a cloth simulation deformation to the entire mesh

    :param type: Filter Type, Operation that is going to be applied to the mesh * GRAVITY Gravity, Applies gravity to the simulation. * INFLATE Inflate, Inflates the cloth. * EXPAND Expand, Expands the cloth's dimensions. * PINCH Pinch, Pulls the cloth to the cursor's start position. * SCALE Scale, Scales the mesh as a soft body using the origin of the object as scale.
    :type type: typing.Union[int, str]
    :param strength: Strength, Filter strength
    :type strength: float
    :param force_axis: Force Axis, Apply the force in the selected axis * X X, Apply force in the X axis. * Y Y, Apply force in the Y axis. * Z Z, Apply force in the Z axis.
    :type force_axis: typing.Union[typing.Set[int], typing.Set[str]]
    :param orientation: Orientation, Orientation of the axis to limit the filter force * LOCAL Local, Use the local axis to limit the force and set the gravity direction. * WORLD World, Use the global axis to limit the force and set the gravity direction. * VIEW View, Use the view axis to limit the force and set the gravity direction.
    :type orientation: typing.Union[int, str]
    :param cloth_mass: Cloth Mass, Mass of each simulation particle
    :type cloth_mass: float
    :param cloth_damping: Cloth Damping, How much the applied forces are propagated through the cloth
    :type cloth_damping: float
    :param use_face_sets: Use Face Sets, Apply the filter only to the Face Set under the cursor
    :type use_face_sets: bool
    :param use_collisions: Use Collisions, Collide with other collider objects in the scene
    :type use_collisions: bool
    '''

    pass


def color_filter(type: typing.Union[int, str] = 'HUE',
                 strength: float = 1.0,
                 fill_color: typing.List[float] = (0.0, 0.0, 0.0)):
    ''' Applies a filter to modify the current sculpt vertex colors

    :param type: Filter Type * FILL Fill, Fill with a specific color. * HUE Hue, Change hue. * SATURATION Saturation, Change saturation. * VALUE Value, Change value. * BRIGTHNESS Brightness, Change brightness. * CONTRAST Contrast, Change contrast. * SMOOTH Smooth, Smooth colors. * RED Red, Change red channel. * GREEN Green, Change green channel. * BLUE Blue, Change blue channel.
    :type type: typing.Union[int, str]
    :param strength: Strength, Filter strength
    :type strength: float
    :param fill_color: Fill Color
    :type fill_color: typing.List[float]
    '''

    pass


def detail_flood_fill():
    ''' Flood fill the mesh with the selected detail setting

    '''

    pass


def dirty_mask(dirty_only: bool = False):
    ''' Generates a mask based on the geometry cavity and pointiness

    :param dirty_only: Dirty Only, Don't calculate cleans for convex areas
    :type dirty_only: bool
    '''

    pass


def dynamic_topology_toggle():
    ''' Dynamic topology alters the mesh topology while sculpting

    '''

    pass


def dyntopo_detail_size_edit():
    ''' Modify the constant detail size of dyntopo interactively

    '''

    pass


def face_set_box_gesture(xmin: int = 0,
                         xmax: int = 0,
                         ymin: int = 0,
                         ymax: int = 0,
                         wait_for_input: bool = True,
                         use_front_faces_only: bool = False,
                         use_limit_to_segment: bool = False):
    ''' Add face set within the box as you move the brush

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
    :param use_front_faces_only: Front Faces Only, Affect only faces facing towards the view
    :type use_front_faces_only: bool
    :param use_limit_to_segment: Limit to Segment, Apply the gesture action only to the area that is contained within the segment without extending its effect to the entire line
    :type use_limit_to_segment: bool
    '''

    pass


def face_set_change_visibility(mode: typing.Union[int, str] = 'TOGGLE'):
    ''' Change the visibility of the Face Sets of the sculpt

    :param mode: Mode * TOGGLE Toggle Visibility, Hide all Face Sets except for the active one. * SHOW_ACTIVE Show Active Face Set, Show Active Face Set. * HIDE_ACTIVE Hide Active Face Sets, Hide Active Face Sets. * INVERT Invert Face Set Visibility, Invert Face Set Visibility. * SHOW_ALL Show All Face Sets, Show All Face Sets.
    :type mode: typing.Union[int, str]
    '''

    pass


def face_set_edit(mode: typing.Union[int, str] = 'GROW',
                  modify_hidden: bool = True):
    ''' Edits the current active Face Set

    :param mode: Mode * GROW Grow Face Set, Grows the Face Sets boundary by one face based on mesh topology. * SHRINK Shrink Face Set, Shrinks the Face Sets boundary by one face based on mesh topology. * DELETE_GEOMETRY Delete Geometry, Deletes the faces that are assigned to the Face Set. * FAIR_POSITIONS Fair Positions, Creates a smooth as possible geometry patch from the Face Set minimizing changes in vertex positions. * FAIR_TANGENCY Fair Tangency, Creates a smooth as possible geometry patch from the Face Set minimizing changes in vertex tangents.
    :type mode: typing.Union[int, str]
    :param modify_hidden: Modify Hidden, Apply the edit operation to hidden Face Sets
    :type modify_hidden: bool
    '''

    pass


def face_set_lasso_gesture(path: typing.Union[
        typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.
        List['bpy.types.OperatorMousePath'], 'bpy_prop_collection'] = None,
                           use_front_faces_only: bool = False,
                           use_limit_to_segment: bool = False):
    ''' Add face set within the lasso as you move the brush

    :param path: Path
    :type path: typing.Union[typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.List['bpy.types.OperatorMousePath'], 'bpy_prop_collection']
    :param use_front_faces_only: Front Faces Only, Affect only faces facing towards the view
    :type use_front_faces_only: bool
    :param use_limit_to_segment: Limit to Segment, Apply the gesture action only to the area that is contained within the segment without extending its effect to the entire line
    :type use_limit_to_segment: bool
    '''

    pass


def face_sets_create(mode: typing.Union[int, str] = 'MASKED'):
    ''' Create a new Face Set

    :param mode: Mode * MASKED Face Set from Masked, Create a new Face Set from the masked faces. * VISIBLE Face Set from Visible, Create a new Face Set from the visible vertices. * ALL Face Set Full Mesh, Create an unique Face Set with all faces in the sculpt. * SELECTION Face Set from Edit Mode Selection, Create an Face Set corresponding to the Edit Mode face selection.
    :type mode: typing.Union[int, str]
    '''

    pass


def face_sets_init(mode: typing.Union[int, str] = 'LOOSE_PARTS',
                   threshold: float = 0.5):
    ''' Initializes all Face Sets in the mesh

    :param mode: Mode * LOOSE_PARTS Face Sets from Loose Parts, Create a Face Set per loose part in the mesh. * MATERIALS Face Sets from Material Slots, Create a Face Set per Material Slot. * NORMALS Face Sets from Mesh Normals, Create Face Sets for Faces that have similar normal. * UV_SEAMS Face Sets from UV Seams, Create Face Sets using UV Seams as boundaries. * CREASES Face Sets from Edge Creases, Create Face Sets using Edge Creases as boundaries. * BEVEL_WEIGHT Face Sets from Bevel Weight, Create Face Sets using Bevel Weights as boundaries. * SHARP_EDGES Face Sets from Sharp Edges, Create Face Sets using Sharp Edges as boundaries. * FACE_MAPS Face Sets from Face Maps, Create a Face Set per Face Map.
    :type mode: typing.Union[int, str]
    :param threshold: Threshold, Minimum value to consider a certain attribute a boundary when creating the Face Sets
    :type threshold: float
    '''

    pass


def face_sets_randomize_colors():
    ''' Generates a new set of random colors to render the Face Sets in the viewport

    '''

    pass


def loop_to_vertex_colors():
    ''' Copy the active loop color layer to the vertex color

    '''

    pass


def mask_by_color(contiguous: bool = False,
                  invert: bool = False,
                  preserve_previous_mask: bool = False,
                  threshold: float = 0.35):
    ''' Creates a mask based on the sculpt vertex colors

    :param contiguous: Contiguous, Mask only contiguous color areas
    :type contiguous: bool
    :param invert: Invert, Invert the generated mask
    :type invert: bool
    :param preserve_previous_mask: Preserve Previous Mask, Preserve the previous mask and add or subtract the new one generated by the colors
    :type preserve_previous_mask: bool
    :param threshold: Threshold, How much changes in color affect the mask generation
    :type threshold: float
    '''

    pass


def mask_expand(invert: bool = True,
                use_cursor: bool = True,
                update_pivot: bool = True,
                smooth_iterations: int = 2,
                mask_speed: int = 5,
                use_normals: bool = True,
                keep_previous_mask: bool = False,
                edge_sensitivity: int = 300,
                create_face_set: bool = False):
    ''' Expands a mask from the initial active vertex under the cursor

    :param invert: Invert, Invert the new mask
    :type invert: bool
    :param use_cursor: Use Cursor, Expand the mask to the cursor position
    :type use_cursor: bool
    :param update_pivot: Update Pivot Position, Set the pivot position to the mask border after creating the mask
    :type update_pivot: bool
    :param smooth_iterations: Smooth Iterations
    :type smooth_iterations: int
    :param mask_speed: Mask Speed
    :type mask_speed: int
    :param use_normals: Use Normals, Generate the mask using the normals and curvature of the model
    :type use_normals: bool
    :param keep_previous_mask: Keep Previous Mask, Generate the new mask on top of the current one
    :type keep_previous_mask: bool
    :param edge_sensitivity: Edge Detection Sensitivity, Sensitivity for expanding the mask across sculpted sharp edges when using normals to generate the mask
    :type edge_sensitivity: int
    :param create_face_set: Expand Face Mask, Expand a new Face Mask instead of the sculpt mask
    :type create_face_set: bool
    '''

    pass


def mask_filter(filter_type: typing.Union[int, str] = 'SMOOTH',
                iterations: int = 1,
                auto_iteration_count: bool = False):
    ''' Applies a filter to modify the current mask

    :param filter_type: Type, Filter that is going to be applied to the mask * SMOOTH Smooth Mask, Smooth mask. * SHARPEN Sharpen Mask, Sharpen mask. * GROW Grow Mask, Grow mask. * SHRINK Shrink Mask, Shrink mask. * CONTRAST_INCREASE Increase Contrast, Increase the contrast of the paint mask. * CONTRAST_DECREASE Decrease Contrast, Decrease the contrast of the paint mask.
    :type filter_type: typing.Union[int, str]
    :param iterations: Iterations, Number of times that the filter is going to be applied
    :type iterations: int
    :param auto_iteration_count: Auto Iteration Count, Use a automatic number of iterations based on the number of vertices of the sculpt
    :type auto_iteration_count: bool
    '''

    pass


def mesh_filter(type: typing.Union[int, str] = 'INFLATE',
                strength: float = 1.0,
                deform_axis: typing.Union[typing.Set[int], typing.Set[str]] = {
                    'X', 'Y', 'Z'
                },
                orientation: typing.Union[int, str] = 'LOCAL',
                surface_smooth_shape_preservation: float = 0.5,
                surface_smooth_current_vertex: float = 0.5,
                sharpen_smooth_ratio: float = 0.35,
                sharpen_intensify_detail_strength: float = 0.0,
                sharpen_curvature_smooth_iterations: int = 0):
    ''' Applies a filter to modify the current mesh

    :param type: Filter Type, Operation that is going to be applied to the mesh * SMOOTH Smooth, Smooth mesh. * SCALE Scale, Scale mesh. * INFLATE Inflate, Inflate mesh. * SPHERE Sphere, Morph into sphere. * RANDOM Random, Randomize vertex positions. * RELAX Relax, Relax mesh. * RELAX_FACE_SETS Relax Face Sets, Smooth the edges of all the Face Sets. * SURFACE_SMOOTH Surface Smooth, Smooth the surface of the mesh, preserving the volume. * SHARPEN Sharpen, Sharpen the cavities of the mesh. * ENHANCE_DETAILS Enhance Details, Enhance the high frequency surface detail. * ERASE_DISCPLACEMENT Erase Displacement, Deletes the displacement of the Multires Modifier.
    :type type: typing.Union[int, str]
    :param strength: Strength, Filter strength
    :type strength: float
    :param deform_axis: Deform Axis, Apply the deformation in the selected axis * X X, Deform in the X axis. * Y Y, Deform in the Y axis. * Z Z, Deform in the Z axis.
    :type deform_axis: typing.Union[typing.Set[int], typing.Set[str]]
    :param orientation: Orientation, Orientation of the axis to limit the filter displacement * LOCAL Local, Use the local axis to limit the displacement. * WORLD World, Use the global axis to limit the displacement. * VIEW View, Use the view axis to limit the displacement.
    :type orientation: typing.Union[int, str]
    :param surface_smooth_shape_preservation: Shape Preservation, How much of the original shape is preserved when smoothing
    :type surface_smooth_shape_preservation: float
    :param surface_smooth_current_vertex: Per Vertex Displacement, How much the position of each individual vertex influences the final result
    :type surface_smooth_current_vertex: float
    :param sharpen_smooth_ratio: Smooth Ratio, How much smoothing is applied to polished surfaces
    :type sharpen_smooth_ratio: float
    :param sharpen_intensify_detail_strength: Intensify Details, How much creases and valleys are intensified
    :type sharpen_intensify_detail_strength: float
    :param sharpen_curvature_smooth_iterations: Curvature Smooth Iterations, How much smooth the resulting shape is, ignoring high frequency details
    :type sharpen_curvature_smooth_iterations: int
    '''

    pass


def optimize():
    ''' Recalculate the sculpt BVH to improve performance

    '''

    pass


def project_line_gesture(xstart: int = 0,
                         xend: int = 0,
                         ystart: int = 0,
                         yend: int = 0,
                         flip: bool = False,
                         cursor: int = 5,
                         use_front_faces_only: bool = False,
                         use_limit_to_segment: bool = False):
    ''' Project the geometry onto a plane defined by a line

    :param xstart: X Start
    :type xstart: int
    :param xend: X End
    :type xend: int
    :param ystart: Y Start
    :type ystart: int
    :param yend: Y End
    :type yend: int
    :param flip: Flip
    :type flip: bool
    :param cursor: Cursor, Mouse cursor style to use during the modal operator
    :type cursor: int
    :param use_front_faces_only: Front Faces Only, Affect only faces facing towards the view
    :type use_front_faces_only: bool
    :param use_limit_to_segment: Limit to Segment, Apply the gesture action only to the area that is contained within the segment without extending its effect to the entire line
    :type use_limit_to_segment: bool
    '''

    pass


def sample_color():
    ''' Sample the vertex color of the active vertex

    '''

    pass


def sample_detail_size(location: typing.List[int] = (0, 0),
                       mode: typing.Union[int, str] = 'DYNTOPO'):
    ''' Sample the mesh detail on clicked point

    :param location: Location, Screen coordinates of sampling
    :type location: typing.List[int]
    :param mode: Detail Mode, Target sculpting workflow that is going to use the sampled size * DYNTOPO Dyntopo, Sample dyntopo detail. * VOXEL Voxel, Sample mesh voxel size.
    :type mode: typing.Union[int, str]
    '''

    pass


def sculptmode_toggle():
    ''' Toggle sculpt mode in 3D view

    '''

    pass


def set_detail_size():
    ''' Set the mesh detail (either relative or constant one, depending on current dyntopo mode)

    '''

    pass


def set_persistent_base():
    ''' Reset the copy of the mesh that is being sculpted on

    '''

    pass


def set_pivot_position(mode: typing.Union[int, str] = 'UNMASKED',
                       mouse_x: float = 0.0,
                       mouse_y: float = 0.0):
    ''' Sets the sculpt transform pivot position

    :param mode: Mode * ORIGIN Origin, Sets the pivot to the origin of the sculpt. * UNMASKED Unmasked, Sets the pivot position to the average position of the unmasked vertices. * BORDER Mask Border, Sets the pivot position to the center of the border of the mask. * ACTIVE Active Vertex, Sets the pivot position to the active vertex position. * SURFACE Surface, Sets the pivot position to the surface under the cursor.
    :type mode: typing.Union[int, str]
    :param mouse_x: Mouse Position X, Position of the mouse used for "Surface" mode
    :type mouse_x: float
    :param mouse_y: Mouse Position Y, Position of the mouse used for "Surface" mode
    :type mouse_y: float
    '''

    pass


def symmetrize(merge_tolerance: float = 0.001):
    ''' Symmetrize the topology modifications

    :param merge_tolerance: Merge Distance, Distance within which symmetrical vertices are merged
    :type merge_tolerance: float
    '''

    pass


def trim_box_gesture(xmin: int = 0,
                     xmax: int = 0,
                     ymin: int = 0,
                     ymax: int = 0,
                     wait_for_input: bool = True,
                     use_front_faces_only: bool = False,
                     use_limit_to_segment: bool = False,
                     trim_mode: typing.Union[int, str] = 'DIFFERENCE',
                     use_cursor_depth: bool = False,
                     trim_orientation: typing.Union[int, str] = 'VIEW'):
    ''' Trims the mesh within the box as you move the brush

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
    :param use_front_faces_only: Front Faces Only, Affect only faces facing towards the view
    :type use_front_faces_only: bool
    :param use_limit_to_segment: Limit to Segment, Apply the gesture action only to the area that is contained within the segment without extending its effect to the entire line
    :type use_limit_to_segment: bool
    :param trim_mode: Trim Mode * DIFFERENCE Difference, Use a difference boolean operation. * UNION Union, Use a union boolean operation. * JOIN Join, Join the new mesh as separate geometry, without performing any boolean operation.
    :type trim_mode: typing.Union[int, str]
    :param use_cursor_depth: Use Cursor for Depth, Use cursor location and radius for the dimensions and position of the trimming shape
    :type use_cursor_depth: bool
    :param trim_orientation: Shape Orientation * VIEW View, Use the view to orientate the trimming shape. * SURFACE Surface, Use the surface normal to orientate the trimming shape.
    :type trim_orientation: typing.Union[int, str]
    '''

    pass


def trim_lasso_gesture(path: typing.Union[
        typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.
        List['bpy.types.OperatorMousePath'], 'bpy_prop_collection'] = None,
                       use_front_faces_only: bool = False,
                       use_limit_to_segment: bool = False,
                       trim_mode: typing.Union[int, str] = 'DIFFERENCE',
                       use_cursor_depth: bool = False,
                       trim_orientation: typing.Union[int, str] = 'VIEW'):
    ''' Trims the mesh within the lasso as you move the brush

    :param path: Path
    :type path: typing.Union[typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.List['bpy.types.OperatorMousePath'], 'bpy_prop_collection']
    :param use_front_faces_only: Front Faces Only, Affect only faces facing towards the view
    :type use_front_faces_only: bool
    :param use_limit_to_segment: Limit to Segment, Apply the gesture action only to the area that is contained within the segment without extending its effect to the entire line
    :type use_limit_to_segment: bool
    :param trim_mode: Trim Mode * DIFFERENCE Difference, Use a difference boolean operation. * UNION Union, Use a union boolean operation. * JOIN Join, Join the new mesh as separate geometry, without performing any boolean operation.
    :type trim_mode: typing.Union[int, str]
    :param use_cursor_depth: Use Cursor for Depth, Use cursor location and radius for the dimensions and position of the trimming shape
    :type use_cursor_depth: bool
    :param trim_orientation: Shape Orientation * VIEW View, Use the view to orientate the trimming shape. * SURFACE Surface, Use the surface normal to orientate the trimming shape.
    :type trim_orientation: typing.Union[int, str]
    '''

    pass


def uv_sculpt_stroke(mode: typing.Union[int, str] = 'NORMAL'):
    ''' Sculpt UVs using a brush

    :param mode: Mode, Stroke Mode * NORMAL Regular, Apply brush normally. * INVERT Invert, Invert action of brush for duration of stroke. * RELAX Relax, Switch brush to relax mode for duration of stroke.
    :type mode: typing.Union[int, str]
    '''

    pass


def vertex_to_loop_colors():
    ''' Copy the Sculpt Vertex Color to a regular color layer

    '''

    pass
