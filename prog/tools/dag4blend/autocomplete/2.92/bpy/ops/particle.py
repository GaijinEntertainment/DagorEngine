import sys
import typing
import bpy.types


def brush_edit(stroke: typing.Union[
        typing.Dict[str, 'bpy.types.OperatorStrokeElement'], typing.
        List['bpy.types.OperatorStrokeElement'], 'bpy_prop_collection'] = None
               ):
    ''' Apply a stroke of brush to the particles

    :param stroke: Stroke
    :type stroke: typing.Union[typing.Dict[str, 'bpy.types.OperatorStrokeElement'], typing.List['bpy.types.OperatorStrokeElement'], 'bpy_prop_collection']
    '''

    pass


def connect_hair(all: bool = False):
    ''' Connect hair to the emitter mesh

    :param all: All Hair, Connect all hair systems to the emitter mesh
    :type all: bool
    '''

    pass


def copy_particle_systems(space: typing.Union[int, str] = 'OBJECT',
                          remove_target_particles: bool = True,
                          use_active: bool = False):
    ''' Copy particle systems from the active object to selected objects

    :param space: Space, Space transform for copying from one object to another * OBJECT Object, Copy inside each object's local space. * WORLD World, Copy in world space.
    :type space: typing.Union[int, str]
    :param remove_target_particles: Remove Target Particles, Remove particle systems on the target objects
    :type remove_target_particles: bool
    :param use_active: Use Active, Use the active particle system from the context
    :type use_active: bool
    '''

    pass


def delete(type: typing.Union[int, str] = 'PARTICLE'):
    ''' Delete selected particles or keys

    :param type: Type, Delete a full particle or only keys
    :type type: typing.Union[int, str]
    '''

    pass


def disconnect_hair(all: bool = False):
    ''' Disconnect hair from the emitter mesh

    :param all: All Hair, Disconnect all hair systems from the emitter mesh
    :type all: bool
    '''

    pass


def duplicate_particle_system(use_duplicate_settings: bool = False):
    ''' Duplicate particle system within the active object

    :param use_duplicate_settings: Duplicate Settings, Duplicate settings as well, so the new particle system uses its own settings
    :type use_duplicate_settings: bool
    '''

    pass


def dupliob_copy():
    ''' Duplicate the current dupliobject

    '''

    pass


def dupliob_move_down():
    ''' Move dupli object down in the list

    '''

    pass


def dupliob_move_up():
    ''' Move dupli object up in the list

    '''

    pass


def dupliob_refresh():
    ''' Refresh list of dupli objects and their weights

    '''

    pass


def dupliob_remove():
    ''' Remove the selected dupliobject

    '''

    pass


def edited_clear():
    ''' Undo all edition performed on the particle system

    '''

    pass


def hair_dynamics_preset_add(name: str = "",
                             remove_name: bool = False,
                             remove_active: bool = False):
    ''' Add or remove a Hair Dynamics Preset

    :param name: Name, Name of the preset, used to make the path name
    :type name: str
    :param remove_name: remove_name
    :type remove_name: bool
    :param remove_active: remove_active
    :type remove_active: bool
    '''

    pass


def hide(unselected: bool = False):
    ''' Hide selected particles

    :param unselected: Unselected, Hide unselected rather than selected
    :type unselected: bool
    '''

    pass


def mirror():
    ''' Duplicate and mirror the selected particles along the local X axis

    '''

    pass


def new():
    ''' Add new particle settings

    '''

    pass


def new_target():
    ''' Add a new particle target

    '''

    pass


def particle_edit_toggle():
    ''' Toggle particle edit mode

    '''

    pass


def rekey(keys_number: int = 2):
    ''' Change the number of keys of selected particles (root and tip keys included)

    :param keys_number: Number of Keys
    :type keys_number: int
    '''

    pass


def remove_doubles(threshold: float = 0.0002):
    ''' Remove selected particles close enough of others

    :param threshold: Merge Distance, Threshold distance within which particles are removed
    :type threshold: float
    '''

    pass


def reveal(select: bool = True):
    ''' Show hidden particles

    :param select: Select
    :type select: bool
    '''

    pass


def select_all(action: typing.Union[int, str] = 'TOGGLE'):
    ''' (De)select all particles' keys

    :param action: Action, Selection action to execute * TOGGLE Toggle, Toggle selection for all elements. * SELECT Select, Select all elements. * DESELECT Deselect, Deselect all elements. * INVERT Invert, Invert selection of all elements.
    :type action: typing.Union[int, str]
    '''

    pass


def select_less():
    ''' Deselect boundary selected keys of each particle

    '''

    pass


def select_linked():
    ''' Select all keys linked to already selected ones

    '''

    pass


def select_linked_pick(deselect: bool = False,
                       location: typing.List[int] = (0, 0)):
    ''' Select nearest particle from mouse pointer

    :param deselect: Deselect, Deselect linked keys rather than selecting them
    :type deselect: bool
    :param location: Location
    :type location: typing.List[int]
    '''

    pass


def select_more():
    ''' Select keys linked to boundary selected keys of each particle

    '''

    pass


def select_random(percent: float = 50.0,
                  seed: int = 0,
                  action: typing.Union[int, str] = 'SELECT',
                  type: typing.Union[int, str] = 'HAIR'):
    ''' Select a randomly distributed set of hair or points

    :param percent: Percent, Percentage of objects to select randomly
    :type percent: float
    :param seed: Random Seed, Seed for the random number generator
    :type seed: int
    :param action: Action, Selection action to execute * SELECT Select, Select all elements. * DESELECT Deselect, Deselect all elements.
    :type action: typing.Union[int, str]
    :param type: Type, Select either hair or points
    :type type: typing.Union[int, str]
    '''

    pass


def select_roots(action: typing.Union[int, str] = 'SELECT'):
    ''' Select roots of all visible particles

    :param action: Action, Selection action to execute * TOGGLE Toggle, Toggle selection for all elements. * SELECT Select, Select all elements. * DESELECT Deselect, Deselect all elements. * INVERT Invert, Invert selection of all elements.
    :type action: typing.Union[int, str]
    '''

    pass


def select_tips(action: typing.Union[int, str] = 'SELECT'):
    ''' Select tips of all visible particles

    :param action: Action, Selection action to execute * TOGGLE Toggle, Toggle selection for all elements. * SELECT Select, Select all elements. * DESELECT Deselect, Deselect all elements. * INVERT Invert, Invert selection of all elements.
    :type action: typing.Union[int, str]
    '''

    pass


def shape_cut():
    ''' Cut hair to conform to the set shape object

    '''

    pass


def subdivide():
    ''' Subdivide selected particles segments (adds keys)

    '''

    pass


def target_move_down():
    ''' Move particle target down in the list

    '''

    pass


def target_move_up():
    ''' Move particle target up in the list

    '''

    pass


def target_remove():
    ''' Remove the selected particle target

    '''

    pass


def unify_length():
    ''' Make selected hair the same length

    '''

    pass


def weight_set(factor: float = 1.0):
    ''' Set the weight of selected keys

    :param factor: Factor, Interpolation factor between current brush weight, and keys' weights
    :type factor: float
    '''

    pass
