import sys
import typing


def clear_by_owner(owner):
    ''' Clear all subscribers using this owner.

    '''

    pass


def publish_rna(key):
    ''' Notify subscribers of changes to this property (this typically doesn't need to be called explicitly since changes will automatically publish updates). In some cases it may be useful to publish changes explicitly using more general keys.

    :param key: Represents the type of data being subscribed to Arguments include - bpy.types.Property instance. - bpy.types.Struct type. - ( bpy.types.Struct , str) type and property name.
    '''

    pass


def subscribe_rna(key, owner, args, notify, options: set = 'set()'):
    ''' 

    :param key: Represents the type of data being subscribed to Arguments include - bpy.types.Property instance. - bpy.types.Struct type. - ( bpy.types.Struct , str) type and property name.
    :param owner: Handle for this subscription (compared by identity).
    :param options: Change the behavior of the subscriber. - PERSISTENT when set, the subscriber will be kept when remapping ID data.
    :type options: set
    '''

    pass
