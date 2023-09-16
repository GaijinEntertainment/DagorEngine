import sys
import typing
import bpy.types
import bpy.context


class ExportHelper:
    def check(self, _context):
        ''' 

        '''
        pass

    def invoke(self, context, _event):
        ''' 

        '''
        pass


class ImportHelper:
    def check(self, _context):
        ''' 

        '''
        pass

    def invoke(self, context, _event):
        ''' 

        '''
        pass


def axis_conversion(from_forward='Y', from_up='Z', to_forward='Y', to_up='Z'):
    ''' Each argument us an axis in ['X', 'Y', 'Z', '-X', '-Y', '-Z'] where the first 2 are a source and the second 2 are the target.

    '''

    pass


def axis_conversion_ensure(operator: 'bpy.types.Operator', forward_attr: str,
                           up_attr: str) -> bool:
    ''' Function to ensure an operator has valid axis conversion settings, intended to be used from bpy.types.Operator.check .

    :param operator: the operator to access axis attributes from.
    :type operator: 'bpy.types.Operator'
    :param forward_attr: attribute storing the forward axis
    :type forward_attr: str
    :param up_attr: attribute storing the up axis
    :type up_attr: str
    :return: True if the value was modified.
    '''

    pass


def create_derived_objects(scene, ob):
    ''' 

    '''

    pass


def free_derived_objects(ob):
    ''' 

    '''

    pass


def orientation_helper(axis_forward='Y', axis_up='Z'):
    ''' A decorator for import/export classes, generating properties needed by the axis conversion system and IO helpers, with specified default values (axes).

    '''

    pass


def path_reference(filepath: str,
                   base_src: str,
                   base_dst: str,
                   mode: str = 'AUTO',
                   copy_subdir: str = '',
                   copy_set: set = None,
                   library: 'bpy.types.Library' = None) -> str:
    ''' Return a filepath relative to a destination directory, for use with exporters.

    :param filepath: the file path to return, supporting blenders relative '//' prefix.
    :type filepath: str
    :param base_src: the directory the *filepath* is relative too (normally the blend file).
    :type base_src: str
    :param base_dst: the directory the *filepath* will be referenced from (normally the export path).
    :type base_dst: str
    :param mode: the method used get the path in ['AUTO', 'ABSOLUTE', 'RELATIVE', 'MATCH', 'STRIP', 'COPY']
    :type mode: str
    :param copy_subdir: the subdirectory of *base_dst* to use when mode='COPY'.
    :type copy_subdir: str
    :param copy_set: collect from/to pairs when mode='COPY', pass to *path_reference_copy* when exporting is done.
    :type copy_set: set
    :param library: The library this path is relative to.
    :type library: 'bpy.types.Library'
    :return: the new filepath.
    '''

    pass


def path_reference_copy(copy_set: set, report='<built-in function print>'):
    ''' Execute copying files of path_reference

    :param copy_set: set of (from, to) pairs to copy.
    :type copy_set: set
    :param report: function used for reporting warnings, takes a string argument.
    '''

    pass


def unique_name(key: 'bpy.context.object',
                name: str,
                name_dict: dict,
                name_max=-1,
                clean_func=None,
                sep: str = '.'):
    ''' Helper function for storing unique names which may have special characters stripped and restricted to a maximum length.

    :param key: unique item this name belongs to, name_dict[key] will be reused when available. This can be the object, mesh, material, etc instance its self.
    :type key: 'bpy.context.object'
    :param name: The name used to create a unique value in *name_dict*.
    :type name: str
    :param name_dict: This is used to cache namespace to ensure no collisions occur, this should be an empty dict initially and only modified by this function.
    :type name_dict: dict
    :param clean_func: Function to call on *name* before creating a unique value.
    :param sep: Separator to use when between the name and a number when a duplicate name is found.
    :type sep: str
    '''

    pass


def unpack_face_list(list_of_tuples):
    ''' 

    '''

    pass


def unpack_list(list_of_tuples):
    ''' 

    '''

    pass


path_reference_mode = None
''' constant value (<built-in function EnumProperty>, {'name': 'Path Mode', 'description': 'Method used to reference paths', 'items': (('AUTO', 'Auto', 'Use Relative paths with subdirectories only'), ('ABSOLUTE', 'Absolute', 'Always write absolute paths'), ('RELATIVE', 'Relative', 'Always write relative paths (where possible)'), ('MATCH', 'Match', 'Match Absolute/Relative setting with input path'), ('STRIP', 'Strip Path', 'Filename only'), ('COPY', 'Copy', 'Copy the file to the destination path (or subdirectory)')), 'default': 'AUTO', 'attr': 'path_mode'})
'''
