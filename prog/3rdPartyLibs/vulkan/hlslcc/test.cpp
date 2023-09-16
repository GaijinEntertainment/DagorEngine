#include "src/internal_includes/ControlFlowGraph.h"
#include "include/hlslcc.h"

#include <intrin.h>

using namespace std;

#define ADD_INSTRUCTION(op) { bin.push_back(Instruction{}); auto& inst = bin.back(); inst.eOpcode = op; }
#define VERIFY(test) if(!(test)) { __debugbreak(); throw runtime_error{#test" failed"}; }

void checkSimpleBlock()
{
  vector<Instruction> bin;
  ADD_INSTRUCTION(OPCODE_ADD);
  ADD_INSTRUCTION(OPCODE_RET);

  ControlFlow2::Graph graph{ bin };
  VERIFY(distance(graph.begin(), graph.end()) == 1);
  VERIFY(graph.front()->isExit());
  VERIFY(graph.front()->isRoot());
  VERIFY(graph.front()->front().eOpcode == OPCODE_ADD);
  VERIFY(graph.front()->back().eOpcode == OPCODE_RET);
}

bool is_pre_of(ControlFlow2::Block* check, ControlFlow2::Block* pre)
{
  return pre->getPreList().end() != find(pre->getPreList().begin(), pre->getPreList().end(), check);
}

void checkIfBranch()
{
  vector<Instruction> bin;
  ADD_INSTRUCTION(OPCODE_ADD);
  ADD_INSTRUCTION(OPCODE_IF);
  ADD_INSTRUCTION(OPCODE_ADD);
  ADD_INSTRUCTION(OPCODE_ENDIF);
  ADD_INSTRUCTION(OPCODE_RET);

  ControlFlow2::Graph graph{ bin };
  VERIFY(distance(graph.begin(), graph.end()) == 3);
  auto blockA = graph.begin()->get();
  auto blockB = (graph.begin() + 1)->get();
  auto blockC = (graph.begin() + 2)->get();
  VERIFY(blockA->isRoot());
  VERIFY(!blockA->isExit());
  VERIFY(!blockB->isRoot());
  VERIFY(!blockB->isExit());
  VERIFY(!blockC->isRoot());
  VERIFY(blockC->isExit());
  VERIFY(is_pre_of(blockA, blockB));
  VERIFY(is_pre_of(blockB, blockC));
  VERIFY(is_pre_of(blockA, blockC));
}

void checkIfElseBranch()
{
  vector<Instruction> bin;
  ADD_INSTRUCTION(OPCODE_ADD);
  ADD_INSTRUCTION(OPCODE_IF);
  ADD_INSTRUCTION(OPCODE_ADD);
  ADD_INSTRUCTION(OPCODE_ELSE);
  ADD_INSTRUCTION(OPCODE_ADD);
  ADD_INSTRUCTION(OPCODE_ENDIF);
  ADD_INSTRUCTION(OPCODE_RET);

  ControlFlow2::Graph graph{ bin };
  VERIFY(distance(graph.begin(), graph.end()) == 4);
  auto blockA = graph.begin()->get();
  auto blockB = (graph.begin() + 1)->get();
  auto blockC = (graph.begin() + 2)->get();
  auto blockD = (graph.begin() + 3)->get();
  VERIFY(blockA->isRoot());
  VERIFY(!blockA->isExit());
  VERIFY(!blockB->isRoot());
  VERIFY(!blockB->isExit());
  VERIFY(!blockC->isRoot());
  VERIFY(!blockC->isExit());
  VERIFY(!blockD->isRoot());
  VERIFY(blockD->isExit());
  VERIFY(is_pre_of(blockA, blockB));
  VERIFY(is_pre_of(blockA, blockC));
  VERIFY(is_pre_of(blockB, blockD));
  VERIFY(is_pre_of(blockC, blockD));
}

void check2IfBlocks()
{
  vector<Instruction> bin;
  ADD_INSTRUCTION(OPCODE_ADD);
  ADD_INSTRUCTION(OPCODE_IF);
  ADD_INSTRUCTION(OPCODE_IF);
  ADD_INSTRUCTION(OPCODE_ADD);
  ADD_INSTRUCTION(OPCODE_ENDIF);
  ADD_INSTRUCTION(OPCODE_ENDIF);
  ADD_INSTRUCTION(OPCODE_RET);

  ControlFlow2::Graph graph{ bin };
  VERIFY(distance(graph.begin(), graph.end()) == 5);
  auto blockA = graph.begin()->get();
  auto blockB = (graph.begin() + 1)->get();
  auto blockC = (graph.begin() + 2)->get();
  auto blockD = (graph.begin() + 3)->get();
  auto blockE = (graph.begin() + 4)->get();
  VERIFY(blockA->isRoot());
  VERIFY(!blockA->isExit());
  VERIFY(!blockB->isRoot());
  VERIFY(!blockB->isExit());
  VERIFY(!blockC->isRoot());
  VERIFY(!blockC->isExit());
  VERIFY(!blockD->isRoot());
  VERIFY(!blockD->isExit());
  VERIFY(!blockE->isRoot());
  VERIFY(blockE->isExit());
  VERIFY(is_pre_of(blockA, blockB));
  VERIFY(is_pre_of(blockA, blockE));

  VERIFY(is_pre_of(blockB, blockC));
  VERIFY(is_pre_of(blockB, blockD));

  VERIFY(is_pre_of(blockC, blockD));

  VERIFY(is_pre_of(blockD, blockE));
}

