from __future__ import absolute_import
import os, string, codecs
import traceback
import sys

if sys.version_info[0] >= 3:
  from .pyparsing import *
else:
  from pyparsing import *

def getTokensEndLoc():
    """Method to be called from within a parse action to determine the end
       location of the parsed tokens."""
    import inspect
    fstack = inspect.stack()
    try:
        # search up the stack (through intervening argument normalizers) for correct calling routine
        for f in fstack[2:]:
            if f[3] == "_parseNoCache":
                endloc = f[0].f_locals["loc"]
                return endloc
        else:
            raise ParseFatalException("incorrect usage of getTokensEndLoc - may only be called from within a parse action")
    finally:
        del fstack

class DataBlock:
    def __init__(self, fname = None, name = None, include_includes = False, preserve_formating = False, root_directory = None):
      self.blocks = []
      self.params   = []
      self.includes = []
      self.comments = []
      self.emptyLines = []
      self.loadedFrom = fname
      if name == None:
        self.name = "root"
      else:
        self.name = name
      self.root_directory = root_directory if root_directory else "."
      self.parent = None
      if fname != None:
        self.parseFile(fname, include_includes = include_includes, preserve_formating = preserve_formating)

    def sortInAlphabeticalOrder(self):
      self.blocks = sorted(self.blocks, key = lambda block: (block[1].name, block[1].getParam("name")))
      self.params = sorted(self.params, key = lambda param: param[1][0])
      newParams = []
      i = 1
      for param in self.params:
        newParams.append((self.lastLine() + i, param[1]))
      self.params = newParams
      newBlocks = []
      i = 1
      for block in self.blocks:
        block[1].sortInAlphabeticalOrder()
        newBlocks.append((self.lastLine() + i, block[1]))
        i = i + 1
      self.blocks = newBlocks

    def addBlock(self, blk, line = -1):
      if line == -1:
        line = self.lastLine() + 1
      self.blocks.append((line, blk))
      return blk

    def createBlockByPathWithoutIncludes(self, path, line = -1):
      split_index = path.rfind("/")
      if split_index != -1:
        block_path = path[:split_index]
        block = self.getBlockByPathWithoutIncludes(block_path)
        block.createBlock(path[split_index+1:], line)
      else:
        self.createBlock(path, line)

    def createBlock(self, name, line = -1):
      if line == -1:
        line = self.lastLine() + 1
      blk = DataBlock()
      blk.name = name
      blk.parent = self
      blk.root_directory = self.root_directory
      return self.addBlock(blk, line)

    def addInclude(self, include, line = -1, include_includes = False):
      if line == -1:
        line = self.lastLine() + 1
      blk = DataBlock()
      blk.root_directory = self.root_directory
      if include[0] == "#":
        include = os.path.normpath(self.root_directory+os.sep+include[1:])

      blk.parseFile(include, False, include_includes)
      if (include_includes):
        for block in blk.getBlocks():
          self.addBlock(block)
        for param in blk.getParams():
          self.addParam(param[0], param[1], param[2])
      else:
        self.includes.append((line, (include, blk)))

    def getBlocks(self, name = None):
      blocks = self.getBlocksWithoutIncludes(name)
      for (i, (n, blk)) in self.includes:
        blocks.extend(blk.getBlocks(name))
      return blocks

    def getBlocksWithoutIncludes(self, name = None):
      def f(l_blk):
        (l, blk) = l_blk
        return name == None or name == blk.name
      return [__blk[1] for __blk in list(filter(f, self.blocks))]

    def removeParamByPathWithoutIncludes(self, path):
      split_index = path.rfind("/")
      if split_index != -1:
        block_path = path[:split_index]
        block = self.getBlockByPathWithoutIncludes(block_path)
        block.removeParam(path[split_index+1:])
      else:
        self.removeParam(path)

    def removeBlockByPathWithoutIncludes(self, path):
      split_index = path.rfind("/")
      if split_index != -1:
        block_path = path[:split_index]
        block = self.getBlockByPathWithoutIncludes(block_path)
        block.removeBlock(path[split_index+1:])
      else:
        self.removeBlock(path)

    def removeBlock(self, name):
      self.blocks = [__blk1 for __blk1 in self.blocks if __blk1[1].name != name]

    def removeParam(self, name):
      self.params = [l_n_t_v for l_n_t_v in self.params if l_n_t_v[1][0] != name] # pylint: disable=E0602

    def getBlockWithoutIncludes(self, name):
      blocks = self.getBlocksWithoutIncludes(name)
      if blocks:
        return blocks[0]
      return None

    def getBlock(self, name):
      blocks = self.getBlocks(name)
      if blocks:
        return blocks[0]
      return None

    getBlockByName = getBlock

    def getBlockByNameEx(self, name):
      ret = self.getBlockByName(name)
      return ret if ret else DataBlock()

    def getParam(self, name, default = None):
      pars = self.getParams(name)
      if pars:
        return pars[0][2]
      return default

    def getBlockByPathWithoutIncludes(self, path):
      a = filter(None, path.split("/"))
      b = self
      for i in a:
        b = b.getBlockWithoutIncludes(i)
        if b is None:
          return None
      return b

    def getParamsByPathWithoutIncludes(self, path):
      a = path.split("/")
      b = self
      for i in a[:-1]:
        b = b.getBlockWithoutIncludes(i)
        if b is None:
          return []
      return b.getParamsWithoutIncludes(a[-1])

    def getParamByPath(self, path):
      a = path.split("/")
      b = self
      for i in a[:-1]:
        b = b.getBlock(i)
        if b is None:
          return None
      return b.getParam(a[-1])

    def getParamsWithoutIncludes(self, name = None):
      def f(l_n_t_v):
        (l, (n, t, v)) = l_n_t_v
        return name == None or n == name
      pars = [__param[1] for __param in list(filter(f, self.params))]
      return pars

    def getParams(self, name = None):
      pars = self.getParamsWithoutIncludes(name)
      for (i, (n, blk)) in self.includes:
        pars.extend(blk.getParams(name))
      return pars

    def setParamByPathWithoutIncludes(self, path, typ, value, line=-1):
      split_index = path.rfind("/")
      if split_index != -1:
        block_path = path[:split_index]
        block = self.getBlockByPathWithoutIncludes(block_path)
        block.setParam(path[split_index+1:], typ, value, line)
      else:
        self.setParam(path, typ, value, line)

    def setParam(self, name, typ, value, line=-1):
      for i in range(len(self.params)):
        if (self.params[i][1][0] == name):
          self.params[i] = (self.params[i][0], (name, typ, value))
          return
      self.addParam(name, typ, value, line)

    def addParam(self, name, typ, value, line=-1):
      if typ == 'i':
        value = int(value)
      if line == -1:
        line = self.lastLine() + 1
      param = (line, (name, typ, value))
      self.params.append(param)

    def remParam(self, name):
      for i in range(len(self.params)):
        if (self.params[i][1][0] == name):
          self.params.pop(i)
          return

    def saveIncludes(self):
      for l, (inc, blk) in self.includes:
        blk.saveFile(inc)
      for l, blk in self.blocks:
        blk.saveIncludes()

    def saveFile(self, filename, save_includes = True):
      fileDir = os.path.dirname(os.path.normpath(filename))
      if len(fileDir) != 0:
        if not os.path.isdir(fileDir):
          os.makedirs(fileDir)
      else:
        fileDir = "./"

      open(filename, "w").write(self.makeStr())
      if save_includes:
        curDir = os.getcwd()
        os.chdir(fileDir)
        self.saveIncludes()
        os.chdir(curDir)

    def makeStr(self):
      def paramToStr(name, typ, value):
        valStr = ""
        valType = ""
        if isinstance(value, basestring if sys.version_info[0] < 3 else str):
          valStr = "\"" + value + "\""
        elif isinstance(value, tuple):
          if len(value) > 0 and isinstance(value[0], tuple):
            valStr = "[" + " ".join(map(lambda col: "[" + ", ".join(map(lambda x: "%g" % x, col)) + "]", value)) + "]"
          else:
            valStr = ", ".join(map(lambda x: "%g" % x, value))
        elif isinstance(value, bool):
          valStr = ['no', 'yes'][bool(value)]
        elif isinstance(value, float):
          valStr = "%d" % int(value) if typ in ['i','i64'] else "%g" % value
        elif isinstance(value, int):
          valStr = "%d" % value

        if typ != "":
          valType = ":" + typ

        return "%s%s=%s" % (name, valType, valStr)

      lines = {}
      for l in self.emptyLines:
        lines[l] = "\n"

      for l, v in self.params:
        if not l in lines:
         lines[l] = paramToStr(*v)
        else:
          lines[l] += "; " + paramToStr(*v)

      comments = self.comments
      for l, b in self.blocks:
        bstr = b.makeStr().split('\n')
        bstr = list(map(lambda x: "  " + x, bstr))
        comment = ""
        for c in comments:
          if c[0] == l:
            comment = " " + c[1]
            comments.remove(c)
        lines[l] = "%s{%s\n%s\n}" % (b.name, comment, "\n".join(bstr))

      for l, (n, b) in self.includes:
        lines[l] = "include " + n

      for l, c in comments:
        if not l in lines:
          lines[l] = c
        else:
          lines[l] += " " + c

      result = ""
      sorted_lines = sorted(list(lines.keys()))
      for l in sorted_lines[:-1]:
        if lines[l] != "\n":
          result += lines[l]
        result += "\n"
      if len(sorted_lines) > 0:
        result += lines[sorted_lines[-1]]
      return result

    def __repr__(self):
      return self.makeStr()

    def lastLine(self):
      lastline = 0
      if len(self.params) > 0:
        lastline = max(lastline, self.params[-1][0])
      if len(self.includes) > 0:
        lastline = max(lastline, self.includes[-1][0])
      if len(self.comments) > 0:
        lastline = max(lastline, self.comments[-1][0])
      if len(self.emptyLines) > 0:
        lastline = max(lastline, self.emptyLines[-1])
      if len(self.blocks) > 0:
        lastline = max(lastline, self.blocks[-1][0])
      return  lastline

    class Parser:
      def __init__(self, blk, preserve_formating = False, include_includes = False):
        self.activeBlk = blk
        self.lastline = 0
        self.preserveFormating = preserve_formating
        self.includeIncludes = include_includes

        def checkEmpty(sline, eline):
          if sline - self.lastline > 1:
            self.activeBlk.emptyLines.extend(range(self.lastline + 1, sline)) #pylint: disable=W1638
          self.lastline = eline

        def addVar(str_, loc, toks):
          l = lineno(loc, str_)
          if self.preserveFormating:
            checkEmpty(l,l)
          varName = toks[0]
          if varName.startswith(("'", '"')) and (varName[0] == varName[-1]):
            varName = varName[1:-1]
          varType = ""
          varVal = None
          if len(toks) == 2:
            varVal = toks[1]
          elif len(toks) > 2:
            varType = toks[1]
            varVal  = toks[2]
            if varType == 'i':
              varVal = round(varVal)
          self.activeBlk.addParam(varName, varType, varVal, line = l)
          return toks


        def addComment(str_, loc, toks):
          self.activeBlk.comments.append((lineno(loc, str_), toks[0]))
          if self.preserveFormating:
            checkEmpty(lineno(loc, str_), lineno(getTokensEndLoc(), str_))
          return toks

        def addInclude(str_, loc, toks):
          self.activeBlk.addInclude(toks[0], line = lineno(loc, str_), include_includes = self.includeIncludes)
          if self.preserveFormating:
            checkEmpty(lineno(loc, str_), lineno(getTokensEndLoc(), str_))
          return toks

        def beginBlock(str_, loc, toks):
          if self.preserveFormating:
            checkEmpty(lineno(loc, str_), lineno(getTokensEndLoc(), str_))
          newBlk = self.activeBlk.createBlock(toks[0], line = lineno(loc, str_))
          self.activeBlk = newBlk
          return toks

        def endBlock(str_, loc, toks):
          self.activeBlk = self.activeBlk.parent
          if self.preserveFormating:
            l = lineno(loc, str_)
            checkEmpty(l, l)
          return toks

        def parseArray(str_, loc, toks):
          if len(toks) > 1:
            return [tuple(toks)]
          else:
            return [toks[0]]

        name      = dblQuotedString | Word(alphanums + '_' + '-' + '.')
        varType   = Literal('i64') | 'r' | 'b' | 't' | 'p4' | 'p3' | 'p2' | 'ip3' | 'ip2' | 'm' | 'c' | 'i'
        typedVar  = name + Optional(Suppress(':') + varType)
        realNum   = Regex(r"[+-]?\d+(:?\.\d*)?(:?[eE][+-]?\d+)?").setParseAction(lambda s,l,t: [ float(t[0]) ])
        realNum.setName("real")
        vectorVal = delimitedList(realNum)
        vectorVal.setParseAction(parseArray)
        vectorVal.setName("vector")
        matrixColumn = Suppress('[') + vectorVal + Suppress(']')
        matrixVal = Suppress('[') + ZeroOrMore(matrixColumn) + Suppress(']')
        matrixVal.setParseAction(parseArray)
        matrixVal.setName("matrix")

        trueVal   = (CaselessKeyword('yes') | \
                     CaselessKeyword('true') | \
                     CaselessKeyword('on')).setParseAction(lambda s, l, t: [True])

        falseVal  = (CaselessKeyword('no') | \
                     CaselessKeyword('false') |
                     CaselessKeyword('off')).setParseAction(lambda s, l, t: [False])

        boolVal   = trueVal | falseVal
        unquotedString = Word(printables)
        stringVal = dblQuotedString.copy().setParseAction(removeQuotes) | unquotedString
        value     = (matrixVal | vectorVal | boolVal | stringVal)
        assigment = (typedVar + Suppress('=') + value + Optional(';').suppress()).setParseAction(addVar)
        assigment.setName("assigment")
        #assigment.setDebug()
        comment   = (cppStyleComment).setParseAction(addComment)
        filename  = stringVal.copy()
        include   = (Keyword("include").suppress() + filename).setParseAction(addInclude)

        blockContent = Forward()
        lpar = Literal("{").suppress()
        rpar = Literal("}").suppress()
        block = Group((name + Optional(comment) + lpar).setParseAction(beginBlock) + blockContent + rpar.setParseAction(endBlock)) + Optional(';')
        block.setName("block")
        blockContent << ZeroOrMore(assigment | comment | include | block)
        #blockContent.setDebug()
        self.parser = blockContent

    def parseFile(self, fname, preserve_formating = False, include_includes = False):
      self.loadedFrom = fname
      curDir = os.getcwd()
      newDir = os.path.dirname(os.path.normpath(fname))

      with open(fname) if sys.version_info[0] == 2 else open(fname, encoding='utf8') as f:
        content = f.read()
      content = content.lstrip(codecs.BOM_UTF8 if sys.version_info[0] == 2 else codecs.BOM_UTF8.decode())
      if os.path.isdir(newDir):
        os.chdir(newDir)
      try:
        self.parseString(content, preserve_formating, include_includes)
      except ParseException as e:
        traceback.print_exc()
        print("failed to parse file '%s': %s" % (fname, e))
        raise e
      finally:
        os.chdir(curDir)

    def parseString(self, content, preserve_formating = False, include_includes = False):
      parser = DataBlock.Parser(self, preserve_formating, include_includes)
      parser.parser.parseString(content, parseAll = True)

