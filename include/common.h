#ifndef COMMON_H
#define COMMON_H

#define FIFO_CLIENT_TO_SERVER "/tmp/fifo_ctos"
#define FIFO_SERVER_TO_CLIENT "/tmp/fifo_stoc"
#define BUFFER_SIZE 512
#define MAX_DOCS 3000
#define MAX_TITLE 200
#define MAX_AUTHORS 200
#define MAX_YEAR 5    // 4 dígitos + '\0'
#define MAX_PATH 64

typedef struct {
    int id;
    char title[MAX_TITLE];
    char authors[MAX_AUTHORS];
    char year[MAX_YEAR];
    char path[MAX_PATH];
} Document;

#endif