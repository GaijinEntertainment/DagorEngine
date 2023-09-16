import sys
import typing


def align():
    ''' Align selected bones to the active bone (or to their parent)

    '''

    pass


def armature_layers(
        layers: typing.List[bool] = (False, False, False, False, False, False,
                                     False, False, False, False, False, False,
                                     False, False, False, False, False, False,
                                     False, False, False, False, False, False,
                                     False, False, False, False, False, False,
                                     False, False)):
    ''' Change the visible armature layers

    :param layers: Layer, Armature layers to make visible
    :type layers: typing.List[bool]
    '''

    pass


def autoside_names(type: typing.Union[int, str] = 'XAXIS'):
    ''' Automatically renames the selected bones according to which side of the target axis they fall on

    :param type: Axis, Axis tag names with * XAXIS X-Axis, Left/Right. * YAXIS Y-Axis, Front/Back. * ZAXIS Z-Axis, Top/Bottom.
    :type type: typing.Union[int, str]
    '''

    pass


def bone_layers(
        layers: typing.List[bool] = (False, False, False, False, False, False,
                                     False, False, False, False, False, False,
                                     False, False, False, False, False, False,
                                     False, False, False, False, False, False,
                                     False, False, False, False, False, False,
                                     False, False)):
    ''' Change the layers that the selected bones belong to

    :param layers: Layer, Armature layers that bone belongs to
    :type layers: typing.List[bool]
    '''

    pass


def bone_primitive_add(name: str = "Bone"):
    ''' Add a new bone located at the 3D cursor

    :param name: Name, Name of the newly created bone
    :type name: str
    '''

    pass


def calculate_roll(type: typing.Union[int, str] = 'POS_X',
                   axis_flip: bool = False,
                   axis_only: bool = False):
    ''' Automatically fix alignment of select bones' axes

    :param type: Type
    :type type: typing.Union[int, str]
    :param axis_flip: Flip Axis, Negate the alignment axis
    :type axis_flip: bool
    :param axis_only: Shortest Rotation, Ignore the axis direction, use the shortest rotation to align
    :type axis_only: bool
    '''

    pass


def click_extrude():
    ''' Create a new bone going from the last selected joint to the mouse position

    '''

    pass


def delete():
    ''' Remove selected bones from the armature

    '''

    pass


def dissolve():
    ''' Dissolve selected bones from the armature

    '''

    pass


def duplicate(do_flip_names: bool = False):
    ''' Make copies of the selected bones within the same armature

    :param do_flip_names: Flip Names, Try to flip names of the bones, if possible, instead of adding a number extension
    :type do_flip_names: bool
    '''

    pass


def duplicate_move(ARMATURE_OT_duplicate=None, TRANSFORM_OT_translate=None):
    ''' Make copies of the selected bones within the same armature and move them

    :param ARMATURE_OT_duplicate: Duplicate Selected Bone(s), Make copies of the selected bones within the same armature
    :param TRANSFORM_OT_translate: Move, Move selected items
    '''

    pass


def extrude(forked: bool = False):
    ''' Create new bones from the selected joints

    :param forked: Forked
    :type forked: bool
    '''

    pass


def extrude_forked(ARMATURE_OT_extrude=None, TRANSFORM_OT_translate=None):
    ''' Create new bones from the selected joints and move them

    :param ARMATURE_OT_extrude: Extrude, Create new bones from the selected joints
    :param TRANSFORM_OT_translate: Move, Move selected items
    '''

    pass


def extrude_move(ARMATURE_OT_extrude=None, TRANSFORM_OT_translate=None):
    ''' Create new bones from the selected joints and move them

    :param ARMATURE_OT_extrude: Extrude, Create new bones from the selected joints
    :param TRANSFORM_OT_translate: Move, Move selected items
    '''

    pass


def fill():
    ''' Add bone between selected joint(s) and/or 3D cursor

    '''

    pass


def flip_names(do_strip_numbers: bool = False):
    ''' Flips (and corrects) the axis suffixes of the names of selected bones

    :param do_strip_numbers: Strip Numbers, Try to remove right-most dot-number from flipped names (WARNING: may result in incoherent naming in some cases)
    :type do_strip_numbers: bool
    '''

    pass