void checkSimpleBreakLoop()
{
  vector<Instruction> bin;
  ADD_INSTRUCTION(OPCODE_ADD);
  ADD_INSTRUCTION(OPCODE_LOOP);
  ADD_INSTRUCTION(OPCODE_DIV);
  ADD_INSTRUCTION(OPCODE_BREAKC);
  ADD_INSTRUCTION(OPCODE_MUL);
  ADD_INSTRUCTION(OPCODE_ENDLOOP);
  ADD_INSTRUCTION(OPCODE_RET);

  ControlFlow2::Graph graph{ bin };
  VERIFY(distance(graph.begin(), graph.end()) == 4);
  auto blockA = graph.begin()->get();
  auto blockB = (graph.begin() + 1)->get();
  auto blockC = (graph.begin() + 2)->get();
  auto blockD = (graph.begin() + 3)->get();
  // not considered a root, as endloop is considered a pre node
  VERIFY(blockA->isRoot());
  VERIFY(!blockA->isExit());
  VERIFY(!blockB->isRoot());
  VERIFY(!blockB->isExit());
  VERIFY(!blockC->isRoot());
  VERIFY(!blockC->isExit());
  VERIFY(!blockD->isRoot());
  VERIFY(blockD->isExit());
  VERIFY(is_pre_of(blockA, blockB));

  VERIFY(is_pre_of(blockB, blockC));
  VERIFY(is_pre_of(blockB, blockD));

  // loop end also is pre of A
  VERIFY(is_pre_of(blockC, blockB));
}

void checkSimpleContinueLoop()
{
  vector<Instruction> bin;
  ADD_INSTRUCTION(OPCODE_ADD);
  ADD_INSTRUCTION(OPCODE_LOOP);
  ADD_INSTRUCTION(OPCODE_DIV);
  ADD_INSTRUCTION(OPCODE_CONTINUEC);
  ADD_INSTRUCTION(OPCODE_MUL);
  ADD_INSTRUCTION(OPCODE_ENDLOOP);
  ADD_INSTRUCTION(OPCODE_RET);

  ControlFlow2::Graph graph{ bin };
  VERIFY(distance(graph.begin(), graph.end()) == 4);
  auto blockA = graph.begin()->get();
  auto blockB = (graph.begin() + 1)->get();
  auto blockC = (graph.begin() + 2)->get();
  auto blockD = (graph.begin() + 3)->get();
  VERIFY(blockA->isRoot());
  VERIFY(!blockA->isExit());
  VERIFY(!blockB->isRoot());
  VERIFY(!blockB->isExit());
  VERIFY(!blockC->isRoot());
  VERIFY(!blockC->isExit());
  VERIFY(blockD->isRoot());
  VERIFY(blockD->isExit());
  VERIFY(is_pre_of(blockA, blockB));

  VERIFY(is_pre_of(blockB, blockC));
  VERIFY(is_pre_of(blockC, blockB));
}

void checkSimpleSwitch()
{
}

void checkBrokenShader()
{
  GlExtensions ext = {};
  GLSLCrossDependencyData data;
  HLSLccSamplerPrecisionInfo info;
  HLSLccReflection reflect;
  stringstream infoLog;
  GLSLShader shader;
  uint32_t flags = 0;
  flags |= HLSLCC_FLAG_MAP_CONST_BUFFER_TO_REGISTER_ARRAY;
  flags |= HLSLCC_FLAG_GLSL_NO_FORMAT_FOR_IMAGE_WRITES;
  flags |= HLSLCC_FLAG_GLSL_EMBEDED_ATOMIC_COUNTER;
  flags |= HLSLCC_FLAG_GLSL_EXPLICIT_UNIFORM_OFFSET;
  flags |= HLSLCC_FLAG_VULKAN_BINDINGS;
  flags |= HLSLCC_FLAG_SEPARABLE_SHADER_OBJECTS;
  flags |= HLSLCC_FLAG_UNIFORM_BUFFER_OBJECT;
  TranslateHLSLFromFile("G:/hlsl_cc_fixing/last_shader.dxbc", flags, LANG_440, &ext, &data, info, reflect, infoLog, &shader);
}

#define RUN(name) try { printf("running "#name"..."); ++run; name(); puts("passed"); } catch(const runtime_error& e_){++fail; printf("%s", e_.what()); puts("failed");}


int main()
{
  uint32_t run = 0, fail = 0;

  RUN(checkSimpleBlock);
  RUN(checkIfBranch);
  RUN(checkIfElseBranch);
  RUN(check2IfBlocks);
  RUN(checkSimpleBreakLoop);
  RUN(checkSimpleContinueLoop);
  RUN(checkSimpleSwitch);
  RUN(checkBrokenShader);
  
  if (fail > 0)
    return -1;
  return 0;
}