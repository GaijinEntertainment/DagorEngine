import sys
import typing


def BoolProperty(name: str = "",
                 description: str = "",
                 default=False,
                 options: set = {'ANIMATABLE'},
                 override: set = 'set()',
                 tags: set = 'set()',
                 subtype: str = 'NONE',
                 update=None,
                 get=None,
                 set=None):
    ''' Returns a new boolean property definition.

    :param name: Name used in the user interface.
    :type name: str
    :param description: Text used for the tooltip and api documentation.
    :type description: str
    :param options: Enumerator in ['HIDDEN', 'SKIP_SAVE', 'ANIMATABLE', 'LIBRARY_EDITABLE', 'PROPORTIONAL','TEXTEDIT_UPDATE'].
    :type options: set
    :param override: Enumerator in ['LIBRARY_OVERRIDABLE'].
    :type override: set
    :param tags: Enumerator of tags that are defined by parent class.
    :type tags: set
    :param subtype: Enumerator in ['PIXEL', 'UNSIGNED', 'PERCENTAGE', 'FACTOR', 'ANGLE', 'TIME', 'DISTANCE', 'DISTANCE_CAMERA', 'POWER', 'TEMPERATURE', 'NONE'].
    :type subtype: str
    :param update: Function to be called when this value is modified, This function must take 2 values (self, context) and return None. *Warning* there are no safety checks to avoid infinite recursion.
    :param get: Function to be called when this value is 'read', This function must take 1 value (self) and return the value of the property.
    :param set: Function to be called when this value is 'written', This function must take 2 values (self, value) and return None.
    '''

    pass


def BoolVectorProperty(name: str = "",
                       description: str = "",
                       default: list = (False, False, False),
                       options: set = {'ANIMATABLE'},
                       override: set = 'set()',
                       tags: set = 'set()',
                       subtype: str = 'NONE',
                       size: int = 3,
                       update=None,
                       get=None,
                       set=None):
    ''' Returns a new vector boolean property definition.

    :param name: Name used in the user interface.
    :type name: str
    :param description: Text used for the tooltip and api documentation.
    :type description: str
    :param default: sequence of booleans the length of *size*.
    :type default: list
    :param options: Enumerator in ['HIDDEN', 'SKIP_SAVE', 'ANIMATABLE', 'LIBRARY_EDITABLE', 'PROPORTIONAL','TEXTEDIT_UPDATE'].
    :type options: set
    :param override: Enumerator in ['LIBRARY_OVERRIDABLE'].
    :type override: set
    :param tags: Enumerator of tags that are defined by parent class.
    :type tags: set
    :param subtype: Enumerator in ['COLOR', 'TRANSLATION', 'DIRECTION', 'VELOCITY', 'ACCELERATION', 'MATRIX', 'EULER', 'QUATERNION', 'AXISANGLE', 'XYZ', 'XYZ_LENGTH', 'COLOR_GAMMA', 'COORDINATES', 'LAYER', 'LAYER_MEMBER', 'NONE'].
    :type subtype: str
    :param size: Vector dimensions in [1, 32].
    :type size: int
    :param update: Function to be called when this value is modified, This function must take 2 values (self, context) and return None. *Warning* there are no safety checks to avoid infinite recursion.
    :param get: Function to be called when this value is 'read', This function must take 1 value (self) and return the value of the property.
    :param set: Function to be called when this value is 'written', This function must take 2 values (self, value) and return None.
    '''

    pass


def CollectionProperty(type=None,
                       name: str = "",
                       description: str = "",
                       options: set = {'ANIMATABLE'},
                       override: set = 'set()',
                       tags: set = 'set()'):
    ''' Returns a new collection property definition.

    :param type: bpy.types.ID .
    :param name: Name used in the user interface.
    :type name: str
    :param description: Text used for the tooltip and api documentation.
    :type description: str
    :param options: Enumerator in ['HIDDEN', 'SKIP_SAVE', 'ANIMATABLE', 'LIBRARY_EDITABLE', 'PROPORTIONAL','TEXTEDIT_UPDATE'].
    :type options: set
    :param override: Enumerator in ['LIBRARY_OVERRIDABLE', 'NO_PROPERTY_NAME', 'USE_INSERTION'].
    :type override: set
    :param tags: Enumerator of tags that are defined by parent class.
    :type tags: set
    '''

    pass


