import os
import sys

# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'Dagor Engine Render'
copyright = '2023, Gaijin Entertainment'
author = 'Gaijin Entertainment'
release = '0.1.0'

dagor_prog_root = os.path.abspath('../../../..')
sys.path.append(os.path.join(dagor_prog_root, "1stPartyLibs/daScript/doc/source"))

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
   'sphinx.ext.duration',
   'sphinx.ext.autodoc',
   'sphinx.ext.autosummary',
   'sphinx.ext.viewcode',
   'breathe',
   'dascript',
]

# This is a 1337 hack to move all sources to a subdirectory.
def setup(app):
    app.srcdir = 'source'

rst_prolog = """

.. role:: cpp(code)
   :language: cpp

"""

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'

# -- Options for breathe -----------------------------------------------------

# You see, all of spinx config stuff is relative to conf.py, INCLUDING
# breathe's `breathe_projects` and `breathe_projects_source` options, right?
# BUT `breathe_build_directory` IS DIFFERENT! It's relative to the cwd
# of the script that runs sphinx-build.
# BEWARE OF THIS IF YOU EVER WANT TO CHANGE THIS!

breathe_build_directory = '_build'

breathe_projects = {
    'd3dAPI' : '_build/breathe/doxygen/d3dAPI/xml',
    'daBFG' : '_build/breathe/doxygen/daBFG/xml',
    'resourceSlot' : '_build/breathe/doxygen/resourceSlot/xml',
}

def get_headers(path):
    return path, [filename for filename in os.listdir(path) if filename.endswith(".h")]

breathe_projects_source = {
    'd3dAPI': (os.path.join(dagor_prog_root, 'dagorInclude/3d'), [
        'dag_drv3d_buffers.h',
        'dag_drv3d.h',
        'dag_drv3dConsts.h',
        'dag_tex3d.h',
    ]),
    "daBFG": get_headers(os.path.join(dagor_prog_root, 'gameLibs/publicInclude/render/daBfg')),
    "resourceSlot": get_headers(os.path.join(dagor_prog_root, 'gameLibs/publicInclude/render/resourceSlot')),
}

breathe_show_include = False

