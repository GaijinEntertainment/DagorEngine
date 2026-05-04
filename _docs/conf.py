# Configuration file for Dagor Documentation build with Sphinx

import os
import sys
import time
import subprocess

# ==============================================================================
# Path setup
# ==============================================================================

# Add project-specific directories to sys.path
sys.path.insert(0, os.path.abspath('source/'))
sys.path.insert(0, os.path.abspath('.'))
sys.path.insert(0, os.path.abspath('..'))
dagor_prog_root = os.path.abspath('../prog')
sys.path.append(os.path.join(dagor_prog_root, "1stPartyLibs/daScript/doc/source"))

# ==============================================================================
# General configuration
# ==============================================================================

# List of enabled Sphinx extensions
extensions = [
  'quirrel',
  'myst_parser',
  'quirrel_pygment_lexer',
  'sphinx-design',
  'breathe',
  'daslang',
  'sphinx.ext.duration',
  'sphinx.ext.autodoc',
  'sphinx.ext.autosummary',
  'sphinx.ext.viewcode',
  'sphinx_markdown_tables',
  'sphinx_tags',
  'hoverxref.extension',
  'versionwarning.extension',
  'sphinx_copybutton',
  'sphinxcontrib.video',
]

# ==============================================================================
# MyST parser configuration
# ==============================================================================

# List of enabled MyST extensions
myst_enable_extensions = [
    "amsmath",
    "attrs_inline",
    "colon_fence",
    "deflist",
    "dollarmath",
    "fieldlist",
    "html_admonition",
    "html_image",
    "linkify",
    "replacements",
    "smartquotes",
    "strikethrough",
    "substitution",
    "tasklist",
]

# Maximum heading level for generating anchor links (H1 to H7)
myst_heading_anchors = 7

# ==============================================================================
# sphinx-tags configuration | currently disabled
# ==============================================================================

# Enable tag processing. Default: False
tags_create_tags = False

# File extensions to scan for tags
tags_extension = ["md", "rst"]

# Title for the tag index page. Default: My tags
tags_page_title = 'Tags'

# Enable displaying tags as badges. Default: False
tags_create_badges = False

# Badge colors by tag name. Default: {}
tags_badge_colors = {}

# Example of tag badge color configuration:
# tags_badge_colors = {
#    "tag_type_1_*": "primary",
#    "tag_type_2_*": "warning",
#    "*": "muted",  # Used as a default value
# }

# ==============================================================================
# Warning and error handling
# ==============================================================================

# Suppress duplicate label warnings
suppress_warnings = ['autosectionlabel.*', 'docutils']

# ==============================================================================
# Templates and sources
# ==============================================================================

# Paths to custom templates
templates_path = ['_templates']

# Supported source file extensions
source_suffix = ['.rst', '.md']

# Root document name
master_doc = 'index'

# Patterns to exclude from source scanning
exclude_patterns = []

# ==============================================================================
# Project information
# ==============================================================================

project = u'Dagor Documentation'
copyright = 'Gaijin Entertainment %s' % time.strftime('%Y')
author = u'Gaijin Entertainment'
version = u'0.1.0'

# ==============================================================================
# Language and localization
# ==============================================================================

language = 'en'

# ==============================================================================
# Output and syntax highlighting configuration
# ==============================================================================

# Pygments style for code blocks
pygments_style = 'manni'

# Add custom lexers directory to the Python path
sys.path.insert(0, os.path.abspath("_lexers"))

# Import custom lexers
import blk_lexer
import dascript_lexer
import quirrel_pygment_lexer

# Include TODO and todoList entries in output
todo_include_todos = True

# ==============================================================================
# Build timer
# ==============================================================================

# Adds timing info: prints build duration in seconds on finish
def setup(app):
    start_time = time.time()

    def report_build_time(app, exception):
        duration = time.time() - start_time
        print(f"\nBuild finished in {duration:.2f} seconds.")

    app.connect('build-finished', report_build_time)

# ==============================================================================
# HTML output configuration
# ==============================================================================

# Theme to use for HTML and HTML Help pages
html_theme = 'sphinx_rtd_theme'

