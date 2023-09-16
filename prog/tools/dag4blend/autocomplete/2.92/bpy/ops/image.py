import sys
import typing
import bpy.types


def add_render_slot():
    ''' Add a new render slot

    '''

    pass


def change_frame(frame: int = 0):
    ''' Interactively change the current frame number

    :param frame: Frame
    :type frame: int
    '''

    pass


def clear_render_border():
    ''' Clear the boundaries of the render region and disable render region

    '''

    pass


def clear_render_slot():
    ''' Clear the currently selected render slot

    '''

    pass


def curves_point_set(point: typing.Union[int, str] = 'BLACK_POINT',
                     size: int = 1):
    ''' Set black point or white point for curves

    :param point: Point, Set black point or white point for curves
    :type point: typing.Union[int, str]
    :param size: Sample Size
    :type size: int
    '''

    pass


def cycle_render_slot(reverse: bool = False):
    ''' Cycle through all non-void render slots

    :param reverse: Cycle in Reverse
    :type reverse: bool
    '''

    pass


def external_edit(filepath: str = ""):
    ''' Edit image in an external application

    :param filepath: filepath
    :type filepath: str
    '''

    pass


def invert(invert_r: bool = False,
           invert_g: bool = False,
           invert_b: bool = False,
           invert_a: bool = False):
    ''' Invert image's channels

    :param invert_r: Red, Invert red channel
    :type invert_r: bool
    :param invert_g: Green, Invert green channel
    :type invert_g: bool
    :param invert_b: Blue, Invert blue channel
    :type invert_b: bool
    :param invert_a: Alpha, Invert alpha channel
    :type invert_a: bool
    '''

    pass


def match_movie_length():
    ''' Set image's user's length to the one of this video

    '''

    pass


def new(name: str = "Untitled",
        width: int = 1024,
        height: int = 1024,
        color: typing.List[float] = (0.0, 0.0, 0.0, 1.0),
        alpha: bool = True,
        generated_type: typing.Union[int, str] = 'BLANK',
        float: bool = False,
        use_stereo_3d: bool = False,
        tiled: bool = False):
    ''' Create a new image

    :param name: Name, Image data-block name
    :type name: str
    :param width: Width, Image width
    :type width: int
    :param height: Height, Image height
    :type height: int
    :param color: Color, Default fill color
    :type color: typing.List[float]
    :param alpha: Alpha, Create an image with an alpha channel
    :type alpha: bool
    :param generated_type: Generated Type, Fill the image with a grid for UV map testing * BLANK Blank, Generate a blank image. * UV_GRID UV Grid, Generated grid to test UV mappings. * COLOR_GRID Color Grid, Generated improved UV grid to test UV mappings.
    :type generated_type: typing.Union[int, str]
    :param float: 32-bit Float, Create image with 32-bit floating-point bit depth
    :type float: bool
    :param use_stereo_3d: Stereo 3D, Create an image with left and right views
    :type use_stereo_3d: bool
    :param tiled: Tiled, Create a tiled image
    :type tiled: bool
    '''

    pass


def open(filepath: str = "",
         directory: str = "",
         files: typing.
         Union[typing.Dict[str, 'bpy.types.OperatorFileListElement'], typing.
               List['bpy.types.OperatorFileListElement'],
               'bpy_prop_collection'] = None,
         hide_props_region: bool = True,
         filter_blender: bool = False,
         filter_backup: bool = False,
         filter_image: bool = True,
         filter_movie: bool = True,
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
         relative_path: bool = True,
         show_multiview: bool = False,
         use_multiview: bool = False,
         display_type: typing.Union[int, str] = 'DEFAULT',
         sort_method: typing.Union[int, str] = '',
         use_sequence_detection: bool = True,
         use_udim_detecting: bool = True):
    ''' Open image

    :param filepath: File Path, Path to file
    :type filepath: str
    :param directory: Directory, Directory of the file
    :type directory: str
    :param files: Files
    :type files: typing.Union[typing.Dict[str, 'bpy.types.OperatorFileListElement'], typing.List['bpy.types.OperatorFileListElement'], 'bpy_prop_collection']
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
    :param use_sequence_detection: Detect Sequences, Automatically detect animated sequences in selected images (based on file names)
    :type use_sequence_detection: bool
    :param use_udim_detecting: Detect UDIMs, Detect selected UDIM files and load all matching tiles
    :type use_udim_detecting: bool
    '''

    pass


