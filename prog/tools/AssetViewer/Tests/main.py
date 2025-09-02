import os
from ctypes import *
from imgui_bundle import imgui
import dag_imgui_te
import importlib
import tests


def register_tests_callback():
    engine = dag_imgui_te.get_test_engine()
    tests.register_cases(engine)


def get_abs_path_for_file(filename: str) -> str:
    if os.path.isfile(filename):
        return os.path.abspath(filename)
    return filename


if __name__ == "__main__":
    dll_path = os.path.join(os.path.dirname(__file__), 'assetViewer2-dev.dll')
    config_path = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'application.blk')

    dll = WinDLL(dll_path)

    dag_imgui_te.register_library(dll, register_tests_callback)

    string_buffers = [
        create_string_buffer(dll_path.encode()),
        create_string_buffer(config_path.encode())
    ]
    pointers = (c_char_p*2)(*map(addressof, string_buffers))
    dll.dagor_lib_main(2, pointers)
