#include <stdio.h>
#include <stdlib.h>
#include "server.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <document_folder> <cache_size>\n", argv[0]);
        return 1;
    }
    const char *folder = argv[1];
    int cache_size = atoi(argv[2]);

    start_server(folder, cache_size);
    return 0;
}