#ifndef COPY_CLOX_COMPILER_H
#define COPY_CLOX_COMPILER_H

#include "chunk.h"
#include "object.h"
#include "vm.h"

ObjFunction *Compile(const char *source);

#endif // COPY_CLOX_COMPILER_H
