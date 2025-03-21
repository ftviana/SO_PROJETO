#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define FIFO_CLIENT_TO_SERVER "/tmp/fifo_ctos"
#define FIFO_SERVER_TO_CLIENT "/tmp/fifo_stoc"
#define BUFFER_SIZE 512

void usage() {
    fprintf(stderr, "Uso:\n");
    fprintf(stderr, "  dclient -a \"title\" \"authors\" \"year\" \"path\"\n");
    fprintf(stderr, "  dclient -c \"key\"\n");
    fprintf(stderr, "  dclient -d \"key\"\n");
    fprintf(stderr, "  dclient -l \"key\" \"keyword\"\n");
    fprintf(stderr, "  dclient -s \"keyword\" [nr_proc]\n");
    fprintf(stderr, "  dclient -f\n");
    fprintf(stderr, "  dclient --batch catalog.tsv\n");
}

int send_request(const char *request, char *response) {
    int fd_write = open(FIFO_CLIENT_TO_SERVER, O_WRONLY);
    if (fd_write < 0) {
        perror("Erro ao abrir FIFO de escrita");
        return 1;
    }

    if (write(fd_write, request, strlen(request)) < 0) {
        perror("Erro ao escrever no FIFO");
        close(fd_write);
        return 1;
    }
    close(fd_write);

    int fd_read = open(FIFO_SERVER_TO_CLIENT, O_RDONLY);
    if (fd_read < 0) {
        perror("Erro ao abrir FIFO de leitura");
        return 1;
    }

    ssize_t n = read(fd_read, response, BUFFER_SIZE - 1);
    if (n < 0) {
        perror("Erro ao ler do FIFO");
        close(fd_read);
        return 1;
    }

    response[n] = '\0';
    close(fd_read);
    return 0;
}

void handle_batch(const char *catalog_path) {
    FILE *f = fopen(catalog_path, "r");
    if (!f) {
        perror("Erro ao abrir ficheiro do catálogo");
        return;
    }

    char line[1024];
    fgets(line, sizeof(line), f); // Ignorar cabeçalho

    while (fgets(line, sizeof(line), f)) {
        char id[16], title[200], authors[200], year[8], path[64];

        if (sscanf(line, "%15[^\t]\t%199[^\t]\t%199[^\t]\t%7[^\t]\t%63[^\n\r]",
                   id, title, authors, year, path) != 5) {
            fprintf(stderr, "Linha inválida, a saltar...\n");
            continue;
        }

        char full_path[256];
        snprintf(full_path, sizeof(full_path), "DatasetTest/Gdataset/%s", path);

        char buffer[BUFFER_SIZE];
        snprintf(buffer, BUFFER_SIZE, "ADD|%s|%s|%s|%s", title, authors, year, full_path);

        char response[BUFFER_SIZE];
        if (send_request(buffer, response) == 0) {
            printf("[%s] %s\n", id, response);
        }
    }

    fclose(f);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage();
        return 1;
    }

    if (strcmp(argv[1], "--batch") == 0 && argc == 3) {
        handle_batch(argv[2]);
        return 0;
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    if (strcmp(argv[1], "-a") == 0 && argc == 6) {
        snprintf(buffer, BUFFER_SIZE, "ADD|%s|%s|%s|%s", argv[2], argv[3], argv[4], argv[5]);
    } else if (strcmp(argv[1], "-c") == 0 && argc == 3) {
        snprintf(buffer, BUFFER_SIZE, "CONSULT|%s", argv[2]);
    } else if (strcmp(argv[1], "-d") == 0 && argc == 3) {
        snprintf(buffer, BUFFER_SIZE, "DELETE|%s", argv[2]);
    } else if (strcmp(argv[1], "-l") == 0 && argc == 4) {
        snprintf(buffer, BUFFER_SIZE, "LINES|%s|%s", argv[2], argv[3]);
    } else if (strcmp(argv[1], "-s") == 0 && (argc == 3 || argc == 4)) {
        if (argc == 3)
            snprintf(buffer, BUFFER_SIZE, "SEARCH|%s", argv[2]);
        else
            snprintf(buffer, BUFFER_SIZE, "SEARCH_PAR|%s|%s", argv[2], argv[3]);
    } else if (strcmp(argv[1], "-f") == 0 && argc == 2) {
        snprintf(buffer, BUFFER_SIZE, "SHUTDOWN");
    } else {
        usage();
        return 1;
    }

    char response[BUFFER_SIZE];
    if (send_request(buffer, response) == 0) {
        printf("%s\n", response);
    }

    return 0;
}