import sys
import typing


def autocomplete():
    ''' Show a list of used text in the open document

    '''

    pass


def comment_toggle(type: typing.Union[int, str] = 'TOGGLE'):
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.

    :param type: Type, Add or remove comments
    :type type: typing.Union[int, str]
    '''

    pass


def convert_whitespace(type: typing.Union[int, str] = 'SPACES'):
    ''' Convert whitespaces by type

    :param type: Type, Type of whitespace to convert to
    :type type: typing.Union[int, str]
    '''

    pass


def copy():
    ''' Copy selected text to clipboard

    '''

    pass


def cursor_set(x: int = 0, y: int = 0):
    ''' Set cursor position

    :param x: X
    :type x: int
    :param y: Y
    :type y: int
    '''

    pass


def cut():
    ''' Cut selected text to clipboard

    '''

    pass


def delete(type: typing.Union[int, str] = 'NEXT_CHARACTER'):
    ''' Delete text by cursor position

    :param type: Type, Which part of the text to delete
    :type type: typing.Union[int, str]
    '''

    pass


def duplicate_line():
    ''' Duplicate the current line

    '''

    pass


def find():
    ''' Find specified text

    '''

    pass


def find_set_selected():
    ''' Find specified text and set as selected

    '''

    pass


def indent():
    ''' Indent selected text

    '''

    pass


def indent_or_autocomplete():
    ''' Indent selected text or autocomplete

    '''

    pass


def insert(text: str = ""):
    ''' Insert text at cursor position

    :param text: Text, Text to insert at the cursor position
    :type text: str
    '''

    pass


def jump(line: int = 1):
    ''' Jump cursor to line

    :param line: Line, Line number to jump to
    :type line: int
    '''

    pass


def line_break():
    ''' Insert line break at cursor position

    '''

    pass


def line_number():
    ''' The current line number

    '''

    pass


def make_internal():
    ''' Make active text file internal

    '''

    pass


def move(type: typing.Union[int, str] = 'LINE_BEGIN'):
    ''' Move cursor to position type

    :param type: Type, Where to move cursor to
    :type type: typing.Union[int, str]
    '''

    pass


def move_lines(direction: typing.Union[int, str] = 'DOWN'):
    ''' Move the currently selected line(s) up/down

    :param direction: Direction
    :type direction: typing.Union[int, str]
    '''

    pass


def move_select(type: typing.Union[int, str] = 'LINE_BEGIN'):
    ''' Move the cursor while selecting

    :param type: Type, Where to move cursor to, to make a selection
    :type type: typing.Union[int, str]
    '''

    pass


def new():
    ''' Create a new text data-block

    '''

    pass


def open(filepath: str = "",
         hide_props_region: bool = True,
         filter_blender: bool = False,
         filter_backup: bool = False,
         filter_image: bool = False,
         filter_movie: bool = False,
         filter_python: bool = True,
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
         sort_method: typing.Union[int, str] = '',
         internal: bool = False):
    ''' Open a new text data-block

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
    :param internal: Make Internal, Make text file internal after loading
    :type internal: bool
    '''

    pass


def overwrite_toggle():
    ''' Toggle overwrite while typing

    '''

    pass


def paste(selection: bool = False):
    ''' Paste text from clipboard

    :param selection: Selection, Paste text selected elsewhere rather than copied (X11 only)
    :type selection: bool
    '''

    pass


def refresh_pyconstraints():
    ''' Refresh all pyconstraints

    '''

    pass


def reload():
    ''' Reload active text data-block from its file

    '''

    pass


def replace(all: bool = False):
    ''' Replace text with the specified text

    :param all: Replace All, Replace all occurrences
    :type all: bool
    '''

    pass


def replace_set_selected():
    ''' Replace text with specified text and set as selected

    '''

    pass


def resolve_conflict(resolution: typing.Union[int, str] = 'IGNORE'):
    ''' When external text is out of sync, resolve the conflict

    :param resolution: Resolution, How to solve conflict due to differences in internal and external text
    :type resolution: typing.Union[int, str]
    '''

    pass


def run_script():
    ''' Run active script

    '''

    pass


def save():
    ''' Save active text data-block

    '''

    pass


def save_as(filepath: str = "",
            hide_props_region: bool = True,
            check_existing: bool = True,
            filter_blender: bool = False,
            filter_backup: bool = False,
            filter_image: bool = False,
            filter_movie: bool = False,
            filter_python: bool = True,
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
    ''' Save active text file with options

    :param filepath: File Path, Path to file
    :type filepath: str
    :param hide_props_region: Hide Operator Properties, Collapse the region displaying the operator settings
    :type hide_props_region: bool
    :param check_existing: Check Existing, Check and warn on overwriting existing files
    :type check_existing: bool
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


def scroll(lines: int = 1):
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.

    :param lines: Lines, Number of lines to scroll
    :type lines: int
    '''

    pass


def scroll_bar(lines: int = 1):
    ''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.

    :param lines: Lines, Number of lines to scroll
    :type lines: int
    '''

    pass


def select_all():
    ''' Select all text

    '''

    pass


def select_line():
    ''' Select text by line

    '''

    pass


def select_word():
    ''' Select word under cursor

    '''

    pass


def selection_set():
    ''' Set cursor selection

    '''

    pass


def start_find():
    ''' Start searching text

    '''

    pass


def to_3d_object(split_lines: bool = False):
    ''' Create 3D text object from active text data-block

    :param split_lines: Split Lines, Create one object per line in the text
    :type split_lines: bool
    '''

    pass


def unindent():
    ''' Unindent selected text

    '''

    pass


def unlink():
    ''' Unlink active text data-block

    '''

    pass
