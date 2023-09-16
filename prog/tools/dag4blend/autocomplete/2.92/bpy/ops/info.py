import sys
import typing


def report_copy():
    ''' Copy selected reports to clipboard

    '''

    pass


def report_delete():
    ''' Delete selected reports

    '''

    pass


def report_replay():
    ''' Replay selected reports

    '''

    pass


def reports_display_update():
    ''' Update the display of reports in Blender UI (internal use)

    '''

    pass


def select_all(action: typing.Union[int, str] = 'SELECT'):
    ''' Change selection of all visible reports

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
    ''' Toggle box selection

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


def select_pick(report_index: int = 0, extend: bool = False):
    ''' Select reports by index

    :param report_index: Report, Index of the report
    :type report_index: int
    :param extend: Extend, Extend report selection
    :type extend: bool
    '''

    pass
