import sys
import typing
import bpy.types


def clean(threshold: float = 0.001, channels: bool = False):
    ''' Simplify F-Curves by removing closely spaced keyframes

    :param threshold: Threshold
    :type threshold: float
    :param channels: Channels
    :type channels: bool
    '''

    pass


def clickselect(wait_to_deselect_others: bool = False,
                mouse_x: int = 0,
                mouse_y: int = 0,
                extend: bool = False,
                deselect_all: bool = False,
                column: bool = False,
                channel: bool = False):
    ''' Select keyframes by clicking on them

    :param wait_to_deselect_others: Wait to Deselect Others
    :type wait_to_deselect_others: bool
    :param mouse_x: Mouse X
    :type mouse_x: int
    :param mouse_y: Mouse Y
    :type mouse_y: int
    :param extend: Extend Select, Toggle keyframe selection instead of leaving newly selected keyframes only
    :type extend: bool
    :param deselect_all: Deselect On Nothing, Deselect all when nothing under the cursor
    :type deselect_all: bool
    :param column: Column Select, Select all keyframes that occur on the same frame as the one under the mouse
    :type column: bool
    :param channel: Only Channel, Select all the keyframes in the channel under the mouse
    :type channel: bool
    '''

    pass


def copy():
    ''' Copy selected keyframes to the copy/paste buffer

    '''

    pass


def delete(confirm: bool = True):
    ''' Remove all selected keyframes

    :param confirm: Confirm, Prompt for confirmation
    :type confirm: bool
    '''

    pass


def duplicate():
    ''' Make a copy of all selected keyframes

    '''

    pass


def duplicate_move(ACTION_OT_duplicate=None, TRANSFORM_OT_transform=None):
    ''' Make a copy of all selected keyframes and move them

    :param ACTION_OT_duplicate: Duplicate Keyframes, Make a copy of all selected keyframes
    :param TRANSFORM_OT_transform: Transform, Transform selected items by mode type
    '''

    pass


def easing_type(type: typing.Union[int, str] = 'AUTO'):
    ''' Set easing type for the F-Curve segments starting from the selected keyframes

    :param type: Type * AUTO Automatic Easing, Easing type is chosen automatically based on what the type of interpolation used (e.g. 'Ease In' for transitional types, and 'Ease Out' for dynamic effects). * EASE_IN Ease In, Only on the end closest to the next keyframe. * EASE_OUT Ease Out, Only on the end closest to the first keyframe. * EASE_IN_OUT Ease In and Out, Segment between both keyframes.
    :type type: typing.Union[int, str]
    '''

    pass


def extrapolation_type(type: typing.Union[int, str] = 'CONSTANT'):
    ''' Set extrapolation mode for selected F-Curves

    :param type: Type * CONSTANT Constant Extrapolation, Values on endpoint keyframes are held. * LINEAR Linear Extrapolation, Straight-line slope of end segments are extended past the endpoint keyframes. * MAKE_CYCLIC Make Cyclic (F-Modifier), Add Cycles F-Modifier if one doesn't exist already. * CLEAR_CYCLIC Clear Cyclic (F-Modifier), Remove Cycles F-Modifier if not needed anymore.
    :type type: typing.Union[int, str]
    '''

    pass


def frame_jump():
    ''' Set the current frame to the average frame value of selected keyframes

    '''

    pass


def handle_type(type: typing.Union[int, str] = 'FREE'):
    ''' Set type of handle for selected keyframes

    :param type: Type * FREE Free, Completely independent manually set handle. * ALIGNED Aligned, Manually set handle with rotation locked together with its pair. * VECTOR Vector, Automatic handles that create straight lines. * AUTO Automatic, Automatic handles that create smooth curves. * AUTO_CLAMPED Auto Clamped, Automatic handles that create smooth curves which only change direction at keyframes.
    :type type: typing.Union[int, str]
    '''

    pass


