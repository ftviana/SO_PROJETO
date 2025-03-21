#include <stdlib.h>
#include <string.h>

#include "cache.h"

static Document *cache = NULL;
static int *ids = NULL;
static int cache_size = 0;
static int count = 0;
static int start = 0;

void cache_init(int size) {
    cache_size = size;
    cache = malloc(sizeof(Document) * size);
    ids = malloc(sizeof(int) * size);
    count = 0;
    start = 0;
}

void cache_clear() {
    free(cache);
    free(ids);
    cache = NULL;
    ids = NULL;
    count = 0;
    start = 0;
}

Document *cache_get(int id) {
    for (int i = 0; i < count; i++) {
        int idx = (start + i) % cache_size;
        if (ids[idx] == id)
            return &cache[idx];
    }
    return NULL;
}

void cache_put(Document *doc) {
    int idx;
    if (count < cache_size) {
        idx = (start + count) % cache_size;
        count++;
    } else {
        idx = start;
        start = (start + 1) % cache_size;
    }

    cache[idx] = *doc;
    ids[idx] = doc->id;
}