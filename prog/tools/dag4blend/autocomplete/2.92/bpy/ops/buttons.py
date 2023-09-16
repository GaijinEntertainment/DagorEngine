import sys
import typing


def clear_filter():
    ''' Clear the search filter

    '''

    pass


def context_menu():
    ''' Display properties editor context_menu

    '''

    pass


def directory_browse(directory: str = "",
                     hide_props_region: bool = True,
                     filter_blender: bool = False,
                     filter_backup: bool = False,
                     filter_image: bool = False,
                     filter_movie: bool = False,
                     filter_python: bool = False,
                     filter_font: bool = False,
                     filter_sound: bool = False,
                     filter_text: bool = False,
                     filter_archive: bool = False,
                     filter_btx: bool = False,
                     filter_collada: bool = False,
                     filter_alembic: bool = False,
                     filter_usd: bool = False,
                     filter_volume: bool = False,
                     filter_folder: bool = False,
                     filter_blenlib: bool = False,
                     filemode: int = 9,
                     relative_path: bool = True,
                     display_type: typing.Union[int, str] = 'DEFAULT',
                     sort_method: typing.Union[int, str] = ''):
    ''' Open a directory browser, hold Shift to open the file, Alt to browse containing directory

    :param directory: Directory, Directory of the file
    :type directory: str
    :param hide_props_region: Hide Operator Properties, Collapse the region displaying the operator settings
    :type hide_props_region: bool
    :param filter_blender: Filter .blend files
    :type filter_blender: bool
    :param filter_backup: Filter .blend files
    :type filter_backup: bool
    :param filter_image: Filter image files
    :type filter_image: bool
    :param filter_movie: Filter movie files
    :type filter_movie: bool
    :param filter_python: Filter python files
    :type filter_python: bool
    :param filter_font: Filter font files
    :type filter_font: bool
    :param filter_sound: Filter sound files
    :type filter_sound: bool
    :param filter_text: Filter text files
    :type filter_text: bool
    :param filter_archive: Filter archive files
    :type filter_archive: bool
    :param filter_btx: Filter btx files
    :type filter_btx: bool
    :param filter_collada: Filter COLLADA files
    :type filter_collada: bool
    :param filter_alembic: Filter Alembic files
    :type filter_alembic: bool
    :param filter_usd: Filter USD files
    :type filter_usd: bool
    :param filter_volume: Filter OpenVDB volume files
    :type filter_volume: bool
    :param filter_folder: Filter folders
    :type filter_folder: bool
    :param filter_blenlib: Filter Blender IDs
    :type filter_blenlib: bool
    :param filemode: File Browser Mode, The setting for the file browser mode to load a .blend file, a library or a special file
    :type filemode: int
    :param relative_path: Relative Path, Select the file relative to the blend file
    :type relative_path: bool
    :param display_type: Display Type * DEFAULT Default, Automatically determine display type for files. * LIST_VERTICAL Short List, Display files as short list. * LIST_HORIZONTAL Long List, Display files as a detailed list. * THUMBNAIL Thumbnails, Display files as thumbnails.
    :type display_type: typing.Union[int, str]
    :param sort_method: File sorting mode
    :type sort_method: typing.Union[int, str]
    '''

    pass


def file_browse(filepath: str = "",
                hide_props_region: bool = True,
                filter_blender: bool = False,
                filter_backup: bool = False,
                filter_image: bool = False,
                filter_movie: bool = False,
                filter_python: bool = False,
                filter_font: bool = False,
                filter_sound: bool = False,
                filter_text: bool = False,
                filter_archive: bool = False,
                filter_btx: bool = False,
                filter_collada: bool = False,
                filter_alembic: bool = False,
                filter_usd: bool = False,
                filter_volume: bool = False,
                filter_folder: bool = False,
                filter_blenlib: bool = False,
                filemode: int = 9,
                relative_path: bool = True,
                display_type: typing.Union[int, str] = 'DEFAULT',
                sort_method: typing.Union[int, str] = ''):
    ''' Open a file browser, hold Shift to open the file, Alt to browse containing directory

    :param filepath: File Path, Path to file
    :type filepath: str
    :param hide_props_region: Hide Operator Properties, Collapse the region displaying the operator settings
    :type hide_props_region: bool
    :param filter_blender: Filter .blend files
    :type filter_blender: bool
    :param filter_backup: Filter .blend files
    :type filter_backup: bool
    :param filter_image: Filter image files
    :type filter_image: bool
    :param filter_movie: Filter movie files
    :type filter_movie: bool
    :param filter_python: Filter python files
    :type filter_python: bool
    :param filter_font: Filter font files
    :type filter_font: bool
    :param filter_sound: Filter sound files
    :type filter_sound: bool
    :param filter_text: Filter text files
    :type filter_text: bool
    :param filter_archive: Filter archive files
    :type filter_archive: bool
    :param filter_btx: Filter btx files
    :type filter_btx: bool
    :param filter_collada: Filter COLLADA files
    :type filter_collada: bool
    :param filter_alembic: Filter Alembic files
    :type filter_alembic: bool
    :param filter_usd: Filter USD files
    :type filter_usd: bool
    :param filter_volume: Filter OpenVDB volume files
    :type filter_volume: bool
    :param filter_folder: Filter folders
    :type filter_folder: bool
    :param filter_blenlib: Filter Blender IDs
    :type filter_blenlib: bool
    :param filemode: File Browser Mode, The setting for the file browser mode to load a .blend file, a library or a special file
    :type filemode: int
    :param relative_path: Relative Path, Select the file relative to the blend file
    :type relative_path: bool
    :param display_type: Display Type * DEFAULT Default, Automatically determine display type for files. * LIST_VERTICAL Short List, Display files as short list. * LIST_HORIZONTAL Long List, Display files as a detailed list. * THUMBNAIL Thumbnails, Display files as thumbnails.
    :type display_type: typing.Union[int, str]
    :param sort_method: File sorting mode
    :type sort_method: typing.Union[int, str]
    '''

    pass


def start_filter():
    ''' Start entering filter text

    '''

    pass


def toggle_pin():
    ''' Keep the current data-block displayed

    '''

    pass
