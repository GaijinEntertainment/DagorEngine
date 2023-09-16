import sys
import typing
import bmesh.types
import bpy.types
import bpy.context
import mathutils


class BVHTree:
    @classmethod
    def FromBMesh(cls, bmesh: 'bmesh.types.BMesh', epsilon: float = 0.0):
        ''' BVH tree based on BMesh data.

        :param bmesh: BMesh data.
        :type bmesh: 'bmesh.types.BMesh'
        :param epsilon: Increase the threshold for detecting overlap and raycast hits.
        :type epsilon: float
        '''
        pass

    @classmethod
    def FromObject(cls,
                   object: 'bpy.types.Object',
                   depsgraph: 'bpy.types.Depsgraph',
                   deform: bool = True,
                   render=False,
                   cage: bool = False,
                   epsilon: float = 0.0):
        ''' BVH tree based on Object data.

        :param object: Object data.
        :type object: 'bpy.types.Object'
        :param depsgraph: Depsgraph to use for evaluating the mesh.
        :type depsgraph: 'bpy.types.Depsgraph'
        :param deform: Use mesh with deformations.
        :type deform: bool
        :param cage: Use modifiers cage.
        :type cage: bool
        :param epsilon: Increase the threshold for detecting overlap and raycast hits.
        :type epsilon: float
        '''
        pass

    @classmethod
    def FromPolygons(cls,
                     vertices: typing.List[float],
                     polygons: typing.
                     Union['bpy.types.Sequence', 'bpy.context.sequences'],
                     all_triangles: bool = False,
                     epsilon: float = 0.0):
        ''' BVH tree constructed geometry passed in as arguments.

        :param vertices: float triplets each representing (x, y, z)
        :type vertices: typing.List[float]
        :param polygons: Sequence of polyugons, each containing indices to the vertices argument.
        :type polygons: typing.Union['bpy.types.Sequence', 'bpy.context.sequences']
        :param all_triangles: Use when all **polygons** are triangles for more efficient conversion.
        :type all_triangles: bool
        :param epsilon: Increase the threshold for detecting overlap and raycast hits.
        :type epsilon: float
        '''
        pass

    def find_nearest(self, origin, distance: float = 1.84467e+19) -> tuple:
        ''' Find the nearest element (typically face index) to a point.

        :param co: Find nearest element to this point.
        :type co: 'mathutils.Vector'
        :param distance: Maximum distance threshold.
        :type distance: float
        :rtype: tuple
        :return: Returns a tuple ( Vector location, Vector normal, int index, float distance), Values will all be None if no hit is found.
        '''
        pass

    def find_nearest_range(self, origin,
                           distance: float = 1.84467e+19) -> list:
        ''' Find the nearest elements (typically face index) to a point in the distance range.

        :param co: Find nearest elements to this point.
        :type co: 'mathutils.Vector'
        :param distance: Maximum distance threshold.
        :type distance: float
        :rtype: list
        :return: Returns a list of tuples ( Vector location, Vector normal, int index, float distance),
        '''
        pass

    def overlap(self, other_tree: 'BVHTree') -> list:
        ''' Find overlapping indices between 2 trees.

        :param other_tree: Other tree to perform overlap test on.
        :type other_tree: 'BVHTree'
        :rtype: list
        :return: Returns a list of unique index pairs, the first index referencing this tree, the second referencing the **other_tree**.
        '''
        pass

    def ray_cast(self,
                 origin,
                 direction: 'mathutils.Vector',
                 distance: float = 'sys.float_info.max') -> tuple:
        ''' Cast a ray onto the mesh.

        :param co: Start location of the ray in object space.
        :type co: 'mathutils.Vector'
        :param direction: Direction of the ray in object space.
        :type direction: 'mathutils.Vector'
        :param distance: Maximum distance threshold.
        :type distance: float
        :rtype: tuple
        :return: Returns a tuple ( Vector location, Vector normal, int index, float distance), Values will all be None if no hit is found.
        '''
        pass

    def __init__(self, size):
        ''' 

        '''
        pass
