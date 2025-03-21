#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

#include "handlers.h"
#include "db.h"
#include "cache.h"
#include "common.h"

int running = 1;

void cleanup() {
    unlink(FIFO_CLIENT_TO_SERVER);
    unlink(FIFO_SERVER_TO_CLIENT);
    cache_clear();
}

void sigint_handler(int sig) {
    printf("\nServidor a terminar...\n");
    cleanup();
    exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, sigint_handler);

    if (argc < 2) {
        fprintf(stderr, "Uso: %s <cache_size>\n", argv[0]);
        return 1;
    }

    int cache_size = atoi(argv[1]);
    if (cache_size <= 0) cache_size = 10; // valor por defeito

    mkfifo(FIFO_CLIENT_TO_SERVER, 0666);
    mkfifo(FIFO_SERVER_TO_CLIENT, 0666);

    db_init();
    db_load("docs.db");
    cache_init(cache_size);

    char buffer[BUFFER_SIZE];

    printf("Servidor iniciado com cache_size = %d. À escuta...\n", cache_size);

    while (running) {
        int fd_read = open(FIFO_CLIENT_TO_SERVER, O_RDONLY);
        if (fd_read < 0) {
            perror("Erro ao abrir FIFO de leitura");
            continue;
        }

        ssize_t n = read(fd_read, buffer, BUFFER_SIZE - 1);
        close(fd_read);

        if (n <= 0) continue;

        buffer[n] = '\0';
        char *cmd = strtok(buffer, "|");
        char response[BUFFER_SIZE] = "Comando não reconhecido.";

        if (strcmp(cmd, "ADD") == 0) {
            handle_add(buffer, response);
            db_save("docs.db");
        } else if (strcmp(cmd, "CONSULT") == 0) {
            handle_consult(buffer, response);
        } else if (strcmp(cmd, "DELETE") == 0) {
            handle_delete(buffer, response);
            db_save("docs.db");
        } else if (strcmp(cmd, "SHUTDOWN") == 0) {
            snprintf(response, BUFFER_SIZE, "Server is shutting down");
            running = 0;
        } else if (strcmp(cmd, "LINES") == 0) {
            handle_lines(buffer, response);
        } else if (strcmp(cmd, "SEARCH") == 0) {
            handle_search(buffer, response);
        } else if (strcmp(cmd, "SEARCH_PAR") == 0) {
            handle_search_parallel(buffer, response);
        }

        int fd_write = open(FIFO_SERVER_TO_CLIENT, O_WRONLY);
        if (fd_write >= 0) {
            write(fd_write, response, strlen(response));
            close(fd_write);
        }
    }

    cleanup();
    return 0;
}
