import sys
import typing


def armature_apply(selected: bool = False):
    ''' Apply the current pose as the new rest pose

    :param selected: Selected Only, Only apply the selected bones (with propagation to children)
    :type selected: bool
    '''

    pass


def autoside_names(axis: typing.Union[int, str] = 'XAXIS'):
    ''' Automatically renames the selected bones according to which side of the target axis they fall on

    :param axis: Axis, Axis tag names with * XAXIS X-Axis, Left/Right. * YAXIS Y-Axis, Front/Back. * ZAXIS Z-Axis, Top/Bottom.
    :type axis: typing.Union[int, str]
    '''

    pass


def bone_layers(
        layers: typing.List[bool] = (False, False, False, False, False, False,
                                     False, False, False, False, False, False,
                                     False, False, False, False, False, False,
                                     False, False, False, False, False, False,
                                     False, False, False, False, False, False,
                                     False, False)):
    ''' Change the layers that the selected bones belong to

    :param layers: Layer, Armature layers that bone belongs to
    :type layers: typing.List[bool]
    '''

    pass


def breakdown(percentage: float = 0.5,
              prev_frame: int = 0,
              next_frame: int = 0,
              channels: typing.Union[int, str] = 'ALL',
              axis_lock: typing.Union[int, str] = 'FREE'):
    ''' Create a suitable breakdown pose on the current frame

    :param percentage: Percentage, Weighting factor for which keyframe is favored more
    :type percentage: float
    :param prev_frame: Previous Keyframe, Frame number of keyframe immediately before the current frame
    :type prev_frame: int
    :param next_frame: Next Keyframe, Frame number of keyframe immediately after the current frame
    :type next_frame: int
    :param channels: Channels, Set of properties that are affected * ALL All Properties, All properties, including transforms, bendy bone shape, and custom properties. * LOC Location, Location only. * ROT Rotation, Rotation only. * SIZE Scale, Scale only. * BBONE Bendy Bone, Bendy Bone shape properties. * CUSTOM Custom Properties, Custom properties.
    :type channels: typing.Union[int, str]
    :param axis_lock: Axis Lock, Transform axis to restrict effects to * FREE Free, All axes are affected. * X X, Only X-axis transforms are affected. * Y Y, Only Y-axis transforms are affected. * Z Z, Only Z-axis transforms are affected.
    :type axis_lock: typing.Union[int, str]
    '''

    pass


def constraint_add(type: typing.Union[int, str] = ''):
    ''' Add a constraint to the active bone

    :param type: Type * CAMERA_SOLVER Camera Solver. * FOLLOW_TRACK Follow Track. * OBJECT_SOLVER Object Solver. * COPY_LOCATION Copy Location, Copy the location of a target (with an optional offset), so that they move together. * COPY_ROTATION Copy Rotation, Copy the rotation of a target (with an optional offset), so that they rotate together. * COPY_SCALE Copy Scale, Copy the scale factors of a target (with an optional offset), so that they are scaled by the same amount. * COPY_TRANSFORMS Copy Transforms, Copy all the transformations of a target, so that they move together. * LIMIT_DISTANCE Limit Distance, Restrict movements to within a certain distance of a target (at the time of constraint evaluation only). * LIMIT_LOCATION Limit Location, Restrict movement along each axis within given ranges. * LIMIT_ROTATION Limit Rotation, Restrict rotation along each axis within given ranges. * LIMIT_SCALE Limit Scale, Restrict scaling along each axis with given ranges. * MAINTAIN_VOLUME Maintain Volume, Compensate for scaling one axis by applying suitable scaling to the other two axes. * TRANSFORM Transformation, Use one transform property from target to control another (or same) property on owner. * TRANSFORM_CACHE Transform Cache, Look up the transformation matrix from an external file. * CLAMP_TO Clamp To, Restrict movements to lie along a curve by remapping location along curve's longest axis. * DAMPED_TRACK Damped Track, Point towards a target by performing the smallest rotation necessary. * IK Inverse Kinematics, Control a chain of bones by specifying the endpoint target (Bones only). * LOCKED_TRACK Locked Track, Rotate around the specified ('locked') axis to point towards a target. * SPLINE_IK Spline IK, Align chain of bones along a curve (Bones only). * STRETCH_TO Stretch To, Stretch along Y-Axis to point towards a target. * TRACK_TO Track To, Legacy tracking constraint prone to twisting artifacts. * ACTION Action, Use transform property of target to look up pose for owner from an Action. * ARMATURE Armature, Apply weight-blended transformation from multiple bones like the Armature modifier. * CHILD_OF Child Of, Make target the 'detachable' parent of owner. * FLOOR Floor, Use position (and optionally rotation) of target to define a 'wall' or 'floor' that the owner can not cross. * FOLLOW_PATH Follow Path, Use to animate an object/bone following a path. * PIVOT Pivot, Change pivot point for transforms (buggy). * SHRINKWRAP Shrinkwrap, Restrict movements to surface of target mesh.
    :type type: typing.Union[int, str]
    '''

    pass


