#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"

int main(int argc, char *argv[]) {
    Request req = {0};

    if (argc < 2) {
        fprintf(stderr, "Uso: %s -a|-c|-d|-l|-s|-f ...\n", argv[0]);
        return 1;
    }

    req.operation = argv[1][1];

    switch (req.operation) {
        case 'a':
            if (argc != 6) return fprintf(stderr, "Uso: %s -a \"title\" \"authors\" \"year\" \"path\"\n", argv[0]), 1;
            strncpy(req.title, argv[2], MAX_TITLE);
            strncpy(req.authors, argv[3], MAX_AUTHORS);
            strncpy(req.year, argv[4], MAX_YEAR);
            strncpy(req.path, argv[5], MAX_PATH);
            break;
        case 'c':
        case 'd':
            if (argc != 3) return fprintf(stderr, "Uso: %s -%c <key>\n", argv[0], req.operation), 1;
            req.key = atoi(argv[2]);
            break;
        case 'l':
            if (argc != 4) return fprintf(stderr, "Uso: %s -l <key> <keyword>\n", argv[0]), 1;
            req.key = atoi(argv[2]);
            strncpy(req.keyword, argv[3], 64);
            break;
        case 's':
            if (argc != 3 && argc != 4) return fprintf(stderr, "Uso: %s -s <keyword> [max_proc]\n", argv[0]), 1;
            strncpy(req.keyword, argv[2], 64);
            if (argc == 4) req.max_proc = atoi(argv[3]);
            break;
        case 'f':
            break;
        default:
            fprintf(stderr, "Operação inválida.\n");
            return 1;
    }

    send_request(&req);
    return 0;
}