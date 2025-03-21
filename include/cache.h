#ifndef CACHE_H
#define CACHE_H

#include "common.h"

void cache_init(int size);
void cache_clear();
Document *cache_get(int id);
void cache_put(Document *doc);

#endif