def pack():
    ''' Pack an image as embedded data into the .blend file

    '''

    pass


def project_apply():
    ''' Project edited image back onto the object :file: startup/bl_operators/image.py\:196 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/image.py$196> _

    '''

    pass


def project_edit():
    ''' Edit a snapshot of the 3D Viewport in an external image editor :file: startup/bl_operators/image.py\:126 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/image.py$126> _

    '''

    pass


def read_viewlayers():
    ''' Read all the current scene's view layers from cache, as needed

    '''

    pass


def reload():
    ''' Reload current image from disk

    '''

    pass


def remove_render_slot():
    ''' Remove the current render slot

    '''

    pass


def render_border(xmin: int = 0,
                  xmax: int = 0,
                  ymin: int = 0,
                  ymax: int = 0,
                  wait_for_input: bool = True):
    ''' Set the boundaries of the render region and enable render region

    :param xmin: X Min
    :type xmin: int
    :param xmax: X Max
    :type xmax: int
    :param ymin: Y Min
    :type ymin: int
    :param ymax: Y Max
    :type ymax: int
    :param wait_for_input: Wait for Input
    :type wait_for_input: bool
    '''

    pass


def replace(filepath: str = "",
            hide_props_region: bool = True,
            filter_blender: bool = False,
            filter_backup: bool = False,
            filter_image: bool = True,
            filter_movie: bool = True,
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
            relative_path: bool = True,
            show_multiview: bool = False,
            use_multiview: bool = False,
            display_type: typing.Union[int, str] = 'DEFAULT',
            sort_method: typing.Union[int, str] = ''):
    ''' Replace current image by another one from disk

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
    '''

    pass


def resize(size: typing.List[int] = (0, 0)):
    ''' Resize the image

    :param size: Size
    :type size: typing.List[int]
    '''

    pass


def sample(size: int = 1):
    ''' Use mouse to sample a color in current image

    :param size: Sample Size
    :type size: int
    '''

    pass


def sample_line(xstart: int = 0,
                xend: int = 0,
                ystart: int = 0,
                yend: int = 0,
                flip: bool = False,
                cursor: int = 5):
    ''' Sample a line and show it in Scope panels

    :param xstart: X Start
    :type xstart: int
    :param xend: X End
    :type xend: int
    :param ystart: Y Start
    :type ystart: int
    :param yend: Y End
    :type yend: int
    :param flip: Flip
    :type flip: bool
    :param cursor: Cursor, Mouse cursor style to use during the modal operator
    :type cursor: int
    '''

    pass


def save():
    ''' Save the image with current name and settings

    '''

    pass


def save_all_modified():
    ''' Save all modified images

    '''

    pass


