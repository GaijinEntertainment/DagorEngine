import sys
import typing
import bpy.types
import mathutils
import bpy
import bpy.context


class AddObjectHelper:
    def align_update_callback(self, _context):
        ''' 

        '''
        pass


def add_object_align_init(
        context: 'bpy.types.Context',
        operator: 'bpy.types.Operator') -> 'mathutils.Matrix':
    ''' Return a matrix using the operator settings and view context.

    :param context: The context to use.
    :type context: 'bpy.types.Context'
    :param operator: The operator, checked for location and rotation properties.
    :type operator: 'bpy.types.Operator'
    :return: the matrix from the context and settings.
    '''

    pass


def object_add_grid_scale(context):
    ''' Return scale which should be applied on object data to align it to grid scale

    '''

    pass


def object_add_grid_scale_apply_operator(operator, context):
    ''' Scale an operators distance values by the grid size.

    '''

    pass


def object_data_add(context: 'bpy.types.Context',
                    obdata: typing.Union['bpy.data', 'bpy.context.object'],
                    operator: 'bpy.types.Operator' = None,
                    name: str = None) -> 'bpy.types.Object':
    ''' Add an object using the view context and preference to initialize the location, rotation and layer.

    :param context: The context to use.
    :type context: 'bpy.types.Context'
    :param obdata: the data used for the new object.
    :type obdata: typing.Union['bpy.data', 'bpy.context.object']
    :param operator: The operator, checked for location and rotation properties.
    :type operator: 'bpy.types.Operator'
    :param name: Optional name
    :type name: str
    :return: the newly created object in the scene.
    '''

    pass


def world_to_camera_view(scene: 'bpy.types.Scene', obj: 'bpy.types.Object',
                         coord: 'mathutils.Vector') -> 'mathutils.Vector':
    ''' Returns the camera space coords for a 3d point. (also known as: normalized device coordinates - NDC). Where (0, 0) is the bottom left and (1, 1) is the top right of the camera frame. values outside 0-1 are also supported. A negative 'z' value means the point is behind the camera. Takes shift-x/y, lens angle and sensor size into account as well as perspective/ortho projections.

    :param scene: Scene to use for frame size.
    :type scene: 'bpy.types.Scene'
    :param obj: Camera object.
    :type obj: 'bpy.types.Object'
    :param coord: World space location.
    :type coord: 'mathutils.Vector'
    :return: a vector where X and Y map to the view plane and Z is the depth on the view axis.
    '''

    pass
