import sys
import typing
from . import translations
from . import timers
from . import handlers
from . import icons

alembic = None
''' constant value bpy.app.alembic(supported=True, version=(1, 7, 12), version_string=' 1, 7, 12')
'''

autoexec_fail = None
''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.
'''

autoexec_fail_message = None
''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.
'''

autoexec_fail_quiet = None
''' Undocumented, consider contributing <https://developer.blender.org/T51061> __.
'''

background = None
''' Boolean, True when blender is running without a user interface (started with -b)
'''

binary_path = None
''' The location of Blender's executable, useful for utilities that open new instances
'''

binary_path_python = None
''' String, the path to the python executable (read-only). Deprecated! Use sys.executable instead.
'''

build_branch = None
''' The branch this blender instance was built from
'''

build_cflags = None
''' C compiler flags
'''

build_commit_date = None
''' The date of commit this blender instance was built
'''

build_commit_time = None
''' The time of commit this blender instance was built
'''

build_commit_timestamp = None
''' The unix timestamp of commit this blender instance was built
'''

build_cxxflags = None
''' C++ compiler flags
'''

build_date = None
''' The date this blender instance was built
'''

build_hash = None
''' The commit hash this blender instance was built with
'''

build_linkflags = None
''' Binary linking flags
'''

build_options = None
''' constant value bpy.app.build_options(bullet=True, codec_avi=True, codec_ffmpeg=True, codec_sndfile=True, compositor=True, cycles=True, cycles_osl=True, freestyle=True, image_cineon=True, image_dds=True, image_hdr=True, image_openexr=True, image_openjpeg=True, image_tiff=True, input_ndof=True, audaspace=True, international=True, openal=True, opensubdiv=True, sdl=True, sdl_dynload=True, jack=True, libmv=True, mod_oceansim=True, mod_remesh=True, collada=True, opencolorio=True, openmp=False, openvdb=True, alembic=True, ...)
'''

build_platform = None
''' The platform this blender instance was built for
'''

build_system = None
''' Build system used
'''

build_time = None
''' The time this blender instance was built
'''

build_type = None
''' The type of build (Release, Debug)
'''

debug = None
''' Boolean, for debug info (started with --debug / --debug_* matching this attribute name)
'''

debug_depsgraph = None
''' Boolean, for debug info (started with --debug / --debug_* matching this attribute name)
'''

debug_depsgraph_build = None
''' Boolean, for debug info (started with --debug / --debug_* matching this attribute name)
'''

debug_depsgraph_eval = None
''' Boolean, for debug info (started with --debug / --debug_* matching this attribute name)
'''

debug_depsgraph_pretty = None
''' Boolean, for debug info (started with --debug / --debug_* matching this attribute name)
'''

debug_depsgraph_tag = None
''' Boolean, for debug info (started with --debug / --debug_* matching this attribute name)
'''

debug_depsgraph_time = None
''' Boolean, for debug info (started with --debug / --debug_* matching this attribute name)
'''

debug_events = None
''' Boolean, for debug info (started with --debug / --debug_* matching this attribute name)
'''

debug_ffmpeg = None
''' Boolean, for debug info (started with --debug / --debug_* matching this attribute name)
'''

debug_freestyle = None
''' Boolean, for debug info (started with --debug / --debug_* matching this attribute name)
'''

debug_gpumem = None
''' Boolean, for debug info (started with --debug / --debug_* matching this attribute name)
'''

debug_handlers = None
''' Boolean, for debug info (started with --debug / --debug_* matching this attribute name)
'''

debug_io = None
''' Boolean, for debug info (started with --debug / --debug_* matching this attribute name)
'''

debug_python = None
''' Boolean, for debug info (started with --debug / --debug_* matching this attribute name)
'''

debug_simdata = None
''' Boolean, for debug info (started with --debug / --debug_* matching this attribute name)
'''

debug_value = None
''' Short, number which can be set to non-zero values for testing purposes
'''

debug_wm = None
''' Boolean, for debug info (started with --debug / --debug_* matching this attribute name)
'''

driver_namespace = None
''' Dictionary for drivers namespace, editable in-place, reset on file load (read-only)
'''

factory_startup = None
''' Boolean, True when blender is running with --factory-startup)
'''

ffmpeg = None
''' constant value bpy.app.ffmpeg(supported=True, avcodec_version=(58, 54, 100), avcodec_version_string='58, 54, 100', avdevice_version=(58, 8, 100), avdevice_version_string='58, 8, 100', avformat_version=(58, 29, 100), avformat_version_string='58, 29, 100', avutil_version=(56, 31, 100), avutil_version_string='56, 31, 100', swscale_version=(5, 5, 100), swscale_version_string=' 5, 5, 100')
'''

ocio = None
''' constant value bpy.app.ocio(supported=True, version=(1, 1, 1), version_string=' 1, 1, 1')
'''

oiio = None
''' constant value bpy.app.oiio(supported=True, version=(2, 1, 15), version_string=' 2, 1, 15')
'''

opensubdiv = None
''' constant value bpy.app.opensubdiv(supported=True, version=(0, 0, 0), version_string=' 0, 0, 0')
'''

openvdb = None
''' constant value bpy.app.openvdb(supported=True, version=(7, 0, 0), version_string=' 7, 0, 0')
'''

render_icon_size = None
''' Reference size for icon/preview renders (read-only)
'''

render_preview_size = None
''' Reference size for icon/preview renders (read-only)
'''

sdl = None
''' constant value bpy.app.sdl(supported=True, version=(0, 0, 0), version_string='Unknown', available=False)
'''

tempdir = None
''' String, the temp directory used by blender (read-only)
'''

usd = None
''' constant value bpy.app.usd(supported=True, version=(0, 20, 5), version_string=' 0, 20, 5')
'''

use_event_simulate = None
''' Boolean, for application behavior (started with --enable-* matching this attribute name)
'''

use_userpref_skip_save_on_exit = None
''' Boolean, for application behavior (started with --enable-* matching this attribute name)
'''

version = None
''' The Blender version as a tuple of 3 numbers. eg. (2, 83, 1)
'''

version_char = None
''' Deprecated, always an empty string
'''

version_cycle = None
''' The release status of this build alpha/beta/rc/release
'''

version_file = None
''' The blend file version, compatible with bpy.data.version
'''

version_string = None
''' The Blender version formatted as a string
'''
