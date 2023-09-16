import sys
import typing


def rule_add(type: typing.Union[int, str] = 'GOAL'):
    ''' Add a boid rule to the current boid state

    :param type: Type * GOAL Goal, Go to assigned object or loudest assigned signal source. * AVOID Avoid, Get away from assigned object or loudest assigned signal source. * AVOID_COLLISION Avoid Collision, Maneuver to avoid collisions with other boids and deflector objects in near future. * SEPARATE Separate, Keep from going through other boids. * FLOCK Flock, Move to center of neighbors and match their velocity. * FOLLOW_LEADER Follow Leader, Follow a boid or assigned object. * AVERAGE_SPEED Average Speed, Maintain speed, flight level or wander. * FIGHT Fight, Go to closest enemy and attack when in range.
    :type type: typing.Union[int, str]
    '''

    pass


def rule_del():
    ''' Delete current boid rule

    '''

    pass


def rule_move_down():
    ''' Move boid rule down in the list

    '''

    pass


def rule_move_up():
    ''' Move boid rule up in the list

    '''

    pass


def state_add():
    ''' Add a boid state to the particle system

    '''

    pass


def state_del():
    ''' Delete current boid state

    '''

    pass


def state_move_down():
    ''' Move boid state down in the list

    '''

    pass


def state_move_up():
    ''' Move boid state up in the list

    '''

    pass
