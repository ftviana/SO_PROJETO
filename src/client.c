#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include "client.h"

void send_request(Request *req) {
    // Criar FIFO do cliente antes de tudo
    mkfifo(FIFO_CLIENT, 0666);

    // Enviar pedido ao servidor
    int fd = open(FIFO_SERVER, O_WRONLY);
    if (fd == -1) {
        perror("Erro ao abrir FIFO do servidor");
        exit(1);
    }
    write(fd, req, sizeof(Request));
    close(fd);

    // Esperar resposta do servidor
    fd = open(FIFO_CLIENT, O_RDONLY);
    if (fd == -1) {
        perror("Erro ao abrir FIFO do cliente");
        exit(1);
    }
    char buffer[MAX_MSG];
    int bytes = read(fd, buffer, MAX_MSG);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("%s", buffer);
    }
    close(fd);
    unlink(FIFO_CLIENT);
}