#ifndef COPY_CLOX_OBJECT_H
#define COPY_CLOX_OBJECT_H

#include "chunk.h"
#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_STRING(value) IsObjType(value, OBJ_STRING)
#define IS_FUNCTION(value) IsObjType(value, OBJ_FUNCTION)
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))

typedef enum {
  OBJ_STRING,
  OBJ_FUNCTION,
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

struct ObjString {
  Object object;
  int length;
  char *chars;
  uint32_t hash;
};

ObjFunction *NewFunction();

ObjString *GetString(char *chars, int length);

ObjString *CopyString(const char *chars, int length);

static inline bool IsObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

void PrintObject(Value value);

#endif // COPY_CLOX_OBJECT_H
