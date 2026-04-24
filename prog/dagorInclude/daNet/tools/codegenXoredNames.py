import sys
import re

output_header="""#pragma once
// This file was automatically generated, so do not put anything here manually, instead
// put it into .gen files which your .bat files will create into such .h files.

"""

output_decode_const="""
static const int {decode_xor_name} = {xor_bit};

"""

output_format="""static const char {var_name}[] =
"{coded_name}";
"""

def parseNameFile():
  if len(sys.argv) != 5:
    print("Usage codegenXoredNames.py inputFile outputFile decodeConstName xorBit")
  else:
    lines = open(sys.argv[1]).readlines()
    outputFile = open(sys.argv[2], "w")
    outputFile.write(output_header)
    outputFile.write(output_decode_const.format(decode_xor_name = sys.argv[3], xor_bit = sys.argv[4]))
    for line in lines:
      m = re.match(r"^(.*)\[(.*)\]", line)
      if len(m.groups()) <= 0:
        continue
      varName = m.groups()[0]
      postfix = m.groups()[1]
      codedName = ''.join([r'\x%02x' % (ord(c) ^ int(sys.argv[4])) for c in varName])
      if postfix != None:
        varName = varName + postfix
      outputFile.write(output_format.format(var_name = varName, coded_name = codedName))

if __name__ == '__main__':
  parseNameFile()
