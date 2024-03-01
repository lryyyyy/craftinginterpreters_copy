#ifndef COPY_CLOX_VM_H
#define COPY_CLOX_VM_H

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
  Chunk *chunk;
  uint8_t *ip;
  Value stack[STACK_MAX];
  Value *stackTop;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void InitVM();
void FreeVM();
InterpretResult Interpret(const char *source);
void Push(Value value);
Value Pop();

#endif // COPY_CLOX_VM_H