import sys
import typing


def add_point(location: typing.List[int] = (0, 0)):
    ''' Add New Paint Curve Point

    :param location: Location, Location of vertex in area space
    :type location: typing.List[int]
    '''

    pass


def add_point_slide(PAINTCURVE_OT_add_point=None, PAINTCURVE_OT_slide=None):
    ''' Add new curve point and slide it

    :param PAINTCURVE_OT_add_point: Add New Paint Curve Point, Add New Paint Curve Point
    :param PAINTCURVE_OT_slide: Slide Paint Curve Point, Select and slide paint curve point
    '''

    pass


def cursor():
    ''' Place cursor

    '''

    pass


def delete_point():
    ''' Remove Paint Curve Point

    '''

    pass


def draw():
    ''' Draw curve

    '''

    pass


def new():
    ''' Add new paint curve

    '''

    pass


def select(location: typing.List[int] = (0, 0),
           toggle: bool = False,
           extend: bool = False):
    ''' Select a paint curve point

    :param location: Location, Location of vertex in area space
    :type location: typing.List[int]
    :param toggle: Toggle, (De)select all
    :type toggle: bool
    :param extend: Extend, Extend selection
    :type extend: bool
    '''

    pass


def slide(align: bool = False, select: bool = True):
    ''' Select and slide paint curve point

    :param align: Align Handles, Aligns opposite point handle during transform
    :type align: bool
    :param select: Select, Attempt to select a point handle before transform
    :type select: bool
    '''

    pass