def interpolation_type(type: typing.Union[int, str] = 'CONSTANT'):
    ''' Set interpolation mode for the F-Curve segments starting from the selected keyframes

    :param type: Type * CONSTANT Constant, No interpolation, value of A gets held until B is encountered. * LINEAR Linear, Straight-line interpolation between A and B (i.e. no ease in/out). * BEZIER Bezier, Smooth interpolation between A and B, with some control over curve shape. * SINE Sinusoidal, Sinusoidal easing (weakest, almost linear but with a slight curvature). * QUAD Quadratic, Quadratic easing. * CUBIC Cubic, Cubic easing. * QUART Quartic, Quartic easing. * QUINT Quintic, Quintic easing. * EXPO Exponential, Exponential easing (dramatic). * CIRC Circular, Circular easing (strongest and most dynamic). * BACK Back, Cubic easing with overshoot and settle. * BOUNCE Bounce, Exponentially decaying parabolic bounce, like when objects collide. * ELASTIC Elastic, Exponentially decaying sine wave, like an elastic band.
    :type type: typing.Union[int, str]
    '''

    pass


def keyframe_insert(type: typing.Union[int, str] = 'ALL'):
    ''' Insert keyframes for the specified channels

    :param type: Type
    :type type: typing.Union[int, str]
    '''

    pass


def keyframe_type(type: typing.Union[int, str] = 'KEYFRAME'):
    ''' Set type of keyframe for the selected keyframes

    :param type: Type * KEYFRAME Keyframe, Normal keyframe, e.g. for key poses. * BREAKDOWN Breakdown, A breakdown pose, e.g. for transitions between key poses. * MOVING_HOLD Moving Hold, A keyframe that is part of a moving hold. * EXTREME Extreme, An 'extreme' pose, or some other purpose as needed. * JITTER Jitter, A filler or baked keyframe for keying on ones, or some other purpose as needed.
    :type type: typing.Union[int, str]
    '''

    pass


def layer_next():
    ''' Switch to editing action in animation layer above the current action in the NLA Stack

    '''

    pass


def layer_prev():
    ''' Switch to editing action in animation layer below the current action in the NLA Stack

    '''

    pass


def markers_make_local():
    ''' Move selected scene markers to the active Action as local 'pose' markers

    '''

    pass


def mirror(type: typing.Union[int, str] = 'CFRA'):
    ''' Flip selected keyframes over the selected mirror line

    :param type: Type * CFRA By Times Over Current Frame, Flip times of selected keyframes using the current frame as the mirror line. * XAXIS By Values Over Zero Value, Flip values of selected keyframes (i.e. negative values become positive, and vice versa). * MARKER By Times Over First Selected Marker, Flip times of selected keyframes using the first selected marker as the reference point.
    :type type: typing.Union[int, str]
    '''

    pass


def new():
    ''' Create new action

    '''

    pass


def paste(offset: typing.Union[int, str] = 'START',
          merge: typing.Union[int, str] = 'MIX',
          flipped: bool = False):
    ''' Paste keyframes from copy/paste buffer for the selected channels, starting on the current frame

    :param offset: Offset, Paste time offset of keys * START Frame Start, Paste keys starting at current frame. * END Frame End, Paste keys ending at current frame. * RELATIVE Frame Relative, Paste keys relative to the current frame when copying. * NONE No Offset, Paste keys from original time.
    :type offset: typing.Union[int, str]
    :param merge: Type, Method of merging pasted keys and existing * MIX Mix, Overlay existing with new keys. * OVER_ALL Overwrite All, Replace all keys. * OVER_RANGE Overwrite Range, Overwrite keys in pasted range. * OVER_RANGE_ALL Overwrite Entire Range, Overwrite keys in pasted range, using the range of all copied keys.
    :type merge: typing.Union[int, str]
    :param flipped: Flipped, Paste keyframes from mirrored bones if they exist
    :type flipped: bool
    '''

    pass


def previewrange_set():
    ''' Set Preview Range based on extents of selected Keyframes

    '''

    pass


def push_down():
    ''' Push action down on to the NLA stack as a new strip

    '''

    pass


def sample():
    ''' Add keyframes on every frame between the selected keyframes

    '''

    pass


def select_all(action: typing.Union[int, str] = 'TOGGLE'):
    ''' Toggle selection of all keyframes

    :param action: Action, Selection action to execute * TOGGLE Toggle, Toggle selection for all elements. * SELECT Select, Select all elements. * DESELECT Deselect, Deselect all elements. * INVERT Invert, Invert selection of all elements.
    :type action: typing.Union[int, str]
    '''

    pass