def EnumProperty(items: typing.Union[typing.List[typing.Tuple], typing.Tuple],
                 name: str = "",
                 description: str = "",
                 default: typing.Union[typing.Set[str], str] = None,
                 options: set = {'ANIMATABLE'},
                 override: set = 'set()',
                 tags: set = 'set()',
                 update=None,
                 get=None,
                 set=None):
    ''' Returns a new enumerator property definition.

    :param items: [(identifier, name, description, icon, number), ...] . The first three elements of the tuples are mandatory. :identifier: The identifier is used for Python access. :name: Name for the interface. :description: Used for documentation and tooltips. :icon: An icon string identifier or integer icon value (e.g. returned by bpy.types.UILayout.icon ) :number: Unique value used as the identifier for this item (stored in file data). Use when the identifier may need to change. If the *ENUM_FLAG* option is used, the values are bitmasks and should be powers of two. When an item only contains 4 items they define (identifier, name, description, number) . Separators may be added using None instead of a tuple. For dynamic values a callback can be passed which returns a list in the same format as the static list. This function must take 2 arguments (self, context) , **context may be None**. .. warning:: There is a known bug with using a callback, Python must keep a reference to the strings returned by the callback or Blender will misbehave or even crash.
    :type items: typing.List[str]
    :param name: Name used in the user interface.
    :type name: str
    :param description: Text used for the tooltip and api documentation.
    :type description: str
    :param default: The default value for this enum, a string from the identifiers used in *items*, or integer matching an item number. If the *ENUM_FLAG* option is used this must be a set of such string identifiers instead. WARNING: Strings can not be specified for dynamic enums (i.e. if a callback function is given as *items* parameter).
    :type default: typing.Set[str]
    :param options: Enumerator in ['HIDDEN', 'SKIP_SAVE', 'ANIMATABLE', 'ENUM_FLAG', 'LIBRARY_EDITABLE'].
    :type options: set
    :param override: Enumerator in ['LIBRARY_OVERRIDABLE'].
    :type override: set
    :param tags: Enumerator of tags that are defined by parent class.
    :type tags: set
    :param update: Function to be called when this value is modified, This function must take 2 values (self, context) and return None. *Warning* there are no safety checks to avoid infinite recursion.
    :param get: Function to be called when this value is 'read', This function must take 1 value (self) and return the value of the property.
    :param set: Function to be called when this value is 'written', This function must take 2 values (self, value) and return None.
    '''

    pass


