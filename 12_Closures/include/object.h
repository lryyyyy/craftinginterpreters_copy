#ifndef COPY_CLOX_OBJECT_H
#define COPY_CLOX_OBJECT_H

#include "chunk.h"
#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_STRING(value) IsObjType(value, OBJ_STRING)
#define IS_FUNCTION(value) IsObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value) IsObjType(value, OBJ_NATIVE)
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative *)AS_OBJ(value))->function)

typedef enum {
  OBJ_STRING,
  OBJ_FUNCTION,
  OBJ_NATIVE,
} ObjType;

struct Object {
  ObjType type;
  struct Object *next;
};

typedef struct {
  Object obj;
  int arity;
  Chunk chunk;
  ObjString *name;
} ObjFunction;

typedef Value (*NativeFn)(int arg_count, Value *args);

typedef struct {
  Object obj;
  NativeFn function;
} ObjNative;

struct ObjString {
  Object object;
  int length;
  char *chars;
  uint32_t hash;
};

ObjFunction *NewFunction();

ObjNative *NewNative(NativeFn function);

ObjString *GetString(char *chars, int length);

ObjString *CopyString(const char *chars, int length);

static inline bool IsObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

void PrintObject(Value value);

#endif // COPY_CLOX_OBJECT_H
