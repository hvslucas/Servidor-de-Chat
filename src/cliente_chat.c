#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

int client_socket;
int running = 1;

void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    
    while (running && (bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
        fflush(stdout);
    }
    
    if (bytes_received <= 0) {
        printf("\nConexão com o servidor foi perdida.\n");
        running = 0;
    }
    
    pthread_exit(NULL);
}

int main() {
    struct sockaddr_in server_addr;
    pthread_t recv_thread;
    char buffer[BUFFER_SIZE];
    char nickname[32];
    
    printf("=== Cliente de Chat TCP ===\n");
    
    // Obter nickname
    printf("Digite seu nickname: ");
    fflush(stdout);
    fgets(nickname, sizeof(nickname), stdin);
    nickname[strcspn(nickname, "\n")] = '\0';
    
    // Criar socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }
    
    // Configurar endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    
    // Conectar ao servidor
    printf("\n\nConectando ao servidor %s:%d...\n", SERVER_IP, SERVER_PORT);
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao conectar");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    
    printf("Conectado! Digite 'SAIR' para desconectar.\n\n");
    
    // Enviar nickname
    send(client_socket, nickname, strlen(nickname), 0);
    
    // Criar thread para receber mensagens
    if (pthread_create(&recv_thread, NULL, receive_messages, NULL) != 0) {
        perror("Erro ao criar thread de recebimento");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    
    // Loop para enviar mensagens
    while (running) {
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        
        if (strlen(buffer) == 0) {
            continue;
        }
        
        if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
            perror("Erro ao enviar mensagem");
            break;
        }
        
        if (strcmp(buffer, "SAIR") == 0) {
            running = 0;
            break;
        }
    }
    
    running = 0;
    pthread_join(recv_thread, NULL);
    close(client_socket);
    
    printf("Cliente finalizado.\n");
    return 0;
}