def FloatProperty(name: str = "",
                  description: str = "",
                  default=0.0,
                  min: float = -3.402823e+38,
                  max: float = 3.402823e+38,
                  soft_min: float = -3.402823e+38,
                  soft_max: float = 3.402823e+38,
                  step: int = 3,
                  precision: int = 2,
                  options: set = {'ANIMATABLE'},
                  override: set = 'set()',
                  tags: set = 'set()',
                  subtype: str = 'NONE',
                  unit: str = 'NONE',
                  update=None,
                  get=None,
                  set=None):
    ''' Returns a new float (single precision) property definition.

    :param name: Name used in the user interface.
    :type name: str
    :param description: Text used for the tooltip and api documentation.
    :type description: str
    :param min: Hard minimum, trying to assign a value below will silently assign this minimum instead.
    :type min: float
    :param max: Hard maximum, trying to assign a value above will silently assign this maximum instead.
    :type max: float
    :param soft_min: Soft minimum (>= *min*), user won't be able to drag the widget below this value in the UI.
    :type soft_min: float
    :param soft_max: Soft maximum (<= *max*), user won't be able to drag the widget above this value in the UI.
    :type soft_max: float
    :param step: actual value is /100).
    :type step: int
    :param precision: Maximum number of decimal digits to display, in [0, 6].
    :type precision: int
    :param options: Enumerator in ['HIDDEN', 'SKIP_SAVE', 'ANIMATABLE', 'LIBRARY_EDITABLE', 'PROPORTIONAL','TEXTEDIT_UPDATE'].
    :type options: set
    :param override: Enumerator in ['LIBRARY_OVERRIDABLE'].
    :type override: set
    :param tags: Enumerator of tags that are defined by parent class.
    :type tags: set
    :param subtype: Enumerator in ['PIXEL', 'UNSIGNED', 'PERCENTAGE', 'FACTOR', 'ANGLE', 'TIME', 'DISTANCE', 'DISTANCE_CAMERA', 'POWER', 'TEMPERATURE', 'NONE'].
    :type subtype: str
    :param unit: Enumerator in ['NONE', 'LENGTH', 'AREA', 'VOLUME', 'ROTATION', 'TIME', 'VELOCITY', 'ACCELERATION', 'MASS', 'CAMERA', 'POWER'].
    :type unit: str
    :param update: Function to be called when this value is modified, This function must take 2 values (self, context) and return None. *Warning* there are no safety checks to avoid infinite recursion.
    :param get: Function to be called when this value is 'read', This function must take 1 value (self) and return the value of the property.
    :param set: Function to be called when this value is 'written', This function must take 2 values (self, value) and return None.
    '''

    pass


def FloatVectorProperty(name: str = "",
                        description: str = "",
                        default: list = (0.0, 0.0, 0.0),
                        min: float = 'sys.float_info.min',
                        max: float = 'sys.float_info.max',
                        soft_min: float = 'sys.float_info.min',
                        soft_max: float = 'sys.float_info.max',
                        step: int = 3,
                        precision: int = 2,
                        options: set = {'ANIMATABLE'},
                        override: set = 'set()',
                        tags: set = 'set()',
                        subtype: str = 'NONE',
                        unit: str = 'NONE',
                        size: int = 3,
                        update=None,
                        get=None,
                        set=None):
    ''' Returns a new vector float property definition.

    :param name: Name used in the user interface.
    :type name: str
    :param description: Text used for the tooltip and api documentation.
    :type description: str
    :param default: sequence of floats the length of *size*.
    :type default: list
    :param min: Hard minimum, trying to assign a value below will silently assign this minimum instead.
    :type min: float
    :param max: Hard maximum, trying to assign a value above will silently assign this maximum instead.
    :type max: float
    :param soft_min: Soft minimum (>= *min*), user won't be able to drag the widget below this value in the UI.
    :type soft_min: float
    :param soft_max: Soft maximum (<= *max*), user won't be able to drag the widget above this value in the UI.
    :type soft_max: float
    :param options: Enumerator in ['HIDDEN', 'SKIP_SAVE', 'ANIMATABLE', 'LIBRARY_EDITABLE', 'PROPORTIONAL','TEXTEDIT_UPDATE'].
    :type options: set
    :param override: Enumerator in ['LIBRARY_OVERRIDABLE'].
    :type override: set
    :param tags: Enumerator of tags that are defined by parent class.
    :type tags: set
    :param step: actual value is /100).
    :type step: int
    :param precision: Maximum number of decimal digits to display, in [0, 6].
    :type precision: int
    :param subtype: Enumerator in ['COLOR', 'TRANSLATION', 'DIRECTION', 'VELOCITY', 'ACCELERATION', 'MATRIX', 'EULER', 'QUATERNION', 'AXISANGLE', 'XYZ', 'XYZ_LENGTH', 'COLOR_GAMMA', 'COORDINATES', 'LAYER', 'LAYER_MEMBER', 'NONE'].
    :type subtype: str
    :param unit: Enumerator in ['NONE', 'LENGTH', 'AREA', 'VOLUME', 'ROTATION', 'TIME', 'VELOCITY', 'ACCELERATION', 'MASS', 'CAMERA', 'POWER'].
    :type unit: str
    :param size: Vector dimensions in [1, 32].
    :type size: int
    :param update: Function to be called when this value is modified, This function must take 2 values (self, context) and return None. *Warning* there are no safety checks to avoid infinite recursion.
    :param get: Function to be called when this value is 'read', This function must take 1 value (self) and return the value of the property.
    :param set: Function to be called when this value is 'written', This function must take 2 values (self, value) and return None.
    '''

    pass


