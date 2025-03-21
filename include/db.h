#ifndef DB_H
#define DB_H

#include "common.h"

void db_init();
int db_add(Document d);
Document *db_get(int id);
int db_delete(int id);
int db_count();
Document *db_get_all();
int db_load(const char *filepath);
int db_save(const char *filepath);

#endif