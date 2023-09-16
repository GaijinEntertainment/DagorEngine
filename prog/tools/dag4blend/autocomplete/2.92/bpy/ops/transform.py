import sys
import typing


def bbone_resize(value: typing.List[float] = (1.0, 1.0, 1.0),
                 orient_type: typing.Union[int, str] = 'GLOBAL',
                 orient_matrix: typing.List[float] = ((0.0, 0.0,
                                                       0.0), (0.0, 0.0, 0.0),
                                                      (0.0, 0.0, 0.0)),
                 orient_matrix_type: typing.Union[int, str] = 'GLOBAL',
                 constraint_axis: typing.List[bool] = (False, False, False),
                 mirror: bool = False,
                 release_confirm: bool = False,
                 use_accurate: bool = False):
    ''' Scale selected bendy bones display size

    :param value: Display Size
    :type value: typing.List[float]
    :param orient_type: Orientation, Transformation orientation
    :type orient_type: typing.Union[int, str]
    :param orient_matrix: Matrix
    :type orient_matrix: typing.List[float]
    :param orient_matrix_type: Matrix Orientation
    :type orient_matrix_type: typing.Union[int, str]
    :param constraint_axis: Constraint Axis
    :type constraint_axis: typing.List[bool]
    :param mirror: Mirror Editing
    :type mirror: bool
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def bend(value: typing.List[float] = (0.0),
         mirror: bool = False,
         use_proportional_edit: bool = False,
         proportional_edit_falloff: typing.Union[int, str] = 'SMOOTH',
         proportional_size: float = 1.0,
         use_proportional_connected: bool = False,
         use_proportional_projected: bool = False,
         snap: bool = False,
         snap_target: typing.Union[int, str] = 'CLOSEST',
         snap_point: typing.List[float] = (0.0, 0.0, 0.0),
         snap_align: bool = False,
         snap_normal: typing.List[float] = (0.0, 0.0, 0.0),
         gpencil_strokes: bool = False,
         center_override: typing.List[float] = (0.0, 0.0, 0.0),
         release_confirm: bool = False,
         use_accurate: bool = False):
    ''' Bend selected items between the 3D cursor and the mouse

    :param value: Angle
    :type value: typing.List[float]
    :param mirror: Mirror Editing
    :type mirror: bool
    :param use_proportional_edit: Proportional Editing
    :type use_proportional_edit: bool
    :param proportional_edit_falloff: Proportional Falloff, Falloff type for proportional editing mode * SMOOTH Smooth, Smooth falloff. * SPHERE Sphere, Spherical falloff. * ROOT Root, Root falloff. * INVERSE_SQUARE Inverse Square, Inverse Square falloff. * SHARP Sharp, Sharp falloff. * LINEAR Linear, Linear falloff. * CONSTANT Constant, Constant falloff. * RANDOM Random, Random falloff.
    :type proportional_edit_falloff: typing.Union[int, str]
    :param proportional_size: Proportional Size
    :type proportional_size: float
    :param use_proportional_connected: Connected
    :type use_proportional_connected: bool
    :param use_proportional_projected: Projected (2D)
    :type use_proportional_projected: bool
    :param snap: Use Snapping Options
    :type snap: bool
    :param snap_target: Target * CLOSEST Closest, Snap closest point onto target. * CENTER Center, Snap transformation center onto target. * MEDIAN Median, Snap median onto target. * ACTIVE Active, Snap active onto target.
    :type snap_target: typing.Union[int, str]
    :param snap_point: Point
    :type snap_point: typing.List[float]
    :param snap_align: Align with Point Normal
    :type snap_align: bool
    :param snap_normal: Normal
    :type snap_normal: typing.List[float]
    :param gpencil_strokes: Edit Grease Pencil, Edit selected Grease Pencil strokes
    :type gpencil_strokes: bool
    :param center_override: Center Override, Force using this center value (when set)
    :type center_override: typing.List[float]
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def create_orientation(name: str = "",
                       use_view: bool = False,
                       use: bool = False,
                       overwrite: bool = False):
    ''' Create transformation orientation from selection

    :param name: Name, Name of the new custom orientation
    :type name: str
    :param use_view: Use View, Use the current view instead of the active object to create the new orientation
    :type use_view: bool
    :param use: Use After Creation, Select orientation after its creation
    :type use: bool
    :param overwrite: Overwrite Previous, Overwrite previously created orientation with same name
    :type overwrite: bool
    '''

    pass


def delete_orientation():
    ''' Delete transformation orientation

    '''

    pass