# Theme-specific customization options
html_theme_options = {
    'navigation_depth': -1,
}

# HTML title (defaults to "<project> v<release> documentation")
html_title = ''

# Logo image shown at the top of the sidebar
html_logo = '_static/_images/logo.png'

# Favicon image for the docs (should be .ico, 16x16 or 32x32 px)
html_favicon = '_static/_images/favicon.ico'

# Paths with custom static files (CSS, images, etc.)
html_static_path = ['_static']

# Additional CSS files to include
html_css_files = [
    'custom.css',
]

# Format for 'Last updated on:' timestamp on each page
html_last_updated_fmt = '%b %d, %Y'

# Show "Created using Sphinx" footer. Default: True
html_show_sphinx = False

# Base name for HTML help builder output files
htmlhelp_basename = 'dagor_doc'

# A shorter title for the navigation bar.  Default is the same as html_title.
#html_short_title = None

# Extra files copied to root (e.g. robots.txt)
#html_extra_path = []

# Enable SmartyPants for typographic quotes and dashes
# html_use_smartypants = True

# Custom sidebar templates
# html_sidebars = {}

# Additional pages rendered from templates
# html_additional_pages = {}

# Generate module index. Default: True
# html_domain_indices = True

# Generate general index. Default: True
# html_use_index = True

# Split index into pages per letter. Default: False
# html_split_index = False

# Show links to source files on pages. Default: True
# html_show_sourcelink = True

# Show copyright info in footer. Default: True
# html_show_copyright = True

# Enable OpenSearch description file (provide base URL)
# html_use_opensearch = ''

# File suffix for HTML files. Default: None
# html_file_suffix = None

# Language for HTML search index. Default: 'en'
# Sphinx supports the following languages:
#   'da', 'de', 'en', 'es', 'fi', 'fr', 'hu', 'it', 'ja'
#   'nl', 'no', 'pt', 'ro', 'ru', 'sv', 'tr'
# html_search_language = 'en'

# ==============================================================================
# LaTeX output configuration
# ==============================================================================

# Use XeLaTeX engine for better Unicode and font support
latex_engine = 'xelatex'

# Define the document type: the full project or single chapter
pdf_subdoc = os.getenv('PDF_SUBDOC')
if pdf_subdoc == 'maxscript_toolbox':
    latex_title = 'Dagor MaxScript Toolbox' # Document title
    latex_documents = [(
        'index',                            # Chapter source file
        'dagor_maxscript_toolbox.tex',      # Output .tex filename
        latex_title,
        'Gaijin Entertainment',  # Author
        'manual',                # Document type: 'manual' = book-style with chapters
    )]
else:
    latex_title = 'Dagor Documentation'     # Document title
    latex_documents = [(
        'index',                 # Root source file
        'dagor.tex',             # Output .tex filename
        latex_title,
        'Gaijin Entertainment',  # Author
        'manual',                # Document type: 'manual' = book-style with chapters
    )]

# General parameters
latex_logo = '_static/_images/logo.png' # Logo to display on the title page of the PDF

