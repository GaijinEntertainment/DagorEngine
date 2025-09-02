from pyparsing import *
import string
import sys
import re

default_base_class = "mpi::Message"
default_transmission = "mpi::RELIABLE_ORDERED_TRANSMISSION"
default_listeners = "MPI_LISTENER_ALL"
output_file_template = \
"""{hdr_comment}
{header}
#include <memory/dag_framemem.h>
#include <ioSys/dag_dataBlock.h>

//-V::568

{decl_section}

{msgs_section}

{footer}"""

debug_file_template = \
"""#pragma once

class DebugMpiMessage
{{
public:
#if DEBUG_INTERFACE_ENABLED
  static DataBlock genBlkFromMessage(danet::BitStream& message)
  {{
    mpi::ObjectID oid;
    mpi::MessageID mid;
    message.Read(oid);
    message.Read(mid);
    mid = mid & 0x0fff;
    DataBlock blk;
    blk.setBlockName(mpiNamesTable[mid]);
    switch (mid)
    {{
      ///MESSAGES
      {messages_section}
      ///MESSAGES
    }};
  }}
#endif
  static DataBlock genBlockFromMessage() {{ return DataBlock(); }}
}};
"""

debug_message_template = \
"""
      // {msg_name}
      ///MSG
      case {msg_id}:
        {{
{msg_body}
        }}
        break;
      ///MSG
"""

mpi_message_template = \
"""class {msg_name} : public {base_class}
{{{transmission_type_declaration}{set_fields}
public:
  typedef {base_class} BaseMsgClass;

{params_decl}

  {msg_name}(mpi::IObject* o, mpi::MessageID mid, IMemAlloc* a) :
{params_init}
    {base_class}(o, mid, a)
  {{
  }}
#if DEBUG_INTERFACE_ENABLED
  const char* getDebugMpiName() const {{ return "{msg_name}"; }}
  static DataBlock getDebugMpiBlk(danet::BitStream& bs)
  {{
    G_UNREFERENCED(bs);

    DataBlock blk;
{debug_code_body}
    return blk;
  }}
#else
  const char* getDebugMpiName() const {{ return ""; }}
  static DataBlock getDebugMpiBlk(danet::BitStream &/*bs*/) {{ return DataBlock(); }}
#endif

  static void sendto
    (
      mpi::SystemID receiver,
      mpi::IObject* o{send_params}
    )
  {{
    {msg_name} m(o, {msg_id}, framemem_ptr());
{send_body}
    m.writePayload();
    mpi::sendto(&m, receiver);
  }}

  static void send
    (
      mpi::IObject* o{send_params}
    )
  {{
    sendto(mpi::INVALID_SYSTEM_ID, o{call_args});
  }}

  void writePayload()
  {{
{write_payload_body}
  }}

  bool readPayload()
  {{
{read_payload_body}
  }}

{custom_block}
}};
"""

standard_var_format="""
    {real_type} {name};
    memset(&{name}, 0, sizeof({name}));
    mpi::read_type_proxy(bs, {name});
    blk.add{type}("{name}", {name});
"""

unknown_var_format ="""
    {real_type} {name};
    memset(&{name}, 0, sizeof({name}));
    mpi::read_type_proxy(bs, {name});
    blk.addStr("{name}", "Unknown type '{real_type}'");
"""

datablock_format="""
    DataBlock {name};
    mpi::read_type_proxy(bs, {name});
    blk.addNewBlock(&{name}, "{name}");
"""

custom_function_var_format="""
    {real_type} {name};
    mpi::read_type_proxy(bs, {name});
    blk.add{type}("{name}", {custom_name});
"""

bitstream_format = """
    String bs_{name}_str;
    danet::BitStream {name};
    mpi::read_type_proxy(bs, {name});
    {{
      uint8_t bits;
      while ({name}.Read(bits))
        bs_{name}_str.aprintf(512, "%02x", bits);
    }}
    blk.addStr("{name}", bs_{name}_str.str());
"""

