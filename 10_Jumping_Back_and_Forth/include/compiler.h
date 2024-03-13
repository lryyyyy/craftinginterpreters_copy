#ifndef COPY_CLOX_COMPILER_H
#define COPY_CLOX_COMPILER_H

#include "chunk.h"
#include "vm.h"

bool Compile(const char *source, Chunk *chunk);

#endif // COPY_CLOX_COMPILER_H