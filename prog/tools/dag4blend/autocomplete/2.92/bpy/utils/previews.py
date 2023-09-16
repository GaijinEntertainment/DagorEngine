import sys
import typing
import bpy.types


class ImagePreviewCollection:
    ''' Dictionary-like class of previews. This is a subclass of Python's built-in dict type, used to store multiple image previews.
    '''

    def clear(self):
        ''' Clear all previews.

        '''
        pass

    def close(self):
        ''' Close the collection and clear all previews.

        '''
        pass

    def load(self,
             name: str,
             filepath: str,
             filetype: str,
             force_reload: bool = False) -> 'bpy.types.ImagePreview':
        ''' Generate a new preview from given file path. :raises KeyError: if name already exists.

        :param name: The name (unique id) identifying the preview.
        :type name: str
        :param filepath: The file path to generate the preview from.
        :type filepath: str
        :param filetype: The type of file, needed to generate the preview in ['IMAGE', 'MOVIE', 'BLEND', 'FONT'].
        :type filetype: str
        :param force_reload: If True, force running thumbnail manager even if preview already exists in cache.
        :type force_reload: bool
        :rtype: 'bpy.types.ImagePreview'
        :return: The Preview matching given name, or a new empty one.
        '''
        pass

    def new(self, name: str) -> 'bpy.types.ImagePreview':
        ''' Generate a new empty preview. :raises KeyError: if name already exists.

        :param name: The name (unique id) identifying the preview.
        :type name: str
        :rtype: 'bpy.types.ImagePreview'
        :return: The Preview matching given name, or a new empty one.
        '''
        pass


def new() -> 'ImagePreviewCollection':
    ''' 

    :return: a new preview collection.
    '''

    pass


def remove(pcoll: 'ImagePreviewCollection'):
    ''' Remove the specified previews collection.

    :param pcoll: Preview collection to close.
    :type pcoll: 'ImagePreviewCollection'
    '''

    pass