def IntProperty(name: str = "",
                description: str = "",
                default=0,
                min: int = -2**31,
                max: int = 2**31 - 1,
                soft_min: int = -2**31,
                soft_max: int = 2**31 - 1,
                step: int = 1,
                options: set = {'ANIMATABLE'},
                override: set = 'set()',
                tags: set = 'set()',
                subtype: str = 'NONE',
                update=None,
                get=None,
                set=None):
    ''' Returns a new int property definition.

    :param name: Name used in the user interface.
    :type name: str
    :param description: Text used for the tooltip and api documentation.
    :type description: str
    :param min: Hard minimum, trying to assign a value below will silently assign this minimum instead.
    :type min: int
    :param max: Hard maximum, trying to assign a value above will silently assign this maximum instead.
    :type max: int
    :param soft_min: Soft minimum (>= *min*), user won't be able to drag the widget below this value in the UI.
    :type soft_min: int
    :param soft_max: Soft maximum (<= *max*), user won't be able to drag the widget above this value in the UI.
    :type soft_max: int
    :param step: unused currently!).
    :type step: int
    :param options: Enumerator in ['HIDDEN', 'SKIP_SAVE', 'ANIMATABLE', 'LIBRARY_EDITABLE', 'PROPORTIONAL','TEXTEDIT_UPDATE'].
    :type options: set
    :param override: Enumerator in ['LIBRARY_OVERRIDABLE'].
    :type override: set
    :param tags: Enumerator of tags that are defined by parent class.
    :type tags: set
    :param subtype: Enumerator in ['PIXEL', 'UNSIGNED', 'PERCENTAGE', 'FACTOR', 'ANGLE', 'TIME', 'DISTANCE', 'DISTANCE_CAMERA', 'POWER', 'TEMPERATURE', 'NONE'].
    :type subtype: str
    :param update: Function to be called when this value is modified, This function must take 2 values (self, context) and return None. *Warning* there are no safety checks to avoid infinite recursion.
    :param get: Function to be called when this value is 'read', This function must take 1 value (self) and return the value of the property.
    :param set: Function to be called when this value is 'written', This function must take 2 values (self, value) and return None.
    '''

    pass


def IntVectorProperty(name: str = "",
                      description: str = "",
                      default: list = (0, 0, 0),
                      min: int = -2**31,
                      max: int = 2**31 - 1,
                      soft_min: int = -2**31,
                      soft_max: int = 2**31 - 1,
                      step: int = 1,
                      options: set = {'ANIMATABLE'},
                      override: set = 'set()',
                      tags: set = 'set()',
                      subtype: str = 'NONE',
                      size: int = 3,
                      update=None,
                      get=None,
                      set=None):
    ''' Returns a new vector int property definition.

    :param name: Name used in the user interface.
    :type name: str
    :param description: Text used for the tooltip and api documentation.
    :type description: str
    :param default: sequence of ints the length of *size*.
    :type default: list
    :param min: Hard minimum, trying to assign a value below will silently assign this minimum instead.
    :type min: int
    :param max: Hard maximum, trying to assign a value above will silently assign this maximum instead.
    :type max: int
    :param soft_min: Soft minimum (>= *min*), user won't be able to drag the widget below this value in the UI.
    :type soft_min: int
    :param soft_max: Soft maximum (<= *max*), user won't be able to drag the widget above this value in the UI.
    :type soft_max: int
    :param step: unused currently!).
    :type step: int
    :param options: Enumerator in ['HIDDEN', 'SKIP_SAVE', 'ANIMATABLE', 'LIBRARY_EDITABLE', 'PROPORTIONAL','TEXTEDIT_UPDATE'].
    :type options: set
    :param override: Enumerator in ['LIBRARY_OVERRIDABLE'].
    :type override: set
    :param tags: Enumerator of tags that are defined by parent class.
    :type tags: set
    :param subtype: Enumerator in ['COLOR', 'TRANSLATION', 'DIRECTION', 'VELOCITY', 'ACCELERATION', 'MATRIX', 'EULER', 'QUATERNION', 'AXISANGLE', 'XYZ', 'XYZ_LENGTH', 'COLOR_GAMMA', 'COORDINATES', 'LAYER', 'LAYER_MEMBER', 'NONE'].
    :type subtype: str
    :param size: Vector dimensions in [1, 32].
    :type size: int
    :param update: Function to be called when this value is modified, This function must take 2 values (self, context) and return None. *Warning* there are no safety checks to avoid infinite recursion.
    :param get: Function to be called when this value is 'read', This function must take 1 value (self) and return the value of the property.
    :param set: Function to be called when this value is 'written', This function must take 2 values (self, value) and return None.
    '''

    pass


