import sys
import typing
import bmesh.types


def intersect_face_point(face: 'bmesh.types.BMFace', point: float) -> bool:
    ''' Tests if the projection of a point is inside a face (using the face's normal).

    :param face: The face to test.
    :type face: 'bmesh.types.BMFace'
    :param point: The point to test.
    :type point: float
    :return: True when the projection of the point is in the face.
    '''

    pass
