import sys
import typing


def autocomplete():
    ''' Evaluate the namespace up until the cursor and give a list of options or complete the name if there is only one :file: startup/bl_operators/console.py\:72 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/console.py$72> _

    '''

    pass


def banner():
    ''' Print a message when the terminal initializes :file: startup/bl_operators/console.py\:117 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/console.py$117> _

    '''

    pass


def clear(scrollback: bool = True, history: bool = False):
    ''' Clear text by type

    :param scrollback: Scrollback, Clear the scrollback history
    :type scrollback: bool
    :param history: History, Clear the command history
    :type history: bool
    '''

    pass


def clear_line():
    ''' Clear the line and store in history

    '''

    pass


def copy():
    ''' Copy selected text to clipboard

    '''

    pass


def copy_as_script():
    ''' Copy the console contents for use in a script :file: startup/bl_operators/console.py\:94 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/console.py$94> _

    '''

    pass


def delete(type: typing.Union[int, str] = 'NEXT_CHARACTER'):
    ''' Delete text by cursor position

    :param type: Type, Which part of the text to delete
    :type type: typing.Union[int, str]
    '''

    pass


def execute(interactive: bool = False):
    ''' Execute the current console line as a python expression

    :param interactive: interactive
    :type interactive: bool
    '''

    pass


def history_append(text: str = "",
                   current_character: int = 0,
                   remove_duplicates: bool = False):
    ''' Append history at cursor position

    :param text: Text, Text to insert at the cursor position
    :type text: str
    :param current_character: Cursor, The index of the cursor
    :type current_character: int
    :param remove_duplicates: Remove Duplicates, Remove duplicate items in the history
    :type remove_duplicates: bool
    '''

    pass


def history_cycle(reverse: bool = False):
    ''' Cycle through history

    :param reverse: Reverse, Reverse cycle history
    :type reverse: bool
    '''

    pass


def indent():
    ''' Add 4 spaces at line beginning

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


def language(language: str = ""):
    ''' Set the current language for this console

    :param language: Language
    :type language: str
    '''

    pass


def move(type: typing.Union[int, str] = 'LINE_BEGIN'):
    ''' Move cursor position

    :param type: Type, Where to move cursor to
    :type type: typing.Union[int, str]
    '''

    pass


def paste():
    ''' Paste text from clipboard

    '''

    pass


def scrollback_append(text: str = "", type: typing.Union[int, str] = 'OUTPUT'):
    ''' Append scrollback text by type

    :param text: Text, Text to insert at the cursor position
    :type text: str
    :param type: Type, Console output type
    :type type: typing.Union[int, str]
    '''

    pass


def select_set():
    ''' Set the console selection

    '''

    pass


def select_word():
    ''' Select word at cursor position

    '''

    pass


def unindent():
    ''' Delete 4 spaces from line beginning

    '''

    pass
