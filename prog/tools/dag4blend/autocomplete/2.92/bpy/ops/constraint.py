import sys
import typing


def add_target():
    ''' Add a target to the constraint :file: startup/bl_operators/constraint.py\:35 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/constraint.py$35> _

    '''

    pass


def childof_clear_inverse(constraint: str = "",
                          owner: typing.Union[int, str] = 'OBJECT'):
    ''' Clear inverse correction for Child Of constraint

    :param constraint: Constraint, Name of the constraint to edit
    :type constraint: str
    :param owner: Owner, The owner of this constraint * OBJECT Object, Edit a constraint on the active object. * BONE Bone, Edit a constraint on the active bone.
    :type owner: typing.Union[int, str]
    '''

    pass


def childof_set_inverse(constraint: str = "",
                        owner: typing.Union[int, str] = 'OBJECT'):
    ''' Set inverse correction for Child Of constraint

    :param constraint: Constraint, Name of the constraint to edit
    :type constraint: str
    :param owner: Owner, The owner of this constraint * OBJECT Object, Edit a constraint on the active object. * BONE Bone, Edit a constraint on the active bone.
    :type owner: typing.Union[int, str]
    '''

    pass


def delete(constraint: str = "",
           owner: typing.Union[int, str] = 'OBJECT',
           report: bool = False):
    ''' Remove constraint from constraint stack

    :param constraint: Constraint, Name of the constraint to edit
    :type constraint: str
    :param owner: Owner, The owner of this constraint * OBJECT Object, Edit a constraint on the active object. * BONE Bone, Edit a constraint on the active bone.
    :type owner: typing.Union[int, str]
    :param report: Report, Create a notification after the operation
    :type report: bool
    '''

    pass


def disable_keep_transform():
    ''' Set the influence of this constraint to zero while trying to maintain the object's transformation. Other active constraints can still influence the final transformation :file: startup/bl_operators/constraint.py\:85 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/constraint.py$85> _

    '''

    pass


def followpath_path_animate(constraint: str = "",
                            owner: typing.Union[int, str] = 'OBJECT',
                            frame_start: int = 1,
                            length: int = 100):
    ''' Add default animation for path used by constraint if it isn't animated already

    :param constraint: Constraint, Name of the constraint to edit
    :type constraint: str
    :param owner: Owner, The owner of this constraint * OBJECT Object, Edit a constraint on the active object. * BONE Bone, Edit a constraint on the active bone.
    :type owner: typing.Union[int, str]
    :param frame_start: Start Frame, First frame of path animation
    :type frame_start: int
    :param length: Length, Number of frames that path animation should take
    :type length: int
    '''

    pass


def limitdistance_reset(constraint: str = "",
                        owner: typing.Union[int, str] = 'OBJECT'):
    ''' Reset limiting distance for Limit Distance Constraint

    :param constraint: Constraint, Name of the constraint to edit
    :type constraint: str
    :param owner: Owner, The owner of this constraint * OBJECT Object, Edit a constraint on the active object. * BONE Bone, Edit a constraint on the active bone.
    :type owner: typing.Union[int, str]
    '''

    pass


def move_down(constraint: str = "", owner: typing.Union[int, str] = 'OBJECT'):
    ''' Move constraint down in constraint stack

    :param constraint: Constraint, Name of the constraint to edit
    :type constraint: str
    :param owner: Owner, The owner of this constraint * OBJECT Object, Edit a constraint on the active object. * BONE Bone, Edit a constraint on the active bone.
    :type owner: typing.Union[int, str]
    '''

    pass


def move_to_index(constraint: str = "",
                  owner: typing.Union[int, str] = 'OBJECT',
                  index: int = 0):
    ''' Change the constraint's position in the list so it evaluates after the set number of others

    :param constraint: Constraint, Name of the constraint to edit
    :type constraint: str
    :param owner: Owner, The owner of this constraint * OBJECT Object, Edit a constraint on the active object. * BONE Bone, Edit a constraint on the active bone.
    :type owner: typing.Union[int, str]
    :param index: Index, The index to move the constraint to
    :type index: int
    '''

    pass


def move_up(constraint: str = "", owner: typing.Union[int, str] = 'OBJECT'):
    ''' Move constraint up in constraint stack

    :param constraint: Constraint, Name of the constraint to edit
    :type constraint: str
    :param owner: Owner, The owner of this constraint * OBJECT Object, Edit a constraint on the active object. * BONE Bone, Edit a constraint on the active bone.
    :type owner: typing.Union[int, str]
    '''

    pass


def normalize_target_weights():
    ''' Normalize weights of all target bones :file: startup/bl_operators/constraint.py\:60 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/constraint.py$60> _

    '''

    pass


def objectsolver_clear_inverse(constraint: str = "",
                               owner: typing.Union[int, str] = 'OBJECT'):
    ''' Clear inverse correction for Object Solver constraint

    :param constraint: Constraint, Name of the constraint to edit
    :type constraint: str
    :param owner: Owner, The owner of this constraint * OBJECT Object, Edit a constraint on the active object. * BONE Bone, Edit a constraint on the active bone.
    :type owner: typing.Union[int, str]
    '''

    pass


def objectsolver_set_inverse(constraint: str = "",
                             owner: typing.Union[int, str] = 'OBJECT'):
    ''' Set inverse correction for Object Solver constraint

    :param constraint: Constraint, Name of the constraint to edit
    :type constraint: str
    :param owner: Owner, The owner of this constraint * OBJECT Object, Edit a constraint on the active object. * BONE Bone, Edit a constraint on the active bone.
    :type owner: typing.Union[int, str]
    '''

    pass


def remove_target(index: int = 0):
    ''' Remove the target from the constraint

    :param index: index
    :type index: int
    '''

    pass


def stretchto_reset(constraint: str = "",
                    owner: typing.Union[int, str] = 'OBJECT'):
    ''' Reset original length of bone for Stretch To Constraint

    :param constraint: Constraint, Name of the constraint to edit
    :type constraint: str
    :param owner: Owner, The owner of this constraint * OBJECT Object, Edit a constraint on the active object. * BONE Bone, Edit a constraint on the active bone.
    :type owner: typing.Union[int, str]
    '''

    pass
