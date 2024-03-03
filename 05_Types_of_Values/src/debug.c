#include "debug.h"

void DisassembleChunk(Chunk *chunk, const char *name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->count;) {
    offset = DisassembleInstruction(chunk, offset);
  }
}

static int SimpleInstruction(const char *name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

static int ConstantInstruction(const char *name, Chunk *chunk, int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d '%g'\n", name, constant, chunk->constants.values[constant]);
  return offset + 2;
}

int DisassembleInstruction(Chunk *chunk, int offset) {
  printf("%04d ", offset);
  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[offset]);
  }

  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
  case OP_CONSTANT:
    return ConstantInstruction("OP_CONSTANT", chunk, offset);
  case OP_ADD:
    return SimpleInstruction("OP_ADD", offset);
  case OP_SUBTRACT:
    return SimpleInstruction("OP_SUBTRACT", offset);
  case OP_MULTIPLY:
    return SimpleInstruction("OP_MULTIPLY", offset);
  case OP_DIVIDE:
    return SimpleInstruction("OP_DIVIDE", offset);
  case OP_NEGATE:
    return SimpleInstruction("OP_NEGATE", offset);
  case OP_RETURN:
    return SimpleInstruction("OP_RETURN", offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}