def constraint_add_with_targets(type: typing.Union[int, str] = ''):
    ''' Add a constraint to the active bone, with target (where applicable) set to the selected Objects/Bones

    :param type: Type * CAMERA_SOLVER Camera Solver. * FOLLOW_TRACK Follow Track. * OBJECT_SOLVER Object Solver. * COPY_LOCATION Copy Location, Copy the location of a target (with an optional offset), so that they move together. * COPY_ROTATION Copy Rotation, Copy the rotation of a target (with an optional offset), so that they rotate together. * COPY_SCALE Copy Scale, Copy the scale factors of a target (with an optional offset), so that they are scaled by the same amount. * COPY_TRANSFORMS Copy Transforms, Copy all the transformations of a target, so that they move together. * LIMIT_DISTANCE Limit Distance, Restrict movements to within a certain distance of a target (at the time of constraint evaluation only). * LIMIT_LOCATION Limit Location, Restrict movement along each axis within given ranges. * LIMIT_ROTATION Limit Rotation, Restrict rotation along each axis within given ranges. * LIMIT_SCALE Limit Scale, Restrict scaling along each axis with given ranges. * MAINTAIN_VOLUME Maintain Volume, Compensate for scaling one axis by applying suitable scaling to the other two axes. * TRANSFORM Transformation, Use one transform property from target to control another (or same) property on owner. * TRANSFORM_CACHE Transform Cache, Look up the transformation matrix from an external file. * CLAMP_TO Clamp To, Restrict movements to lie along a curve by remapping location along curve's longest axis. * DAMPED_TRACK Damped Track, Point towards a target by performing the smallest rotation necessary. * IK Inverse Kinematics, Control a chain of bones by specifying the endpoint target (Bones only). * LOCKED_TRACK Locked Track, Rotate around the specified ('locked') axis to point towards a target. * SPLINE_IK Spline IK, Align chain of bones along a curve (Bones only). * STRETCH_TO Stretch To, Stretch along Y-Axis to point towards a target. * TRACK_TO Track To, Legacy tracking constraint prone to twisting artifacts. * ACTION Action, Use transform property of target to look up pose for owner from an Action. * ARMATURE Armature, Apply weight-blended transformation from multiple bones like the Armature modifier. * CHILD_OF Child Of, Make target the 'detachable' parent of owner. * FLOOR Floor, Use position (and optionally rotation) of target to define a 'wall' or 'floor' that the owner can not cross. * FOLLOW_PATH Follow Path, Use to animate an object/bone following a path. * PIVOT Pivot, Change pivot point for transforms (buggy). * SHRINKWRAP Shrinkwrap, Restrict movements to surface of target mesh.
    :type type: typing.Union[int, str]
    '''

    pass


def constraints_clear():
    ''' Clear all the constraints for the selected bones

    '''

    pass


def constraints_copy():
    ''' Copy constraints to other selected bones

    '''

    pass


def copy():
    ''' Copies the current pose of the selected bones to copy/paste buffer

    '''

    pass


def flip_names(do_strip_numbers: bool = False):
    ''' Flips (and corrects) the axis suffixes of the names of selected bones

    :param do_strip_numbers: Strip Numbers, Try to remove right-most dot-number from flipped names (WARNING: may result in incoherent naming in some cases)
    :type do_strip_numbers: bool
    '''

    pass


def group_add():
    ''' Add a new bone group

    '''

    pass


def group_assign(type: int = 0):
    ''' Add selected bones to the chosen bone group

    :param type: Bone Group Index
    :type type: int
    '''

    pass


