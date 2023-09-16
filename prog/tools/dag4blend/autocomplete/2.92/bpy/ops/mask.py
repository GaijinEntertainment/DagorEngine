import sys
import typing
import bpy.types


def add_feather_vertex(location: typing.List[float] = (0.0, 0.0)):
    ''' Add vertex to feather

    :param location: Location, Location of vertex in normalized space
    :type location: typing.List[float]
    '''

    pass


def add_feather_vertex_slide(MASK_OT_add_feather_vertex=None,
                             MASK_OT_slide_point=None):
    ''' Add new vertex to feather and slide it

    :param MASK_OT_add_feather_vertex: Add Feather Vertex, Add vertex to feather
    :param MASK_OT_slide_point: Slide Point, Slide control points
    '''

    pass


def add_vertex(location: typing.List[float] = (0.0, 0.0)):
    ''' Add vertex to active spline

    :param location: Location, Location of vertex in normalized space
    :type location: typing.List[float]
    '''

    pass


def add_vertex_slide(MASK_OT_add_vertex=None, MASK_OT_slide_point=None):
    ''' Add new vertex and slide it

    :param MASK_OT_add_vertex: Add Vertex, Add vertex to active spline
    :param MASK_OT_slide_point: Slide Point, Slide control points
    '''

    pass


def copy_splines():
    ''' Copy selected splines to clipboard

    '''

    pass


def cyclic_toggle():
    ''' Toggle cyclic for selected splines

    '''

    pass


def delete():
    ''' Delete selected control points or splines

    '''

    pass


def duplicate():
    ''' Duplicate selected control points and segments between them

    '''

    pass


def duplicate_move(MASK_OT_duplicate=None, TRANSFORM_OT_translate=None):
    ''' Duplicate mask and move

    :param MASK_OT_duplicate: Duplicate Mask, Duplicate selected control points and segments between them
    :param TRANSFORM_OT_translate: Move, Move selected items
    '''

    pass


def feather_weight_clear():
    ''' Reset the feather weight to zero

    '''

    pass


def handle_type_set(type: typing.Union[int, str] = 'AUTO'):
    ''' Set type of handles for selected control points

    :param type: Type, Spline type
    :type type: typing.Union[int, str]
    '''

    pass


def hide_view_clear(select: bool = True):
    ''' Reveal the layer by setting the hide flag

    :param select: Select
    :type select: bool
    '''

    pass


def hide_view_set(unselected: bool = False):
    ''' Hide the layer by setting the hide flag

    :param unselected: Unselected, Hide unselected rather than selected layers
    :type unselected: bool
    '''

    pass


def layer_move(direction: typing.Union[int, str] = 'UP'):
    ''' Move the active layer up/down in the list

    :param direction: Direction, Direction to move the active layer
    :type direction: typing.Union[int, str]
    '''

    pass


def layer_new(name: str = ""):
    ''' Add new mask layer for masking

    :param name: Name, Name of new mask layer
    :type name: str
    '''

    pass


def layer_remove():
    ''' Remove mask layer

    '''

    pass


def new(name: str = ""):
    ''' Create new mask

    :param name: Name, Name of new mask
    :type name: str
    '''

    pass


def normals_make_consistent():
    ''' Recalculate the direction of selected handles

    '''

    pass


def parent_clear():
    ''' Clear the mask's parenting

    '''

    pass


def parent_set():
    ''' Set the mask's parenting

    '''

    pass


def paste_splines():
    ''' Paste splines from clipboard

    '''

    pass


def primitive_circle_add(size: float = 100.0,
                         location: typing.List[float] = (0.0, 0.0)):
    ''' Add new circle-shaped spline

    :param size: Size, Size of new circle
    :type size: float
    :param location: Location, Location of new circle
    :type location: typing.List[float]
    '''

    pass


def primitive_square_add(size: float = 100.0,
                         location: typing.List[float] = (0.0, 0.0)):
    ''' Add new square-shaped spline

    :param size: Size, Size of new circle
    :type size: float
    :param location: Location, Location of new circle
    :type location: typing.List[float]
    '''

    pass


def select(extend: bool = False,
           deselect: bool = False,
           toggle: bool = False,
           deselect_all: bool = False,
           location: typing.List[float] = (0.0, 0.0)):
    ''' Select spline points

    :param extend: Extend, Extend selection instead of deselecting everything first
    :type extend: bool
    :param deselect: Deselect, Remove from selection
    :type deselect: bool
    :param toggle: Toggle Selection, Toggle the selection
    :type toggle: bool
    :param deselect_all: Deselect On Nothing, Deselect all when nothing under the cursor
    :type deselect_all: bool
    :param location: Location, Location of vertex in normalized space
    :type location: typing.List[float]
    '''

    pass


def select_all(action: typing.Union[int, str] = 'TOGGLE'):
    ''' Change selection of all curve points

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
    :param mode: Mode * SET Set, Set a new selection. * ADD Extend, Extend existing selection. * SUB Subtract, Subtract existing selection.
    :type mode: typing.Union[int, str]
    '''

    pass


def select_circle(x: int = 0,
                  y: int = 0,
                  radius: int = 25,
                  wait_for_input: bool = True,
                  mode: typing.Union[int, str] = 'SET'):
    ''' Select curve points using circle selection

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
    ''' Select curve points using lasso selection

    :param path: Path
    :type path: typing.Union[typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.List['bpy.types.OperatorMousePath'], 'bpy_prop_collection']
    :param mode: Mode * SET Set, Set a new selection. * ADD Extend, Extend existing selection. * SUB Subtract, Subtract existing selection.
    :type mode: typing.Union[int, str]
    '''

    pass


def select_less():
    ''' Deselect spline points at the boundary of each selection region

    '''

    pass


def select_linked():
    ''' Select all curve points linked to already selected ones

    '''

    pass


def select_linked_pick(deselect: bool = False):
    ''' (De)select all points linked to the curve under the mouse cursor

    :param deselect: Deselect
    :type deselect: bool
    '''

    pass


def select_more():
    ''' Select more spline points connected to initial selection

    '''

    pass


def shape_key_clear():
    ''' Remove mask shape keyframe for active mask layer at the current frame

    '''

    pass


def shape_key_feather_reset():
    ''' Reset feather weights on all selected points animation values

    '''

    pass


def shape_key_insert():
    ''' Insert mask shape keyframe for active mask layer at the current frame

    '''

    pass


def shape_key_rekey(location: bool = True, feather: bool = True):
    ''' Recalculate animation data on selected points for frames selected in the dopesheet

    :param location: Location
    :type location: bool
    :param feather: Feather
    :type feather: bool
    '''

    pass


def slide_point(slide_feather: bool = False, is_new_point: bool = False):
    ''' Slide control points

    :param slide_feather: Slide Feather, First try to slide feather instead of vertex
    :type slide_feather: bool
    :param is_new_point: Slide New Point, Newly created vertex is being slid
    :type is_new_point: bool
    '''

    pass


def slide_spline_curvature():
    ''' Slide a point on the spline to define its curvature

    '''

    pass


def switch_direction():
    ''' Switch direction of selected splines

    '''

    pass
