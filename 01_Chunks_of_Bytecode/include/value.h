#ifndef COPY_CLOX_VALUE_H
#define COPY_CLOX_VALUE_H

#include "common.h"

typedef double Value;

typedef struct {
  int capacity;
  int count;
  Value *values;
} ValueArray;

void InitValueArray(ValueArray* array);
void WriteValueArray(ValueArray* array, Value value);
void FreeValueArray(ValueArray* array);

#endif // COPY_CLOX_VALUE_H