def select_box(axis_range: bool = False,
               xmin: int = 0,
               xmax: int = 0,
               ymin: int = 0,
               ymax: int = 0,
               wait_for_input: bool = True,
               mode: typing.Union[int, str] = 'SET',
               tweak: bool = False):
    ''' Select all keyframes within the specified region

    :param axis_range: Axis Range
    :type axis_range: bool
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
    :param mode: Mode * SET Set, Set a new selection. * ADD Extend, Extend existing selection. * SUB Subtract, Subtract existing selection.
    :type mode: typing.Union[int, str]
    :param tweak: Tweak, Operator has been activated using a tweak event
    :type tweak: bool
    '''

    pass


def select_circle(x: int = 0,
                  y: int = 0,
                  radius: int = 25,
                  wait_for_input: bool = True,
                  mode: typing.Union[int, str] = 'SET'):
    ''' Select keyframe points using circle selection

    :param x: X
    :type x: int
    :param y: Y
    :type y: int
    :param radius: Radius
    :type radius: int
    :param wait_for_input: Wait for Input
    :type wait_for_input: bool
    :param mode: Mode * SET Set, Set a new selection. * ADD Extend, Extend existing selection. * SUB Subtract, Subtract existing selection.
    :type mode: typing.Union[int, str]
    '''

    pass


def select_column(mode: typing.Union[int, str] = 'KEYS'):
    ''' Select all keyframes on the specified frame(s)

    :param mode: Mode
    :type mode: typing.Union[int, str]
    '''

    pass


def select_lasso(path: typing.Union[
        typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.
        List['bpy.types.OperatorMousePath'], 'bpy_prop_collection'] = None,
                 mode: typing.Union[int, str] = 'SET'):
    ''' Select keyframe points using lasso selection

    :param path: Path
    :type path: typing.Union[typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.List['bpy.types.OperatorMousePath'], 'bpy_prop_collection']
    :param mode: Mode * SET Set, Set a new selection. * ADD Extend, Extend existing selection. * SUB Subtract, Subtract existing selection.
    :type mode: typing.Union[int, str]
    '''

    pass


def select_leftright(mode: typing.Union[int, str] = 'CHECK',
                     extend: bool = False):
    ''' Select keyframes to the left or the right of the current frame

    :param mode: Mode
    :type mode: typing.Union[int, str]
    :param extend: Extend Select
    :type extend: bool
    '''

    pass


def select_less():
    ''' Deselect keyframes on ends of selection islands

    '''

    pass


def select_linked():
    ''' Select keyframes occurring in the same F-Curves as selected ones

    '''

    pass


def select_more():
    ''' Select keyframes beside already selected ones

    '''

    pass


def snap(type: typing.Union[int, str] = 'CFRA'):
    ''' Snap selected keyframes to the times specified

    :param type: Type * CFRA Selection to Current Frame, Snap selected keyframes to the current frame. * NEAREST_FRAME Selection to Nearest Frame, Snap selected keyframes to the nearest (whole) frame (use to fix accidental subframe offsets). * NEAREST_SECOND Selection to Nearest Second, Snap selected keyframes to the nearest second. * NEAREST_MARKER Selection to Nearest Marker, Snap selected keyframes to the nearest marker.
    :type type: typing.Union[int, str]
    '''

    pass


def stash(create_new: bool = True):
    ''' Store this action in the NLA stack as a non-contributing strip for later use

    :param create_new: Create New Action, Create a new action once the existing one has been safely stored
    :type create_new: bool
    '''

    pass


def stash_and_create():
    ''' Store this action in the NLA stack as a non-contributing strip for later use, and create a new action

    '''

    pass


def unlink(force_delete: bool = False):
    ''' Unlink this action from the active action slot (and/or exit Tweak Mode)

    :param force_delete: Force Delete, Clear Fake User and remove copy stashed in this data-block's NLA stack
    :type force_delete: bool
    '''

    pass


def view_all():
    ''' Reset viewable area to show full keyframe range

    '''

    pass


def view_frame():
    ''' Move the view to the current frame

    '''

    pass


def view_selected():
    ''' Reset viewable area to show selected keyframes range

    '''

    pass
