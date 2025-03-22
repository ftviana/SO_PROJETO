#ifndef COMMON_H
#define COMMON_H

#define MAX_TITLE 200
#define MAX_AUTHORS 200
#define MAX_PATH 64
#define MAX_YEAR 6
#define MAX_MSG 512
#define MAX_DOCS 1024

#define FIFO_CLIENT "/tmp/dclient_fifo"
#define FIFO_SERVER "/tmp/dserver_fifo"

typedef struct {
    int id;
    char title[MAX_TITLE];
    char authors[MAX_AUTHORS];
    char year[MAX_YEAR];
    char path[MAX_PATH];
} Document;

typedef struct {
    char operation;
    int key;
    int max_proc;
    char title[MAX_TITLE];
    char authors[MAX_AUTHORS];
    char year[MAX_YEAR];
    char path[MAX_PATH];
    char keyword[64];
} Request;

extern Document documents[MAX_DOCS];
extern int num_docs;
extern int next_id;

#endif