def edge_bevelweight(value: float = 0.0,
                     snap: bool = False,
                     snap_target: typing.Union[int, str] = 'CLOSEST',
                     snap_point: typing.List[float] = (0.0, 0.0, 0.0),
                     snap_align: bool = False,
                     snap_normal: typing.List[float] = (0.0, 0.0, 0.0),
                     release_confirm: bool = False,
                     use_accurate: bool = False):
    ''' Change the bevel weight of edges

    :param value: Factor
    :type value: float
    :param snap: Use Snapping Options
    :type snap: bool
    :param snap_target: Target * CLOSEST Closest, Snap closest point onto target. * CENTER Center, Snap transformation center onto target. * MEDIAN Median, Snap median onto target. * ACTIVE Active, Snap active onto target.
    :type snap_target: typing.Union[int, str]
    :param snap_point: Point
    :type snap_point: typing.List[float]
    :param snap_align: Align with Point Normal
    :type snap_align: bool
    :param snap_normal: Normal
    :type snap_normal: typing.List[float]
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def edge_crease(value: float = 0.0,
                snap: bool = False,
                snap_target: typing.Union[int, str] = 'CLOSEST',
                snap_point: typing.List[float] = (0.0, 0.0, 0.0),
                snap_align: bool = False,
                snap_normal: typing.List[float] = (0.0, 0.0, 0.0),
                release_confirm: bool = False,
                use_accurate: bool = False):
    ''' Change the crease of edges

    :param value: Factor
    :type value: float
    :param snap: Use Snapping Options
    :type snap: bool
    :param snap_target: Target * CLOSEST Closest, Snap closest point onto target. * CENTER Center, Snap transformation center onto target. * MEDIAN Median, Snap median onto target. * ACTIVE Active, Snap active onto target.
    :type snap_target: typing.Union[int, str]
    :param snap_point: Point
    :type snap_point: typing.List[float]
    :param snap_align: Align with Point Normal
    :type snap_align: bool
    :param snap_normal: Normal
    :type snap_normal: typing.List[float]
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def edge_slide(value: float = 0.0,
               single_side: bool = False,
               use_even: bool = False,
               flipped: bool = False,
               use_clamp: bool = True,
               mirror: bool = False,
               snap: bool = False,
               snap_target: typing.Union[int, str] = 'CLOSEST',
               snap_point: typing.List[float] = (0.0, 0.0, 0.0),
               snap_align: bool = False,
               snap_normal: typing.List[float] = (0.0, 0.0, 0.0),
               correct_uv: bool = True,
               release_confirm: bool = False,
               use_accurate: bool = False):
    ''' Slide an edge loop along a mesh

    :param value: Factor
    :type value: float
    :param single_side: Single Side
    :type single_side: bool
    :param use_even: Even, Make the edge loop match the shape of the adjacent edge loop
    :type use_even: bool
    :param flipped: Flipped, When Even mode is active, flips between the two adjacent edge loops
    :type flipped: bool
    :param use_clamp: Clamp, Clamp within the edge extents
    :type use_clamp: bool
    :param mirror: Mirror Editing
    :type mirror: bool
    :param snap: Use Snapping Options
    :type snap: bool
    :param snap_target: Target * CLOSEST Closest, Snap closest point onto target. * CENTER Center, Snap transformation center onto target. * MEDIAN Median, Snap median onto target. * ACTIVE Active, Snap active onto target.
    :type snap_target: typing.Union[int, str]
    :param snap_point: Point
    :type snap_point: typing.List[float]
    :param snap_align: Align with Point Normal
    :type snap_align: bool
    :param snap_normal: Normal
    :type snap_normal: typing.List[float]
    :param correct_uv: Correct UVs, Correct UV coordinates when transforming
    :type correct_uv: bool
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def from_gizmo():
    ''' Transform selected items by mode type

    '''

    pass


def mirror(orient_type: typing.Union[int, str] = 'GLOBAL',
           orient_matrix: typing.List[float] = ((0.0, 0.0, 0.0),
                                                (0.0, 0.0, 0.0), (0.0, 0.0,
                                                                  0.0)),
           orient_matrix_type: typing.Union[int, str] = 'GLOBAL',
           constraint_axis: typing.List[bool] = (False, False, False),
           gpencil_strokes: bool = False,
           center_override: typing.List[float] = (0.0, 0.0, 0.0),
           release_confirm: bool = False,
           use_accurate: bool = False):
    ''' Mirror selected items around one or more axes

    :param orient_type: Orientation, Transformation orientation
    :type orient_type: typing.Union[int, str]
    :param orient_matrix: Matrix
    :type orient_matrix: typing.List[float]
    :param orient_matrix_type: Matrix Orientation
    :type orient_matrix_type: typing.Union[int, str]
    :param constraint_axis: Constraint Axis
    :type constraint_axis: typing.List[bool]
    :param gpencil_strokes: Edit Grease Pencil, Edit selected Grease Pencil strokes
    :type gpencil_strokes: bool
    :param center_override: Center Override, Force using this center value (when set)
    :type center_override: typing.List[float]
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def push_pull(value: float = 0.0,
              mirror: bool = False,
              use_proportional_edit: bool = False,
              proportional_edit_falloff: typing.Union[int, str] = 'SMOOTH',
              proportional_size: float = 1.0,
              use_proportional_connected: bool = False,
              use_proportional_projected: bool = False,
              snap: bool = False,
              snap_target: typing.Union[int, str] = 'CLOSEST',
              snap_point: typing.List[float] = (0.0, 0.0, 0.0),
              snap_align: bool = False,
              snap_normal: typing.List[float] = (0.0, 0.0, 0.0),
              center_override: typing.List[float] = (0.0, 0.0, 0.0),
              release_confirm: bool = False,
              use_accurate: bool = False):
    ''' Push/Pull selected items

    :param value: Distance
    :type value: float
    :param mirror: Mirror Editing
    :type mirror: bool
    :param use_proportional_edit: Proportional Editing
    :type use_proportional_edit: bool
    :param proportional_edit_falloff: Proportional Falloff, Falloff type for proportional editing mode * SMOOTH Smooth, Smooth falloff. * SPHERE Sphere, Spherical falloff. * ROOT Root, Root falloff. * INVERSE_SQUARE Inverse Square, Inverse Square falloff. * SHARP Sharp, Sharp falloff. * LINEAR Linear, Linear falloff. * CONSTANT Constant, Constant falloff. * RANDOM Random, Random falloff.
    :type proportional_edit_falloff: typing.Union[int, str]
    :param proportional_size: Proportional Size
    :type proportional_size: float
    :param use_proportional_connected: Connected
    :type use_proportional_connected: bool
    :param use_proportional_projected: Projected (2D)
    :type use_proportional_projected: bool
    :param snap: Use Snapping Options
    :type snap: bool
    :param snap_target: Target * CLOSEST Closest, Snap closest point onto target. * CENTER Center, Snap transformation center onto target. * MEDIAN Median, Snap median onto target. * ACTIVE Active, Snap active onto target.
    :type snap_target: typing.Union[int, str]
    :param snap_point: Point
    :type snap_point: typing.List[float]
    :param snap_align: Align with Point Normal
    :type snap_align: bool
    :param snap_normal: Normal
    :type snap_normal: typing.List[float]
    :param center_override: Center Override, Force using this center value (when set)
    :type center_override: typing.List[float]
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def resize(value: typing.List[float] = (1.0, 1.0, 1.0),
           orient_type: typing.Union[int, str] = 'GLOBAL',
           orient_matrix: typing.List[float] = ((0.0, 0.0, 0.0),
                                                (0.0, 0.0, 0.0), (0.0, 0.0,
                                                                  0.0)),
           orient_matrix_type: typing.Union[int, str] = 'GLOBAL',
           constraint_axis: typing.List[bool] = (False, False, False),
           mirror: bool = False,
           use_proportional_edit: bool = False,
           proportional_edit_falloff: typing.Union[int, str] = 'SMOOTH',
           proportional_size: float = 1.0,
           use_proportional_connected: bool = False,
           use_proportional_projected: bool = False,
           snap: bool = False,
           snap_target: typing.Union[int, str] = 'CLOSEST',
           snap_point: typing.List[float] = (0.0, 0.0, 0.0),
           snap_align: bool = False,
           snap_normal: typing.List[float] = (0.0, 0.0, 0.0),
           gpencil_strokes: bool = False,
           texture_space: bool = False,
           remove_on_cancel: bool = False,
           center_override: typing.List[float] = (0.0, 0.0, 0.0),
           release_confirm: bool = False,
           use_accurate: bool = False):
    ''' Scale (resize) selected items

    :param value: Scale
    :type value: typing.List[float]
    :param orient_type: Orientation, Transformation orientation
    :type orient_type: typing.Union[int, str]
    :param orient_matrix: Matrix
    :type orient_matrix: typing.List[float]
    :param orient_matrix_type: Matrix Orientation
    :type orient_matrix_type: typing.Union[int, str]
    :param constraint_axis: Constraint Axis
    :type constraint_axis: typing.List[bool]
    :param mirror: Mirror Editing
    :type mirror: bool
    :param use_proportional_edit: Proportional Editing
    :type use_proportional_edit: bool
    :param proportional_edit_falloff: Proportional Falloff, Falloff type for proportional editing mode * SMOOTH Smooth, Smooth falloff. * SPHERE Sphere, Spherical falloff. * ROOT Root, Root falloff. * INVERSE_SQUARE Inverse Square, Inverse Square falloff. * SHARP Sharp, Sharp falloff. * LINEAR Linear, Linear falloff. * CONSTANT Constant, Constant falloff. * RANDOM Random, Random falloff.
    :type proportional_edit_falloff: typing.Union[int, str]
    :param proportional_size: Proportional Size
    :type proportional_size: float
    :param use_proportional_connected: Connected
    :type use_proportional_connected: bool
    :param use_proportional_projected: Projected (2D)
    :type use_proportional_projected: bool
    :param snap: Use Snapping Options
    :type snap: bool
    :param snap_target: Target * CLOSEST Closest, Snap closest point onto target. * CENTER Center, Snap transformation center onto target. * MEDIAN Median, Snap median onto target. * ACTIVE Active, Snap active onto target.
    :type snap_target: typing.Union[int, str]
    :param snap_point: Point
    :type snap_point: typing.List[float]
    :param snap_align: Align with Point Normal
    :type snap_align: bool
    :param snap_normal: Normal
    :type snap_normal: typing.List[float]
    :param gpencil_strokes: Edit Grease Pencil, Edit selected Grease Pencil strokes
    :type gpencil_strokes: bool
    :param texture_space: Edit Texture Space, Edit object data texture space
    :type texture_space: bool
    :param remove_on_cancel: Remove on Cancel, Remove elements on cancel
    :type remove_on_cancel: bool
    :param center_override: Center Override, Force using this center value (when set)
    :type center_override: typing.List[float]
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def rotate(value: float = 0.0,
           orient_axis: typing.Union[int, str] = 'Z',
           orient_type: typing.Union[int, str] = 'GLOBAL',
           orient_matrix: typing.List[float] = ((0.0, 0.0, 0.0),
                                                (0.0, 0.0, 0.0), (0.0, 0.0,
                                                                  0.0)),
           orient_matrix_type: typing.Union[int, str] = 'GLOBAL',
           constraint_axis: typing.List[bool] = (False, False, False),
           mirror: bool = False,
           use_proportional_edit: bool = False,
           proportional_edit_falloff: typing.Union[int, str] = 'SMOOTH',
           proportional_size: float = 1.0,
           use_proportional_connected: bool = False,
           use_proportional_projected: bool = False,
           snap: bool = False,
           snap_target: typing.Union[int, str] = 'CLOSEST',
           snap_point: typing.List[float] = (0.0, 0.0, 0.0),
           snap_align: bool = False,
           snap_normal: typing.List[float] = (0.0, 0.0, 0.0),
           gpencil_strokes: bool = False,
           center_override: typing.List[float] = (0.0, 0.0, 0.0),
           release_confirm: bool = False,
           use_accurate: bool = False):
    ''' Rotate selected items

    :param value: Angle
    :type value: float
    :param orient_axis: Axis
    :type orient_axis: typing.Union[int, str]
    :param orient_type: Orientation, Transformation orientation
    :type orient_type: typing.Union[int, str]
    :param orient_matrix: Matrix
    :type orient_matrix: typing.List[float]
    :param orient_matrix_type: Matrix Orientation
    :type orient_matrix_type: typing.Union[int, str]
    :param constraint_axis: Constraint Axis
    :type constraint_axis: typing.List[bool]
    :param mirror: Mirror Editing
    :type mirror: bool
    :param use_proportional_edit: Proportional Editing
    :type use_proportional_edit: bool
    :param proportional_edit_falloff: Proportional Falloff, Falloff type for proportional editing mode * SMOOTH Smooth, Smooth falloff. * SPHERE Sphere, Spherical falloff. * ROOT Root, Root falloff. * INVERSE_SQUARE Inverse Square, Inverse Square falloff. * SHARP Sharp, Sharp falloff. * LINEAR Linear, Linear falloff. * CONSTANT Constant, Constant falloff. * RANDOM Random, Random falloff.
    :type proportional_edit_falloff: typing.Union[int, str]
    :param proportional_size: Proportional Size
    :type proportional_size: float
    :param use_proportional_connected: Connected
    :type use_proportional_connected: bool
    :param use_proportional_projected: Projected (2D)
    :type use_proportional_projected: bool
    :param snap: Use Snapping Options
    :type snap: bool
    :param snap_target: Target * CLOSEST Closest, Snap closest point onto target. * CENTER Center, Snap transformation center onto target. * MEDIAN Median, Snap median onto target. * ACTIVE Active, Snap active onto target.
    :type snap_target: typing.Union[int, str]
    :param snap_point: Point
    :type snap_point: typing.List[float]
    :param snap_align: Align with Point Normal
    :type snap_align: bool
    :param snap_normal: Normal
    :type snap_normal: typing.List[float]
    :param gpencil_strokes: Edit Grease Pencil, Edit selected Grease Pencil strokes
    :type gpencil_strokes: bool
    :param center_override: Center Override, Force using this center value (when set)
    :type center_override: typing.List[float]
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def rotate_normal(value: float = 0.0,
                  orient_axis: typing.Union[int, str] = 'Z',
                  orient_type: typing.Union[int, str] = 'GLOBAL',
                  orient_matrix: typing.List[float] = ((0.0, 0.0,
                                                        0.0), (0.0, 0.0, 0.0),
                                                       (0.0, 0.0, 0.0)),
                  orient_matrix_type: typing.Union[int, str] = 'GLOBAL',
                  constraint_axis: typing.List[bool] = (False, False, False),
                  mirror: bool = False,
                  release_confirm: bool = False,
                  use_accurate: bool = False):
    ''' Rotate split normal of selected items

    :param value: Angle
    :type value: float
    :param orient_axis: Axis
    :type orient_axis: typing.Union[int, str]
    :param orient_type: Orientation, Transformation orientation
    :type orient_type: typing.Union[int, str]
    :param orient_matrix: Matrix
    :type orient_matrix: typing.List[float]
    :param orient_matrix_type: Matrix Orientation
    :type orient_matrix_type: typing.Union[int, str]
    :param constraint_axis: Constraint Axis
    :type constraint_axis: typing.List[bool]
    :param mirror: Mirror Editing
    :type mirror: bool
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def select_orientation(orientation: typing.Union[int, str] = 'GLOBAL'):
    ''' Select transformation orientation

    :param orientation: Orientation, Transformation orientation
    :type orientation: typing.Union[int, str]
    '''

    pass


