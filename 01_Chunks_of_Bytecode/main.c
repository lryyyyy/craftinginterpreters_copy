#include "chunk.h"
#include "common.h"
#include "debug.h"

int main(int argc, const char *argv[]) {
  Chunk chunk;
  InitChunk(&chunk);
  int constant = AddConstant(&chunk, 1.2);
  WriteChunk(&chunk, OP_CONSTANT, 1);
  WriteChunk(&chunk, constant, 1);
  WriteChunk(&chunk, OP_RETURN, 1);
  DisassembleChunk(&chunk, "test chunk");
  FreeChunk(&chunk);
  return 0;
}
