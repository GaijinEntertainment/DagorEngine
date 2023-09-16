import sys
import typing
from . import noise
from . import kdtree
from . import bvhtree
from . import interpolate
from . import geometry


class Color:
    ''' This object gives access to Colors in Blender. :param rgb: (r, g, b) color values :type rgb: 3d vector
    '''

    b: float = None
    ''' Blue color channel.

    :type: float
    '''

    g: float = None
    ''' Green color channel.

    :type: float
    '''

    h: float = None
    ''' HSV Hue component in [0, 1].

    :type: float
    '''

    hsv: float = None
    ''' HSV Values in [0, 1].

    :type: float
    '''

    is_frozen: bool = None
    ''' True when this object has been frozen (read-only).

    :type: bool
    '''

    is_wrapped: bool = None
    ''' True when this object wraps external data (read-only).

    :type: bool
    '''

    owner = None
    ''' The item this is wrapping or None (read-only).'''

    r: float = None
    ''' Red color channel.

    :type: float
    '''

    s: float = None
    ''' HSV Saturation component in [0, 1].

    :type: float
    '''

    v: float = None
    ''' HSV Value component in [0, 1].

    :type: float
    '''

    @staticmethod
    def copy() -> 'Color':
        ''' Returns a copy of this color.

        :rtype: 'Color'
        :return: A copy of the color.
        '''
        pass

    @staticmethod
    def freeze():
        ''' Make this object immutable. After this the object can be hashed, used in dictionaries & sets.

        '''
        pass

    def __init__(self, rgb=(0.0, 0.0, 0.0)):
        ''' 

        '''
        pass


class Euler:
    ''' This object gives access to Eulers in Blender. :param angles: Three angles, in radians. :type angles: 3d vector :param order: Optional order of the angles, a permutation of XYZ . :type order: str
    '''

    is_frozen: bool = None
    ''' True when this object has been frozen (read-only).

    :type: bool
    '''

    is_wrapped: bool = None
    ''' True when this object wraps external data (read-only).

    :type: bool
    '''

    order: str = None
    ''' Euler rotation order.

    :type: str
    '''

    owner = None
    ''' The item this is wrapping or None (read-only).'''

    x: float = None
    ''' Euler axis angle in radians.

    :type: float
    '''

    y: float = None
    ''' Euler axis angle in radians.

    :type: float
    '''

    z: float = None
    ''' Euler axis angle in radians.

    :type: float
    '''

    @staticmethod
    def copy() -> 'Euler':
        ''' Returns a copy of this euler.

        :rtype: 'Euler'
        :return: A copy of the euler.
        '''
        pass

    @staticmethod
    def freeze():
        ''' Make this object immutable. After this the object can be hashed, used in dictionaries & sets.

        '''
        pass

    def make_compatible(self, other):
        ''' Make this euler compatible with another, so interpolating between them works as intended.

        '''
        pass

    def rotate(self, other: typing.Union['Euler', 'Matrix', 'Quaternion']):
        ''' Rotates the euler by another mathutils value.

        :param other: rotation component of mathutils value
        :type other: typing.Union['Euler', 'Matrix', 'Quaternion']
        '''
        pass

    def rotate_axis(self, axis: str, angle: float):
        ''' Rotates the euler a certain amount and returning a unique euler rotation (no 720 degree pitches).

        :param axis: single character in ['X, 'Y', 'Z'].
        :type axis: str
        :param angle: angle in radians.
        :type angle: float
        '''
        pass

    def to_matrix(self) -> 'Matrix':
        ''' Return a matrix representation of the euler.

        :rtype: 'Matrix'
        :return: A 3x3 rotation matrix representation of the euler.
        '''
        pass

    def to_quaternion(self) -> 'Quaternion':
        ''' Return a quaternion representation of the euler.

        :rtype: 'Quaternion'
        :return: Quaternion representation of the euler.
        '''
        pass

    def zero(self):
        ''' Set all values to zero.

        '''
        pass

    def __init__(self, angles=(0.0, 0.0, 0.0), order='XYZ'):
        ''' 

        '''
        pass