def save_as(save_as_render: bool = False,
            copy: bool = False,
            filepath: str = "",
            check_existing: bool = True,
            filter_blender: bool = False,
            filter_backup: bool = False,
            filter_image: bool = True,
            filter_movie: bool = True,
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
            relative_path: bool = True,
            show_multiview: bool = False,
            use_multiview: bool = False,
            display_type: typing.Union[int, str] = 'DEFAULT',
            sort_method: typing.Union[int, str] = ''):
    ''' Save the image with another name and/or settings

    :param save_as_render: Save As Render, Apply render part of display transform when saving byte image
    :type save_as_render: bool
    :param copy: Copy, Create a new image file without modifying the current image in blender
    :type copy: bool
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


def save_sequence():
    ''' Save a sequence of images

    '''

    pass


def tile_add(number: int = 1002,
             count: int = 1,
             label: str = "",
             fill: bool = True,
             color: typing.List[float] = (0.0, 0.0, 0.0, 1.0),
             generated_type: typing.Union[int, str] = 'BLANK',
             width: int = 1024,
             height: int = 1024,
             float: bool = False,
             alpha: bool = True):
    ''' Adds a tile to the image

    :param number: Number, UDIM number of the tile
    :type number: int
    :param count: Count, How many tiles to add
    :type count: int
    :param label: Label, Optional tile label
    :type label: str
    :param fill: Fill, Fill new tile with a generated image
    :type fill: bool
    :param color: Color, Default fill color
    :type color: typing.List[float]
    :param generated_type: Generated Type, Fill the image with a grid for UV map testing * BLANK Blank, Generate a blank image. * UV_GRID UV Grid, Generated grid to test UV mappings. * COLOR_GRID Color Grid, Generated improved UV grid to test UV mappings.
    :type generated_type: typing.Union[int, str]
    :param width: Width, Image width
    :type width: int
    :param height: Height, Image height
    :type height: int
    :param float: 32-bit Float, Create image with 32-bit floating-point bit depth
    :type float: bool
    :param alpha: Alpha, Create an image with an alpha channel
    :type alpha: bool
    '''

    pass


def tile_fill(color: typing.List[float] = (0.0, 0.0, 0.0, 1.0),
              generated_type: typing.Union[int, str] = 'BLANK',
              width: int = 1024,
              height: int = 1024,
              float: bool = False,
              alpha: bool = True):
    ''' Fill the current tile with a generated image

    :param color: Color, Default fill color
    :type color: typing.List[float]
    :param generated_type: Generated Type, Fill the image with a grid for UV map testing * BLANK Blank, Generate a blank image. * UV_GRID UV Grid, Generated grid to test UV mappings. * COLOR_GRID Color Grid, Generated improved UV grid to test UV mappings.
    :type generated_type: typing.Union[int, str]
    :param width: Width, Image width
    :type width: int
    :param height: Height, Image height
    :type height: int
    :param float: 32-bit Float, Create image with 32-bit floating-point bit depth
    :type float: bool
    :param alpha: Alpha, Create an image with an alpha channel
    :type alpha: bool
    '''

    pass


def tile_remove():
    ''' Removes a tile from the image

    '''

    pass


def unpack(method: typing.Union[int, str] = 'USE_LOCAL', id: str = ""):
    ''' Save an image packed in the .blend file to disk

    :param method: Method, How to unpack
    :type method: typing.Union[int, str]
    :param id: Image Name, Image data-block name to unpack
    :type id: str
    '''

    pass


def view_all(fit_view: bool = False):
    ''' View the entire image

    :param fit_view: Fit View, Fit frame to the viewport
    :type fit_view: bool
    '''

    pass


def view_center_cursor():
    ''' Center the view so that the cursor is in the middle of the view

    '''

    pass


def view_ndof():
    ''' Use a 3D mouse device to pan/zoom the view

    '''

    pass


def view_pan(offset: typing.List[float] = (0.0, 0.0)):
    ''' Pan the view

    :param offset: Offset, Offset in floating-point units, 1.0 is the width and height of the image
    :type offset: typing.List[float]
    '''

    pass


def view_selected():
    ''' View all selected UVs

    '''

    pass


def view_zoom(factor: float = 0.0, use_cursor_init: bool = True):
    ''' Zoom in/out the image

    :param factor: Factor, Zoom factor, values higher than 1.0 zoom in, lower values zoom out
    :type factor: float
    :param use_cursor_init: Use Mouse Position, Allow the initial mouse position to be used
    :type use_cursor_init: bool
    '''

    pass


def view_zoom_border(xmin: int = 0,
                     xmax: int = 0,
                     ymin: int = 0,
                     ymax: int = 0,
                     wait_for_input: bool = True,
                     zoom_out: bool = False):
    ''' Zoom in the view to the nearest item contained in the border

    :param xmin: X Min
    :type xmin: int
    :param xmax: X Max
    :type xmax: int
    :param ymin: Y Min
    :type ymin: int
    :param ymax: Y Max
    :type ymax: int
    :param wait_for_input: Wait for Input
    :type wait_for_input: bool
    :param zoom_out: Zoom Out
    :type zoom_out: bool
    '''

    pass


def view_zoom_in(location: typing.List[float] = (0.0, 0.0)):
    ''' Zoom in the image (centered around 2D cursor)

    :param location: Location, Cursor location in screen coordinates
    :type location: typing.List[float]
    '''

    pass


def view_zoom_out(location: typing.List[float] = (0.0, 0.0)):
    ''' Zoom out the image (centered around 2D cursor)

    :param location: Location, Cursor location in screen coordinates
    :type location: typing.List[float]
    '''

    pass


def view_zoom_ratio(ratio: float = 0.0):
    ''' Set zoom ratio of the view

    :param ratio: Ratio, Zoom ratio, 1.0 is 1:1, higher is zoomed in, lower is zoomed out
    :type ratio: float
    '''

    pass
