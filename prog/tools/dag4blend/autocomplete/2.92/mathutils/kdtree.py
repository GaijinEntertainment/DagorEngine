import sys
import typing


class KDTree:
    ''' KdTree(size) -> new kd-tree initialized to hold size items.
    '''

    def balance(self):
        ''' Balance the tree.

        '''
        pass

    def find(self, co: float, filter=None) -> tuple:
        ''' Find nearest point to co .

        :param co: 3d coordinates.
        :type co: float
        :param filter: function which takes an index and returns True for indices to include in the search.
        :type filter: 
        :rtype: tuple
        :return: Vector , index, distance).
        '''
        pass

    def find_n(self, co: float, n: int) -> list:
        ''' Find nearest n points to co .

        :param co: 3d coordinates.
        :type co: float
        :param n: Number of points to find.
        :type n: int
        :rtype: list
        :return: Vector , index, distance).
        '''
        pass

    def find_range(self, co: float, radius: float) -> list:
        ''' Find all points within radius of co .

        :param co: 3d coordinates.
        :type co: float
        :param radius: Distance to search for points.
        :type radius: float
        :rtype: list
        :return: Vector , index, distance).
        '''
        pass

    def insert(self, co: float, index: int):
        ''' Insert a point into the KDTree.

        :param co: Point 3d position.
        :type co: float
        :param index: The index of the point.
        :type index: int
        '''
        pass

    def __init__(self, size):
        ''' 

        '''
        pass
