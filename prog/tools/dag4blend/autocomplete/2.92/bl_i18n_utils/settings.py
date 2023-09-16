import sys
import typing


class I18nSettings:
    BRANCHES_DIR = None
    ''' '''

    FILE_NAME_POT = None
    ''' '''

    GIT_I18N_PO_DIR = None
    ''' '''

    GIT_I18N_ROOT = None
    ''' '''

    MO_PATH_ROOT = None
    ''' '''

    MO_PATH_TEMPLATE = None
    ''' '''

    POTFILES_SOURCE_DIR = None
    ''' '''

    PY_SYS_PATHS = None
    ''' '''

    TRUNK_DIR = None
    ''' '''

    TRUNK_MO_DIR = None
    ''' '''

    TRUNK_PO_DIR = None
    ''' '''

    def from_dict(self, mapping):
        ''' 

        '''
        pass

    def from_json(self, string):
        ''' 

        '''
        pass

    def load(self, fname, reset):
        ''' 

        '''
        pass

    def save(self, fname):
        ''' 

        '''
        pass

    def to_dict(self):
        ''' 

        '''
        pass

    def to_json(self):
        ''' 

        '''
        pass