def group_deselect():
    ''' Deselect bones of active Bone Group

    '''

    pass


def group_move(direction: typing.Union[int, str] = 'UP'):
    ''' Change position of active Bone Group in list of Bone Groups

    :param direction: Direction, Direction to move the active Bone Group towards
    :type direction: typing.Union[int, str]
    '''

    pass


def group_remove():
    ''' Remove the active bone group

    '''

    pass


def group_select():
    ''' Select bones in active Bone Group

    '''

    pass


def group_sort():
    ''' Sort Bone Groups by their names in ascending order

    '''

    pass


def group_unassign():
    ''' Remove selected bones from all bone groups

    '''

    pass


def hide(unselected: bool = False):
    ''' Tag selected bones to not be visible in Pose Mode

    :param unselected: Unselected
    :type unselected: bool
    '''

    pass


def ik_add(with_targets: bool = True):
    ''' Add IK Constraint to the active Bone

    :param with_targets: With Targets, Assign IK Constraint with targets derived from the select bones/objects
    :type with_targets: bool
    '''

    pass


def ik_clear():
    ''' Remove all IK Constraints from selected bones

    '''

    pass


def loc_clear():
    ''' Reset locations of selected bones to their default values

    '''

    pass


def paste(flipped: bool = False, selected_mask: bool = False):
    ''' Paste the stored pose on to the current pose

    :param flipped: Flipped on X-Axis, Paste the stored pose flipped on to current pose
    :type flipped: bool
    :param selected_mask: On Selected Only, Only paste the stored pose on to selected bones in the current pose
    :type selected_mask: bool
    '''

    pass


def paths_calculate(start_frame: int = 1,
                    end_frame: int = 250,
                    bake_location: typing.Union[int, str] = 'HEADS'):
    ''' Calculate paths for the selected bones

    :param start_frame: Start, First frame to calculate bone paths on
    :type start_frame: int
    :param end_frame: End, Last frame to calculate bone paths on
    :type end_frame: int
    :param bake_location: Bake Location, Which point on the bones is used when calculating paths * HEADS Heads, Calculate bone paths from heads. * TAILS Tails, Calculate bone paths from tails.
    :type bake_location: typing.Union[int, str]
    '''

    pass


def paths_clear(only_selected: bool = False):
    ''' Clear path caches for all bones, hold Shift key for selected bones only

    :param only_selected: Only Selected, Only clear paths from selected bones
    :type only_selected: bool
    '''

    pass


def paths_range_update():
    ''' Update frame range for motion paths from the Scene's current frame range

    '''

    pass


def paths_update():
    ''' Recalculate paths for bones that already have them

    '''

    pass


def propagate(mode: typing.Union[int, str] = 'WHILE_HELD',
              end_frame: float = 250.0):
    ''' Copy selected aspects of the current pose to subsequent poses already keyframed

    :param mode: Terminate Mode, Method used to determine when to stop propagating pose to keyframes * WHILE_HELD While Held, Propagate pose to all keyframes after current frame that don't change (Default behavior). * NEXT_KEY To Next Keyframe, Propagate pose to first keyframe following the current frame only. * LAST_KEY To Last Keyframe, Propagate pose to the last keyframe only (i.e. making action cyclic). * BEFORE_FRAME Before Frame, Propagate pose to all keyframes between current frame and 'Frame' property. * BEFORE_END Before Last Keyframe, Propagate pose to all keyframes from current frame until no more are found. * SELECTED_KEYS On Selected Keyframes, Propagate pose to all selected keyframes. * SELECTED_MARKERS On Selected Markers, Propagate pose to all keyframes occurring on frames with Scene Markers after the current frame.
    :type mode: typing.Union[int, str]
    :param end_frame: End Frame, Frame to stop propagating frames to (for 'Before Frame' mode)
    :type end_frame: float
    '''

    pass