latex_elements = {
    'pointsize': '10pt',         # Default font size
    'papersize': 'a4paper',      # Default paper size
    'maxlistdepth': '20',        # Number of nested levels
    'figure_align': 'H',         # Default figure alignment
    'tocdepth': '2',             # TOC depth

    'preamble': r'''
    \setcounter{secnumdepth}{2}
    \setcounter{tocdepth}{2}
    \usepackage{graphicx}
    \usepackage[svgnames]{xcolor}
    \definecolor{darkbgr}{rgb}{0.102, 0.110, 0.118}
    \definecolor{mdarkbgr}{rgb}{0.204, 0.192, 0.192}
    \usepackage{hyperref}
    \hypersetup{
      colorlinks=true,
      linkcolor=DarkBlue,
      filecolor=DarkBlue,
      urlcolor=DarkBlue,
    }
    \usepackage{tikz}
    \newcommand{\cnum}[1]{%
      \tikz[baseline=-0.6ex]{
        \node[shape=circle, draw, line width=0.8pt, inner sep=1pt, minimum size=1.3em, text centered] (char) {\scalebox{0.7}{\textbf{#1}}};
      }%
    }
    ''',

    'fontpkg': r'''
    \setmainfont{FreeSans}
    \setsansfont{FreeSans}
    \setmonofont{FreeMono}
    ''',

    # Title page with dynamic title substitution
    'maketitle': r'''
    \begin{titlepage}
    \begin{tikzpicture}[remember picture, overlay, shift={(current page.south west)}]
    \fill[mdarkbgr] (0,0) rectangle (5cm,\paperheight);
    \end{tikzpicture}
    \begin{tikzpicture}[remember picture, overlay, shift={(current page.south west)}]
    \fill[darkbgr] (0,\paperheight - 10cm) rectangle (\paperwidth, \paperheight - 5cm);
    \node[anchor=south east, font=\bfseries\Huge, text=white] at (\paperwidth - 1cm, \paperheight - 8cm) {%s};
    \node[anchor=south east, font=\bfseries\large, text=darkbgr] at (\paperwidth - 1cm, \paperheight - 11.5cm) {Gaijin Entertainment};
    \node[anchor=north west] at (0.4cm, \paperheight - 5.4cm) {\includegraphics[width=4cm]{logo}};
    \draw[anchor=north west, draw=darkbgr, line width=2pt] (2.55cm, \paperheight - 7.55cm) circle(2cm);
    \end{tikzpicture}
    \end{titlepage}
    ''' % latex_title,  # Title is substituted here
}

# ==============================================================================
# Manual page output configuration
# ==============================================================================

# One entry per manual page.
# Format: (source start file, name, description, authors, manual section).
# Used when building Unix man pages via `sphinx-build -b man`.
man_pages = [
    (master_doc, 'Dagor Docs', u'Dagor Documentation', [author], 1)
]

# If true, show raw URL addresses after external links (not usually needed).
# man_show_urls = False

# ==============================================================================
# Texinfo output configuration
# ==============================================================================

# Configuration for Texinfo output (used for GNU info manuals).
# Format: (start file, target name, title, author, menu entry, description, category).
texinfo_documents = [
    (master_doc, 'Dagor Docs', u'Dagor Docs',
     author, 'Dagor', 'Modules and Libraries.',
     'Miscellaneous'),
]

# Append additional documents as appendices to all Texinfo manuals.
# texinfo_appendices = []

# If false, no module index is generated in the output.
# texinfo_domain_indices = True

# How to display URLs: 'footnote', 'no', or 'inline'.
# texinfo_show_urls = 'footnote'

# If true, suppress the creation of the @detailmenu in the top node.
# texinfo_no_detailmenu = False

# ==============================================================================
# Reserved setup hook (currently unused)
# ==============================================================================

# This setup function can be used to customize the Sphinx app initialization,
# e.g., to set the source directory or add custom CSS files directly.
# At the moment, it is commented out because these settings are handled elsewhere.
# Uncomment and adjust if needed for advanced customization.

# def setup(app):
#    # Add custom CSS file to all HTML pages
#    app.add_css_file('custom.css')

# ==============================================================================
# Inline syntax highlighting roles (rst_prolog)
# ==============================================================================

# Defines global inline roles for syntax highlighting in reStructuredText.
# These can be used as :cpp:`std::vector` or :sq:`foo()` inside the text.
rst_prolog = """
.. role:: sq(code)
   :language: sq

.. role:: cpp(code)
   :language: cpp
"""

# ==============================================================================
# Breathe extension configuration
# ==============================================================================

# Directory where Doxygen XML output is expected for Breathe to consume
breathe_build_directory = '_build/breathe/doxygen'

# Mapping of project names to their respective Doxygen XML output directories
breathe_projects = {
    'd3dAPI' :             '_build/breathe/doxygen/d3dAPI/xml',
    'd3dHelpers' :         '_build/breathe/doxygen/d3dHelpers/xml',
    'daFrameGraph' :       '_build/breathe/doxygen/daFrameGraph/xml',
    'resourceSlot' :       '_build/breathe/doxygen/resourceSlot/xml',
    'shaders' :            '_build/breathe/doxygen/shaders/xml',
    'EditorCore':          '_build/breathe/doxygen/EditorCore/xml',
    'maxplug':             '_build/breathe/doxygen/maxplug/xml',
    'ShaderCompiler2':     '_build/breathe/doxygen/ShaderCompiler2/xml',
    'libTools-renderUtil': '_build/breathe/doxygen/libTools-renderUtil/xml',
    'libTools-util':       '_build/breathe/doxygen/libTools-util/xml',
}

