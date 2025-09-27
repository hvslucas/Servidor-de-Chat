#include "../include/libtslog.h"
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

static logger_t* log_global = NULL;

logger_t* log_init(const char *nomeArquivo) {
    if (log_global != NULL) {
        return log_global;
    }

    logger_t *log = (logger_t*)malloc(sizeof(logger_t));
    if (log == NULL) {
        return NULL;
    }

    log->arquivo = fopen(nomeArquivo, "a");
    if (log->arquivo == NULL) {
        fprintf(stderr, "[ERRO] Não foi possível abrir o arquivo de log: %s\n", nomeArquivo);
        free(log);
        return NULL;
    }

    if (pthread_mutex_init(&log->mutex, NULL) != 0) {
        fprintf(stderr, "[ERRO] Falha ao inicializar mutex.\n");
        fclose(log->arquivo);
        free(log);
        return NULL;
    }

    log_global = log;
    return log_global;
}

void log_write(logger_t *log, const char *mensagem) {
    if (log == NULL || mensagem == NULL) {
        fprintf(stderr, "[ERRO] Logger ou mensagem inválida.\n");
        return;
    }

    // Obter o horário atual
    time_t agora = time(NULL);
    struct tm *info_tempo = localtime(&agora);
    if (info_tempo == NULL) {
        fprintf(stderr, "[ERRO] Falha ao obter horário local.\n");
    }

    char horario[20]; // Formato: "YYYY-MM-DD HH:MM:SS"
    strftime(horario, sizeof(horario), "%Y-%m-%d %H:%M:%S", info_tempo);

    pthread_mutex_lock(&log->mutex);
    fprintf(log->arquivo, "[%s] %s\n", horario, mensagem);
    fflush(log->arquivo);
    pthread_mutex_unlock(&log->mutex);
}

void log_destroy(logger_t *log) {
    if (!log) return;
    pthread_mutex_destroy(&log->mutex);
    fclose(log->arquivo);
    free(log);

    if (log == log_global) {
        log_global = NULL;
    }
}