def push(percentage: float = 0.5,
         prev_frame: int = 0,
         next_frame: int = 0,
         channels: typing.Union[int, str] = 'ALL',
         axis_lock: typing.Union[int, str] = 'FREE'):
    ''' Exaggerate the current pose in regards to the breakdown pose

    :param percentage: Percentage, Weighting factor for which keyframe is favored more
    :type percentage: float
    :param prev_frame: Previous Keyframe, Frame number of keyframe immediately before the current frame
    :type prev_frame: int
    :param next_frame: Next Keyframe, Frame number of keyframe immediately after the current frame
    :type next_frame: int
    :param channels: Channels, Set of properties that are affected * ALL All Properties, All properties, including transforms, bendy bone shape, and custom properties. * LOC Location, Location only. * ROT Rotation, Rotation only. * SIZE Scale, Scale only. * BBONE Bendy Bone, Bendy Bone shape properties. * CUSTOM Custom Properties, Custom properties.
    :type channels: typing.Union[int, str]
    :param axis_lock: Axis Lock, Transform axis to restrict effects to * FREE Free, All axes are affected. * X X, Only X-axis transforms are affected. * Y Y, Only Y-axis transforms are affected. * Z Z, Only Z-axis transforms are affected.
    :type axis_lock: typing.Union[int, str]
    '''

    pass


def push_rest(percentage: float = 0.5,
              prev_frame: int = 0,
              next_frame: int = 0,
              channels: typing.Union[int, str] = 'ALL',
              axis_lock: typing.Union[int, str] = 'FREE'):
    ''' Push the current pose further away from the rest pose

    :param percentage: Percentage, Weighting factor for which keyframe is favored more
    :type percentage: float
    :param prev_frame: Previous Keyframe, Frame number of keyframe immediately before the current frame
    :type prev_frame: int
    :param next_frame: Next Keyframe, Frame number of keyframe immediately after the current frame
    :type next_frame: int
    :param channels: Channels, Set of properties that are affected * ALL All Properties, All properties, including transforms, bendy bone shape, and custom properties. * LOC Location, Location only. * ROT Rotation, Rotation only. * SIZE Scale, Scale only. * BBONE Bendy Bone, Bendy Bone shape properties. * CUSTOM Custom Properties, Custom properties.
    :type channels: typing.Union[int, str]
    :param axis_lock: Axis Lock, Transform axis to restrict effects to * FREE Free, All axes are affected. * X X, Only X-axis transforms are affected. * Y Y, Only Y-axis transforms are affected. * Z Z, Only Z-axis transforms are affected.
    :type axis_lock: typing.Union[int, str]
    '''

    pass


def quaternions_flip():
    ''' Flip quaternion values to achieve desired rotations, while maintaining the same orientations

    '''

    pass


def relax(percentage: float = 0.5,
          prev_frame: int = 0,
          next_frame: int = 0,
          channels: typing.Union[int, str] = 'ALL',
          axis_lock: typing.Union[int, str] = 'FREE'):
    ''' Make the current pose more similar to its breakdown pose

    :param percentage: Percentage, Weighting factor for which keyframe is favored more
    :type percentage: float
    :param prev_frame: Previous Keyframe, Frame number of keyframe immediately before the current frame
    :type prev_frame: int
    :param next_frame: Next Keyframe, Frame number of keyframe immediately after the current frame
    :type next_frame: int
    :param channels: Channels, Set of properties that are affected * ALL All Properties, All properties, including transforms, bendy bone shape, and custom properties. * LOC Location, Location only. * ROT Rotation, Rotation only. * SIZE Scale, Scale only. * BBONE Bendy Bone, Bendy Bone shape properties. * CUSTOM Custom Properties, Custom properties.
    :type channels: typing.Union[int, str]
    :param axis_lock: Axis Lock, Transform axis to restrict effects to * FREE Free, All axes are affected. * X X, Only X-axis transforms are affected. * Y Y, Only Y-axis transforms are affected. * Z Z, Only Z-axis transforms are affected.
    :type axis_lock: typing.Union[int, str]
    '''

    pass


def relax_rest(percentage: float = 0.5,
               prev_frame: int = 0,
               next_frame: int = 0,
               channels: typing.Union[int, str] = 'ALL',
               axis_lock: typing.Union[int, str] = 'FREE'):
    ''' Make the current pose more similar to the rest pose

    :param percentage: Percentage, Weighting factor for which keyframe is favored more
    :type percentage: float
    :param prev_frame: Previous Keyframe, Frame number of keyframe immediately before the current frame
    :type prev_frame: int
    :param next_frame: Next Keyframe, Frame number of keyframe immediately after the current frame
    :type next_frame: int
    :param channels: Channels, Set of properties that are affected * ALL All Properties, All properties, including transforms, bendy bone shape, and custom properties. * LOC Location, Location only. * ROT Rotation, Rotation only. * SIZE Scale, Scale only. * BBONE Bendy Bone, Bendy Bone shape properties. * CUSTOM Custom Properties, Custom properties.
    :type channels: typing.Union[int, str]
    :param axis_lock: Axis Lock, Transform axis to restrict effects to * FREE Free, All axes are affected. * X X, Only X-axis transforms are affected. * Y Y, Only Y-axis transforms are affected. * Z Z, Only Z-axis transforms are affected.
    :type axis_lock: typing.Union[int, str]
    '''

    pass


