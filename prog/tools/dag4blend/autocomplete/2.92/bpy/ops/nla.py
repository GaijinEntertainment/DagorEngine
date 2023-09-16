import sys
import typing


def action_pushdown(channel_index: int = -1):
    ''' Push action down onto the top of the NLA stack as a new strip

    :param channel_index: Channel Index, Index of NLA action channel to perform pushdown operation on
    :type channel_index: int
    '''

    pass


def action_sync_length(active: bool = True):
    ''' Synchronize the length of the referenced Action with the length used in the strip

    :param active: Active Strip Only, Only sync the active length for the active strip
    :type active: bool
    '''

    pass


def action_unlink(force_delete: bool = False):
    ''' Unlink this action from the active action slot (and/or exit Tweak Mode)

    :param force_delete: Force Delete, Clear Fake User and remove copy stashed in this data-block's NLA stack
    :type force_delete: bool
    '''

    pass


def actionclip_add(action: typing.Union[int, str] = ''):
    ''' Add an Action-Clip strip (i.e. an NLA Strip referencing an Action) to the active track

    :param action: Action
    :type action: typing.Union[int, str]
    '''

    pass


def apply_scale():
    ''' Apply scaling of selected strips to their referenced Actions

    '''

    pass


def bake(
        frame_start: int = 1,
        frame_end: int = 250,
        step: int = 1,
        only_selected: bool = True,
        visual_keying: bool = False,
        clear_constraints: bool = False,
        clear_parents: bool = False,
        use_current_action: bool = False,
        clean_curves: bool = False,
        bake_types: typing.Union[typing.Set[int], typing.Set[str]] = {'POSE'}):
    ''' Bake all selected objects location/scale/rotation animation to an action

    :param frame_start: Start Frame, Start frame for baking
    :type frame_start: int
    :param frame_end: End Frame, End frame for baking
    :type frame_end: int
    :param step: Frame Step, Frame Step
    :type step: int
    :param only_selected: Only Selected Bones, Only key selected bones (Pose baking only)
    :type only_selected: bool
    :param visual_keying: Visual Keying, Keyframe from the final transformations (with constraints applied)
    :type visual_keying: bool
    :param clear_constraints: Clear Constraints, Remove all constraints from keyed object/bones, and do 'visual' keying
    :type clear_constraints: bool
    :param clear_parents: Clear Parents, Bake animation onto the object then clear parents (objects only)
    :type clear_parents: bool
    :param use_current_action: Overwrite Current Action, Bake animation into current action, instead of creating a new one (useful for baking only part of bones in an armature)
    :type use_current_action: bool
    :param clean_curves: Clean Curves, After baking curves, remove redundant keys
    :type clean_curves: bool
    :param bake_types: Bake Data, Which data's transformations to bake * POSE Pose, Bake bones transformations. * OBJECT Object, Bake object transformations.
    :type bake_types: typing.Union[typing.Set[int], typing.Set[str]]
    '''

    pass


def channels_click(extend: bool = False):
    ''' Handle clicks to select NLA channels

    :param extend: Extend Select
    :type extend: bool
    '''

    pass


def clear_scale():
    ''' Reset scaling of selected strips

    '''

    pass


def click_select(wait_to_deselect_others: bool = False,
                 mouse_x: int = 0,
                 mouse_y: int = 0,
                 extend: bool = False,
                 deselect_all: bool = False):
    ''' Handle clicks to select NLA Strips

    :param wait_to_deselect_others: Wait to Deselect Others
    :type wait_to_deselect_others: bool
    :param mouse_x: Mouse X
    :type mouse_x: int
    :param mouse_y: Mouse Y
    :type mouse_y: int
    :param extend: Extend Select
    :type extend: bool
    :param deselect_all: Deselect On Nothing, Deselect all when nothing under the cursor
    :type deselect_all: bool
    '''

    pass


def delete():
    ''' Delete selected strips

    '''

    pass


def duplicate(linked: bool = False,
              mode: typing.Union[int, str] = 'TRANSLATION'):
    ''' Duplicate selected NLA-Strips, adding the new strips in new tracks above the originals

    :param linked: Linked, When duplicating strips, assign new copies of the actions they use
    :type linked: bool
    :param mode: Mode
    :type mode: typing.Union[int, str]
    '''

    pass


