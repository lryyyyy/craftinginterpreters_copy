#ifndef COPY_CLOX_OBJECT_H
#define COPY_CLOX_OBJECT_H

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_STRING(value) IsObjType(value, OBJ_STRING)
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

typedef enum {
  OBJ_STRING,
} ObjType;

struct Object {
  ObjType type;
  struct Object *next;
};

struct ObjString {
  Object object;
  int length;
  char *chars;
  uint32_t hash;
};

ObjString *GetString(char *chars, int length);

ObjString *CopyString(const char *chars, int length);

static inline bool IsObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

void PrintObject(Value value);

#endif // COPY_CLOX_OBJECT_H