#ifndef COPY_CLOX_TABLE_H
#define COPY_CLOX_TABLE_H

#include "common.h"
#include "value.h"

typedef struct {
  ObjString *key;
  Value value;
} Entry;

typedef struct {
  int count;
  int capacity;
  Entry *entries;
} Table;

void InitTable(Table *table);
void FreeTable(Table *table);
bool TableGet(Table *table, ObjString *key, Value *value);
bool TableSet(Table *table, ObjString *key, Value value);
bool TableDelete(Table *table, ObjString *key);
void TableAddAll(Table *from, Table *to);
ObjString *TableFindString(Table *table, const char *chars, int length,
                           uint32_t hash);

#endif // COPY_CLOX_TABLE_H