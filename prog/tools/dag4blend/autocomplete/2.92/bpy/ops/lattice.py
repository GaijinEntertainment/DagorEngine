import sys
import typing


def flip(axis: typing.Union[int, str] = 'U'):
    ''' Mirror all control points without inverting the lattice deform

    :param axis: Flip Axis, Coordinates along this axis get flipped
    :type axis: typing.Union[int, str]
    '''

    pass


def make_regular():
    ''' Set UVW control points a uniform distance apart

    '''

    pass


def select_all(action: typing.Union[int, str] = 'TOGGLE'):
    ''' Change selection of all UVW control points

    :param action: Action, Selection action to execute * TOGGLE Toggle, Toggle selection for all elements. * SELECT Select, Select all elements. * DESELECT Deselect, Deselect all elements. * INVERT Invert, Invert selection of all elements.
    :type action: typing.Union[int, str]
    '''

    pass


def select_less():
    ''' Deselect vertices at the boundary of each selection region

    '''

    pass


def select_mirror(axis: typing.Union[typing.Set[int], typing.Set[str]] = {'X'},
                  extend: bool = False):
    ''' Select mirrored lattice points

    :param axis: Axis
    :type axis: typing.Union[typing.Set[int], typing.Set[str]]
    :param extend: Extend, Extend the selection
    :type extend: bool
    '''

    pass


def select_more():
    ''' Select vertex directly linked to already selected ones

    '''

    pass


def select_random(percent: float = 50.0,
                  seed: int = 0,
                  action: typing.Union[int, str] = 'SELECT'):
    ''' Randomly select UVW control points

    :param percent: Percent, Percentage of objects to select randomly
    :type percent: float
    :param seed: Random Seed, Seed for the random number generator
    :type seed: int
    :param action: Action, Selection action to execute * SELECT Select, Select all elements. * DESELECT Deselect, Deselect all elements.
    :type action: typing.Union[int, str]
    '''

    pass


def select_ungrouped(extend: bool = False):
    ''' Select vertices without a group

    :param extend: Extend, Extend the selection
    :type extend: bool
    '''

    pass
