import sys
import typing


class InfoFunctionRNA:
    args = None
    ''' '''

    bl_func = None
    ''' '''

    description = None
    ''' '''

    global_lookup = None
    ''' '''

    identifier = None
    ''' '''

    is_classmethod = None
    ''' '''

    return_values = None
    ''' '''

    def build(self):
        ''' 

        '''
        pass


class InfoOperatorRNA:
    args = None
    ''' '''

    bl_op = None
    ''' '''

    description = None
    ''' '''

    func_name = None
    ''' '''

    global_lookup = None
    ''' '''

    identifier = None
    ''' '''

    module_name = None
    ''' '''

    name = None
    ''' '''

    def build(self):
        ''' 

        '''
        pass

    def get_location(self):
        ''' 

        '''
        pass


class InfoPropertyRNA:
    array_dimensions = None
    ''' '''

    array_length = None
    ''' '''

    bl_prop = None
    ''' '''

    collection_type = None
    ''' '''

    default = None
    ''' '''

    default_str = None
    ''' '''

    description = None
    ''' '''

    enum_items = None
    ''' '''

    fixed_type = None
    ''' '''

    global_lookup = None
    ''' '''

    identifier = None
    ''' '''

    is_argument_optional = None
    ''' '''

    is_enum_flag = None
    ''' '''

    is_never_none = None
    ''' '''

    is_readonly = None
    ''' '''

    is_required = None
    ''' '''

    max = None
    ''' '''

    min = None
    ''' '''

    name = None
    ''' '''

    srna = None
    ''' '''

    type = None
    ''' '''

    def build(self):
        ''' 

        '''
        pass

    def get_arg_default(self, force):
        ''' 

        '''
        pass

    def get_type_description(self, as_ret, as_arg, class_fmt, collection_id):
        ''' 

        '''
        pass


class InfoStructRNA:
    base = None
    ''' '''

    bl_rna = None
    ''' '''

    children = None
    ''' '''

    description = None
    ''' '''

    full_path = None
    ''' '''

    functions = None
    ''' '''

    global_lookup = None
    ''' '''

    identifier = None
    ''' '''

    module_name = None
    ''' '''

    name = None
    ''' '''

    nested = None
    ''' '''

    properties = None
    ''' '''

    py_class = None
    ''' '''

    references = None
    ''' '''

    def build(self):
        ''' 

        '''
        pass

    def get_bases(self):
        ''' 

        '''
        pass

    def get_nested_properties(self, ls):
        ''' 

        '''
        pass

    def get_py_c_functions(self):
        ''' 

        '''
        pass

    def get_py_functions(self):
        ''' 

        '''
        pass

    def get_py_properties(self):
        ''' 

        '''
        pass


def BuildRNAInfo():
    ''' 

    '''

    pass


def GetInfoFunctionRNA(bl_rna, parent_id):
    ''' 

    '''

    pass


def GetInfoOperatorRNA(bl_rna):
    ''' 

    '''

    pass


def GetInfoPropertyRNA(bl_rna, parent_id):
    ''' 

    '''

    pass


def GetInfoStructRNA(bl_rna):
    ''' 

    '''

    pass


def float_as_string(f):
    ''' 

    '''

    pass


def get_direct_functions(rna_type):
    ''' 

    '''

    pass


def get_direct_properties(rna_type):
    ''' 

    '''

    pass


def get_py_class_from_rna(rna_type):
    ''' 

    '''

    pass


def main():
    ''' 

    '''

    pass


def range_str(val):
    ''' 

    '''

    pass


def rna_id_ignore(rna_id):
    ''' 

    '''

    pass