def seq_slide(value: typing.List[float] = (0.0, 0.0),
              snap: bool = False,
              snap_target: typing.Union[int, str] = 'CLOSEST',
              snap_point: typing.List[float] = (0.0, 0.0, 0.0),
              snap_align: bool = False,
              snap_normal: typing.List[float] = (0.0, 0.0, 0.0),
              release_confirm: bool = False,
              use_accurate: bool = False):
    ''' Slide a sequence strip in time

    :param value: Offset
    :type value: typing.List[float]
    :param snap: Use Snapping Options
    :type snap: bool
    :param snap_target: Target * CLOSEST Closest, Snap closest point onto target. * CENTER Center, Snap transformation center onto target. * MEDIAN Median, Snap median onto target. * ACTIVE Active, Snap active onto target.
    :type snap_target: typing.Union[int, str]
    :param snap_point: Point
    :type snap_point: typing.List[float]
    :param snap_align: Align with Point Normal
    :type snap_align: bool
    :param snap_normal: Normal
    :type snap_normal: typing.List[float]
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def shear(value: float = 0.0,
          orient_axis: typing.Union[int, str] = 'Z',
          orient_axis_ortho: typing.Union[int, str] = 'X',
          orient_type: typing.Union[int, str] = 'GLOBAL',
          orient_matrix: typing.List[float] = ((0.0, 0.0, 0.0),
                                               (0.0, 0.0, 0.0), (0.0, 0.0,
                                                                 0.0)),
          orient_matrix_type: typing.Union[int, str] = 'GLOBAL',
          mirror: bool = False,
          use_proportional_edit: bool = False,
          proportional_edit_falloff: typing.Union[int, str] = 'SMOOTH',
          proportional_size: float = 1.0,
          use_proportional_connected: bool = False,
          use_proportional_projected: bool = False,
          snap: bool = False,
          snap_target: typing.Union[int, str] = 'CLOSEST',
          snap_point: typing.List[float] = (0.0, 0.0, 0.0),
          snap_align: bool = False,
          snap_normal: typing.List[float] = (0.0, 0.0, 0.0),
          gpencil_strokes: bool = False,
          release_confirm: bool = False,
          use_accurate: bool = False):
    ''' Shear selected items along the horizontal screen axis

    :param value: Offset
    :type value: float
    :param orient_axis: Axis
    :type orient_axis: typing.Union[int, str]
    :param orient_axis_ortho: Axis Ortho
    :type orient_axis_ortho: typing.Union[int, str]
    :param orient_type: Orientation, Transformation orientation
    :type orient_type: typing.Union[int, str]
    :param orient_matrix: Matrix
    :type orient_matrix: typing.List[float]
    :param orient_matrix_type: Matrix Orientation
    :type orient_matrix_type: typing.Union[int, str]
    :param mirror: Mirror Editing
    :type mirror: bool
    :param use_proportional_edit: Proportional Editing
    :type use_proportional_edit: bool
    :param proportional_edit_falloff: Proportional Falloff, Falloff type for proportional editing mode * SMOOTH Smooth, Smooth falloff. * SPHERE Sphere, Spherical falloff. * ROOT Root, Root falloff. * INVERSE_SQUARE Inverse Square, Inverse Square falloff. * SHARP Sharp, Sharp falloff. * LINEAR Linear, Linear falloff. * CONSTANT Constant, Constant falloff. * RANDOM Random, Random falloff.
    :type proportional_edit_falloff: typing.Union[int, str]
    :param proportional_size: Proportional Size
    :type proportional_size: float
    :param use_proportional_connected: Connected
    :type use_proportional_connected: bool
    :param use_proportional_projected: Projected (2D)
    :type use_proportional_projected: bool
    :param snap: Use Snapping Options
    :type snap: bool
    :param snap_target: Target * CLOSEST Closest, Snap closest point onto target. * CENTER Center, Snap transformation center onto target. * MEDIAN Median, Snap median onto target. * ACTIVE Active, Snap active onto target.
    :type snap_target: typing.Union[int, str]
    :param snap_point: Point
    :type snap_point: typing.List[float]
    :param snap_align: Align with Point Normal
    :type snap_align: bool
    :param snap_normal: Normal
    :type snap_normal: typing.List[float]
    :param gpencil_strokes: Edit Grease Pencil, Edit selected Grease Pencil strokes
    :type gpencil_strokes: bool
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def shrink_fatten(value: float = 0.0,
                  use_even_offset: bool = False,
                  mirror: bool = False,
                  use_proportional_edit: bool = False,
                  proportional_edit_falloff: typing.Union[int, str] = 'SMOOTH',
                  proportional_size: float = 1.0,
                  use_proportional_connected: bool = False,
                  use_proportional_projected: bool = False,
                  snap: bool = False,
                  snap_target: typing.Union[int, str] = 'CLOSEST',
                  snap_point: typing.List[float] = (0.0, 0.0, 0.0),
                  snap_align: bool = False,
                  snap_normal: typing.List[float] = (0.0, 0.0, 0.0),
                  release_confirm: bool = False,
                  use_accurate: bool = False):
    ''' Shrink/fatten selected vertices along normals

    :param value: Offset
    :type value: float
    :param use_even_offset: Offset Even, Scale the offset to give more even thickness
    :type use_even_offset: bool
    :param mirror: Mirror Editing
    :type mirror: bool
    :param use_proportional_edit: Proportional Editing
    :type use_proportional_edit: bool
    :param proportional_edit_falloff: Proportional Falloff, Falloff type for proportional editing mode * SMOOTH Smooth, Smooth falloff. * SPHERE Sphere, Spherical falloff. * ROOT Root, Root falloff. * INVERSE_SQUARE Inverse Square, Inverse Square falloff. * SHARP Sharp, Sharp falloff. * LINEAR Linear, Linear falloff. * CONSTANT Constant, Constant falloff. * RANDOM Random, Random falloff.
    :type proportional_edit_falloff: typing.Union[int, str]
    :param proportional_size: Proportional Size
    :type proportional_size: float
    :param use_proportional_connected: Connected
    :type use_proportional_connected: bool
    :param use_proportional_projected: Projected (2D)
    :type use_proportional_projected: bool
    :param snap: Use Snapping Options
    :type snap: bool
    :param snap_target: Target * CLOSEST Closest, Snap closest point onto target. * CENTER Center, Snap transformation center onto target. * MEDIAN Median, Snap median onto target. * ACTIVE Active, Snap active onto target.
    :type snap_target: typing.Union[int, str]
    :param snap_point: Point
    :type snap_point: typing.List[float]
    :param snap_align: Align with Point Normal
    :type snap_align: bool
    :param snap_normal: Normal
    :type snap_normal: typing.List[float]
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def skin_resize(value: typing.List[float] = (1.0, 1.0, 1.0),
                orient_type: typing.Union[int, str] = 'GLOBAL',
                orient_matrix: typing.List[float] = ((0.0, 0.0,
                                                      0.0), (0.0, 0.0, 0.0),
                                                     (0.0, 0.0, 0.0)),
                orient_matrix_type: typing.Union[int, str] = 'GLOBAL',
                constraint_axis: typing.List[bool] = (False, False, False),
                mirror: bool = False,
                use_proportional_edit: bool = False,
                proportional_edit_falloff: typing.Union[int, str] = 'SMOOTH',
                proportional_size: float = 1.0,
                use_proportional_connected: bool = False,
                use_proportional_projected: bool = False,
                snap: bool = False,
                snap_target: typing.Union[int, str] = 'CLOSEST',
                snap_point: typing.List[float] = (0.0, 0.0, 0.0),
                snap_align: bool = False,
                snap_normal: typing.List[float] = (0.0, 0.0, 0.0),
                release_confirm: bool = False,
                use_accurate: bool = False):
    ''' Scale selected vertices' skin radii

    :param value: Scale
    :type value: typing.List[float]
    :param orient_type: Orientation, Transformation orientation
    :type orient_type: typing.Union[int, str]
    :param orient_matrix: Matrix
    :type orient_matrix: typing.List[float]
    :param orient_matrix_type: Matrix Orientation
    :type orient_matrix_type: typing.Union[int, str]
    :param constraint_axis: Constraint Axis
    :type constraint_axis: typing.List[bool]
    :param mirror: Mirror Editing
    :type mirror: bool
    :param use_proportional_edit: Proportional Editing
    :type use_proportional_edit: bool
    :param proportional_edit_falloff: Proportional Falloff, Falloff type for proportional editing mode * SMOOTH Smooth, Smooth falloff. * SPHERE Sphere, Spherical falloff. * ROOT Root, Root falloff. * INVERSE_SQUARE Inverse Square, Inverse Square falloff. * SHARP Sharp, Sharp falloff. * LINEAR Linear, Linear falloff. * CONSTANT Constant, Constant falloff. * RANDOM Random, Random falloff.
    :type proportional_edit_falloff: typing.Union[int, str]
    :param proportional_size: Proportional Size
    :type proportional_size: float
    :param use_proportional_connected: Connected
    :type use_proportional_connected: bool
    :param use_proportional_projected: Projected (2D)
    :type use_proportional_projected: bool
    :param snap: Use Snapping Options
    :type snap: bool
    :param snap_target: Target * CLOSEST Closest, Snap closest point onto target. * CENTER Center, Snap transformation center onto target. * MEDIAN Median, Snap median onto target. * ACTIVE Active, Snap active onto target.
    :type snap_target: typing.Union[int, str]
    :param snap_point: Point
    :type snap_point: typing.List[float]
    :param snap_align: Align with Point Normal
    :type snap_align: bool
    :param snap_normal: Normal
    :type snap_normal: typing.List[float]
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def tilt(value: float = 0.0,
         mirror: bool = False,
         use_proportional_edit: bool = False,
         proportional_edit_falloff: typing.Union[int, str] = 'SMOOTH',
         proportional_size: float = 1.0,
         use_proportional_connected: bool = False,
         use_proportional_projected: bool = False,
         snap: bool = False,
         snap_target: typing.Union[int, str] = 'CLOSEST',
         snap_point: typing.List[float] = (0.0, 0.0, 0.0),
         snap_align: bool = False,
         snap_normal: typing.List[float] = (0.0, 0.0, 0.0),
         release_confirm: bool = False,
         use_accurate: bool = False):
    ''' Tilt selected control vertices of 3D curve

    :param value: Angle
    :type value: float
    :param mirror: Mirror Editing
    :type mirror: bool
    :param use_proportional_edit: Proportional Editing
    :type use_proportional_edit: bool
    :param proportional_edit_falloff: Proportional Falloff, Falloff type for proportional editing mode * SMOOTH Smooth, Smooth falloff. * SPHERE Sphere, Spherical falloff. * ROOT Root, Root falloff. * INVERSE_SQUARE Inverse Square, Inverse Square falloff. * SHARP Sharp, Sharp falloff. * LINEAR Linear, Linear falloff. * CONSTANT Constant, Constant falloff. * RANDOM Random, Random falloff.
    :type proportional_edit_falloff: typing.Union[int, str]
    :param proportional_size: Proportional Size
    :type proportional_size: float
    :param use_proportional_connected: Connected
    :type use_proportional_connected: bool
    :param use_proportional_projected: Projected (2D)
    :type use_proportional_projected: bool
    :param snap: Use Snapping Options
    :type snap: bool
    :param snap_target: Target * CLOSEST Closest, Snap closest point onto target. * CENTER Center, Snap transformation center onto target. * MEDIAN Median, Snap median onto target. * ACTIVE Active, Snap active onto target.
    :type snap_target: typing.Union[int, str]
    :param snap_point: Point
    :type snap_point: typing.List[float]
    :param snap_align: Align with Point Normal
    :type snap_align: bool
    :param snap_normal: Normal
    :type snap_normal: typing.List[float]
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def tosphere(value: float = 0.0,
             mirror: bool = False,
             use_proportional_edit: bool = False,
             proportional_edit_falloff: typing.Union[int, str] = 'SMOOTH',
             proportional_size: float = 1.0,
             use_proportional_connected: bool = False,
             use_proportional_projected: bool = False,
             snap: bool = False,
             snap_target: typing.Union[int, str] = 'CLOSEST',
             snap_point: typing.List[float] = (0.0, 0.0, 0.0),
             snap_align: bool = False,
             snap_normal: typing.List[float] = (0.0, 0.0, 0.0),
             gpencil_strokes: bool = False,
             center_override: typing.List[float] = (0.0, 0.0, 0.0),
             release_confirm: bool = False,
             use_accurate: bool = False):
    ''' Move selected items outward in a spherical shape around geometric center

    :param value: Factor
    :type value: float
    :param mirror: Mirror Editing
    :type mirror: bool
    :param use_proportional_edit: Proportional Editing
    :type use_proportional_edit: bool
    :param proportional_edit_falloff: Proportional Falloff, Falloff type for proportional editing mode * SMOOTH Smooth, Smooth falloff. * SPHERE Sphere, Spherical falloff. * ROOT Root, Root falloff. * INVERSE_SQUARE Inverse Square, Inverse Square falloff. * SHARP Sharp, Sharp falloff. * LINEAR Linear, Linear falloff. * CONSTANT Constant, Constant falloff. * RANDOM Random, Random falloff.
    :type proportional_edit_falloff: typing.Union[int, str]
    :param proportional_size: Proportional Size
    :type proportional_size: float
    :param use_proportional_connected: Connected
    :type use_proportional_connected: bool
    :param use_proportional_projected: Projected (2D)
    :type use_proportional_projected: bool
    :param snap: Use Snapping Options
    :type snap: bool
    :param snap_target: Target * CLOSEST Closest, Snap closest point onto target. * CENTER Center, Snap transformation center onto target. * MEDIAN Median, Snap median onto target. * ACTIVE Active, Snap active onto target.
    :type snap_target: typing.Union[int, str]
    :param snap_point: Point
    :type snap_point: typing.List[float]
    :param snap_align: Align with Point Normal
    :type snap_align: bool
    :param snap_normal: Normal
    :type snap_normal: typing.List[float]
    :param gpencil_strokes: Edit Grease Pencil, Edit selected Grease Pencil strokes
    :type gpencil_strokes: bool
    :param center_override: Center Override, Force using this center value (when set)
    :type center_override: typing.List[float]
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def trackball(value: typing.List[float] = (0.0, 0.0),
              mirror: bool = False,
              use_proportional_edit: bool = False,
              proportional_edit_falloff: typing.Union[int, str] = 'SMOOTH',
              proportional_size: float = 1.0,
              use_proportional_connected: bool = False,
              use_proportional_projected: bool = False,
              snap: bool = False,
              snap_target: typing.Union[int, str] = 'CLOSEST',
              snap_point: typing.List[float] = (0.0, 0.0, 0.0),
              snap_align: bool = False,
              snap_normal: typing.List[float] = (0.0, 0.0, 0.0),
              gpencil_strokes: bool = False,
              center_override: typing.List[float] = (0.0, 0.0, 0.0),
              release_confirm: bool = False,
              use_accurate: bool = False):
    ''' Trackball style rotation of selected items

    :param value: Angle
    :type value: typing.List[float]
    :param mirror: Mirror Editing
    :type mirror: bool
    :param use_proportional_edit: Proportional Editing
    :type use_proportional_edit: bool
    :param proportional_edit_falloff: Proportional Falloff, Falloff type for proportional editing mode * SMOOTH Smooth, Smooth falloff. * SPHERE Sphere, Spherical falloff. * ROOT Root, Root falloff. * INVERSE_SQUARE Inverse Square, Inverse Square falloff. * SHARP Sharp, Sharp falloff. * LINEAR Linear, Linear falloff. * CONSTANT Constant, Constant falloff. * RANDOM Random, Random falloff.
    :type proportional_edit_falloff: typing.Union[int, str]
    :param proportional_size: Proportional Size
    :type proportional_size: float
    :param use_proportional_connected: Connected
    :type use_proportional_connected: bool
    :param use_proportional_projected: Projected (2D)
    :type use_proportional_projected: bool
    :param snap: Use Snapping Options
    :type snap: bool
    :param snap_target: Target * CLOSEST Closest, Snap closest point onto target. * CENTER Center, Snap transformation center onto target. * MEDIAN Median, Snap median onto target. * ACTIVE Active, Snap active onto target.
    :type snap_target: typing.Union[int, str]
    :param snap_point: Point
    :type snap_point: typing.List[float]
    :param snap_align: Align with Point Normal
    :type snap_align: bool
    :param snap_normal: Normal
    :type snap_normal: typing.List[float]
    :param gpencil_strokes: Edit Grease Pencil, Edit selected Grease Pencil strokes
    :type gpencil_strokes: bool
    :param center_override: Center Override, Force using this center value (when set)
    :type center_override: typing.List[float]
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def transform(mode: typing.Union[int, str] = 'TRANSLATION',
              value: typing.List[float] = (0.0, 0.0, 0.0, 0.0),
              orient_axis: typing.Union[int, str] = 'Z',
              orient_type: typing.Union[int, str] = 'GLOBAL',
              orient_matrix: typing.List[float] = ((0.0, 0.0, 0.0),
                                                   (0.0, 0.0, 0.0), (0.0, 0.0,
                                                                     0.0)),
              orient_matrix_type: typing.Union[int, str] = 'GLOBAL',
              constraint_axis: typing.List[bool] = (False, False, False),
              mirror: bool = False,
              use_proportional_edit: bool = False,
              proportional_edit_falloff: typing.Union[int, str] = 'SMOOTH',
              proportional_size: float = 1.0,
              use_proportional_connected: bool = False,
              use_proportional_projected: bool = False,
              snap: bool = False,
              snap_target: typing.Union[int, str] = 'CLOSEST',
              snap_point: typing.List[float] = (0.0, 0.0, 0.0),
              snap_align: bool = False,
              snap_normal: typing.List[float] = (0.0, 0.0, 0.0),
              gpencil_strokes: bool = False,
              center_override: typing.List[float] = (0.0, 0.0, 0.0),
              release_confirm: bool = False,
              use_accurate: bool = False):
    ''' Transform selected items by mode type

    :param mode: Mode
    :type mode: typing.Union[int, str]
    :param value: Values
    :type value: typing.List[float]
    :param orient_axis: Axis
    :type orient_axis: typing.Union[int, str]
    :param orient_type: Orientation, Transformation orientation * GLOBAL Global, Align the transformation axes to world space. * LOCAL Local, Align the transformation axes to the selected objects' local space. * NORMAL Normal, Align the transformation axes to average normal of selected elements (bone Y axis for pose mode). * GIMBAL Gimbal, Align each axis to the Euler rotation axis as used for input. * VIEW View, Align the transformation axes to the window. * CURSOR Cursor, Align the transformation axes to the 3D cursor.
    :type orient_type: typing.Union[int, str]
    :param orient_matrix: Matrix
    :type orient_matrix: typing.List[float]
    :param orient_matrix_type: Matrix Orientation * GLOBAL Global, Align the transformation axes to world space. * LOCAL Local, Align the transformation axes to the selected objects' local space. * NORMAL Normal, Align the transformation axes to average normal of selected elements (bone Y axis for pose mode). * GIMBAL Gimbal, Align each axis to the Euler rotation axis as used for input. * VIEW View, Align the transformation axes to the window. * CURSOR Cursor, Align the transformation axes to the 3D cursor.
    :type orient_matrix_type: typing.Union[int, str]
    :param constraint_axis: Constraint Axis
    :type constraint_axis: typing.List[bool]
    :param mirror: Mirror Editing
    :type mirror: bool
    :param use_proportional_edit: Proportional Editing
    :type use_proportional_edit: bool
    :param proportional_edit_falloff: Proportional Falloff, Falloff type for proportional editing mode * SMOOTH Smooth, Smooth falloff. * SPHERE Sphere, Spherical falloff. * ROOT Root, Root falloff. * INVERSE_SQUARE Inverse Square, Inverse Square falloff. * SHARP Sharp, Sharp falloff. * LINEAR Linear, Linear falloff. * CONSTANT Constant, Constant falloff. * RANDOM Random, Random falloff.
    :type proportional_edit_falloff: typing.Union[int, str]
    :param proportional_size: Proportional Size
    :type proportional_size: float
    :param use_proportional_connected: Connected
    :type use_proportional_connected: bool
    :param use_proportional_projected: Projected (2D)
    :type use_proportional_projected: bool
    :param snap: Use Snapping Options
    :type snap: bool
    :param snap_target: Target * CLOSEST Closest, Snap closest point onto target. * CENTER Center, Snap transformation center onto target. * MEDIAN Median, Snap median onto target. * ACTIVE Active, Snap active onto target.
    :type snap_target: typing.Union[int, str]
    :param snap_point: Point
    :type snap_point: typing.List[float]
    :param snap_align: Align with Point Normal
    :type snap_align: bool
    :param snap_normal: Normal
    :type snap_normal: typing.List[float]
    :param gpencil_strokes: Edit Grease Pencil, Edit selected Grease Pencil strokes
    :type gpencil_strokes: bool
    :param center_override: Center Override, Force using this center value (when set)
    :type center_override: typing.List[float]
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def translate(value: typing.List[float] = (0.0, 0.0, 0.0),
              orient_type: typing.Union[int, str] = 'GLOBAL',
              orient_matrix: typing.List[float] = ((0.0, 0.0, 0.0),
                                                   (0.0, 0.0, 0.0), (0.0, 0.0,
                                                                     0.0)),
              orient_matrix_type: typing.Union[int, str] = 'GLOBAL',
              constraint_axis: typing.List[bool] = (False, False, False),
              mirror: bool = False,
              use_proportional_edit: bool = False,
              proportional_edit_falloff: typing.Union[int, str] = 'SMOOTH',
              proportional_size: float = 1.0,
              use_proportional_connected: bool = False,
              use_proportional_projected: bool = False,
              snap: bool = False,
              snap_target: typing.Union[int, str] = 'CLOSEST',
              snap_point: typing.List[float] = (0.0, 0.0, 0.0),
              snap_align: bool = False,
              snap_normal: typing.List[float] = (0.0, 0.0, 0.0),
              gpencil_strokes: bool = False,
              cursor_transform: bool = False,
              texture_space: bool = False,
              remove_on_cancel: bool = False,
              release_confirm: bool = False,
              use_accurate: bool = False,
              use_automerge_and_split: bool = False):
    ''' Move selected items

    :param value: Move
    :type value: typing.List[float]
    :param orient_type: Orientation, Transformation orientation * GLOBAL Global, Align the transformation axes to world space. * LOCAL Local, Align the transformation axes to the selected objects' local space. * NORMAL Normal, Align the transformation axes to average normal of selected elements (bone Y axis for pose mode). * GIMBAL Gimbal, Align each axis to the Euler rotation axis as used for input. * VIEW View, Align the transformation axes to the window. * CURSOR Cursor, Align the transformation axes to the 3D cursor.
    :type orient_type: typing.Union[int, str]
    :param orient_matrix: Matrix
    :type orient_matrix: typing.List[float]
    :param orient_matrix_type: Matrix Orientation * GLOBAL Global, Align the transformation axes to world space. * LOCAL Local, Align the transformation axes to the selected objects' local space. * NORMAL Normal, Align the transformation axes to average normal of selected elements (bone Y axis for pose mode). * GIMBAL Gimbal, Align each axis to the Euler rotation axis as used for input. * VIEW View, Align the transformation axes to the window. * CURSOR Cursor, Align the transformation axes to the 3D cursor.
    :type orient_matrix_type: typing.Union[int, str]
    :param constraint_axis: Constraint Axis
    :type constraint_axis: typing.List[bool]
    :param mirror: Mirror Editing
    :type mirror: bool
    :param use_proportional_edit: Proportional Editing
    :type use_proportional_edit: bool
    :param proportional_edit_falloff: Proportional Falloff, Falloff type for proportional editing mode * SMOOTH Smooth, Smooth falloff. * SPHERE Sphere, Spherical falloff. * ROOT Root, Root falloff. * INVERSE_SQUARE Inverse Square, Inverse Square falloff. * SHARP Sharp, Sharp falloff. * LINEAR Linear, Linear falloff. * CONSTANT Constant, Constant falloff. * RANDOM Random, Random falloff.
    :type proportional_edit_falloff: typing.Union[int, str]
    :param proportional_size: Proportional Size
    :type proportional_size: float
    :param use_proportional_connected: Connected
    :type use_proportional_connected: bool
    :param use_proportional_projected: Projected (2D)
    :type use_proportional_projected: bool
    :param snap: Use Snapping Options
    :type snap: bool
    :param snap_target: Target * CLOSEST Closest, Snap closest point onto target. * CENTER Center, Snap transformation center onto target. * MEDIAN Median, Snap median onto target. * ACTIVE Active, Snap active onto target.
    :type snap_target: typing.Union[int, str]
    :param snap_point: Point
    :type snap_point: typing.List[float]
    :param snap_align: Align with Point Normal
    :type snap_align: bool
    :param snap_normal: Normal
    :type snap_normal: typing.List[float]
    :param gpencil_strokes: Edit Grease Pencil, Edit selected Grease Pencil strokes
    :type gpencil_strokes: bool
    :param cursor_transform: Transform Cursor
    :type cursor_transform: bool
    :param texture_space: Edit Texture Space, Edit object data texture space
    :type texture_space: bool
    :param remove_on_cancel: Remove on Cancel, Remove elements on cancel
    :type remove_on_cancel: bool
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    :param use_automerge_and_split: Auto Merge & Split, Forces the use of Auto Merge and Split
    :type use_automerge_and_split: bool
    '''

    pass


