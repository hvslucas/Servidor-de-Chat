#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include "../include/libtslog.h"
#include "../include/thread_safe_queue.h"

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MAX_HISTORY 100
#define MESSAGE_QUEUE_CAPACITY 100

typedef struct {
    int socket;
    struct sockaddr_in address;
    char nickname[32];
    int active;
} client_t;

// Estrutura para mensagens na queue
typedef struct {
    char message[BUFFER_SIZE];
    int sender_socket;
    int is_system_message; // Flag para mensagens do sistema
} message_data_t;

// Estruturas compartilhadas
client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
char message_history[MAX_HISTORY][BUFFER_SIZE];
int history_count = 0;
pthread_mutex_t history_mutex = PTHREAD_MUTEX_INITIALIZER;

// Queue para mensagens e variáveis de controle
ThreadSafeQueue *message_queue;
pthread_t message_worker_thread;
int message_worker_running = 0;

logger_t *logger;
int server_running = 1;

// Função para adicionar mensagem ao histórico
void add_to_history(const char *message) {
    pthread_mutex_lock(&history_mutex);
    
    if (history_count < MAX_HISTORY) {
        strncpy(message_history[history_count], message, BUFFER_SIZE - 1);
        message_history[history_count][BUFFER_SIZE - 1] = '\0';
        history_count++;
    } else {
        // Desloca o histórico (FIFO)
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            strcpy(message_history[i], message_history[i + 1]);
        }
        strncpy(message_history[MAX_HISTORY - 1], message, BUFFER_SIZE - 1);
        message_history[MAX_HISTORY - 1][BUFFER_SIZE - 1] = '\0';
    }
    
    pthread_mutex_unlock(&history_mutex);
}

// Função para enviar histórico para novo cliente
void send_history(int client_socket) {
    pthread_mutex_lock(&history_mutex);
    
    if (history_count > 0) {
        char history_msg[BUFFER_SIZE + 50];
        snprintf(history_msg, sizeof(history_msg), "[SERVIDOR] Últimas %d mensagens:\n", history_count);
        send(client_socket, history_msg, strlen(history_msg), 0);
        
        for (int i = 0; i < history_count; i++) {
            send(client_socket, message_history[i], strlen(message_history[i]), 0);
        }
    }
    
    pthread_mutex_unlock(&history_mutex);
}

