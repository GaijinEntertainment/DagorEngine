import sys
import typing


def attribute_add(name: str = "Attribute",
                  data_type: typing.Union[int, str] = 'FLOAT',
                  domain: typing.Union[int, str] = 'POINT'):
    ''' Add attribute to geometry

    :param name: Name, Name of new attribute
    :type name: str
    :param data_type: Data Type, Type of data stored in attribute * FLOAT Float, Floating-point value. * INT Integer, 32-bit integer. * FLOAT_VECTOR Vector, 3D vector with floating-point values. * FLOAT_COLOR Color, RGBA color with floating-point precisions. * BYTE_COLOR Byte Color, RGBA color with 8-bit precision. * STRING String, Text string. * BOOLEAN Boolean, True or false.
    :type data_type: typing.Union[int, str]
    :param domain: Domain, Type of element that attribute is stored on * POINT Point, Attribute on point. * EDGE Edge, Attribute on mesh edge. * CORNER Corner, Attribute on mesh polygon corner. * POLYGON Polygon, Attribute on mesh polygons. * CURVE Curve, Attribute on hair curve.
    :type domain: typing.Union[int, str]
    '''

    pass


def attribute_remove():
    ''' Remove attribute from geometry

    '''

    pass
