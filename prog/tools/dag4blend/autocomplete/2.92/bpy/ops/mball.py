import sys
import typing


def delete_metaelems():
    ''' Delete selected metaelement(s)

    '''

    pass


def duplicate_metaelems():
    ''' Duplicate selected metaelement(s)

    '''

    pass


def duplicate_move(MBALL_OT_duplicate_metaelems=None,
                   TRANSFORM_OT_translate=None):
    ''' Make copies of the selected metaelements and move them

    :param MBALL_OT_duplicate_metaelems: Duplicate Metaelements, Duplicate selected metaelement(s)
    :param TRANSFORM_OT_translate: Move, Move selected items
    '''

    pass


def hide_metaelems(unselected: bool = False):
    ''' Hide (un)selected metaelement(s)

    :param unselected: Unselected, Hide unselected rather than selected
    :type unselected: bool
    '''

    pass


def reveal_metaelems(select: bool = True):
    ''' Reveal all hidden metaelements

    :param select: Select
    :type select: bool
    '''

    pass


def select_all(action: typing.Union[int, str] = 'TOGGLE'):
    ''' Change selection of all meta elements

    :param action: Action, Selection action to execute * TOGGLE Toggle, Toggle selection for all elements. * SELECT Select, Select all elements. * DESELECT Deselect, Deselect all elements. * INVERT Invert, Invert selection of all elements.
    :type action: typing.Union[int, str]
    '''

    pass


def select_random_metaelems(percent: float = 50.0,
                            seed: int = 0,
                            action: typing.Union[int, str] = 'SELECT'):
    ''' Randomly select metaelements

    :param percent: Percent, Percentage of objects to select randomly
    :type percent: float
    :param seed: Random Seed, Seed for the random number generator
    :type seed: int
    :param action: Action, Selection action to execute * SELECT Select, Select all elements. * DESELECT Deselect, Deselect all elements.
    :type action: typing.Union[int, str]
    '''

    pass


def select_similar(type: typing.Union[int, str] = 'TYPE',
                   threshold: float = 0.1):
    ''' Select similar metaballs by property types

    :param type: Type
    :type type: typing.Union[int, str]
    :param threshold: Threshold
    :type threshold: float
    '''

    pass
