import sys
import typing


def bake_animation():
    ''' Update the audio animation cache

    '''

    pass


def mixdown(filepath: str = "",
            check_existing: bool = True,
            filter_blender: bool = False,
            filter_backup: bool = False,
            filter_image: bool = False,
            filter_movie: bool = False,
            filter_python: bool = False,
            filter_font: bool = False,
            filter_sound: bool = True,
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
            sort_method: typing.Union[int, str] = '',
            accuracy: int = 1024,
            container: typing.Union[int, str] = 'FLAC',
            codec: typing.Union[int, str] = 'FLAC',
            format: typing.Union[int, str] = 'S16',
            bitrate: int = 192,
            split_channels: bool = False):
    ''' Mix the scene's audio to a sound file

    :param filepath: File Path, Path to file
    :type filepath: str
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
    :param relative_path: Relative Path, Select the file relative to the blend file
    :type relative_path: bool
    :param display_type: Display Type * DEFAULT Default, Automatically determine display type for files. * LIST_VERTICAL Short List, Display files as short list. * LIST_HORIZONTAL Long List, Display files as a detailed list. * THUMBNAIL Thumbnails, Display files as thumbnails.
    :type display_type: typing.Union[int, str]
    :param sort_method: File sorting mode
    :type sort_method: typing.Union[int, str]
    :param accuracy: Accuracy, Sample accuracy, important for animation data (the lower the value, the more accurate)
    :type accuracy: int
    :param container: Container, File format * AC3 ac3, Dolby Digital ATRAC 3. * FLAC flac, Free Lossless Audio Codec. * MATROSKA mkv, Matroska. * MP2 mp2, MPEG-1 Audio Layer II. * MP3 mp3, MPEG-2 Audio Layer III. * OGG ogg, Xiph.Org Ogg Container. * WAV wav, Waveform Audio File Format.
    :type container: typing.Union[int, str]
    :param codec: Codec, Audio Codec * AAC AAC, Advanced Audio Coding. * AC3 AC3, Dolby Digital ATRAC 3. * FLAC FLAC, Free Lossless Audio Codec. * MP2 MP2, MPEG-1 Audio Layer II. * MP3 MP3, MPEG-2 Audio Layer III. * PCM PCM, Pulse Code Modulation (RAW). * VORBIS Vorbis, Xiph.Org Vorbis Codec.
    :type codec: typing.Union[int, str]
    :param format: Format, Sample format * U8 U8, 8-bit unsigned. * S16 S16, 16-bit signed. * S24 S24, 24-bit signed. * S32 S32, 32-bit signed. * F32 F32, 32-bit floating-point. * F64 F64, 64-bit floating-point.
    :type format: typing.Union[int, str]
    :param bitrate: Bitrate, Bitrate in kbit/s
    :type bitrate: int
    :param split_channels: Split channels, Each channel will be rendered into a mono file
    :type split_channels: bool
    '''

    pass


def open(filepath: str = "",
         hide_props_region: bool = True,
         filter_blender: bool = False,
         filter_backup: bool = False,
         filter_image: bool = False,
         filter_movie: bool = True,
         filter_python: bool = False,
         filter_font: bool = False,
         filter_sound: bool = True,
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
         show_multiview: bool = False,
         use_multiview: bool = False,
         display_type: typing.Union[int, str] = 'DEFAULT',
         sort_method: typing.Union[int, str] = '',
         cache: bool = False,
         mono: bool = False):
    ''' Load a sound file

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
    :param show_multiview: Enable Multi-View
    :type show_multiview: bool
    :param use_multiview: Use Multi-View
    :type use_multiview: bool
    :param display_type: Display Type * DEFAULT Default, Automatically determine display type for files. * LIST_VERTICAL Short List, Display files as short list. * LIST_HORIZONTAL Long List, Display files as a detailed list. * THUMBNAIL Thumbnails, Display files as thumbnails.
    :type display_type: typing.Union[int, str]
    :param sort_method: File sorting mode
    :type sort_method: typing.Union[int, str]
    :param cache: Cache, Cache the sound in memory
    :type cache: bool
    :param mono: Mono, Merge all the sound's channels into one
    :type mono: bool
    '''

    pass


def open_mono(filepath: str = "",
              hide_props_region: bool = True,
              filter_blender: bool = False,
              filter_backup: bool = False,
              filter_image: bool = False,
              filter_movie: bool = True,
              filter_python: bool = False,
              filter_font: bool = False,
              filter_sound: bool = True,
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
              show_multiview: bool = False,
              use_multiview: bool = False,
              display_type: typing.Union[int, str] = 'DEFAULT',
              sort_method: typing.Union[int, str] = '',
              cache: bool = False,
              mono: bool = True):
    ''' Load a sound file as mono

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
    :param show_multiview: Enable Multi-View
    :type show_multiview: bool
    :param use_multiview: Use Multi-View
    :type use_multiview: bool
    :param display_type: Display Type * DEFAULT Default, Automatically determine display type for files. * LIST_VERTICAL Short List, Display files as short list. * LIST_HORIZONTAL Long List, Display files as a detailed list. * THUMBNAIL Thumbnails, Display files as thumbnails.
    :type display_type: typing.Union[int, str]
    :param sort_method: File sorting mode
    :type sort_method: typing.Union[int, str]
    :param cache: Cache, Cache the sound in memory
    :type cache: bool
    :param mono: Mono, Mixdown the sound to mono
    :type mono: bool
    '''

    pass


def pack():
    ''' Pack the sound into the current blend file

    '''

    pass


def unpack(method: typing.Union[int, str] = 'USE_LOCAL', id: str = ""):
    ''' Unpack the sound to the samples filename

    :param method: Method, How to unpack
    :type method: typing.Union[int, str]
    :param id: Sound Name, Sound data-block name to unpack
    :type id: str
    '''

    pass


def update_animation_flags():
    ''' Update animation flags

    '''

    pass