def vert_slide(value: float = 0.0,
               use_even: bool = False,
               flipped: bool = False,
               use_clamp: bool = True,
               mirror: bool = False,
               snap: bool = False,
               snap_target: typing.Union[int, str] = 'CLOSEST',
               snap_point: typing.List[float] = (0.0, 0.0, 0.0),
               snap_align: bool = False,
               snap_normal: typing.List[float] = (0.0, 0.0, 0.0),
               correct_uv: bool = True,
               release_confirm: bool = False,
               use_accurate: bool = False):
    ''' Slide a vertex along a mesh

    :param value: Factor
    :type value: float
    :param use_even: Even, Make the edge loop match the shape of the adjacent edge loop
    :type use_even: bool
    :param flipped: Flipped, When Even mode is active, flips between the two adjacent edge loops
    :type flipped: bool
    :param use_clamp: Clamp, Clamp within the edge extents
    :type use_clamp: bool
    :param mirror: Mirror Editing
    :type mirror: bool
    :param snap: Use Snapping Options
    :type snap: bool
    :param snap_target: Target * CLOSEST Closest, Snap closest point onto target. * CENTER Center, Snap transformation center onto target. * MEDIAN Median, Snap median onto target. * ACTIVE Active, Snap active onto target.
    :type snap_target: typing.Union[int, str]
    :param snap_point: Point
    :type snap_point: typing.List[float]
    :param snap_align: Align with Point Normal
    :type snap_align: bool
    :param snap_normal: Normal
    :type snap_normal: typing.List[float]
    :param correct_uv: Correct UVs, Correct UV coordinates when transforming
    :type correct_uv: bool
    :param release_confirm: Confirm on Release, Always confirm operation when releasing button
    :type release_confirm: bool
    :param use_accurate: Accurate, Use accurate transformation
    :type use_accurate: bool
    '''

    pass