def fmodifier_add(type: typing.Union[int, str] = 'NULL',
                  only_active: bool = True):
    ''' Add F-Modifier to the active/selected NLA-Strips

    :param type: Type * NULL Invalid. * GENERATOR Generator, Generate a curve using a factorized or expanded polynomial. * FNGENERATOR Built-In Function, Generate a curve using standard math functions such as sin and cos. * ENVELOPE Envelope, Reshape F-Curve values, e.g. change amplitude of movements. * CYCLES Cycles, Cyclic extend/repeat keyframe sequence. * NOISE Noise, Add pseudo-random noise on top of F-Curves. * LIMITS Limits, Restrict maximum and minimum values of F-Curve. * STEPPED Stepped Interpolation, Snap values to nearest grid step, e.g. for a stop-motion look.
    :type type: typing.Union[int, str]
    :param only_active: Only Active, Only add a F-Modifier of the specified type to the active strip
    :type only_active: bool
    '''

    pass


def fmodifier_copy():
    ''' Copy the F-Modifier(s) of the active NLA-Strip

    '''

    pass


def fmodifier_paste(only_active: bool = True, replace: bool = False):
    ''' Add copied F-Modifiers to the selected NLA-Strips

    :param only_active: Only Active, Only paste F-Modifiers on active strip
    :type only_active: bool
    :param replace: Replace Existing, Replace existing F-Modifiers, instead of just appending to the end of the existing list
    :type replace: bool
    '''

    pass


def make_single_user():
    ''' Ensure that each action is only used once in the set of strips selected

    '''

    pass


def meta_add():
    ''' Add new meta-strips incorporating the selected strips

    '''

    pass


def meta_remove():
    ''' Separate out the strips held by the selected meta-strips

    '''

    pass


def move_down():
    ''' Move selected strips down a track if there's room

    '''

    pass


def move_up():
    ''' Move selected strips up a track if there's room

    '''

    pass


def mute_toggle():
    ''' Mute or un-mute selected strips

    '''

    pass


def previewrange_set():
    ''' Automatically set Preview Range based on range of keyframes

    '''

    pass


def select_all(action: typing.Union[int, str] = 'TOGGLE'):
    ''' Select or deselect all NLA-Strips

    :param action: Action, Selection action to execute * TOGGLE Toggle, Toggle selection for all elements. * SELECT Select, Select all elements. * DESELECT Deselect, Deselect all elements. * INVERT Invert, Invert selection of all elements.
    :type action: typing.Union[int, str]
    '''

    pass


def select_box(axis_range: bool = False,
               tweak: bool = False,
               xmin: int = 0,
               xmax: int = 0,
               ymin: int = 0,
               ymax: int = 0,
               wait_for_input: bool = True,
               mode: typing.Union[int, str] = 'SET'):
    ''' Use box selection to grab NLA-Strips

    :param axis_range: Axis Range
    :type axis_range: bool
    :param tweak: Tweak, Operator has been activated using a tweak event
    :type tweak: bool
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
    '''

    pass


def select_leftright(mode: typing.Union[int, str] = 'CHECK',
                     extend: bool = False):
    ''' Select strips to the left or the right of the current frame

    :param mode: Mode
    :type mode: typing.Union[int, str]
    :param extend: Extend Select
    :type extend: bool
    '''

    pass


def selected_objects_add():
    ''' Make selected objects appear in NLA Editor by adding Animation Data

    '''

    pass


def snap(type: typing.Union[int, str] = 'CFRA'):
    ''' Move start of strips to specified time

    :param type: Type
    :type type: typing.Union[int, str]
    '''

    pass


def soundclip_add():
    ''' Add a strip for controlling when speaker plays its sound clip

    '''

    pass


def split():
    ''' Split selected strips at their midpoints

    '''

    pass


def swap():
    ''' Swap order of selected strips within tracks

    '''

    pass


def tracks_add(above_selected: bool = False):
    ''' Add NLA-Tracks above/after the selected tracks

    :param above_selected: Above Selected, Add a new NLA Track above every existing selected one
    :type above_selected: bool
    '''

    pass


def tracks_delete():
    ''' Delete selected NLA-Tracks and the strips they contain

    '''

    pass


def transition_add():
    ''' Add a transition strip between two adjacent selected strips

    '''

    pass


def tweakmode_enter(isolate_action: bool = False):
    ''' Enter tweaking mode for the action referenced by the active strip to edit its keyframes

    :param isolate_action: Isolate Action, Enable 'solo' on the NLA Track containing the active strip, to edit it without seeing the effects of the NLA stack
    :type isolate_action: bool
    '''

    pass


def tweakmode_exit(isolate_action: bool = False):
    ''' Exit tweaking mode for the action referenced by the active strip

    :param isolate_action: Isolate Action, Disable 'solo' on any of the NLA Tracks after exiting tweak mode to get things back to normal
    :type isolate_action: bool
    '''

    pass


def view_all():
    ''' Reset viewable area to show full strips range

    '''

    pass


def view_frame():
    ''' Move the view to the current frame

    '''

    pass


def view_selected():
    ''' Reset viewable area to show selected strips range

    '''

    pass