class Matrix:
    ''' This object gives access to Matrices in Blender, supporting square and rectangular matrices from 2x2 up to 4x4. :param rows: Sequence of rows. When omitted, a 4x4 identity matrix is constructed. :type rows: 2d number sequence
    '''

    col: 'Matrix' = None
    ''' Access the matrix by columns, 3x3 and 4x4 only, (read-only).

    :type: 'Matrix'
    '''

    is_frozen: bool = None
    ''' True when this object has been frozen (read-only).

    :type: bool
    '''

    is_negative: bool = None
    ''' True if this matrix results in a negative scale, 3x3 and 4x4 only, (read-only).

    :type: bool
    '''

    is_orthogonal: bool = None
    ''' True if this matrix is orthogonal, 3x3 and 4x4 only, (read-only).

    :type: bool
    '''

    is_orthogonal_axis_vectors: bool = None
    ''' True if this matrix has got orthogonal axis vectors, 3x3 and 4x4 only, (read-only).

    :type: bool
    '''

    is_wrapped: bool = None
    ''' True when this object wraps external data (read-only).

    :type: bool
    '''

    median_scale: float = None
    ''' The average scale applied to each axis (read-only).

    :type: float
    '''

    owner = None
    ''' The item this is wrapping or None (read-only).'''

    row: 'Matrix' = None
    ''' Access the matrix by rows (default), (read-only).

    :type: 'Matrix'
    '''

    translation: 'Vector' = None
    ''' The translation component of the matrix.

    :type: 'Vector'
    '''

    @classmethod
    def Diagonal(cls, vector: 'Vector') -> 'Matrix':
        ''' Create a diagonal (scaling) matrix using the values from the vector.

        :param vector: The vector of values for the diagonal.
        :type vector: 'Vector'
        :rtype: 'Matrix'
        :return: A diagonal matrix.
        '''
        pass

    @classmethod
    def Identity(cls, size: int) -> 'Matrix':
        ''' Create an identity matrix.

        :param size: The size of the identity matrix to construct [2, 4].
        :type size: int
        :rtype: 'Matrix'
        :return: A new identity matrix.
        '''
        pass

    @classmethod
    def OrthoProjection(cls, axis: typing.Union[str, 'Vector'],
                        size: int) -> 'Matrix':
        ''' Create a matrix to represent an orthographic projection.

        :param axis: ['X', 'Y', 'XY', 'XZ', 'YZ'], where a single axis is for a 2D matrix. Or a vector for an arbitrary axis
        :type axis: typing.Union[str, 'Vector']
        :param size: The size of the projection matrix to construct [2, 4].
        :type size: int
        :rtype: 'Matrix'
        :return: A new projection matrix.
        '''
        pass

    @classmethod
    def Rotation(cls, angle: float, size: int,
                 axis: typing.Union[str, 'Vector']) -> 'Matrix':
        ''' Create a matrix representing a rotation.

        :param angle: The angle of rotation desired, in radians.
        :type angle: float
        :param size: The size of the rotation matrix to construct [2, 4].
        :type size: int
        :param axis: a string in ['X', 'Y', 'Z'] or a 3D Vector Object (optional when size is 2).
        :type axis: typing.Union[str, 'Vector']
        :rtype: 'Matrix'
        :return: A new rotation matrix.
        '''
        pass

    @classmethod
    def Scale(cls, factor: float, size: int, axis: 'Vector') -> 'Matrix':
        ''' Create a matrix representing a scaling.

        :param factor: The factor of scaling to apply.
        :type factor: float
        :param size: The size of the scale matrix to construct [2, 4].
        :type size: int
        :param axis: Direction to influence scale. (optional).
        :type axis: 'Vector'
        :rtype: 'Matrix'
        :return: A new scale matrix.
        '''
        pass

    @classmethod
    def Shear(cls, plane: str, size: int, factor: float) -> 'Matrix':
        ''' Create a matrix to represent an shear transformation.

        :param plane: ['X', 'Y', 'XY', 'XZ', 'YZ'], where a single axis is for a 2D matrix only.
        :type plane: str
        :param size: The size of the shear matrix to construct [2, 4].
        :type size: int
        :param factor: The factor of shear to apply. For a 3 or 4 *size* matrix pass a pair of floats corresponding with the *plane* axis.
        :type factor: float
        :rtype: 'Matrix'
        :return: A new shear matrix.
        '''
        pass

    @classmethod
    def Translation(cls, vector: 'Vector') -> 'Matrix':
        ''' Create a matrix representing a translation.

        :param vector: The translation vector.
        :type vector: 'Vector'
        :rtype: 'Matrix'
        :return: An identity matrix with a translation.
        '''
        pass

    def adjugate(self):
        ''' Set the matrix to its adjugate. :raises ValueError: if the matrix cannot be adjugate.

        '''
        pass

    def adjugated(self) -> 'Matrix':
        ''' Return an adjugated copy of the matrix. :raises ValueError: if the matrix cannot be adjugated

        :rtype: 'Matrix'
        :return: the adjugated matrix.
        '''
        pass

    def copy(self) -> 'Matrix':
        ''' Returns a copy of this matrix.

        :rtype: 'Matrix'
        :return: an instance of itself
        '''
        pass

    def decompose(self) -> 'Vector':
        ''' Return the translation, rotation, and scale components of this matrix.

        :rtype: 'Vector'
        :return: tuple of translation, rotation, and scale
        '''
        pass

    def determinant(self) -> float:
        ''' Return the determinant of a matrix.

        :rtype: float
        :return: Return the determinant of a matrix.
        '''
        pass

    @staticmethod
    def freeze():
        ''' Make this object immutable. After this the object can be hashed, used in dictionaries & sets.

        '''
        pass

    def identity(self):
        ''' Set the matrix to the identity matrix.

        '''
        pass

    def invert(self, fallback: 'Matrix' = None):
        ''' Set the matrix to its inverse.

        :param fallback: Set the matrix to this value when the inverse cannot be calculated (instead of raising a :exc: ValueError exception).
        :type fallback: 'Matrix'
        '''
        pass

    def invert_safe(self):
        ''' Set the matrix to its inverse, will never error. If degenerated (e.g. zero scale on an axis), add some epsilon to its diagonal, to get an invertible one. If tweaked matrix is still degenerated, set to the identity matrix instead.

        '''
        pass

    def inverted(self, fallback=None) -> 'Matrix':
        ''' Return an inverted copy of the matrix.

        :param fallback: return this when the inverse can't be calculated (instead of raising a :exc: ValueError ).
        :type fallback: 
        :rtype: 'Matrix'
        :return: the inverted matrix or fallback when given.
        '''
        pass

    def inverted_safe(self) -> 'Matrix':
        ''' Return an inverted copy of the matrix, will never error. If degenerated (e.g. zero scale on an axis), add some epsilon to its diagonal, to get an invertible one. If tweaked matrix is still degenerated, return the identity matrix instead.

        :rtype: 'Matrix'
        :return: the inverted matrix.
        '''
        pass

    @staticmethod
    def lerp(other: 'Matrix', factor: float) -> 'Matrix':
        ''' Returns the interpolation of two matrices. Uses polar decomposition, see "Matrix Animation and Polar Decomposition", Shoemake and Duff, 1992.

        :param other: value to interpolate with.
        :type other: 'Matrix'
        :param factor: The interpolation value in [0.0, 1.0].
        :type factor: float
        :rtype: 'Matrix'
        :return: The interpolated matrix.
        '''
        pass

    def normalize(self):
        ''' Normalize each of the matrix columns.

        '''
        pass

    def normalized(self) -> 'Matrix':
        ''' Return a column normalized matrix

        :rtype: 'Matrix'
        :return: a column normalized matrix
        '''
        pass

    def resize_4x4(self):
        ''' Resize the matrix to 4x4.

        '''
        pass

    def rotate(self, other: typing.Union['Euler', 'Matrix', 'Quaternion']):
        ''' Rotates the matrix by another mathutils value.

        :param other: rotation component of mathutils value
        :type other: typing.Union['Euler', 'Matrix', 'Quaternion']
        '''
        pass

    def to_2x2(self) -> 'Matrix':
        ''' Return a 2x2 copy of this matrix.

        :rtype: 'Matrix'
        :return: a new matrix.
        '''
        pass

    def to_3x3(self) -> 'Matrix':
        ''' Return a 3x3 copy of this matrix.

        :rtype: 'Matrix'
        :return: a new matrix.
        '''
        pass

    def to_4x4(self) -> 'Matrix':
        ''' Return a 4x4 copy of this matrix.

        :rtype: 'Matrix'
        :return: a new matrix.
        '''
        pass

    def to_euler(self, order: str, euler_compat: 'Euler') -> 'Euler':
        ''' Return an Euler representation of the rotation matrix (3x3 or 4x4 matrix only).

        :param order: Optional rotation order argument in ['XYZ', 'XZY', 'YXZ', 'YZX', 'ZXY', 'ZYX'].
        :type order: str
        :param euler_compat: Optional euler argument the new euler will be made compatible with (no axis flipping between them). Useful for converting a series of matrices to animation curves.
        :type euler_compat: 'Euler'
        :rtype: 'Euler'
        :return: Euler representation of the matrix.
        '''
        pass

    def to_quaternion(self) -> 'Quaternion':
        ''' Return a quaternion representation of the rotation matrix.

        :rtype: 'Quaternion'
        :return: Quaternion representation of the rotation matrix.
        '''
        pass

    def to_scale(self) -> 'Vector':
        ''' Return the scale part of a 3x3 or 4x4 matrix.

        :rtype: 'Vector'
        :return: Return the scale of a matrix.
        '''
        pass

    def to_translation(self) -> 'Vector':
        ''' Return the translation part of a 4 row matrix.

        :rtype: 'Vector'
        :return: Return the translation of a matrix.
        '''
        pass

    def transpose(self):
        ''' Set the matrix to its transpose.

        '''
        pass

    def transposed(self) -> 'Matrix':
        ''' Return a new, transposed matrix.

        :rtype: 'Matrix'
        :return: a transposed matrix
        '''
        pass

    def zero(self):
        ''' Set all the matrix values to zero.

        '''
        pass

    def __init__(self,
                 rows=((1.0, 0.0, 0.0, 0.0), (0.0, 1.0, 0.0, 0.0),
                       (0.0, 0.0, 1.0, 0.0), (0.0, 0.0, 0.0, 1.0))):
        ''' 

        '''
        pass


