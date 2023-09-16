import sys
import typing


def new_triangles(range, coords, colors) -> int:
    ''' Create a new icon from triangle geometry.

    :param range: Pair of ints.
    :param coords: Sequence of bytes (6 floats for one triangle) for (X, Y) coordinates.
    :param colors: Sequence of ints (12 for one triangles) for RGBA.
    :return: Unique icon value (pass to interface icon_value argument).
    '''

    pass


def new_triangles_from_file(filename) -> int:
    ''' Create a new icon from triangle geometry.

    :param filename: File path.
    :return: Unique icon value (pass to interface icon_value argument).
    '''

    pass


def release(icon_id):
    ''' Release the icon.

    '''

    pass