default_values = {
  'int'      : '0',
  'float'    : '0.f',
  'double'   : '0.0',
  'bool'     : 'false',
  'short'    : '0',
  'int8_t'   : '0',
  'int16_t'  : '0',
  'int32_t'  : '0',  
  'uint8_t'  : '0u',
  'uint16_t' : '0u',
  'uint32_t' : '0u',  
  'Point2'   : '0.f, 0.f',  
  'Point3'   : '0.f, 0.f, 0.f',  
  'Point4'   : '0.f, 0.f, 0.f, 0.f',  
  'danet::BitStream' : 'a', # alloc
}

class MpiMessage:
  def __init__(self, name):
    self.params = [] # [{typ, name, isReference, aliasType, isPointer}]
    self.params_all = [] # [{typ, name, isReference, aliasType, isPointer}]
    self.inherits = default_base_class
    self.transType = default_transmission
    self.sendTo = default_listeners
    self.customBlock = ""
    self.name = name
    self.idName = re.sub(r'([a-z])([A-Z])', r'\1_\2', name).upper()
    self.tmpSeq = -1

  def generateId(self):
    return "#define {id_name} ({send_to} | {id_})".format(id_name=self.idName, send_to=self.sendTo, id_=str(self.myId))
  
  def generateDebugCode(self):
    output = ''
    for p in self.params:
      if 'Point3' in p['typ']:
        output += standard_var_format.format(type="Point3", name = p['name'], real_type = p['typ'])
      elif 'Point2' in p['typ']:
        output += standard_var_format.format(type="Point2", name = p['name'], real_type = p['typ'])
      elif 'Point4' in p['typ']:
        output += standard_var_format.format(type="Point4", name = p['name'], real_type = p['typ'])
      elif 'TMatrix' in p['typ']:
        output += standard_var_format.format(type="Tm", name = p['name'], real_type = p['typ'])
      elif 'ObjectID' in p['typ']:
        output += standard_var_format.format(type="Int", name = p['name'], real_type = p['typ'])
      elif 'DataBlock' in p['typ']:
        output += datablock_format.format(name = p['name'])
      elif 'void' in p['typ']:
        continue
      elif 'Unit' in p['typ']:
        output += standard_var_format.format(type="Int", name = p['name'], real_type = "mpi::ObjectID")
      elif 'float' in p['typ']:
        output += standard_var_format.format(type="Real", name = p['name'], real_type = p['typ'])
      elif 'double' in p['typ']:
        output += standard_var_format.format(type="Real", name = p['name'], real_type = p['typ'])
      elif 'short' in p['typ']:
        output += standard_var_format.format(type="Int", name = p['name'], real_type = p['typ'])
      elif 'int64_t' in p['typ']:
        output += standard_var_format.format(type="Int64", name = p['name'], real_type = p['typ'])
      elif 'int' in p['typ']:
        output += standard_var_format.format(type="Int", name = p['name'], real_type = p['typ'])
      elif 'const char*' in p['typ']:
        output += standard_var_format.format(type="Str", name = p['name'], real_type = p['typ'])
      elif 'String' in p['typ']:
        output += custom_function_var_format.format(type="Str", name = p['name'], real_type = p['typ'], custom_name = p['name'] + ".str()")
      elif 'bool' in p['typ']:
        output += standard_var_format.format(type="Bool", name = p['name'], real_type = p['typ'])
      elif 'char' in p['typ']:
        output += standard_var_format.format(type="Int", name = p['name'], real_type = p['typ'])
      elif 'PackedQuat' in p['typ']:
        output += custom_function_var_format.format(type="Str", name = p['name'], real_type = p['typ'], custom_name = '\n      String(128,\n        "x = %f, y = %f, z = %f, w = %f",\n        {name}.unpack(1.f).x, {name}.unpack(1.f).y,\n        {name}.unpack(1.f).z, {name}.unpack(1.f).w).str()'.format(name = p['name']))
      elif 'Quat' in p['typ']:
        output += custom_function_var_format.format(type="Str", name = p['name'], real_type = p['typ'], custom_name = '\n      String(128,\n        "x = %f, y = %f, z = %f, w = %f",\n        {name}.x, {name}.y, {name}.z, {name}.w).str()'.format(name = p['name']))
      elif 'SystemAddress' in p['typ']:
        output += custom_function_var_format.format(type="Str", name = p['name'], real_type = p['typ'], custom_name = '{name}.ToString(true)'.format(name = p['name']))
      elif 'danet::BitStream' in p['typ']:
        output += bitstream_format.format(name = p['name']);
      elif 'OffenderData' in p['typ']:
        output += custom_function_var_format.format(type="Int", name = p['name'], real_type = "OffenderData", custom_name = p['name'] + ".getUID()")
      elif 'matching::' in p['typ']:
        output += standard_var_format.format(type="Int64", name = p['name'], real_type = p['typ'])
      else:
        output += unknown_var_format.format(name = p['name'], real_type = p['typ'])
    return output

  def generateCode(self):
    longest_type = reduce(max, [len(p['typ']) for p in ([dict(name='',typ='')], self.params_all)[bool(self.params)]])
    longest_name = reduce(max, [len(p['name']) for p in ([dict(name='',typ='')], self.params_all)[bool(self.params)]])

    def get_send_param_type(p):
      if p['typ'] == p['aliasType']:
        if p['isReference'] or p['isPointer']:
          return 'const %s%s' % (p['typ'], '&' if p['isReference'] else '*')
        elif p['isMove']:
          return '%s&&' % (p['typ'])
        else:
          return p['typ']
      else:
        return p['aliasType']

    def get_send_body_param_name(p):
      if p['isMove']:
        return 'eastl::move(%s__)' % (p['name'])
      return '%s__' % (p['name'])

    def get_default_init_value(p):
      if 'defValue' in p:
        return p['defValue']
      else:
        return default_values.get(p['typ'], '')
      
    def get_default_arg_value(p):
      if 'defValue' in p:
        return (' = ' + p['defValue'])
      else:
        return ''

    def compose_flag(flag, p):
      bit = p['seq']
      val = 0 if bit < 0 else 1 << bit
      if (flag) :
        assert (flag < val), "fields numbering order is not ascending"
      return flag | val

    reserve_payload_classic = '    payload.reserveBits(BYTES_TO_BITS(\n%s 0));\n' %\
      '\n'.join([('      sizeof(%s) +' % p['name']) for p in self.params]) if self.params else ''
    write_payload_classic=(reserve_payload_classic+'\n'.join(['    mpi::write_type_proxy(payload, {name});'.\
      format(name=p['name']) for p in self.params]))
    read_payload_classic='    return\n' + ("      true;",'\n'.join(['        mpi::read_type_proxy(payload, {name}) &&'.\
      format(name=p['name']) for p in self.params])[:-3]+';')[bool(self.params)]

    reserve_payload_fields = ('    payload.reserveBits(BYTES_TO_BITS(\n      sizeof(__fields) +\n%s sizeof(uint16_t)*%d));\n' %\
      ('\n'.join([('      sizeof(%s) +' % p['name']) for p in self.params]) if self.params else '', len(self.params) + 1)) +\
      '    payload.Write((uint16_t)0);\n    payload.WriteCompressed(__fields);\n'
    write_payload_fields=(reserve_payload_fields+'\n'.join(['    mpi::write_type_proxy(this, payload, {name});'.\
      format(name=p['name']) for p in self.params])+'\n    writeFieldsSize();')
    read_payload_header='    uint32_t _fields = readFieldsSizeAndFlag();\n    int k = 0;\n' +\
      '    while (_fields)\n    {\n      uint32_t bit = __bsf(_fields);\n      _fields &= ~(1 << bit);\n      switch (bit)\n      {\n'
    skip_reading='\n      default:\n        skipReadingField(k);\n      }\n      ++k;\n    }\n    return true;'
    read_payload_fields=(read_payload_header+'\n'.join(['      case {num}:\n        mpi::read_type_proxy(this, k, payload, {name});\n        break;'.\
      format(num=str(p['seq']), name=p['name']) for p in self.params]) + skip_reading)

    ff = 0
    for p in self.params :
      ff = compose_flag(ff, p)

    return mpi_message_template.format(
        msg_name=self.name,
        msg_id=self.idName,
        base_class=self.inherits,
        transmission_type_declaration=format('\n  SET_TRANSMISSION_TYPE(%s)' % self.transType) if self.transType!=default_transmission else '',
        params_decl='\n'.join(['  {typ}{align}{name};'.\
          format(typ=((p['typ'], 'const %s*' % p['typ'])[p['isPointer']]), align=' '*(longest_type-len(p['typ'])+1), name=p['name']) for p in self.params_all]),
        params_init='\n'.join(['    {name}({default}),'.\
          format(name=p['name'], default=get_default_init_value(p)) for p in self.params_all]),
        set_fields='' if ff == 0 else '\n  static const uint32_t __fields = ' + str(ff) + ';',
        send_params=''.join([',\n      {typ} {name}__{val}'.\
          format(typ=get_send_param_type(p), name=p['name'], val=get_default_arg_value(p)) for p in self.params_all]),
        call_args=''.join([',\n           {name}'.format(name=get_send_body_param_name(p)) for p in self.params_all]),
        send_body='\n'.join(['    m.{name}{align} = {nameCpy};'.\
          format(name=p['name'], nameCpy=get_send_body_param_name(p), align=' '*(longest_name-len(p['name'])+1)) for p in self.params_all]),
        write_payload_body=write_payload_classic if ff == 0 else write_payload_fields,
        read_payload_body=read_payload_classic if ff == 0 else read_payload_fields,
        custom_block=self.customBlock,
        debug_code_body=self.generateDebugCode()
      )

