#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include "server.h"

#define MAX_CACHE 1024

Document documents[MAX_CACHE];
int num_docs = 0;
int max_cache_size = 0;
int next_id = 1;
const char *document_folder;

void handle_add(Request *req, char *resposta) {
    if (num_docs >= max_cache_size) {
        for (int i = 1; i < num_docs; i++) {
            documents[i - 1] = documents[i];
        }
        num_docs--;
    }
    Document *doc = &documents[num_docs];
    doc->id = next_id++;
    strncpy(doc->title, req->title, MAX_TITLE);
    strncpy(doc->authors, req->authors, MAX_AUTHORS);
    strncpy(doc->year, req->year, MAX_YEAR);
    strncpy(doc->path, req->path, MAX_PATH);

    snprintf(resposta, MAX_MSG, "Document %d indexed\n", doc->id);
    num_docs++;
}

void handle_consult(Request *req, char *resposta) {
    int id = req->key;
    for (int i = 0; i < num_docs; i++) {
        if (documents[i].id == id) {
            snprintf(resposta, MAX_MSG,
                     "Title: %s\nAuthors: %s\nYear: %s\nPath: %s\n",
                     documents[i].title,
                     documents[i].authors,
                     documents[i].year,
                     documents[i].path);
            return;
        }
    }
    snprintf(resposta, MAX_MSG, "[Servidor] Documento com ID %d não encontrado.\n", id);
}

void handle_delete(Request *req, char *resposta) {
    int id = req->key;
    int found = 0;
    for (int i = 0; i < num_docs; i++) {
        if (documents[i].id == id) {
            for (int j = i + 1; j < num_docs; j++) {
                documents[j - 1] = documents[j];
            }
            num_docs--;
            found = 1;
            break;
        }
    }
    if (found)
        snprintf(resposta, MAX_MSG, "Index entry %d deleted\n", id);
    else
        snprintf(resposta, MAX_MSG, "[Servidor] Documento com ID %d não encontrado.\n", id);
}

int count_lines_with_word(const char *path, const char *word) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;

    char buffer[4096];
    char line[1024];
    int line_index = 0, count = 0;
    ssize_t bytes_read;

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n' || line_index >= 1022) {
                line[line_index] = '\0';
                if (strstr(line, word)) count++;
                line_index = 0;
            } else {
                line[line_index++] = buffer[i];
            }
        }
    }
    close(fd);
    return count;
}

void handle_linecount(Request *req, char *resposta) {
    int id = req->key;
    for (int i = 0; i < num_docs; i++) {
        if (documents[i].id == id) {
            char fullpath[256];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", document_folder, documents[i].path);
            int lines = count_lines_with_word(fullpath, req->keyword);
            if (lines >= 0)
                snprintf(resposta, MAX_MSG, "%d\n", lines);
            else
                snprintf(resposta, MAX_MSG, "[Servidor] Erro ao abrir ficheiro '%s'\n", fullpath);
            return;
        }
    }
    snprintf(resposta, MAX_MSG, "[Servidor] Documento com ID %d não encontrado.\n", id);
}

void handle_shutdown(char *resposta) {
    snprintf(resposta, MAX_MSG, "Server is shutting down\n");
    // FIFO será apagado no main após resposta ser enviada
}

void handle_search_sequential(Request *req, char *resposta) {
    DIR *dir = opendir(document_folder);
    if (!dir) {
        snprintf(resposta, MAX_MSG, "[Servidor] Erro ao abrir pasta '%s'\n", document_folder);
        return;
    }
    struct dirent *entry;
    int id = 1;
    bool first = true;
    strcat(resposta, "[");

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) continue;
        const char *name = entry->d_name;
        size_t len = strlen(name);
        if (len < 4 || strcmp(name + len - 4, ".txt") != 0) continue;

        char fullpath[256];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", document_folder, name);
        int lines = count_lines_with_word(fullpath, req->keyword);
        if (lines > 0) {
            char tmp[16];
            snprintf(tmp, sizeof(tmp), first ? "%d" : ", %d", id);
            strcat(resposta, tmp);
            first = false;
        }
        id++;
    }
    closedir(dir);
    strcat(resposta, "]\n");
}

