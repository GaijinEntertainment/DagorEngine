import logging

_log = logging.getLogger('qdox')
logging.basicConfig(format='[%(levelname)s] %(message)s', level=logging.DEBUG)

class Counter:
  def __init__(self):
    self.errors = 0
    self.warnings = 0

erorrscounter = Counter()

def logerror(error):
  erorrscounter.errors +=1
  _log.error(error)

def logwarning(error):
  erorrscounter.warnings +=1
  _log.warning(error)

