#ifndef COPY_CLOX_CHUNK_H
#define COPY_CLOX_CHUNK_H

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
  OP_RETURN,
} OpCode;

typedef struct {
  int count;
  int capacity;
  uint8_t *code;
  int *lines;
  ValueArray constants;
} Chunk;

void InitChunk(Chunk *chunk);

void FreeChunk(Chunk *chunk);

void WriteChunk(Chunk *chunk, uint8_t byte, int line);

int AddConstant(Chunk *chunk, Value value);

#endif // COPY_CLOX_CHUNK_H