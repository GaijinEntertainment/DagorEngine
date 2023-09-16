import sys
import typing
import freestyle.types


def get_border() -> tuple:
    ''' Returns the border.

    :return: A tuple of 4 numbers (xmin, ymin, xmax, ymax).
    '''

    pass


def get_canvas_height() -> int:
    ''' Returns the canvas height.

    :return: The canvas height.
    '''

    pass


def get_canvas_width() -> int:
    ''' Returns the canvas width.

    :return: The canvas width.
    '''

    pass


def get_selected_fedge() -> 'freestyle.types.FEdge':
    ''' Returns the selected FEdge.

    :return: The selected FEdge.
    '''

    pass


def get_time_stamp() -> int:
    ''' Returns the system time stamp.

    :return: The system time stamp.
    '''

    pass


def load_map(file_name: str,
             map_name: str,
             num_levels: int = 4,
             sigma: float = 1.0):
    ''' Loads an image map for further reading.

    :param file_name: The name of the image file.
    :type file_name: str
    :param map_name: The name that will be used to access this image.
    :type map_name: str
    :param num_levels: The number of levels in the map pyramid (default = 4). If num_levels == 0, the complete pyramid is built.
    :type num_levels: int
    :param sigma: The sigma value of the gaussian function.
    :type sigma: float
    '''

    pass


def read_complete_view_map_pixel(level: int, x: int, y: int) -> float:
    ''' Reads a pixel in the complete view map.

    :param level: The level of the pyramid in which we wish to read the pixel.
    :type level: int
    :param x: The x coordinate of the pixel we wish to read. The origin is in the lower-left corner.
    :type x: int
    :param y: The y coordinate of the pixel we wish to read. The origin is in the lower-left corner.
    :type y: int
    :return: The floating-point value stored for that pixel.
    '''

    pass


def read_directional_view_map_pixel(orientation: int, level: int, x: int,
                                    y: int) -> float:
    ''' Reads a pixel in one of the oriented view map images.

    :param orientation: The number telling which orientation we want to check.
    :type orientation: int
    :param level: The level of the pyramid in which we wish to read the pixel.
    :type level: int
    :param x: The x coordinate of the pixel we wish to read. The origin is in the lower-left corner.
    :type x: int
    :param y: The y coordinate of the pixel we wish to read. The origin is in the lower-left corner.
    :type y: int
    :return: The floating-point value stored for that pixel.
    '''

    pass


def read_map_pixel(map_name: str, level: int, x: int, y: int) -> float:
    ''' Reads a pixel in a user-defined map.

    :param map_name: The name of the map.
    :type map_name: str
    :param level: The level of the pyramid in which we wish to read the pixel.
    :type level: int
    :param x: The x coordinate of the pixel we wish to read. The origin is in the lower-left corner.
    :type x: int
    :param y: The y coordinate of the pixel we wish to read. The origin is in the lower-left corner.
    :type y: int
    :return: The floating-point value stored for that pixel.
    '''

    pass
