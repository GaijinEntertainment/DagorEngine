import sys
import typing


def locale_explode(locale):
    ''' Return all components and their combinations of the given ISO locale string. >>> bpy.app.translations.locale_explode("sr_RS@latin") ("sr", "RS", "latin", "sr_RS", "sr@latin") For non-complete locales, missing elements will be None.

    :type msgid: str
    '''

    pass


def pgettext(msgid: str, msgctxt: str = None):
    ''' Try to translate the given msgid (with optional msgctxt).

    :param msgid: The string to translate.
    :type msgid: str
    :param msgctxt: The translation context (defaults to BLT_I18NCONTEXT_DEFAULT).
    :type msgctxt: str
    '''

    pass


def pgettext_data(msgid: str, msgctxt: str = None):
    ''' Try to translate the given msgid (with optional msgctxt), if new data name's translation is enabled.

    :param msgid: The string to translate.
    :type msgid: str
    :param msgctxt: The translation context (defaults to BLT_I18NCONTEXT_DEFAULT).
    :type msgctxt: str
    '''

    pass


def pgettext_iface(msgid: str, msgctxt: str = None):
    ''' Try to translate the given msgid (with optional msgctxt), if labels' translation is enabled.

    :param msgid: The string to translate.
    :type msgid: str
    :param msgctxt: The translation context (defaults to BLT_I18NCONTEXT_DEFAULT).
    :type msgctxt: str
    '''

    pass


def pgettext_tip(msgid: str, msgctxt: str = None):
    ''' Try to translate the given msgid (with optional msgctxt), if tooltips' translation is enabled.

    :param msgid: The string to translate.
    :type msgid: str
    :param msgctxt: The translation context (defaults to BLT_I18NCONTEXT_DEFAULT).
    :type msgctxt: str
    '''

    pass


def register(module_name: str, translations_dict: dict):
    ''' Registers an addon's UI translations.

    :param module_name: The name identifying the addon.
    :type module_name: str
    :param translations_dict: {locale: {msg_key: msg_translation, ...}, ...}
    :type translations_dict: dict
    '''

    pass


def unregister(module_name: str):
    ''' Unregisters an addon's UI translations.

    :param module_name: The name identifying the addon.
    :type module_name: str
    '''

    pass


contexts = None
''' constant value bpy.app.translations.contexts(default_real=None, default='*', operator_default='Operator', ui_events_keymaps='UI_Events_KeyMaps', plural='Plural', id_action='Action', id_armature='Armature', id_brush='Brush', id_camera='Camera', id_cachefile='CacheFile', id_collection='Collection', id_curve='Curve', id_fs_linestyle='FreestyleLineStyle', id_gpencil='GPencil', id_hair='Hair', id_id='ID', id_image='Image', id_shapekey='Key', id_light='Light', id_library='Library', id_lattice='Lattice', id_mask='Mask', ...)
'''

contexts_C_to_py = None
''' A readonly dict mapping contexts' C-identifiers to their py-identifiers.
'''

locale = None
''' The actual locale currently in use (will always return a void string when Blender is built without internationalization support).
'''

locales = None
''' All locales currently known by Blender (i.e. available as translations).
'''