def vertex_random(offset: float = 0.0,
                  uniform: float = 0.0,
                  normal: float = 0.0,
                  seed: int = 0,
                  wait_for_input: bool = True):
    ''' Randomize vertices

    :param offset: Amount, Distance to offset
    :type offset: float
    :param uniform: Uniform, Increase for uniform offset distance
    :type uniform: float
    :param normal: Normal, Align offset direction to normals
    :type normal: float
    :param seed: Random Seed, Seed for the random number generator
    :type seed: int
    :param wait_for_input: Wait for Input
    :type wait_for_input: bool
    '''

    pass


def vertex_warp(warp_angle: float = 6.28319,
                offset_angle: float = 0.0,
                min: float = -1,
                max: float = 1.0,
                viewmat: typing.List[float] = ((0.0, 0.0, 0.0,
                                                0.0), (0.0, 0.0, 0.0, 0.0),
                                               (0.0, 0.0, 0.0,
                                                0.0), (0.0, 0.0, 0.0, 0.0)),
                center: typing.List[float] = (0.0, 0.0, 0.0)):
    ''' Warp vertices around the cursor

    :param warp_angle: Warp Angle, Amount to warp about the cursor
    :type warp_angle: float
    :param offset_angle: Offset Angle, Angle to use as the basis for warping
    :type offset_angle: float
    :param min: Min
    :type min: float
    :param max: Max
    :type max: float
    :param viewmat: Matrix
    :type viewmat: typing.List[float]
    :param center: Center
    :type center: typing.List[float]
    '''

    pass
