import sys
import typing


def add():
    ''' Add brush by mode type

    '''

    pass


def add_gpencil():
    ''' Add brush for Grease Pencil

    '''

    pass


def curve_preset(shape: typing.Union[int, str] = 'SMOOTH'):
    ''' Set brush shape

    :param shape: Mode
    :type shape: typing.Union[int, str]
    '''

    pass


def reset():
    ''' Return brush to defaults based on current tool

    '''

    pass


def scale_size(scalar: float = 1.0):
    ''' Change brush size by a scalar

    :param scalar: Scalar, Factor to scale brush size by
    :type scalar: float
    '''

    pass


def stencil_control(mode: typing.Union[int, str] = 'TRANSLATION',
                    texmode: typing.Union[int, str] = 'PRIMARY'):
    ''' Control the stencil brush

    :param mode: Tool
    :type mode: typing.Union[int, str]
    :param texmode: Tool
    :type texmode: typing.Union[int, str]
    '''

    pass


def stencil_fit_image_aspect(use_repeat: bool = True,
                             use_scale: bool = True,
                             mask: bool = False):
    ''' When using an image texture, adjust the stencil size to fit the image aspect ratio

    :param use_repeat: Use Repeat, Use repeat mapping values
    :type use_repeat: bool
    :param use_scale: Use Scale, Use texture scale values
    :type use_scale: bool
    :param mask: Modify Mask Stencil, Modify either the primary or mask stencil
    :type mask: bool
    '''

    pass


def stencil_reset_transform(mask: bool = False):
    ''' Reset the stencil transformation to the default

    :param mask: Modify Mask Stencil, Modify either the primary or mask stencil
    :type mask: bool
    '''

    pass
