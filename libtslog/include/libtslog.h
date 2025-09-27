#ifndef LIBTSLOG_H
#define LIBTSLOG_H

#include <stdio.h>
#include <pthread.h>

typedef struct {
    FILE *arquivo;
    pthread_mutex_t mutex;
} logger_t;

/**
 * Inicializa (singleton) o logger.
 * Retorna ponteiro para logger_t ou NULL em erro.
 */
logger_t* log_init(const char *nomeArquivo);

/**
 * Escreve mensagem (thread-safe).
 */
void log_write(logger_t *log, const char *mensagem);

/**
 * Destroi o logger e libera recursos.
 */
void log_destroy(logger_t *log);

#endif // LIBTSLOG_H