def PointerProperty(type=None,
                    name: str = "",
                    description: str = "",
                    options: set = {'ANIMATABLE'},
                    override: set = 'set()',
                    tags: set = 'set()',
                    poll=None,
                    update=None):
    ''' Returns a new pointer property definition.

    :param type: bpy.types.ID .
    :param name: Name used in the user interface.
    :type name: str
    :param description: Text used for the tooltip and api documentation.
    :type description: str
    :param options: Enumerator in ['HIDDEN', 'SKIP_SAVE', 'ANIMATABLE', 'LIBRARY_EDITABLE', 'PROPORTIONAL','TEXTEDIT_UPDATE'].
    :type options: set
    :param override: Enumerator in ['LIBRARY_OVERRIDABLE'].
    :type override: set
    :param tags: Enumerator of tags that are defined by parent class.
    :type tags: set
    :param poll: function to be called to determine whether an item is valid for this property. The function must take 2 values (self, object) and return Bool.
    :param update: Function to be called when this value is modified, This function must take 2 values (self, context) and return None. *Warning* there are no safety checks to avoid infinite recursion.
    '''

    pass


def RemoveProperty(cls, attr: str):
    ''' Removes a dynamically defined property.

    :param cls: The class containing the property (must be a positional argument).
    :param attr: Property name (must be passed as a keyword).
    :type attr: str
    '''

    pass


def StringProperty(name: str = "",
                   description: str = "",
                   default: str = "",
                   maxlen: int = 0,
                   options: set = {'ANIMATABLE'},
                   override: set = 'set()',
                   tags: set = 'set()',
                   subtype: str = 'NONE',
                   update=None,
                   get=None,
                   set=None):
    ''' Returns a new string property definition.

    :param name: Name used in the user interface.
    :type name: str
    :param description: Text used for the tooltip and api documentation.
    :type description: str
    :param default: initializer string.
    :type default: str
    :param maxlen: maximum length of the string.
    :type maxlen: int
    :param options: Enumerator in ['HIDDEN', 'SKIP_SAVE', 'ANIMATABLE', 'LIBRARY_EDITABLE', 'PROPORTIONAL','TEXTEDIT_UPDATE'].
    :type options: set
    :param override: Enumerator in ['LIBRARY_OVERRIDABLE'].
    :type override: set
    :param tags: Enumerator of tags that are defined by parent class.
    :type tags: set
    :param subtype: Enumerator in ['FILE_PATH', 'DIR_PATH', 'FILE_NAME', 'BYTE_STRING', 'PASSWORD', 'NONE'].
    :type subtype: str
    :param update: Function to be called when this value is modified, This function must take 2 values (self, context) and return None. *Warning* there are no safety checks to avoid infinite recursion.
    :param get: Function to be called when this value is 'read', This function must take 1 value (self) and return the value of the property.
    :param set: Function to be called when this value is 'written', This function must take 2 values (self, value) and return None.
    '''

    pass