class IdFile:
  def __init__(self, filename):
    self.messages = {}
    self.maxId = 0
    self.parseFile(filename)

  def parseFile(self, filename):
    def parseRecord(str_, loc, toks):
      id_ = int(toks[1])
      self.messages[toks[0]] = id_
      self.maxId = max(id_, self.maxId)

    name = Word(alphanums + '_')
    record = (name + Keyword("=").suppress() + Word(nums)).setParseAction(parseRecord)

    ZeroOrMore(record).parseString(open(filename).read())

  def getId(self, name):
    val = self.messages.get(name)
    if val == None:
      self.maxId += 1
      self.messages[name] = self.maxId
      return self.maxId
    return val

  def genFile(self, filename):
    records = sorted(self.messages.items(), key=lambda item: item[1])
    open(filename, "w").write('\n'.join(['%s = %s' % (r[0], r[1]) for r in records]))

class MpiFile:
  def __init__(self, templateFilename, outputFilename, idFilename):
    self.mpiMessages = []
    self.mpiHeader = self.mpiFooter = ""
    self.activeMessage = None
    self.idFilename = idFilename
    self.idFile = IdFile(self.idFilename)
    self.templateFilename = templateFilename
    self.parseFile(self.templateFilename)
    self.outputFilename = outputFilename

  def generateFile(self):
    open(self.outputFilename, "w").write(self.generateOutputFile(self.templateFilename))
    self.idFile.genFile(self.idFilename)
    print ("%s -> %s" % (self.templateFilename, self.outputFilename))

  def parseFile(self, filename):
    def beginMessage(str_, loc, toks):
      newMessage = MpiMessage(toks[0])
      self.activeMessage = newMessage
      self.mpiMessages.append(newMessage)
      self.activeMessage.myId = self.idFile.getId(toks[0])
      self.activeMessage.inherits = toks[1]
      return toks

    def endMessage(str_, loc, toks):
      self.activeMessage = None
      return toks

    def addCustomBlock(str_, loc, toks):
      getActiveMsg().customBlock = toks[0].strip(' ').replace('\r\n','\n')
      return toks

    def sendToProc(str_, loc, toks):
      getActiveMsg().sendTo = toks[0]
      return toks

    def transTypeProc(str_, loc, toks):
      getActiveMsg().transType = toks[0]
      return toks

    def parseLabel(str_, loc, toks):
      getActiveMsg().tmpSeq = -1 if not len(toks) else int(toks[0])
      return toks;

    def parseParam(str_, loc, toks):
      p = {}
      p['typ'] = toks[0].rstrip('& ').rstrip('* ')
      ex_param = toks[1].endswith('#')
      p['name'] = toks[1].rstrip('#')
      p['isPointer'] = toks[0].endswith('*')
      p['isMove'] = False

      opt = 2 # all other tokens are optional

      # optional defaul value
      if len(toks) > opt:
        if toks[opt] is '=':
          p['defValue'] = str(toks[opt + 1])
          opt = opt + 2

      # optional alias type
      if len(toks) > opt:
        p['aliasType'] = ' '.join(toks[opt])
        p['isReference'] = False
      else: # alias type is not exists?
        p['aliasType'] = p['typ']
        p['isMove'] = toks[0].endswith('&&')
        p['isReference'] = toks[0].endswith('&') if not p['isMove'] else False

      p['seq'] = getActiveMsg().tmpSeq
      getActiveMsg().params_all.append(p)
      if not ex_param:
        getActiveMsg().params.append(p)
      return toks

    def parseHeaderSection(str_, loc, toks):
      self.mpiHeader = toks[0].replace('\r\n','\n')
      return toks

    def parseFooterSection(str_, loc, toks):
      self.mpiFooter = toks[0].replace('\r\n','\n')
      return toks

    def getActiveMsg():
      return (MpiMessage("dummy"), self.activeMessage)[bool(self.activeMessage)]

    name = QuotedString('"') | Word(alphanums + '_#') 

    doubleCol = Literal("::")
    className = Combine(name + ZeroOrMore(doubleCol + name))
    label = Word(nums) + ':'

    semi = Optional(Literal(";"))
    transTypeName = (Keyword("transType").suppress() + Literal("=").suppress() + name).setParseAction(transTypeProc) + semi
    sendToName = (Keyword("sendTo").suppress() + Literal("=").suppress() + name).setParseAction(sendToProc) + semi
    typename = Word(alphanums + '<>.:_*&')
    aliasType = Group(OneOrMore(typename))
    paramDefaultValue = Word(alphanums + '.')
    param = Optional(label).setParseAction(parseLabel) + (typename + name + Optional(Literal("=") + paramDefaultValue) + Optional(Keyword(':').suppress() + aliasType)).setParseAction(parseParam) + semi
    customBlock = QuotedString('"""', multiline=True).setParseAction(addCustomBlock)
    messageContent = Optional(transTypeName) + Optional(sendToName) + ZeroOrMore(param) + Optional(customBlock)
    inheritedName = Keyword(":").suppress() + className
    lpar = Literal("{").suppress()
    rpar = Literal("}").suppress()
    message = Group((name + inheritedName + lpar).setParseAction(beginMessage) + messageContent + rpar.setParseAction(endMessage))
    messages = ZeroOrMore(message)

    headerSection = (StringStart() + SkipTo(Literal('%%%').suppress(), include=True)).setParseAction(parseHeaderSection)
    footerSection = (Literal('%%%').suppress() + SkipTo(StringEnd())).setParseAction(parseFooterSection)

    grammar = headerSection + messages + footerSection
    grammar.parseFile(filename)
    
  def generateOutputFile(self, templateFilename):
    header_comment = "//\n// This file is autogenerated from '%s' template. Please, do not edit it manually\n//" % templateFilename
    return output_file_template.format(
        hdr_comment=header_comment,
        header=self.mpiHeader,
        decl_section='\n'.join([m.generateId() for m in self.mpiMessages]),
        msgs_section='\n'.join([m.generateCode() for m in self.mpiMessages]),
        footer=self.mpiFooter
      )

def parseFile():
  if len(sys.argv) != 4:
    print("Wrong number of arguments presented.")
    print("Usage: codegenMpi.py inputFile outputFile idFile")
  else:
    mpiFile = MpiFile(sys.argv[1], sys.argv[2], sys.argv[3])
    mpiFile.generateFile()

if __name__ == '__main__':
  parseFile()

