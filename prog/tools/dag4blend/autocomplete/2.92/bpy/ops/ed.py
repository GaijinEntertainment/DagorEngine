import sys
import typing


def flush_edits():
    ''' Flush edit data from active editing modes

    '''

    pass


def lib_id_fake_user_toggle():
    ''' Save this data-block even if it has no users

    '''

    pass


def lib_id_generate_preview():
    ''' Create an automatic preview for the selected data-block

    '''

    pass


def lib_id_load_custom_preview(
        filepath: str = "",
        hide_props_region: bool = True,
        filter_blender: bool = False,
        filter_backup: bool = False,
        filter_image: bool = True,
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
        filter_folder: bool = True,
        filter_blenlib: bool = False,
        filemode: int = 9,
        show_multiview: bool = False,
        use_multiview: bool = False,
        display_type: typing.Union[int, str] = 'DEFAULT',
        sort_method: typing.Union[int, str] = ''):
    ''' Choose an image to help identify the data-block visually

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
    :param show_multiview: Enable Multi-View
    :type show_multiview: bool
    :param use_multiview: Use Multi-View
    :type use_multiview: bool
    :param display_type: Display Type * DEFAULT Default, Automatically determine display type for files. * LIST_VERTICAL Short List, Display files as short list. * LIST_HORIZONTAL Long List, Display files as a detailed list. * THUMBNAIL Thumbnails, Display files as thumbnails.
    :type display_type: typing.Union[int, str]
    :param sort_method: File sorting mode
    :type sort_method: typing.Union[int, str]
    '''

    pass


def lib_id_unlink():
    ''' Remove a usage of a data-block, clearing the assignment

    '''

    pass


def redo():
    ''' Redo previous action

    '''

    pass


def undo():
    ''' Undo previous action

    '''

    pass


def undo_history(item: int = 0):
    ''' Redo specific action in history

    :param item: Item
    :type item: int
    '''

    pass


def undo_push(message: str = "Add an undo step *function may be moved*"):
    ''' Add an undo state (internal use only)

    :param message: Undo Message
    :type message: str
    '''

    pass


def undo_redo():
    ''' Undo and redo previous action

    '''

    pass
