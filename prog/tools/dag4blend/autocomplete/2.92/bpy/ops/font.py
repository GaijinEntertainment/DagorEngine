import sys
import typing


def case_set(case: typing.Union[int, str] = 'LOWER'):
    ''' Set font case

    :param case: Case, Lower or upper case
    :type case: typing.Union[int, str]
    '''

    pass


def case_toggle():
    ''' Toggle font case

    '''

    pass


def change_character(delta: int = 1):
    ''' Change font character code

    :param delta: Delta, Number to increase or decrease character code with
    :type delta: int
    '''

    pass


def change_spacing(delta: int = 1):
    ''' Change font spacing

    :param delta: Delta, Amount to decrease or increase character spacing with
    :type delta: int
    '''

    pass


def delete(type: typing.Union[int, str] = 'PREVIOUS_CHARACTER'):
    ''' Delete text by cursor position

    :param type: Type, Which part of the text to delete
    :type type: typing.Union[int, str]
    '''

    pass


def line_break():
    ''' Insert line break at cursor position

    '''

    pass


def move(type: typing.Union[int, str] = 'LINE_BEGIN'):
    ''' Move cursor to position type

    :param type: Type, Where to move cursor to
    :type type: typing.Union[int, str]
    '''

    pass


def move_select(type: typing.Union[int, str] = 'LINE_BEGIN'):
    ''' Move the cursor while selecting

    :param type: Type, Where to move cursor to, to make a selection
    :type type: typing.Union[int, str]
    '''

    pass


def open(filepath: str = "",
         hide_props_region: bool = True,
         filter_blender: bool = False,
         filter_backup: bool = False,
         filter_image: bool = False,
         filter_movie: bool = False,
         filter_python: bool = False,
         filter_font: bool = True,
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
         relative_path: bool = True,
         display_type: typing.Union[int, str] = 'DEFAULT',
         sort_method: typing.Union[int, str] = ''):
    ''' Load a new font from a file

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


def select_all():
    ''' Select all text

    '''

    pass


def style_set(style: typing.Union[int, str] = 'BOLD', clear: bool = False):
    ''' Set font style

    :param style: Style, Style to set selection to
    :type style: typing.Union[int, str]
    :param clear: Clear, Clear style rather than setting it
    :type clear: bool
    '''

    pass


def style_toggle(style: typing.Union[int, str] = 'BOLD'):
    ''' Toggle font style

    :param style: Style, Style to set selection to
    :type style: typing.Union[int, str]
    '''

    pass


def text_copy():
    ''' Copy selected text to clipboard

    '''

    pass


def text_cut():
    ''' Cut selected text to clipboard

    '''

    pass


def text_insert(text: str = "", accent: bool = False):
    ''' Insert text at cursor position

    :param text: Text, Text to insert at the cursor position
    :type text: str
    :param accent: Accent Mode, Next typed character will strike through previous, for special character input
    :type accent: bool
    '''

    pass


def text_paste():
    ''' Paste text from clipboard

    '''

    pass


def text_paste_from_file(filepath: str = "",
                         hide_props_region: bool = True,
                         filter_blender: bool = False,
                         filter_backup: bool = False,
                         filter_image: bool = False,
                         filter_movie: bool = False,
                         filter_python: bool = False,
                         filter_font: bool = False,
                         filter_sound: bool = False,
                         filter_text: bool = True,
                         filter_archive: bool = False,
                         filter_btx: bool = False,
                         filter_collada: bool = False,
                         filter_alembic: bool = False,
                         filter_usd: bool = False,
                         filter_volume: bool = False,
                         filter_folder: bool = True,
                         filter_blenlib: bool = False,
                         filemode: int = 9,
                         display_type: typing.Union[int, str] = 'DEFAULT',
                         sort_method: typing.Union[int, str] = ''):
    ''' Paste contents from file

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
    :param display_type: Display Type * DEFAULT Default, Automatically determine display type for files. * LIST_VERTICAL Short List, Display files as short list. * LIST_HORIZONTAL Long List, Display files as a detailed list. * THUMBNAIL Thumbnails, Display files as thumbnails.
    :type display_type: typing.Union[int, str]
    :param sort_method: File sorting mode
    :type sort_method: typing.Union[int, str]
    '''

    pass


def textbox_add():
    ''' Add a new text box

    '''

    pass


def textbox_remove(index: int = 0):
    ''' Remove the text box

    :param index: Index, The current text box
    :type index: int
    '''

    pass


def unlink():
    ''' Unlink active font data-block

    '''

    pass
