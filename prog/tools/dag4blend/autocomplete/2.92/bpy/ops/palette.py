import sys
import typing


def color_add():
    ''' Add new color to active palette

    '''

    pass


def color_delete():
    ''' Remove active color from palette

    '''

    pass


def color_move(type: typing.Union[int, str] = 'UP'):
    ''' Move the active Color up/down in the list

    :param type: Type
    :type type: typing.Union[int, str]
    '''

    pass


def extract_from_image(threshold: int = 1):
    ''' Extract all colors used in Image and create a Palette

    :param threshold: Threshold
    :type threshold: int
    '''

    pass


def join(palette: str = ""):
    ''' Join Palette Swatches

    :param palette: Palette, Name of the Palette
    :type palette: str
    '''

    pass


def new():
    ''' Add new palette

    '''

    pass


def sort(type: typing.Union[int, str] = 'HSV'):
    ''' Sort Palette Colors

    :param type: Type
    :type type: typing.Union[int, str]
    '''

    pass