void handle_search_parallel(Request *req, char *resposta) {
    DIR *dir = opendir(document_folder);
    if (!dir) {
        snprintf(resposta, MAX_MSG, "[Servidor] Erro ao abrir pasta '%s'\n", document_folder);
        return;
    }
    struct dirent *entry;
    char *files[4096];
    int total = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) continue;
        const char *name = entry->d_name;
        size_t len = strlen(name);
        if (len < 4 || strcmp(name + len - 4, ".txt") != 0) continue;
        files[total] = strdup(name);
        total++;
    }
    closedir(dir);

    int proc = req->max_proc;
    if (proc <= 0 || proc > total) proc = 1;

    int pipefd[proc][2];
    for (int i = 0; i < proc; i++) pipe(pipefd[i]);

    for (int i = 0; i < proc; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            close(pipefd[i][0]);
            int start = i * total / proc;
            int end = (i + 1) * total / proc;
            for (int j = start; j < end; j++) {
                char fullpath[256];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", document_folder, files[j]);
                int lines = count_lines_with_word(fullpath, req->keyword);
                if (lines > 0) {
                    char tmp[16];
                    snprintf(tmp, sizeof(tmp), "%d ", j + 1);
                    write(pipefd[i][1], tmp, strlen(tmp));
                }
            }
            close(pipefd[i][1]);
            exit(0);
        }
        close(pipefd[i][1]);
    }

    char buffer[4096] = "";
    for (int i = 0; i < proc; i++) {
        char tmp[1024];
        int n = read(pipefd[i][0], tmp, sizeof(tmp) - 1);
        if (n > 0) {
            tmp[n] = '\0';
            strcat(buffer, tmp);
        }
        close(pipefd[i][0]);
    }
    for (int i = 0; i < proc; i++) wait(NULL);

    // Remover duplicados e ordenar IDs
    bool seen[10000] = {false};
    int ids[4096], count = 0;
    char *token = strtok(buffer, " ");
    while (token) {
        int val = atoi(token);
        if (val > 0 && !seen[val]) {
            ids[count++] = val;
            seen[val] = true;
        }
        token = strtok(NULL, " ");
    }
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (ids[i] > ids[j]) {
                int tmp = ids[i]; ids[i] = ids[j]; ids[j] = tmp;
            }
        }
    }

    strcat(resposta, "[");
    for (int i = 0; i < count; i++) {
        char tmp[16];
        snprintf(tmp, sizeof(tmp), i == 0 ? "%d" : ", %d", ids[i]);
        strcat(resposta, tmp);
    }
    strcat(resposta, "]\n");

    for (int i = 0; i < total; i++) free(files[i]);
}

void start_server(const char *folder, int cache_size) {
    document_folder = folder;
    max_cache_size = cache_size;
    mkfifo(FIFO_SERVER, 0666);
    printf("[Servidor] A escutar pedidos em %s...\n", FIFO_SERVER);

    int fd_read = open(FIFO_SERVER, O_RDONLY);
    int fd_dummy = open(FIFO_SERVER, O_WRONLY);

    while (1) {
        Request req;
        int n = read(fd_read, &req, sizeof(Request));

        if (n > 0) {
            char resposta[MAX_MSG];
            memset(resposta, 0, sizeof(resposta));

            if (req.operation == 's' || req.operation == 'l') {
                pid_t pid = fork();
                if (pid == 0) {
                    // filho
                    if (req.operation == 's') {
                        if (req.max_proc <= 1) handle_search_sequential(&req, resposta);
                        else handle_search_parallel(&req, resposta);
                    } else if (req.operation == 'l') {
                        handle_linecount(&req, resposta);
                    }
                    int out = open(FIFO_CLIENT, O_WRONLY);
                    if (out != -1) {
                        write(out, resposta, strlen(resposta));
                        close(out);
                    }
                    exit(0);
                }
            } else {
                switch (req.operation) {
                    case 'a': handle_add(&req, resposta); break;
                    case 'c': handle_consult(&req, resposta); break;
                    case 'd': handle_delete(&req, resposta); break;
                    case 'f':
                        handle_shutdown(resposta);
                        {
                            int out = open(FIFO_CLIENT, O_WRONLY);
                            if (out != -1) {
                                write(out, resposta, strlen(resposta));
                                close(out);
                            }
                        }
                        unlink(FIFO_SERVER);
                        exit(0);
                    default:
                        snprintf(resposta, MAX_MSG, "[Servidor] Operação '%c' não implementada ainda.\n", req.operation);
                        break;
                }

                int out = open(FIFO_CLIENT, O_WRONLY);
                if (out != -1) {
                    write(out, resposta, strlen(resposta));
                    close(out);
                }
            }
        }
    }
}