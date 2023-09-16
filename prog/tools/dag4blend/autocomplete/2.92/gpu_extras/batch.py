import sys
import typing
import gpu.types


def batch_for_shader(shader: 'gpu.types.GPUShader',
                     type: str,
                     content: dict,
                     indices=None):
    ''' Return a batch already configured and compatible with the shader.

    :param shader: shader for which a compatible format will be computed.
    :type shader: 'gpu.types.GPUShader'
    :param type: "'POINTS', 'LINES', 'TRIS' or 'LINES_ADJ'".
    :type type: str
    :param content: Maps the name of the shader attribute with the data to fill the vertex buffer.
    :type content: dict
    :return: compatible batch
    '''

    pass
