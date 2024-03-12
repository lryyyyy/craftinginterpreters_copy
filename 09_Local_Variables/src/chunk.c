#include "chunk.h"
#include "memory.h"

void InitChunk(Chunk *chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  InitValueArray(&chunk->constants);
}

void FreeChunk(Chunk *chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(int, chunk->lines, chunk->capacity);
  FreeValueArray(&chunk->constants);
  InitChunk(chunk);
}

void WriteChunk(Chunk *chunk, uint8_t byte, int line) {
  if (chunk->capacity < chunk->count + 1) {
    int old_apacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(old_apacity);
    chunk->code =
        GROW_ARRAY(uint8_t, chunk->code, old_apacity, chunk->capacity);
    chunk->lines = GROW_ARRAY(int, chunk->lines, old_apacity, chunk->capacity);
  }

  chunk->code[chunk->count] = byte;
  chunk->lines[chunk->count] = line;
  chunk->count++;
}

int AddConstant(Chunk *chunk, Value value) {
  WriteValueArray(&chunk->constants, value);
  return chunk->constants.count - 1;
}