breathe_doxygen_aliases = {
    # used by the documentation for DeviceDriverCapabilities to add notes with platform specific constants
    'constcap{1}': r'\copybrief \1 \ref \1',
    # used by the documentation for DeviceDriverCapabilities to add references to the base type
    'basecap{1}': r'\details \copybrief \1 \see \1',
    # used by the documentation for DeviceDriverCapabilities to add notes for platform specific runtime values
    'runtimecap{2}': r'Runtime defined on \2. \ref \1',
    # here are some shortcuts for names of platforms, vendors and such, that are commonly used in the documentation
    'microsoft': 'Microsoft',
    'win32': 'Windows',
    'windows': r'\win32',
    'xbone': 'Xbox One',
    'xboxone': r'\xbone',
    'scarlett': 'Xbox Series X / S',
    'xbox': r'\xbone and \scarlett',
    'sony': 'Sony',
    'ps4': 'PlayStation 4',
    'ps5': 'PlayStation 5',
    'ps': r'\ps4 and \ps5',
    'nintendo': 'Nintendo',
    'nswitch': r'\nintendo Switch',
    'apple': 'Apple',
    'ios': 'iOS',
    'mac': 'macOS',
    'tvos': 'tvOS',
    'google': 'Google',
    'android': 'Android',
    'linux': 'Linux',
    'nvidia': 'Nvidia',
    'AMD': 'AMD',
    'amd': r'\AMD',
    'ATI': 'AMD / ATI',
    'ati': r'\ATI',
    'intel': 'Intel',
    'ARM': 'ARM',
    'arm': r'\ARM',
    'mali': r'\arm Mali',
    'imgtec': 'imgTec',
    'powervrr': r'\imgtec Power VR Rogue',
    'qcomm': 'Qualcomm',
    'adreno': r'\qcomm Adreno',
    'mesa': 'Mesa 3D',
    'dx11': 'DirectX 11',
    'dx12': 'DirectX 12',
    'khronos': 'Khronos',
    'vk': 'Vulkan',
    'metal': 'Metal',
    'llvmpipe': r'\mesa LLVM pipe',
    # pretty much all briefs of the device driver feature caps start with the same phrase, so we have a alias for it, to cut down on typing
    'capbrief': r'\brief Indicates that the device driver',
    'briefconstcap{2}': r'\brief Constant \1\basecap{\2}',
    'NYI': r'\remarks This feature can be supported, but is not yet implemented.',
    'someNYI': r'\remarks Some drivers without support could support this feature, but do not implement it yet.',
    'constissue{1}': r'\copybrief \1 \ref \1',
    'baseissue{1}': r'\details \copybrief \1 \see \1',
    'briefconstissue{3}': r'\brief Is constant \1 on \2 \baseissue{\3}',
    'runtimeissue{2}': r'Runtime defined on \2. \ref \1',
    'caprefc{3}': r'\ref \3::\2 \"\1\"',
    'caprefa{3}': r'\ref \3::\2 \"\1\"',
    'caprefr{3}': r'\ref \3 \"\1\"',
    'caprefc{4}': r'\ref \3::\2 \"\1\"',
    'caprefa{4}': r'\ref \4::\2 \"\1\"',
    'caprefr{4}': r'\ref \3 \"\1\"',
    'capvaluec{2}' : r'\copybrief \2::\1',
    'capvaluea{2}' : r'\copybrief \2::\1',
    'capvaluer{2}' : r'Runtime determined.',
    'capvaluec{3}' : r'\copybrief \2::\1',
    'capvaluea{3}' : r'\copybrief \3::\1',
    'capvaluer{3}' : r'Runtime determined.',
    'platformtable{12}': r'''\note <table>^^\
    <tr><th>Platform<th>Value^^\
    <tr><td>\capref\2{\xbone,\1,DeviceDriverCapabilitiesXboxOne} <td>\capvalue\2{\1,DeviceDriverCapabilitiesXboxOne} ^^\
    <tr><td>\capref\3{\scarlett,\1,DeviceDriverCapabilitiesScarlett,DeviceDriverCapabilitiesXboxOne} <td>\capvalue\3{\1,DeviceDriverCapabilitiesScarlett,DeviceDriverCapabilitiesXboxOne} ^^\
    <tr><td>\capref\4{\ps4,\1,DeviceDriverCapabilitiesPS4} <td>\capvalue\4{\1,DeviceDriverCapabilitiesPS4} ^^\
    <tr><td>\capref\5{\ps5,\1,DeviceDriverCapabilitiesPS5,DeviceDriverCapabilitiesPS4} <td>\capvalue\5{\1,DeviceDriverCapabilitiesPS5,DeviceDriverCapabilitiesPS4} ^^\
    <tr><td>\capref\6{\ios,\1,DeviceDriverCapabilitiesIOS} <td>\capvalue\6{\1,DeviceDriverCapabilitiesIOS} ^^\
    <tr><td>\capref\7{\tvos,\1,DeviceDriverCapabilitiesTVOS} <td>\capvalue\7{\1,DeviceDriverCapabilitiesTVOS} ^^\
    <tr><td>\capref\8{\nswitch,\1,DeviceDriverCapabilitiesNintendoSwitch} <td>\capvalue\8{\1,DeviceDriverCapabilitiesNintendoSwitch} ^^\
    <tr><td>\capref\9{\android,\1,DeviceDriverCapabilitiesAndroid} <td>\capvalue\9{\1,DeviceDriverCapabilitiesAndroid} ^^\
    <tr><td>\capref\10{\mac,\1,DeviceDriverCapabilitiesMacOSX} <td>\capvalue\10{\1,DeviceDriverCapabilitiesMacOSX} ^^\
    <tr><td>\capref\11{\linux,\1,DeviceDriverCapabilitiesLinux} <td>\capvalue\11{\1,DeviceDriverCapabilitiesLinux} ^^\
    <tr><td>\capref\12{\win32,\1,DeviceDriverCapabilitiesWindows} <td>\capvalue\12{\1,DeviceDriverCapabilitiesWindows} ^^\
    </table>''',
}

breathe_doxygen_config_options = {
    'ENABLE_PREPROCESSING': 'YES',
    # this might be a bad idea
    'MACRO_EXPANSION': 'YES',
    # Omitting the __restrict specifier from documentation is BAD,
    # but we have to do it because breathe doesn't seem to support it.
    'PREDEFINED': 'DOXYGEN declare_new(x)= __restrict=',
}
