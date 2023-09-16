import sys
import typing


def to_string(unit_system: str,
              unit_category: str,
              value: float,
              precision: int = 3,
              split_unit: bool = False,
              compatible_unit: bool = False) -> str:
    ''' Convert a given input float value into a string with units. :raises ValueError: if conversion fails to generate a valid python string.

    :param unit_system: bpy.utils.units.systems .
    :type unit_system: str
    :param unit_category: The category of data we are converting (length, area, rotation, etc.), from :attr: bpy.utils.units.categories .
    :type unit_category: str
    :param value: The value to convert to a string.
    :type value: float
    :param precision: Number of digits after the comma.
    :type precision: int
    :param split_unit: Whether to use several units if needed (1m1cm), or always only one (1.01m).
    :type split_unit: bool
    :param compatible_unit: Whether to use keyboard-friendly units (1m2) or nicer utf-8 ones (1m²).
    :type compatible_unit: bool
    :return: The converted string.
    '''

    pass


def to_value(unit_system: str,
             unit_category: str,
             str_input: str,
             str_ref_unit: str = None) -> float:
    ''' Convert a given input string into a float value. :raises ValueError: if conversion fails to generate a valid python float value.

    :param unit_system: bpy.utils.units.systems .
    :type unit_system: str
    :param unit_category: The category of data we are converting (length, area, rotation, etc.), from :attr: bpy.utils.units.categories .
    :type unit_category: str
    :param str_input: The string to convert to a float value.
    :type str_input: str
    :param str_ref_unit: A reference string from which to extract a default unit, if none is found in str_input .
    :type str_ref_unit: str
    :return: The converted/interpreted value.
    '''

    pass


categories = None
''' constant value bpy.utils.units.categories(NONE='NONE', LENGTH='LENGTH', AREA='AREA', VOLUME='VOLUME', MASS='MASS', ROTATION='ROTATION', TIME='TIME', VELOCITY='VELOCITY', ACCELERATION='ACCELERATION', CAMERA='CAMERA', POWER='POWER')
'''

systems = None
''' constant value bpy.utils.units.systems(NONE='NONE', METRIC='METRIC', IMPERIAL='IMPERIAL')
'''
