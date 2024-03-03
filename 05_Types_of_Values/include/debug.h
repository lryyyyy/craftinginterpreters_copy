#ifndef COPY_CLOX_DEBUG_H
#define COPY_CLOX_DEBUG_H

#include "chunk.h"

void DisassembleChunk(Chunk* chunk, const char* name);
int DisassembleInstruction(Chunk* chunk, int offset);

#endif // COPY_CLOX_DEBUG_H