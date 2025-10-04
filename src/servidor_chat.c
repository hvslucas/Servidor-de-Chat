#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../include/libtslog.h"

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int socket;
    struct sockaddr_in address;
    char nickname[32];
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
logger_t *logger;

void broadcast_message(const char *message, int sender_socket) {
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->socket != sender_socket) {
            if (send(clients[i]->socket, message, strlen(message), 0) < 0) {
                perror("Erro ao enviar broadcast");
            }
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

void add_client(client_t *client) {
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i]) {
            clients[i] = client;
            break;
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int socket) {
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->socket == socket) {
            free(clients[i]);
            clients[i] = NULL;
            break;
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

void print_client_info(client_t *client) {
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client->address.sin_addr, client_ip, INET_ADDRSTRLEN);
    printf("Cliente: %s (%s:%d) - Socket: %d\n", 
           client->nickname, client_ip, ntohs(client->address.sin_port), client->socket);
}

void *handle_client(void *arg) {
    client_t *client = (client_t *)arg;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE + 64];
    
    // Receber nickname
    int bytes_received = recv(client->socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0) {
        remove_client(client->socket);
        pthread_exit(NULL);
    }
    
    buffer[bytes_received] = '\0';
    strncpy(client->nickname, buffer, sizeof(client->nickname) - 1);
    
    // Log de conexão
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), "Cliente conectado: %s (socket: %d)", 
             client->nickname, client->socket);
    log_write(logger, log_msg);
    
    // Imprimir no console do servidor
    printf("--------------------------------------------------\n");
    printf("NOVO CLIENTE CONECTADO:\n");
    print_client_info(client);
    printf("Total de clientes conectados: ");

    pthread_mutex_lock(&clients_mutex);
    int count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) count++;
    }
    pthread_mutex_unlock(&clients_mutex);
    
    printf("%d\n", count);
    printf("--------------------------------------------------\n");
    
    // Broadcast de entrada
    snprintf(message, sizeof(message), "[SERVIDOR] %s entrou no chat!\n", client->nickname);
    broadcast_message(message, client->socket);
    
    // Loop de mensagens
    while ((bytes_received = recv(client->socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        
        // Log da mensagem
        snprintf(log_msg, sizeof(log_msg), "Mensagem de %s: %s", client->nickname, buffer);
        log_write(logger, log_msg);
        
        // Imprimir mensagem no console do servidor
        printf("[MENSAGEM RECEBIDA] %s: %s\n", client->nickname, buffer);
        
        // Se mensagem for "SAIR", desconectar
        if (strcmp(buffer, "SAIR") == 0) {
            break;
        }
        
        // Broadcast da mensagem
        snprintf(message, sizeof(message), "[%s] %s\n", client->nickname, buffer);
        broadcast_message(message, client->socket);
    }
    
    // Broadcast de saída
    snprintf(message, sizeof(message), "[SERVIDOR] %s saiu do chat!\n", client->nickname);
    broadcast_message(message, client->socket);
    
    // Log de desconexão
    snprintf(log_msg, sizeof(log_msg), "Cliente desconectado: %s", client->nickname);
    log_write(logger, log_msg);
    
    // Imprimir desconexão no console do servidor
    printf("\nCLIENTE DESCONECTADO:\n");
    print_client_info(client);
    
    pthread_mutex_lock(&clients_mutex);
    count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) count++;
    }
    pthread_mutex_unlock(&clients_mutex);
    
    printf("Total de clientes conectados: %d\n\n", count);
    
    close(client->socket);
    remove_client(client->socket);
    
    pthread_exit(NULL);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    pthread_t thread_id;
    
    // Inicializar logger
    logger = log_init("servidor_chat.log");
    if (!logger) {
        fprintf(stderr, "Erro ao inicializar logger\n");
        exit(EXIT_FAILURE);
    }
    
    log_write(logger, "Servidor de chat iniciado");
    
    // Criar socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }
    
    // Configurar socket para reutilizar endereço
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Configurar endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro no bind");
        exit(EXIT_FAILURE);
    }
    
    // Listen
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Erro no listen");
        exit(EXIT_FAILURE);
    }
    
    printf("=== Servidor de Chat TCP ===\n");
    printf("Servidor ouvindo na porta %d...\n", PORT);
    printf("Logs sendo salvos em: servidor_chat.log\n\n");
    
    // Inicializar lista de clientes
    memset(clients, 0, sizeof(clients));
    
    // Loop principal - aceitar conexões
    while (1) {
        client_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket < 0) {
            perror("Erro ao aceitar conexão");
            continue;
        }
        
        // Criar novo cliente
        client_t *new_client = malloc(sizeof(client_t));
        new_client->socket = client_socket;
        new_client->address = client_addr;
        memset(new_client->nickname, 0, sizeof(new_client->nickname));
        
        add_client(new_client);
        
        // Criar thread para o cliente
        if (pthread_create(&thread_id, NULL, handle_client, (void*)new_client) != 0) {
            perror("Erro ao criar thread");
            free(new_client);
            continue;
        }
        
        pthread_detach(thread_id);
    }
    
    close(server_socket);
    log_destroy(logger);
    return 0;
}