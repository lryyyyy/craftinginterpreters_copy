#ifndef COPY_CLOX_VALUE_H
#define COPY_CLOX_VALUE_H

#include "common.h"

typedef enum {
  VAL_NULL,
  VAL_BOOL,
  VAL_NUMBER,
  VAL_OBJ,
} ValueType;

typedef struct Object Object;

typedef struct ObjString ObjString;

typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
    Object *object;
  } as;
} Value;

#define IS_NULL(value) ((value).type == VAL_NULL)
#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.object)

#define NULL_VAL ((Value){VAL_NULL, {.number = 0}})
#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(obj) ((Value){VAL_OBJ, {.object = (Object *)obj}})

typedef struct {
  int capacity;
  int count;
  Value *values;
} ValueArray;

void InitValueArray(ValueArray *array);
void WriteValueArray(ValueArray *array, Value value);
void FreeValueArray(ValueArray *array);
void PrintValue(Value value);
bool ValueEqual(Value a, Value b);

#endif // COPY_CLOX_VALUE_H