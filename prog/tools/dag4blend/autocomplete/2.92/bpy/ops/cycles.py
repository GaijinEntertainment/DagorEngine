import sys
import typing


def denoise_animation(input_filepath: str = "", output_filepath: str = ""):
    ''' Denoise rendered animation sequence using current scene and view layer settings. Requires denoising data passes and output to OpenEXR multilayer files

    :param input_filepath: Input Filepath, File path for image to denoise. If not specified, uses the render file path and frame range from the scene
    :type input_filepath: str
    :param output_filepath: Output Filepath, If not specified, renders will be denoised in-place
    :type output_filepath: str
    '''

    pass


def merge_images(input_filepath1: str = "",
                 input_filepath2: str = "",
                 output_filepath: str = ""):
    ''' Combine OpenEXR multilayer images rendered with different sample ranges into one image with reduced noise

    :param input_filepath1: Input Filepath, File path for image to merge
    :type input_filepath1: str
    :param input_filepath2: Input Filepath, File path for image to merge
    :type input_filepath2: str
    :param output_filepath: Output Filepath, File path for merged image
    :type output_filepath: str
    '''

    pass


def use_shading_nodes():
    ''' Enable nodes on a material, world or light :file: addons/cycles/operators.py\:36 <https://developer.blender.org/diffusion/BA/addons/cycles/operators.py$36> _

    '''

    pass
