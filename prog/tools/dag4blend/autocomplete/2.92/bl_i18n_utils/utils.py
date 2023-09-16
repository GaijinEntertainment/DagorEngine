import sys
import typing


class I18n:
    parsers = None
    ''' '''

    py_file = None
    ''' '''

    writers = None
    ''' '''

    def check_py_module_has_translations(self, src, settings):
        ''' 

        '''
        pass

    def escape(self, do_all):
        ''' 

        '''
        pass

    def parse(self, kind, src, langs):
        ''' 

        '''
        pass

    def parse_from_po(self, src, langs):
        ''' 

        '''
        pass

    def parse_from_py(self, src, langs):
        ''' 

        '''
        pass

    def print_stats(self, prefix, print_msgs):
        ''' 

        '''
        pass

    def unescape(self, do_all):
        ''' 

        '''
        pass

    def update_info(self):
        ''' 

        '''
        pass

    def write(self, kind, langs):
        ''' 

        '''
        pass

    def write_to_po(self, langs):
        ''' 

        '''
        pass

    def write_to_py(self, langs):
        ''' 

        '''
        pass


class I18nMessage:
    comment_lines = None
    ''' '''

    is_commented = None
    ''' '''

    is_fuzzy = None
    ''' '''

    is_tooltip = None
    ''' '''

    msgctxt = None
    ''' '''

    msgctxt_lines = None
    ''' '''

    msgid = None
    ''' '''

    msgid_lines = None
    ''' '''

    msgstr = None
    ''' '''

    msgstr_lines = None
    ''' '''

    settings = None
    ''' '''

    sources = None
    ''' '''

    def copy(self):
        ''' 

        '''
        pass

    def do_escape(self, txt):
        ''' 

        '''
        pass

    def do_unescape(self, txt):
        ''' 

        '''
        pass

    def escape(self, do_all):
        ''' 

        '''
        pass

    def normalize(self, max_len):
        ''' 

        '''
        pass

    def unescape(self, do_all):
        ''' 

        '''
        pass


class I18nMessages:
    parsers = None
    ''' '''

    writers = None
    ''' '''

    def check(self, fix):
        ''' 

        '''
        pass

    def clean_commented(self):
        ''' 

        '''
        pass

    def escape(self, do_all):
        ''' 

        '''
        pass

    def find_best_messages_matches(self, msgs, msgmap, rna_ctxt,
                                   rna_struct_name, rna_prop_name,
                                   rna_enum_name):
        ''' 

        '''
        pass

    def gen_empty_messages(self, uid, blender_ver, blender_hash, time, year,
                           default_copyright, settings):
        ''' 

        '''
        pass

    def invalidate_reverse_cache(self, rebuild_now):
        ''' 

        '''
        pass

    def merge(self, msgs, replace):
        ''' 

        '''
        pass

    def normalize(self, max_len):
        ''' 

        '''
        pass

    def parse(self, kind, key, src):
        ''' 

        '''
        pass

    def parse_messages_from_po(self, src, key):
        ''' 

        '''
        pass

    def print_info(self, prefix, output, print_stats, print_errors):
        ''' 

        '''
        pass

    def rtl_process(self):
        ''' 

        '''
        pass

    def unescape(self, do_all):
        ''' 

        '''
        pass

    def update(self, ref, use_similar, keep_old_commented):
        ''' 

        '''
        pass

    def update_info(self):
        ''' 

        '''
        pass

    def write(self, kind, dest):
        ''' 

        '''
        pass

    def write_messages_to_mo(self, fname):
        ''' 

        '''
        pass

    def write_messages_to_po(self, fname, compact):
        ''' 

        '''
        pass


def enable_addons(addons, support, disable, check_only):
    ''' 

    '''

    pass


def find_best_isocode_matches(uid, iso_codes):
    ''' 

    '''

    pass


def get_best_similar(data):
    ''' 

    '''

    pass


def get_po_files_from_dir(root_dir, langs):
    ''' 

    '''

    pass


def is_valid_po_path(path):
    ''' 

    '''

    pass


def list_po_dir(root_path, settings):
    ''' 

    '''

    pass


def locale_explode(locale):
    ''' 

    '''

    pass


def locale_match(loc1, loc2):
    ''' 

    '''

    pass
