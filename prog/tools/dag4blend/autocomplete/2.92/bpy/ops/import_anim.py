import sys
import typing


def bvh(filepath: str = "",
        filter_glob: str = "*.bvh",
        target: typing.Union[int, str] = 'ARMATURE',
        global_scale: float = 1.0,
        frame_start: int = 1,
        use_fps_scale: bool = False,
        update_scene_fps: bool = False,
        update_scene_duration: bool = False,
        use_cyclic: bool = False,
        rotate_mode: typing.Union[int, str] = 'NATIVE',
        axis_forward: typing.Union[int, str] = '-Z',
        axis_up: typing.Union[int, str] = 'Y'):
    ''' Load a BVH motion capture file

    :param filepath: File Path, Filepath used for importing the file
    :type filepath: str
    :param filter_glob: filter_glob
    :type filter_glob: str
    :param target: Target, Import target type
    :type target: typing.Union[int, str]
    :param global_scale: Scale, Scale the BVH by this value
    :type global_scale: float
    :param frame_start: Start Frame, Starting frame for the animation
    :type frame_start: int
    :param use_fps_scale: Scale FPS, Scale the framerate from the BVH to the current scenes, otherwise each BVH frame maps directly to a Blender frame
    :type use_fps_scale: bool
    :param update_scene_fps: Update Scene FPS, Set the scene framerate to that of the BVH file (note that this nullifies the 'Scale FPS' option, as the scale will be 1:1)
    :type update_scene_fps: bool
    :param update_scene_duration: Update Scene Duration, Extend the scene's duration to the BVH duration (never shortens the scene)
    :type update_scene_duration: bool
    :param use_cyclic: Loop, Loop the animation playback
    :type use_cyclic: bool
    :param rotate_mode: Rotation, Rotation conversion * QUATERNION Quaternion, Convert rotations to quaternions. * NATIVE Euler (Native), Use the rotation order defined in the BVH file. * XYZ Euler (XYZ), Convert rotations to euler XYZ. * XZY Euler (XZY), Convert rotations to euler XZY. * YXZ Euler (YXZ), Convert rotations to euler YXZ. * YZX Euler (YZX), Convert rotations to euler YZX. * ZXY Euler (ZXY), Convert rotations to euler ZXY. * ZYX Euler (ZYX), Convert rotations to euler ZYX.
    :type rotate_mode: typing.Union[int, str]
    :param axis_forward: Forward
    :type axis_forward: typing.Union[int, str]
    :param axis_up: Up
    :type axis_up: typing.Union[int, str]
    '''

    pass
