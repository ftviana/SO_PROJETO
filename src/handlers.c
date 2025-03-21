#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>

#include "handlers.h"
#include "db.h"
#include "common.h"
#include "cache.h"

void handle_add(char *data, char *response) {
    char *title = strtok(NULL, "|");
    char *authors = strtok(NULL, "|");
    char *year = strtok(NULL, "|");
    char *path = strtok(NULL, "|");

    if (!title || !authors || !year || !path) {
        snprintf(response, BUFFER_SIZE, "Erro: dados incompletos.");
        return;
    }

    Document d;
    strncpy(d.title, title, MAX_TITLE);
    strncpy(d.authors, authors, MAX_AUTHORS);
    strncpy(d.year, year, MAX_YEAR);
    strncpy(d.path, path, MAX_PATH);

    int id = db_add(d);
    if (id < 0) {
        snprintf(response, BUFFER_SIZE, "Erro: limite de documentos atingido.");
        return;
    }

    snprintf(response, BUFFER_SIZE, "Document %d indexed", id);
}

void handle_consult(char *data, char *response) {
    char *id_str = strtok(NULL, "|");
    if (!id_str) {
        snprintf(response, BUFFER_SIZE, "Erro: identificador em falta.");
        return;
    }

    int id = atoi(id_str);
    Document *d = cache_get(id);
    if (!d) {
        d = db_get(id);
        if (d) cache_put(d);
    }

    if (!d) {
        snprintf(response, BUFFER_SIZE, "Erro: documento %d não encontrado.", id);
        return;
    }

    snprintf(response, BUFFER_SIZE, "Title: %s\nAuthors: %s\nYear: %s\nPath: %s",
             d->title, d->authors, d->year, d->path);
}

void handle_delete(char *data, char *response) {
    char *id_str = strtok(NULL, "|");
    if (!id_str) {
        snprintf(response, BUFFER_SIZE, "Erro: identificador em falta.");
        return;
    }

    int id = atoi(id_str);
    if (db_delete(id)) {
        snprintf(response, BUFFER_SIZE, "Index entry %d deleted", id);
    } else {
        snprintf(response, BUFFER_SIZE, "Erro: documento %d não encontrado.", id);
    }
}

void handle_lines(char *data, char *response) {
    char *id_str = strtok(NULL, "|");
    char *keyword = strtok(NULL, "|");

    if (!id_str || !keyword) {
        snprintf(response, BUFFER_SIZE, "Erro: argumentos em falta.");
        return;
    }

    int id = atoi(id_str);
    Document *doc = db_get(id);
    if (!doc) {
        snprintf(response, BUFFER_SIZE, "Erro: documento %d não encontrado.", id);
        return;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        snprintf(response, BUFFER_SIZE, "Erro ao criar pipe.");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        snprintf(response, BUFFER_SIZE, "Erro ao fazer fork.");
        return;
    }

    if (pid == 0) {
        // Processo filho: redireciona stdout para o pipe de escrita
        close(pipefd[0]); // fecha leitura
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execlp("grep", "grep", "-c", keyword, doc->path, NULL);
        perror("execlp");
        exit(1);
    } else {
        // Processo pai: lê o output do filho
        close(pipefd[1]); // fecha escrita
        char result[BUFFER_SIZE];
        ssize_t n = read(pipefd[0], result, BUFFER_SIZE - 1);
        close(pipefd[0]);
        wait(NULL); // espera pelo filho

        if (n <= 0) {
            snprintf(response, BUFFER_SIZE, "Erro ao contar linhas.");
        } else {
            result[n] = '\0';
            snprintf(response, BUFFER_SIZE, "Encontradas %s linhas com a palavra-chave", result);
        }
    }
}

void handle_search(char *data, char *response) {
    char *keyword = strtok(NULL, "|");

    if (!keyword) {
        snprintf(response, BUFFER_SIZE, "Erro: palavra-chave em falta.");
        return;
    }

    Document *docs = db_get_all();
    int total = db_count();

    char temp[BUFFER_SIZE * 2] = "";
    for (int i = 0; i < total; i++) {
        int pipefd[2];
        pipe(pipefd);
        pid_t pid = fork();

        if (pid == 0) {
            // Filho
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            execlp("grep", "grep", "-q", keyword, docs[i].path, NULL);
            exit(1);
        } else {
            close(pipefd[1]);
            wait(NULL);
            int status;
            if (read(pipefd[0], response, 1) == 0) { // grep -q não imprime
                snprintf(temp + strlen(temp), sizeof(temp) - strlen(temp),
                         "Documento %d (%s) contém a palavra-chave.\n", docs[i].id, docs[i].title);
            }
            close(pipefd[0]);
        }
    }

    if (strlen(temp) == 0)
        snprintf(response, BUFFER_SIZE, "Nenhum documento contém a palavra-chave.");
    else
        snprintf(response, BUFFER_SIZE, "%s", temp);
}

void handle_search_parallel(char *data, char *response) {
    char *keyword = strtok(NULL, "|");
    char *n_str = strtok(NULL, "|");

    if (!keyword || !n_str) {
        snprintf(response, BUFFER_SIZE, "Erro: argumentos em falta.");
        return;
    }

    int n_proc = atoi(n_str);
    if (n_proc <= 0) n_proc = 1;

    Document *docs = db_get_all();
    int total = db_count();
    int per_proc = (total + n_proc - 1) / n_proc;  // divisão arredondada para cima

    int pipefd[n_proc][2];
    pid_t pids[n_proc];

    for (int i = 0; i < n_proc; i++) {
        pipe(pipefd[i]);
        pid_t pid = fork();
        if (pid == 0) {
            close(pipefd[i][0]); // fecha leitura
            dup2(pipefd[i][1], STDOUT_FILENO);
            close(pipefd[i][1]);

            int start = i * per_proc;
            int end = (i + 1) * per_proc;
            if (end > total) end = total;

            for (int j = start; j < end; j++) {
                pid_t pid2 = fork();
                if (pid2 == 0) {
                    execlp("grep", "grep", "-q", keyword, docs[j].path, NULL);
                    exit(1);
                }
                int status;
                waitpid(pid2, &status, 0);
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    printf("Documento %d (%s) contém a palavra-chave.\n", docs[j].id, docs[j].title);
                }
            }

            exit(0);
        } else {
            pids[i] = pid;
            close(pipefd[i][1]); // fecha escrita
        }
    }

    char temp[BUFFER_SIZE * 4] = "";
    for (int i = 0; i < n_proc; i++) {
        char buf[BUFFER_SIZE];
        ssize_t n = read(pipefd[i][0], buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            strncat(temp, buf, sizeof(temp) - strlen(temp) - 1);
        }
        close(pipefd[i][0]);
        waitpid(pids[i], NULL, 0);
    }

    if (strlen(temp) == 0)
        snprintf(response, BUFFER_SIZE, "Nenhum documento contém a palavra-chave.");
    else
        snprintf(response, BUFFER_SIZE, "%s", temp);
}