import sys
import typing
import bpy.types
import freestyle.types

from . import ContextFunctions


class BoundingBox:
    ''' Object representing a bounding box consisting out of 2 2D vectors
    '''

    def inside(self, other):
        ''' True if self inside other, False otherwise

        '''
        pass


class StrokeCollector:
    ''' Collects and Stores stroke objects
    '''

    def shade(self, stroke):
        ''' 

        '''
        pass


def angle_x_normal(it):
    ''' unsigned angle between a Point's normal and the X axis, in radians

    '''

    pass


def bound(lower, x, higher):
    ''' Returns x bounded by a maximum and minimum value. Equivalent to: return min(max(x, lower), higher)

    '''

    pass


def bounding_box(stroke):
    ''' Returns the maximum and minimum coordinates (the bounding box) of the stroke's vertices

    '''

    pass


def curvature_from_stroke_vertex(svert):
    ''' The 3D curvature of an stroke vertex' underlying geometry The result is None or in the range [-inf, inf]

    '''

    pass


def find_matching_vertex(id, it):
    ''' Finds the matching vertex, or returns None.

    '''

    pass


def getCurrentScene() -> 'bpy.types.Scene':
    ''' Returns the current scene.

    :return: The current scene.
    '''

    pass


def get_chain_length(ve, orientation):
    ''' Returns the 2d length of a given ViewEdge.

    '''

    pass


def get_object_name(stroke):
    ''' Returns the name of the object that this stroke is drawn on.

    '''

    pass


def get_strokes():
    ''' Get all strokes that are currently available

    '''

    pass


def get_test_stroke():
    ''' Returns a static stroke object for testing

    '''

    pass


def integrate(func: 'freestyle.types.UnaryFunction0D',
              it: 'freestyle.types.Interface0DIterator',
              it_end: 'freestyle.types.Interface0DIterator',
              integration_type: 'freestyle.types.IntegrationType'
              ) -> typing.Union[float, int]:
    ''' Returns a single value from a set of values evaluated at each 0D element of this 1D element.

    :param func: The UnaryFunction0D used to compute a value at each Interface0D.
    :type func: 'freestyle.types.UnaryFunction0D'
    :param it: The Interface0DIterator used to iterate over the 0D elements of this 1D element. The integration will occur over the 0D elements starting from the one pointed by it.
    :type it: 'freestyle.types.Interface0DIterator'
    :param it_end: The Interface0DIterator pointing the end of the 0D elements of the 1D element.
    :type it_end: 'freestyle.types.Interface0DIterator'
    :param integration_type: The integration method used to compute a single value from a set of values.
    :type integration_type: 'freestyle.types.IntegrationType'
    :return: The single value obtained for the 1D element. The return value type is float if func is of the UnaryFunction0DDouble or UnaryFunction0DFloat type, and int if func is of the UnaryFunction0DUnsigned type.
    '''

    pass


def is_poly_clockwise(stroke):
    ''' True if the stroke is orientated in a clockwise way, False otherwise

    '''

    pass


def iter_distance_along_stroke(stroke):
    ''' Yields the absolute distance along the stroke up to the current vertex.

    '''

    pass


def iter_distance_from_camera(stroke, range_min, range_max, normfac):
    ''' Yields the distance to the camera relative to the maximum possible distance for every stroke vertex, constrained by given minimum and maximum values.

    '''

    pass


def iter_distance_from_object(stroke, location, range_min, range_max, normfac):
    ''' yields the distance to the given object relative to the maximum possible distance for every stroke vertex, constrained by given minimum and maximum values.

    '''

    pass


def iter_material_value(stroke, func, attribute):
    ''' Yields a specific material attribute from the vertex' underlying material.

    '''

    pass


def iter_t2d_along_stroke(stroke):
    ''' Yields the progress along the stroke.

    '''

    pass


def material_from_fedge(fe):
    ''' get the diffuse rgba color from an FEdge

    '''

    pass


def normal_at_I0D(it):
    ''' Normal at an Interface0D object. In contrast to Normal2DF0D this function uses the actual data instead of underlying Fedge objects.

    '''

    pass


def pairwise(iterable, types={}):
    ''' Yields a tuple containing the previous and current object

    '''

    pass


def rgb_to_bw(r, g, b):
    ''' Method to convert rgb to a bw intensity value.

    '''

    pass


def simplify(points, tolerance):
    ''' Simplifies a set of points

    '''

    pass


def stroke_curvature(it):
    ''' Compute the 2D curvature at the stroke vertex pointed by the iterator 'it'. K = 1 / R where R is the radius of the circle going through the current vertex and its neighbors

    '''

    pass


def stroke_normal(stroke):
    ''' Compute the 2D normal at the stroke vertex pointed by the iterator 'it'. It is noted that Normal2DF0D computes normals based on underlying FEdges instead, which is inappropriate for strokes when they have already been modified by stroke geometry modifiers. The returned normals are dynamic: they update when the vertex position (and therefore the vertex normal) changes. for use in geometry modifiers it is advised to cast this generator function to a tuple or list

    '''

    pass


def tripplewise(iterable):
    ''' Yields a tuple containing the current object and its immediate neighbors

    '''

    pass