def hide(unselected: bool = False):
    ''' Tag selected bones to not be visible in Edit Mode

    :param unselected: Unselected, Hide unselected rather than selected
    :type unselected: bool
    '''

    pass


def layers_show_all(all: bool = True):
    ''' Make all armature layers visible

    :param all: All Layers, Enable all layers or just the first 16 (top row)
    :type all: bool
    '''

    pass


def parent_clear(type: typing.Union[int, str] = 'CLEAR'):
    ''' Remove the parent-child relationship between selected bones and their parents

    :param type: Clear Type, What way to clear parenting
    :type type: typing.Union[int, str]
    '''

    pass


def parent_set(type: typing.Union[int, str] = 'CONNECTED'):
    ''' Set the active bone as the parent of the selected bones

    :param type: Parent Type, Type of parenting
    :type type: typing.Union[int, str]
    '''

    pass


def reveal(select: bool = True):
    ''' Reveal all bones hidden in Edit Mode

    :param select: Select
    :type select: bool
    '''

    pass


def roll_clear(roll: float = 0.0):
    ''' Clear roll for selected bones

    :param roll: Roll
    :type roll: float
    '''

    pass


def select_all(action: typing.Union[int, str] = 'TOGGLE'):
    ''' Toggle selection status of all bones

    :param action: Action, Selection action to execute * TOGGLE Toggle, Toggle selection for all elements. * SELECT Select, Select all elements. * DESELECT Deselect, Deselect all elements. * INVERT Invert, Invert selection of all elements.
    :type action: typing.Union[int, str]
    '''

    pass


def select_hierarchy(direction: typing.Union[int, str] = 'PARENT',
                     extend: bool = False):
    ''' Select immediate parent/children of selected bones

    :param direction: Direction
    :type direction: typing.Union[int, str]
    :param extend: Extend, Extend the selection
    :type extend: bool
    '''

    pass


def select_less():
    ''' Deselect those bones at the boundary of each selection region

    '''

    pass


def select_linked(all_forks: bool = False):
    ''' Select all bones linked by parent/child connections to the current selection

    :param all_forks: All Forks, Follow forks in the parents chain
    :type all_forks: bool
    '''

    pass


def select_linked_pick(deselect: bool = False, all_forks: bool = False):
    ''' (De)select bones linked by parent/child connections under the mouse cursor

    :param deselect: Deselect
    :type deselect: bool
    :param all_forks: All Forks, Follow forks in the parents chain
    :type all_forks: bool
    '''

    pass


def select_mirror(only_active: bool = False, extend: bool = False):
    ''' Mirror the bone selection

    :param only_active: Active Only, Only operate on the active bone
    :type only_active: bool
    :param extend: Extend, Extend the selection
    :type extend: bool
    '''

    pass


def select_more():
    ''' Select those bones connected to the initial selection

    '''

    pass


def select_similar(type: typing.Union[int, str] = 'LENGTH',
                   threshold: float = 0.1):
    ''' Select similar bones by property types

    :param type: Type
    :type type: typing.Union[int, str]
    :param threshold: Threshold
    :type threshold: float
    '''

    pass


def separate():
    ''' Isolate selected bones into a separate armature

    '''

    pass


def shortest_path_pick():
    ''' Select shortest path between two bones

    '''

    pass


def split():
    ''' Split off selected bones from connected unselected bones

    '''

    pass


def subdivide(number_cuts: int = 1):
    ''' Break selected bones into chains of smaller bones

    :param number_cuts: Number of Cuts
    :type number_cuts: int
    '''

    pass


def switch_direction():
    ''' Change the direction that a chain of bones points in (head and tail swap)

    '''

    pass


def symmetrize(direction: typing.Union[int, str] = 'NEGATIVE_X'):
    ''' Enforce symmetry, make copies of the selection or use existing

    :param direction: Direction, Which sides to copy from and to (when both are selected)
    :type direction: typing.Union[int, str]
    '''

    pass
