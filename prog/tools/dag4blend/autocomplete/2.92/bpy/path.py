import sys
import typing
import bpy.types


def abspath(path,
            start: typing.Union[bytes, str] = None,
            library: 'bpy.types.Library' = None):
    ''' Returns the absolute path relative to the current blend file using the "//" prefix.

    :param start: Relative to this path, when not set the current filename is used.
    :type start: typing.Union[bytes, str]
    :param library: The library this path is from. This is only included for convenience, when the library is not None its path replaces *start*.
    :type library: 'bpy.types.Library'
    '''

    pass


def basename(path):
    ''' Equivalent to os.path.basename, but skips a "//" prefix. Use for Windows compatibility.

    '''

    pass


def clean_name(name, replace='_'):
    ''' Returns a name with characters replaced that may cause problems under various circumstances, such as writing to a file. All characters besides A-Z/a-z, 0-9 are replaced with "_" or the *replace* argument if defined.

    '''

    pass


def display_name(name, *, has_ext=True):
    ''' Creates a display string from name to be used menus and the user interface. Capitalize the first letter in all lowercase names, mixed case names are kept as is. Intended for use with filenames and module names.

    '''

    pass


def display_name_from_filepath(name):
    ''' Returns the path stripped of directory and extension, ensured to be utf8 compatible.

    '''

    pass


def display_name_to_filepath(name):
    ''' Performs the reverse of display_name using literal versions of characters which aren't supported in a filepath.

    '''

    pass


def ensure_ext(filepath, ext: str, case_sensitive: bool = False):
    ''' Return the path with the extension added if it is not already set.

    :param ext: The extension to check for, can be a compound extension. Should start with a dot, such as '.blend' or '.tar.gz'.
    :type ext: str
    :param case_sensitive: Check for matching case when comparing extensions.
    :type case_sensitive: bool
    '''

    pass


def is_subdir(path: typing.Union[bytes, str], directory):
    ''' Returns true if *path* in a subdirectory of *directory*. Both paths must be absolute.

    :param path: An absolute path.
    :type path: typing.Union[bytes, str]
    '''

    pass


def module_names(path: str, recursive: bool = False) -> list:
    ''' Return a list of modules which can be imported from *path*.

    :param path: a directory to scan.
    :type path: str
    :param recursive: Also return submodule names for packages.
    :type recursive: bool
    :return: a list of string pairs (module_name, module_file).
    '''

    pass


def native_pathsep(path):
    ''' Replace the path separator with the systems native os.sep .

    '''

    pass


def reduce_dirs(dirs: list) -> list:
    ''' Given a sequence of directories, remove duplicates and any directories nested in one of the other paths. (Useful for recursive path searching).

    :param dirs: Sequence of directory paths.
    :type dirs: list
    :return: A unique list of paths.
    '''

    pass


def relpath(path: typing.Union[bytes, str],
            start: typing.Union[bytes, str] = None):
    ''' Returns the path relative to the current blend file using the "//" prefix.

    :param path: An absolute path.
    :type path: typing.Union[bytes, str]
    :param start: Relative to this path, when not set the current filename is used.
    :type start: typing.Union[bytes, str]
    '''

    pass


def resolve_ncase(path):
    ''' Resolve a case insensitive path on a case sensitive system, returning a string with the path if found else return the original path.

    '''

    pass