class Quaternion:
    ''' This object gives access to Quaternions in Blender. :param seq: size 3 or 4 :type seq: Vector :param angle: rotation angle, in radians :type angle: float The constructor takes arguments in various forms: (), *no args* Create an identity quaternion (*wxyz*) Create a quaternion from a (w, x, y, z) vector. (*exponential_map*) Create a quaternion from a 3d exponential map vector. .. seealso:: :meth: to_exponential_map (*axis, angle*) Create a quaternion representing a rotation of *angle* radians over *axis*. .. seealso:: :meth: to_axis_angle
    '''

    angle: float = None
    ''' Angle of the quaternion.

    :type: float
    '''

    axis: 'Vector' = None
    ''' Quaternion axis as a vector.

    :type: 'Vector'
    '''

    is_frozen: bool = None
    ''' True when this object has been frozen (read-only).

    :type: bool
    '''

    is_wrapped: bool = None
    ''' True when this object wraps external data (read-only).

    :type: bool
    '''

    magnitude: float = None
    ''' Size of the quaternion (read-only).

    :type: float
    '''

    owner = None
    ''' The item this is wrapping or None (read-only).'''

    w: float = None
    ''' Quaternion axis value.

    :type: float
    '''

    x: float = None
    ''' Quaternion axis value.

    :type: float
    '''

    y: float = None
    ''' Quaternion axis value.

    :type: float
    '''

    z: float = None
    ''' Quaternion axis value.

    :type: float
    '''

    @staticmethod
    def conjugate():
        ''' Set the quaternion to its conjugate (negate x, y, z).

        '''
        pass

    @staticmethod
    def conjugated() -> 'Quaternion':
        ''' Return a new conjugated quaternion.

        :rtype: 'Quaternion'
        :return: a new quaternion.
        '''
        pass

    @staticmethod
    def copy() -> 'Quaternion':
        ''' Returns a copy of this quaternion.

        :rtype: 'Quaternion'
        :return: A copy of the quaternion.
        '''
        pass

    def cross(self, other: 'Quaternion') -> 'Quaternion':
        ''' Return the cross product of this quaternion and another.

        :param other: The other quaternion to perform the cross product with.
        :type other: 'Quaternion'
        :rtype: 'Quaternion'
        :return: The cross product.
        '''
        pass

    def dot(self, other: 'Quaternion') -> float:
        ''' Return the dot product of this quaternion and another.

        :param other: The other quaternion to perform the dot product with.
        :type other: 'Quaternion'
        :rtype: float
        :return: The dot product.
        '''
        pass

    @staticmethod
    def freeze():
        ''' Make this object immutable. After this the object can be hashed, used in dictionaries & sets.

        '''
        pass

    @staticmethod
    def identity():
        ''' Set the quaternion to an identity quaternion.

        '''
        pass

    @staticmethod
    def invert():
        ''' Set the quaternion to its inverse.

        '''
        pass

    @staticmethod
    def inverted() -> 'Quaternion':
        ''' Return a new, inverted quaternion.

        :rtype: 'Quaternion'
        :return: the inverted value.
        '''
        pass

    def make_compatible(self, other):
        ''' Make this quaternion compatible with another, so interpolating between them works as intended.

        '''
        pass

    @staticmethod
    def negate():
        ''' Set the quaternion to its negative.

        '''
        pass

    @staticmethod
    def normalize():
        ''' Normalize the quaternion.

        '''
        pass

    @staticmethod
    def normalized() -> 'Quaternion':
        ''' Return a new normalized quaternion.

        :rtype: 'Quaternion'
        :return: a normalized copy.
        '''
        pass

    def rotate(self, other: typing.Union['Euler', 'Matrix', 'Quaternion']):
        ''' Rotates the quaternion by another mathutils value.

        :param other: rotation component of mathutils value
        :type other: typing.Union['Euler', 'Matrix', 'Quaternion']
        '''
        pass

    @staticmethod
    def rotation_difference(other: 'Quaternion') -> 'Quaternion':
        ''' Returns a quaternion representing the rotational difference.

        :param other: second quaternion.
        :type other: 'Quaternion'
        :rtype: 'Quaternion'
        :return: the rotational difference between the two quat rotations.
        '''
        pass

    @staticmethod
    def slerp(other: 'Quaternion', factor: float) -> 'Quaternion':
        ''' Returns the interpolation of two quaternions.

        :param other: value to interpolate with.
        :type other: 'Quaternion'
        :param factor: The interpolation value in [0.0, 1.0].
        :type factor: float
        :rtype: 'Quaternion'
        :return: The interpolated rotation.
        '''
        pass

    def to_axis_angle(self) -> typing.Union[float, 'Vector']:
        ''' Return the axis, angle representation of the quaternion.

        :rtype: typing.Union[float, 'Vector']
        :return: axis, angle.
        '''
        pass

    def to_euler(self, order: str, euler_compat: 'Euler') -> 'Euler':
        ''' Return Euler representation of the quaternion.

        :param order: Optional rotation order argument in ['XYZ', 'XZY', 'YXZ', 'YZX', 'ZXY', 'ZYX'].
        :type order: str
        :param euler_compat: Optional euler argument the new euler will be made compatible with (no axis flipping between them). Useful for converting a series of matrices to animation curves.
        :type euler_compat: 'Euler'
        :rtype: 'Euler'
        :return: Euler representation of the quaternion.
        '''
        pass

    def to_exponential_map(self) -> 'Vector':
        ''' Return the exponential map representation of the quaternion. This representation consist of the rotation axis multiplied by the rotation angle. Such a representation is useful for interpolation between multiple orientations. To convert back to a quaternion, pass it to the Quaternion constructor.

        :rtype: 'Vector'
        :return: exponential map.
        '''
        pass

    def to_matrix(self) -> 'Matrix':
        ''' Return a matrix representation of the quaternion.

        :rtype: 'Matrix'
        :return: A 3x3 rotation matrix representation of the quaternion.
        '''
        pass

    def to_swing_twist(self, axis) -> typing.Union[float, 'Quaternion']:
        ''' Split the rotation into a swing quaternion with the specified axis fixed at zero, and the remaining twist rotation angle.

        :param axis: 
        :type axis: 
        :rtype: typing.Union[float, 'Quaternion']
        :return: swing, twist angle.
        '''
        pass

    def __init__(self, seq=(1.0, 0.0, 0.0, 0.0)):
        ''' 

        '''
        pass


