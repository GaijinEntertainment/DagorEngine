import sys
import typing


def add():
    ''' Add a new time marker

    '''

    pass


def camera_bind():
    ''' Bind the selected camera to a marker on the current frame

    '''

    pass


def delete():
    ''' Delete selected time marker(s)

    '''

    pass


def duplicate(frames: int = 0):
    ''' Duplicate selected time marker(s)

    :param frames: Frames
    :type frames: int
    '''

    pass


def make_links_scene(scene: typing.Union[int, str] = ''):
    ''' Copy selected markers to another scene

    :param scene: Scene
    :type scene: typing.Union[int, str]
    '''

    pass


def move(frames: int = 0, tweak: bool = False):
    ''' Move selected time marker(s)

    :param frames: Frames
    :type frames: int
    :param tweak: Tweak, Operator has been activated using a tweak event
    :type tweak: bool
    '''

    pass


def rename(name: str = "RenamedMarker"):
    ''' Rename first selected time marker

    :param name: Name, New name for marker
    :type name: str
    '''

    pass


def select(wait_to_deselect_others: bool = False,
           mouse_x: int = 0,
           mouse_y: int = 0,
           extend: bool = False,
           camera: bool = False):
    ''' Select time marker(s)

    :param wait_to_deselect_others: Wait to Deselect Others
    :type wait_to_deselect_others: bool
    :param mouse_x: Mouse X
    :type mouse_x: int
    :param mouse_y: Mouse Y
    :type mouse_y: int
    :param extend: Extend, Extend the selection
    :type extend: bool
    :param camera: Camera, Select the camera
    :type camera: bool
    '''

    pass


def select_all(action: typing.Union[int, str] = 'TOGGLE'):
    ''' Change selection of all time markers

    :param action: Action, Selection action to execute * TOGGLE Toggle, Toggle selection for all elements. * SELECT Select, Select all elements. * DESELECT Deselect, Deselect all elements. * INVERT Invert, Invert selection of all elements.
    :type action: typing.Union[int, str]
    '''

    pass


def select_box(xmin: int = 0,
               xmax: int = 0,
               ymin: int = 0,
               ymax: int = 0,
               wait_for_input: bool = True,
               mode: typing.Union[int, str] = 'SET',
               tweak: bool = False):
    ''' Select all time markers using box selection

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
    :param tweak: Tweak, Operator has been activated using a tweak event
    :type tweak: bool
    '''

    pass