# Mapping of project names to the corresponding source code directories
# (used for cross-referencing source locations in the documentation)
breathe_project_sources = {
    'd3dAPI':              'dagorInclude/drv/3d',
    'd3dHelpers':          'dagorInclude/3d',
    'daFrameGraph':        'gameLibs/publicInclude/render/daFrameGraph',
    'resourceSlot':        'gameLibs/publicInclude/render/resourceSlot',
    'shaders':             'dagorInclude/shaders',
    'EditorCore':          'tools/sharedInclude/EditorCore',
    'maxplug':             'tools/maxplug',
    'ShaderCompiler2':     'tools/ShaderCompiler2',
    'libTools-renderUtil': 'tools/sharedInclude/libTools/renderUtil',
    'libTools-util':       'tools/sharedInclude/libTools/util',
}

# Aliases for Doxygen commands or macros, used to simplify repetitive
# documentation patterns
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

# ==============================================================================
# Doxygen build trigger (conditional)
# ==============================================================================

# If the environment variable SKIPDOXYGEN is not set, run Doxygen to generate
# XML documentation for each project listed in breathe_project_sources.
if not 'SKIPDOXYGEN' in os.environ:
    # Ensure the Breathe build directory exists
    os.system('bash -c "mkdir -p ' + breathe_build_directory + '"')

    # Iterate over all projects and invoke doxygen in each source directory
    for name, path in breathe_project_sources.items():
        spath = "../prog/" + path
        print(f"Processing project '{name}' in directory: {spath}")
        # Run Doxygen via bash in the project source directory
        os.system('bash -c  "cd ' + spath + '; doxygen; cd -"')

# ==============================================================================
# Custom inline roles
# ==============================================================================

from docutils import nodes
from docutils.parsers.rst import roles

# ------------------------------------------------------------------------------
# Role `:cnum:` for formatting numbers differently depending on output builder
#
# - In LaTeX output (e.g., PDF), wraps the number in a custom LaTeX command \cnum{...}
# - In HTML output, wraps the number in a <span> with class "cnum"
#
# Usage examples:
#   :cnum:`42` in reStructuredText
#   {cnum}`42` in MyST Markdown
#
# This allows consistent styling or formatting of numeric values across formats.
# ------------------------------------------------------------------------------
def cnum_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
    builder = inliner.document.settings.env.app.builder.name

    if builder == 'latex':
        # LaTeX: use \cnum{...}
        latex = rf'\cnum{{{text}}}'
        node = nodes.raw('', latex, format='latex')
    else:
        # HTML: use <span class="cnum">...</span>
        node = nodes.inline(text, text, classes=['cnum'])

    return [node], []

roles.register_local_role('cnum', cnum_role)

# ------------------------------------------------------------------------------
# Factory function to create badge roles
#
# These roles render colored inline badges in HTML output.
# Usage example in Markdown/MyST:
#   {bdg-red}`ERROR`
#   {bdg-green}`SUCCESS`
#
# Each badge wraps text in a <span> with CSS classes for color styling.
# ------------------------------------------------------------------------------
def make_bdg_role(color_class):
    def bdg_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
        builder = inliner.document.settings.env.app.builder.name

        node = nodes.inline(text, text, classes=['bdg', color_class])

        return [node], []
    return bdg_role

bdg_colors = {
    'bdg-blue': 'bdg-blue',
    'bdg-green': 'bdg-green',
    'bdg-grey': 'bdg-grey',
    'bdg-orange': 'bdg-orange',
    'bdg-purple': 'bdg-purple',
    'bdg-red': 'bdg-red',
    'bdg-yellow': 'bdg-yellow',
}

for role_name, css_class in bdg_colors.items():
    roles.register_local_role(role_name, make_bdg_role(css_class))


