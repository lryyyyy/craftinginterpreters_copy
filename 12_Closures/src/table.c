
#include "table.h"
#include "common.h"
#include "memory.h"
#include "object.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void InitTable(Table *table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void FreeTable(Table *table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  InitTable(table);
}

static Entry *FindEntry(Entry *entries, int capacity, ObjString *key) {
  uint32_t index = key->hash % capacity;
  Entry *tombstone = NULL;
  while (true) {
    Entry *entry = &entries[index];
    if (entry->key == NULL) {
      if (IS_NULL(entry->value)) {
        return tombstone != NULL ? tombstone : entry;
      } else {
        if (tombstone == NULL)
          tombstone = entry;
      }
    } else if (entry->key == key) {
      return entry;
    }
    index = (index + 1) % capacity;
  }
}

static void AdjustCapacity(Table *table, int capacity) {
  Entry *entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NULL_VAL;
  }
  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i];
    if (entry->key == NULL)
      continue;

    Entry *dest = FindEntry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }
  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

bool TableGet(Table *table, ObjString *key, Value *value) {
  if (table->count == 0) {
    return false;
  }
  Entry *entry = FindEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) {
    return false;
  }
  *value = entry->value;
  return true;
}

bool TableSet(Table *table, ObjString *key, Value value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    AdjustCapacity(table, capacity);
  }
  Entry *entry = FindEntry(table->entries, table->capacity, key);
  bool is_new_key = entry->key == NULL;
  if (is_new_key && IS_NULL(entry->value)) {
    table->count++;
  }
  entry->key = key;
  entry->value = value;
  return is_new_key;
}

bool TableDelete(Table *table, ObjString *key) {
  if (table->count == 0) {
    return false;
  }
  Entry *entry = FindEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) {
    return false;
  }
  entry->key = NULL;
  entry->value = BOOL_VAL(true);
  return true;
}

void TableAddAll(Table *from, Table *to) {
  for (int i = 0; i < from->capacity; i++) {
    Entry *entry = &from->entries[i];
    if (entry->key != NULL) {
      TableSet(to, entry->key, entry->value);
    }
  }
}

ObjString *TableFindString(Table *table, const char *chars, int length,
                           uint32_t hash) {
  if (table->count == 0) {
    return NULL;
  }
  uint32_t index = hash % table->capacity;
  while (true) {
    Entry *entry = &table->entries[index];
    if (entry->key == NULL) {
      if (IS_NULL(entry->value)) {
        return NULL;
      }
    } else if (entry->key->length == length && entry->key->hash == hash &&
               memcmp(entry->key->chars, chars, length) == 0) {
      return entry->key;
    }
    index = (index + 1) % table->capacity;
  }
}