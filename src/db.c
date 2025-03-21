#include <string.h>
#include <stdio.h>

#include "db.h"

#define DB_FILE "docs.db"

static Document docs[MAX_DOCS];
static int doc_count = 0;

void db_init() {
    doc_count = 0;
}

int db_add(Document d) {
    if (doc_count >= MAX_DOCS)
        return -1;

    d.id = doc_count + 1;
    docs[doc_count++] = d;
    return d.id;
}

Document *db_get(int id) {
    for (int i = 0; i < doc_count; i++) {
        if (docs[i].id == id)
            return &docs[i];
    }
    return NULL;
}

int db_delete(int id) {
    for (int i = 0; i < doc_count; i++) {
        if (docs[i].id == id) {
            docs[i] = docs[--doc_count]; // Troca com o último
            return 1;
        }
    }
    return 0;
}

int db_count() {
    return doc_count;
}

Document *db_get_all() {
    return docs;
}

int db_save(const char *filepath) {
    FILE *f = fopen(filepath, "w");
    if (!f) return 0;

    for (int i = 0; i < doc_count; i++) {
        fprintf(f, "%d\t%s\t%s\t%s\t%s\n",
                docs[i].id,
                docs[i].title,
                docs[i].authors,
                docs[i].year,
                docs[i].path);
    }

    fclose(f);
    return 1;
}

int db_load(const char *filepath) {
    FILE *f = fopen(filepath, "r");
    if (!f) return 0;

    doc_count = 0;
    while (!feof(f)) {
        Document d;
        char line[1024];
        if (!fgets(line, sizeof(line), f)) break;

        if (sscanf(line, "%d\t%199[^\t]\t%199[^\t]\t%4[^\t]\t%63[^\n]",
                   &d.id, d.title, d.authors, d.year, d.path) == 5) {
            docs[doc_count++] = d;
        }
    }

    fclose(f);
    return 1;
}