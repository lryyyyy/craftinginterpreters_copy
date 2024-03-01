#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char *argv[]) {
  InitVM();
  Chunk chunk;
  InitChunk(&chunk);
  int constant = AddConstant(&chunk, 1.2);
  WriteChunk(&chunk, OP_CONSTANT, 1);
  WriteChunk(&chunk, constant, 1);
  constant = AddConstant(&chunk, 3.4);
  WriteChunk(&chunk, OP_CONSTANT, 1);
  WriteChunk(&chunk, constant, 1);
  WriteChunk(&chunk, OP_ADD, 1);
  constant = AddConstant(&chunk, 5.6);
  WriteChunk(&chunk, OP_CONSTANT, 1);
  WriteChunk(&chunk, constant, 1);
  WriteChunk(&chunk, OP_DIVIDE, 1);
  WriteChunk(&chunk, OP_NEGATE, 1);
  WriteChunk(&chunk, OP_RETURN, 1);
  DisassembleChunk(&chunk, "test chunk");
  Interpret(&chunk);
  FreeChunk(&chunk);
  FreeVM();
  return 0;
}
