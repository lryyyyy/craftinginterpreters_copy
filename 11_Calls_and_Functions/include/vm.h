#ifndef COPY_CLOX_VM_H
#define COPY_CLOX_VM_H

#include "chunk.h"
#include "common.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include <stdint.h>

#define FRAME_MAX 64
#define STACK_MAX (FRAME_MAX * UINT8_COUNT)

typedef struct {
  ObjFunction *function;
  uint8_t *ip;
  Value *slots;
} CallFrame;

typedef struct {
  Chunk *chunk;
  uint8_t *ip;
  CallFrame frames[FRAME_MAX];
  int frame_count;
  Value stack[STACK_MAX];
  Value *stack_top;
  Table strings;
  Table globals;
  Object *objects;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

void InitVM();
void FreeVM();
InterpretResult Interpret(const char *source);
void Push(Value value);
Value Pop();

#endif // COPY_CLOX_VM_H