// Função de broadcast síncrono (usada pela thread worker)
void broadcast_message_sync(const char *message, int sender_socket, int is_system_message) {
    pthread_mutex_lock(&clients_mutex);
    
    // Adiciona ao histórico (exceto mensagens do sistema)
    if (!is_system_message && strstr(message, "[SERVIDOR]") == NULL) {
        add_to_history(message);
    }
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->socket != sender_socket && clients[i]->active) {
            if (send(clients[i]->socket, message, strlen(message), 0) < 0) {
                perror("Erro ao enviar broadcast");
                clients[i]->active = 0; // Marca como inativo
            }
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

// Função de broadcast assíncrono (coloca na queue)
void broadcast_message(const char *message, int sender_socket, int is_system_message) {
    if (!message_worker_running) {
        // Fallback síncrono se a thread worker não estiver rodando
        broadcast_message_sync(message, sender_socket, is_system_message);
        return;
    }
    
    message_data_t *msg_data = malloc(sizeof(message_data_t));
    if (!msg_data) {
        perror("Erro ao alocar mensagem para queue");
        return;
    }
    
    strncpy(msg_data->message, message, BUFFER_SIZE - 1);
    msg_data->message[BUFFER_SIZE - 1] = '\0';
    msg_data->sender_socket = sender_socket;
    msg_data->is_system_message = is_system_message;
    
    if (queue_put(message_queue, msg_data) != 0) {
        perror("Erro ao colocar mensagem na queue");
        free(msg_data);
    }
}

// Thread worker para processar mensagens da queue
void *message_worker(void *arg) {
    printf("Thread worker de mensagens iniciada\n");
    log_write(logger, "Thread worker de mensagens iniciada");
    
    while (message_worker_running || queue_size(message_queue) > 0) {
        message_data_t *msg_data = queue_get(message_queue);
        if (msg_data) {
            broadcast_message_sync(msg_data->message, msg_data->sender_socket, 
                                 msg_data->is_system_message);
            free(msg_data);
        }
    }
    
    printf("Thread worker de mensagens finalizada\n");
    log_write(logger, "Thread worker de mensagens finalizada");
    pthread_exit(NULL);
}

void add_client(client_t *client) {
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i]) {
            clients[i] = client;
            clients[i]->active = 1;
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

void cleanup_clients() {
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) {
            close(clients[i]->socket);
            free(clients[i]);
            clients[i] = NULL;
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

int get_active_clients_count() {
    pthread_mutex_lock(&clients_mutex);
    int count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->active) count++;
    }
    pthread_mutex_unlock(&clients_mutex);
    return count;
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
    client->nickname[sizeof(client->nickname) - 1] = '\0';
    
    // Log de conexão
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), "Cliente conectado: %s (socket: %d)", 
             client->nickname, client->socket);
    log_write(logger, log_msg);
    
    // Imprimir no console do servidor
    printf("--------------------------------------------------\n");
    printf("NOVO CLIENTE CONECTADO:\n");
    print_client_info(client);
    
    // Obter contagem de clientes ANTES do broadcast
    int client_count = get_active_clients_count();
    printf("Total de clientes conectados: %d\n", client_count);
    printf("--------------------------------------------------\n");
    
    // Enviar histórico para o novo cliente
    send_history(client->socket);
    
    // Broadcast de entrada (mensagem do sistema)
    snprintf(message, sizeof(message), "[SERVIDOR] %s entrou no chat! (%d usuários online)\n", 
             client->nickname, client_count);
    broadcast_message(message, client->socket, 1); // 1 = system message
    
    // Loop de mensagens
    while (server_running && (bytes_received = recv(client->socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        
        // Log da mensagem
        snprintf(log_msg, sizeof(log_msg), "Mensagem de %s: %s", client->nickname, buffer);
        log_write(logger, log_msg);
        
        // Imprimir mensagem no console do servidor
        printf("[MENSAGEM RECEBIDA] %s: %s", client->nickname, buffer);
        if (buffer[strlen(buffer)-1] != '\n') printf("\n");
        
        // Se mensagem for "SAIR", desconectar
        if (strcmp(buffer, "SAIR") == 0) {
            break;
        }
        
        // Broadcast da mensagem (mensagem normal)
        snprintf(message, sizeof(message), "[%s] %s\n", client->nickname, buffer);
        broadcast_message(message, client->socket, 0); // 0 = normal message
    }
    
    // Broadcast de saída (mensagem do sistema)
    int remaining_clients = get_active_clients_count() - 1;
    snprintf(message, sizeof(message), "[SERVIDOR] %s saiu do chat! (%d usuários online)\n", 
             client->nickname, remaining_clients);
    broadcast_message(message, client->socket, 1); // 1 = system message
    
    // Log de desconexão
    snprintf(log_msg, sizeof(log_msg), "Cliente desconectado: %s", client->nickname);
    log_write(logger, log_msg);
    
    // Imprimir desconexão no console do servidor
    printf("\nCLIENTE DESCONECTADO:\n");
    print_client_info(client);
    printf("Total de clientes conectados: %d\n\n", remaining_clients);
    
    close(client->socket);
    remove_client(client->socket);
    
    pthread_exit(NULL);
}

void signal_handler(int sig) {
    printf("\nRecebido sinal %d. Finalizando servidor...\n", sig);
    server_running = 0;
    message_worker_running = 0;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    pthread_t thread_id;
    
    // Configurar handler de sinais para graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Inicializar logger
    logger = log_init("servidor_chat.log");
    if (!logger) {
        fprintf(stderr, "Erro ao inicializar logger\n");
        exit(EXIT_FAILURE);
    }
    
    log_write(logger, "Servidor de chat iniciado");
    
    // Inicializar queue de mensagens
    message_queue = queue_init(MESSAGE_QUEUE_CAPACITY);
    if (!message_queue) {
        fprintf(stderr, "Erro ao inicializar queue de mensagens\n");
        log_write(logger, "Erro ao inicializar queue de mensagens");
        exit(EXIT_FAILURE);
    }
    
    // Iniciar thread worker para mensagens
    message_worker_running = 1;
    if (pthread_create(&message_worker_thread, NULL, message_worker, NULL) != 0) {
        perror("Erro ao criar thread worker de mensagens");
        log_write(logger, "Erro ao criar thread worker de mensagens");
        queue_destroy(message_queue);
        exit(EXIT_FAILURE);
    }
    
    // Criar socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Erro ao criar socket");
        log_write(logger, "Erro ao criar socket do servidor");
        message_worker_running = 0;
        pthread_join(message_worker_thread, NULL);
        queue_destroy(message_queue);
        exit(EXIT_FAILURE);
    }
    
    // Configurar socket para reutilizar endereço
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Erro ao configurar socket options");
        log_write(logger, "Erro ao configurar SO_REUSEADDR");
    }
    
    // Configurar endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro no bind");
        log_write(logger, "Erro no bind do servidor");
        close(server_socket);
        message_worker_running = 0;
        pthread_join(message_worker_thread, NULL);
        queue_destroy(message_queue);
        exit(EXIT_FAILURE);
    }
    
    // Listen
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Erro no listen");
        log_write(logger, "Erro no listen do servidor");
        close(server_socket);
        message_worker_running = 0;
        pthread_join(message_worker_thread, NULL);
        queue_destroy(message_queue);
        exit(EXIT_FAILURE);
    }
    
    printf("=== Servidor de Chat TCP ===\n");
    printf("Servidor ouvindo na porta %d...\n", PORT);
    printf("Arquitetura: Thread worker + Queue de mensagens\n");
    printf("Logs sendo salvos em: servidor_chat.log\n");
    printf("Pressione Ctrl+C para finalizar o servidor\n\n");
    
    // Inicializar lista de clientes
    memset(clients, 0, sizeof(clients));
    
    // Loop principal - aceitar conexões
    while (server_running) {
        client_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (!server_running) break;
        
        if (client_socket < 0) {
            if (server_running) {
                perror("Erro ao aceitar conexão");
                log_write(logger, "Erro ao aceitar conexão de cliente");
            }
            continue;
        }
        
        // Verificar se há slots disponíveis
        if (get_active_clients_count() >= MAX_CLIENTS) {
            char *msg = "[SERVIDOR] Servidor cheio. Tente novamente mais tarde.\n";
            send(client_socket, msg, strlen(msg), 0);
            close(client_socket);
            log_write(logger, "Conexão rejeitada - servidor cheio");
            continue;
        }
        
        // Criar novo cliente
        client_t *new_client = malloc(sizeof(client_t));
        if (!new_client) {
            perror("Erro ao alocar memória para cliente");
            close(client_socket);
            continue;
        }
        
        new_client->socket = client_socket;
        new_client->address = client_addr;
        memset(new_client->nickname, 0, sizeof(new_client->nickname));
        new_client->active = 1;
        
        add_client(new_client);
        
        // Criar thread para o cliente
        if (pthread_create(&thread_id, NULL, handle_client, (void*)new_client) != 0) {
            perror("Erro ao criar thread");
            log_write(logger, "Erro ao criar thread para cliente");
            free(new_client);
            remove_client(client_socket);
            continue;
        }
        
        pthread_detach(thread_id);
        
        // Pequena pausa para garantir que a thread inicie
        usleep(10000); // 10ms
    }
    
    // Cleanup graceful
    printf("\nFinalizando servidor...\n");
    log_write(logger, "Servidor finalizando");
    
    // Parar thread worker e aguardar
    message_worker_running = 0;
    printf("Aguardando thread worker finalizar...\n");
    pthread_join(message_worker_thread, NULL);
    
    cleanup_clients();
    close(server_socket);
    
    // Destruir queue
    if (message_queue) {
        queue_destroy(message_queue);
    }
    
    log_destroy(logger);
    
    printf("Servidor finalizado com sucesso.\n");
    return 0;
}