class Vector:
    ''' This object gives access to Vectors in Blender. :param seq: Components of the vector, must be a sequence of at least two :type seq: sequence of numbers
    '''

    is_frozen: bool = None
    ''' True when this object has been frozen (read-only).

    :type: bool
    '''

    is_wrapped: bool = None
    ''' True when this object wraps external data (read-only).

    :type: bool
    '''

    length: float = None
    ''' Vector Length.

    :type: float
    '''

    length_squared: float = None
    ''' Vector length squared (v.dot(v)).

    :type: float
    '''

    magnitude: float = None
    ''' Vector Length.

    :type: float
    '''

    owner = None
    ''' The item this is wrapping or None (read-only).'''

    w: float = None
    ''' Vector W axis (4D Vectors only).

    :type: float
    '''

    ww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    www = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwwx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwwy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwwz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wwzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxwx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxwy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxwz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wxzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wyww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wywx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wywy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wywz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wyxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wyxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wyxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wyxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wyyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wyyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wyyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wyyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wyzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wyzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wyzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wyzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzwx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzwy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzwz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    wzzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    x: float = None
    ''' Vector X axis.

    :type: float
    '''

    xw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwwx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwwy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwwz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xwzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxwx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxwy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxwz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xxzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xyww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xywx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xywy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xywz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xyxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xyxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xyxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xyxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xyyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xyyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xyyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xyyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xyzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xyzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xyzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xyzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzwx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzwy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzwz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    xzzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    y: float = None
    ''' Vector Y axis.

    :type: float
    '''

    yw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywwx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywwy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywwz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    ywzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxwx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxwy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxwz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yxzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yyww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yywx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yywy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yywz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yyxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yyxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yyxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yyxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yyyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yyyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yyyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yyyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yyzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yyzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yyzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yyzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzwx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzwy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzwz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    yzzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    z: float = None
    ''' Vector Z axis (3D Vectors only).

    :type: float
    '''

    zw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwwx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwwy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwwz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zwzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxwx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxwy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxwz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zxzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zyww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zywx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zywy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zywz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zyxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zyxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zyxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zyxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zyyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zyyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zyyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zyyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zyzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zyzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zyzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zyzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzww = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzwx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzwy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzwz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzxw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzxx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzxy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzxz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzyw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzyx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzyy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzyz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzzw = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzzx = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzzy = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    zzzz = None
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.'''

    @classmethod
    def Fill(cls, size: int, fill: float = 0.0):
        ''' Create a vector of length size with all values set to fill.

        :param size: The length of the vector to be created.
        :type size: int
        :param fill: The value used to fill the vector.
        :type fill: float
        '''
        pass

    @classmethod
    def Linspace(cls, start: int, stop: int, size: int):
        ''' Create a vector of the specified size which is filled with linearly spaced values between start and stop values.

        :param start: The start of the range used to fill the vector.
        :type start: int
        :param stop: The end of the range used to fill the vector.
        :type stop: int
        :param size: The size of the vector to be created.
        :type size: int
        '''
        pass

    @classmethod
    def Range(cls, start: int = 0, stop: int = -1, step: int = 1):
        ''' Create a filled with a range of values.

        :param start: The start of the range used to fill the vector.
        :type start: int
        :param stop: The end of the range used to fill the vector.
        :type stop: int
        :param step: The step between successive values in the vector.
        :type step: int
        '''
        pass

    @classmethod
    def Repeat(cls, vector, size: int):
        ''' Create a vector by repeating the values in vector until the required size is reached.

        :param tuple: The vector to draw values from.
        :type tuple: 'Vector'
        :param size: The size of the vector to be created.
        :type size: int
        '''
        pass

    @staticmethod
    def angle(other: 'Vector', fallback=None) -> float:
        ''' Return the angle between two vectors.

        :param other: another vector to compare the angle with
        :type other: 'Vector'
        :param fallback: return this when the angle can't be calculated (zero length vector), (instead of raising a :exc: ValueError ).
        :type fallback: 
        :rtype: float
        :return: angle in radians or fallback when given
        '''
        pass

    @staticmethod
    def angle_signed(other: 'Vector', fallback) -> float:
        ''' Return the signed angle between two 2D vectors (clockwise is positive).

        :param other: another vector to compare the angle with
        :type other: 'Vector'
        :param fallback: return this when the angle can't be calculated (zero length vector), (instead of raising a :exc: ValueError ).
        :type fallback: 
        :rtype: float
        :return: angle in radians or fallback when given
        '''
        pass

    @staticmethod
    def copy() -> 'Vector':
        ''' Returns a copy of this vector.

        :rtype: 'Vector'
        :return: A copy of the vector.
        '''
        pass

    def cross(self, other: 'Vector') -> typing.Union[float, 'Vector']:
        ''' Return the cross product of this vector and another.

        :param other: The other vector to perform the cross product with.
        :type other: 'Vector'
        :rtype: typing.Union[float, 'Vector']
        :return: The cross product.
        '''
        pass

    def dot(self, other: 'Vector') -> float:
        ''' Return the dot product of this vector and another.

        :param other: The other vector to perform the dot product with.
        :type other: 'Vector'
        :rtype: float
        :return: The dot product.
        '''
        pass

    @staticmethod
    def freeze():
        ''' Make this object immutable. After this the object can be hashed, used in dictionaries & sets.

        '''
        pass

    @staticmethod
    def lerp(other: 'Vector', factor: float) -> 'Vector':
        ''' Returns the interpolation of two vectors.

        :param other: value to interpolate with.
        :type other: 'Vector'
        :param factor: The interpolation value in [0.0, 1.0].
        :type factor: float
        :rtype: 'Vector'
        :return: The interpolated vector.
        '''
        pass

    def negate(self):
        ''' Set all values to their negative.

        '''
        pass

    def normalize(self):
        ''' Normalize the vector, making the length of the vector always 1.0.

        '''
        pass

    def normalized(self) -> 'Vector':
        ''' Return a new, normalized vector.

        :rtype: 'Vector'
        :return: a normalized copy of the vector
        '''
        pass

    def orthogonal(self) -> 'Vector':
        ''' Return a perpendicular vector.

        :rtype: 'Vector'
        :return: a new vector 90 degrees from this vector.
        '''
        pass

    @staticmethod
    def project(other: 'Vector') -> 'Vector':
        ''' Return the projection of this vector onto the *other*.

        :param other: second vector.
        :type other: 'Vector'
        :rtype: 'Vector'
        :return: the parallel projection vector
        '''
        pass

    def reflect(self, mirror: 'Vector') -> 'Vector':
        ''' Return the reflection vector from the *mirror* argument.

        :param mirror: This vector could be a normal from the reflecting surface.
        :type mirror: 'Vector'
        :rtype: 'Vector'
        :return: The reflected vector matching the size of this vector.
        '''
        pass

    def resize(self, size=3):
        ''' Resize the vector to have size number of elements.

        '''
        pass

    def resize_2d(self):
        ''' Resize the vector to 2D (x, y).

        '''
        pass

    def resize_3d(self):
        ''' Resize the vector to 3D (x, y, z).

        '''
        pass

    def resize_4d(self):
        ''' Resize the vector to 4D (x, y, z, w).

        '''
        pass

    def resized(self, size=3) -> 'Vector':
        ''' Return a resized copy of the vector with size number of elements.

        :rtype: 'Vector'
        :return: a new vector
        '''
        pass

    @staticmethod
    def rotate(other: typing.Union['Euler', 'Matrix', 'Quaternion']):
        ''' Rotate the vector by a rotation value.

        :param other: rotation component of mathutils value
        :type other: typing.Union['Euler', 'Matrix', 'Quaternion']
        '''
        pass

    @staticmethod
    def rotation_difference(other: 'Vector') -> 'Quaternion':
        ''' Returns a quaternion representing the rotational difference between this vector and another.

        :param other: second vector.
        :type other: 'Vector'
        :rtype: 'Quaternion'
        :return: the rotational difference between the two vectors.
        '''
        pass

    @staticmethod
    def slerp(other: 'Vector', factor: float, fallback=None) -> 'Vector':
        ''' Returns the interpolation of two non-zero vectors (spherical coordinates).

        :param other: value to interpolate with.
        :type other: 'Vector'
        :param factor: The interpolation value typically in [0.0, 1.0].
        :type factor: float
        :param fallback: return this when the vector can't be calculated (zero length vector or direct opposites), (instead of raising a :exc: ValueError ).
        :type fallback: 
        :rtype: 'Vector'
        :return: The interpolated vector.
        '''
        pass

    def to_2d(self) -> 'Vector':
        ''' Return a 2d copy of the vector.

        :rtype: 'Vector'
        :return: a new vector
        '''
        pass

    def to_3d(self) -> 'Vector':
        ''' Return a 3d copy of the vector.

        :rtype: 'Vector'
        :return: a new vector
        '''
        pass

    def to_4d(self) -> 'Vector':
        ''' Return a 4d copy of the vector.

        :rtype: 'Vector'
        :return: a new vector
        '''
        pass

    def to_track_quat(self, track: str, up: str) -> 'Quaternion':
        ''' Return a quaternion rotation from the vector and the track and up axis.

        :param track: Track axis in ['X', 'Y', 'Z', '-X', '-Y', '-Z'].
        :type track: str
        :param up: Up axis in ['X', 'Y', 'Z'].
        :type up: str
        :rtype: 'Quaternion'
        :return: rotation from the vector and the track and up axis.
        '''
        pass

    def to_tuple(self, precision: int = -1) -> tuple:
        ''' Return this vector as a tuple with.

        :param precision: The number to round the value to in [-1, 21].
        :type precision: int
        :rtype: tuple
        :return: the values of the vector rounded by *precision*
        '''
        pass

    def zero(self):
        ''' Set all values to zero.

        '''
        pass

    def __init__(self, seq=(0.0, 0.0, 0.0)):
        ''' 

        '''
        pass
