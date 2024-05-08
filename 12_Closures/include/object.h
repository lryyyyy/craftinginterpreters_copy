#ifndef COPY_CLOX_OBJECT_H
#define COPY_CLOX_OBJECT_H

#include "chunk.h"
#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_STRING(value) IsObjType(value, OBJ_STRING)
#define IS_FUNCTION(value) IsObjType(value, OBJ_FUNCTION)
#define IS_CLOSURE(value) IsObjType(value, OBJ_CLOSURE)
#define IS_NATIVE(value) IsObjType(value, OBJ_NATIVE)
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjClosure *)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative *)AS_OBJ(value))->function)

typedef enum {
  OBJ_STRING,
  OBJ_FUNCTION,
  OBJ_CLOSURE,
  OBJ_NATIVE,
  OBJ_UPVALUE,
} ObjType;

struct Object {
  ObjType type;
  struct Object *next;
};

typedef struct {
  Object obj;
  int arity;
  int upvalue_count;
  Chunk chunk;
  ObjString *name;
} ObjFunction;

typedef struct ObjUpvalue {
  Object obj;
  Value *location;
  Value closed;
  struct ObjUpvalue *next;
} ObjUpvalue;

typedef struct {
  Object obj;
  ObjFunction *function;
  ObjUpvalue** upvalues;
  int upvalue_count;
} ObjClosure;

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

ObjClosure *NewClosure(ObjFunction* function);

ObjNative *NewNative(NativeFn function);

ObjString *GetString(char *chars, int length);

ObjString *CopyString(const char *chars, int length);

ObjUpvalue *NewUpvalue(Value *slot);

static inline bool IsObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

void PrintObject(Value value);

#endif // COPY_CLOX_OBJECT_H
