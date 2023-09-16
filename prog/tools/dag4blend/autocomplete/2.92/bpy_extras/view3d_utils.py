import sys
import typing
import bpy.types
import mathutils


def location_3d_to_region_2d(region: 'bpy.types.Region',
                             rv3d: 'bpy.types.RegionView3D',
                             coord,
                             default=None) -> 'mathutils.Vector':
    ''' Return the *region* relative 2d location of a 3d position.

    :param region: region of the 3D viewport, typically bpy.context.region.
    :type region: 'bpy.types.Region'
    :param rv3d: 3D region data, typically bpy.context.space_data.region_3d.
    :type rv3d: 'bpy.types.RegionView3D'
    :param coord: 3d worldspace location.
    :return: 2d location
    '''

    pass


def region_2d_to_location_3d(region: 'bpy.types.Region',
                             rv3d: 'bpy.types.RegionView3D', coord,
                             depth_location) -> 'mathutils.Vector':
    ''' Return a 3d location from the region relative 2d coords, aligned with *depth_location*.

    :param region: region of the 3D viewport, typically bpy.context.region.
    :type region: 'bpy.types.Region'
    :param rv3d: 3D region data, typically bpy.context.space_data.region_3d.
    :type rv3d: 'bpy.types.RegionView3D'
    :param coord: 2d coordinates relative to the region; (event.mouse_region_x, event.mouse_region_y) for example.
    :param depth_location: the returned vectors depth is aligned with this since there is no defined depth with a 2d region input.
    :return: normalized 3d vector.
    '''

    pass


def region_2d_to_origin_3d(region: 'bpy.types.Region',
                           rv3d: 'bpy.types.RegionView3D',
                           coord,
                           clamp: float = None) -> 'mathutils.Vector':
    ''' Return the 3d view origin from the region relative 2d coords.

    :param region: region of the 3D viewport, typically bpy.context.region.
    :type region: 'bpy.types.Region'
    :param rv3d: 3D region data, typically bpy.context.space_data.region_3d.
    :type rv3d: 'bpy.types.RegionView3D'
    :param coord: 2d coordinates relative to the region; (event.mouse_region_x, event.mouse_region_y) for example.
    :param clamp: Clamp the maximum far-clip value used. (negative value will move the offset away from the view_location)
    :type clamp: float
    :return: The origin of the viewpoint in 3d space.
    '''

    pass


def region_2d_to_vector_3d(region: 'bpy.types.Region',
                           rv3d: 'bpy.types.RegionView3D',
                           coord) -> 'mathutils.Vector':
    ''' Return a direction vector from the viewport at the specific 2d region coordinate.

    :param region: region of the 3D viewport, typically bpy.context.region.
    :type region: 'bpy.types.Region'
    :param rv3d: 3D region data, typically bpy.context.space_data.region_3d.
    :type rv3d: 'bpy.types.RegionView3D'
    :param coord: (event.mouse_region_x, event.mouse_region_y) for example.
    :return: normalized 3d vector.
    '''

    pass
