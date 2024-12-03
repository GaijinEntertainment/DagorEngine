import sys, os

sys.path.insert(0, os.path.abspath(os.path.relpath("../../prog/utils", os.path.dirname(__file__))))
from common_validate_scripts import test_csq_analyzer

if __name__ == "__main__":
  commands = [
    {"script":"scripts/outer_space/start.nut"},
    {"script":"scripts/ui/main.nut"},
    {"script":"utils/dev_launcher/scripts/script.nut"},
    {"script":"utils/dev_launcher/scripts/init_launcher.nut"},
  ]
  test_csq_analyzer(commands, fail_unvisited = True)