def reveal(select: bool = True):
    ''' Reveal all bones hidden in Pose Mode

    :param select: Select
    :type select: bool
    '''

    pass


def rot_clear():
    ''' Reset rotations of selected bones to their default values

    '''

    pass


def rotation_mode_set(type: typing.Union[int, str] = 'QUATERNION'):
    ''' Set the rotation representation used by selected bones

    :param type: Rotation Mode * QUATERNION Quaternion (WXYZ), No Gimbal Lock. * XYZ XYZ Euler, XYZ Rotation Order - prone to Gimbal Lock (default). * XZY XZY Euler, XZY Rotation Order - prone to Gimbal Lock. * YXZ YXZ Euler, YXZ Rotation Order - prone to Gimbal Lock. * YZX YZX Euler, YZX Rotation Order - prone to Gimbal Lock. * ZXY ZXY Euler, ZXY Rotation Order - prone to Gimbal Lock. * ZYX ZYX Euler, ZYX Rotation Order - prone to Gimbal Lock. * AXIS_ANGLE Axis Angle, Axis Angle (W+XYZ), defines a rotation around some axis defined by 3D-Vector.
    :type type: typing.Union[int, str]
    '''

    pass


def scale_clear():
    ''' Reset scaling of selected bones to their default values

    '''

    pass


def select_all(action: typing.Union[int, str] = 'TOGGLE'):
    ''' Toggle selection status of all bones

    :param action: Action, Selection action to execute * TOGGLE Toggle, Toggle selection for all elements. * SELECT Select, Select all elements. * DESELECT Deselect, Deselect all elements. * INVERT Invert, Invert selection of all elements.
    :type action: typing.Union[int, str]
    '''

    pass


def select_constraint_target():
    ''' Select bones used as targets for the currently selected bones

    '''

    pass


def select_grouped(extend: bool = False,
                   type: typing.Union[int, str] = 'LAYER'):
    ''' Select all visible bones grouped by similar properties

    :param extend: Extend, Extend selection instead of deselecting everything first
    :type extend: bool
    :param type: Type * LAYER Layer, Shared layers. * GROUP Group, Shared group. * KEYINGSET Keying Set, All bones affected by active Keying Set.
    :type type: typing.Union[int, str]
    '''

    pass


def select_hierarchy(direction: typing.Union[int, str] = 'PARENT',
                     extend: bool = False):
    ''' Select immediate parent/children of selected bones

    :param direction: Direction
    :type direction: typing.Union[int, str]
    :param extend: Extend, Extend the selection
    :type extend: bool
    '''

    pass


def select_linked():
    ''' Select all bones linked by parent/child connections to the current selection

    '''

    pass


def select_linked_pick(extend: bool = False):
    ''' Select bones linked by parent/child connections under the mouse cursor

    :param extend: Extend, Extend selection instead of deselecting everything first
    :type extend: bool
    '''

    pass


def select_mirror(only_active: bool = False, extend: bool = False):
    ''' Mirror the bone selection

    :param only_active: Active Only, Only operate on the active bone
    :type only_active: bool
    :param extend: Extend, Extend the selection
    :type extend: bool
    '''

    pass


def select_parent():
    ''' Select bones that are parents of the currently selected bones

    '''

    pass


def transforms_clear():
    ''' Reset location, rotation, and scaling of selected bones to their default values

    '''

    pass


def user_transforms_clear(only_selected: bool = True):
    ''' Reset pose bone transforms to keyframed state

    :param only_selected: Only Selected, Only visible/selected bones
    :type only_selected: bool
    '''

    pass


def visual_transform_apply():
    ''' Apply final constrained position of pose bones to their transform

    '''

    pass
