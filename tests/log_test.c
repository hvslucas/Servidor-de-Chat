#include "../libtslog/include/libtslog.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>
#include <stdlib.h>

static atomic_int thread_counter = 0;
#define NUM_THREADS_DEFAULT 3
#define MSGS_PER_THREAD 10

typedef struct {
    logger_t *log;
    int msgs;
} tparam_t;

void *thread_function(void *arg) {
    tparam_t *p = (tparam_t*)arg;
    int my_id = atomic_fetch_add(&thread_counter, 1) + 1;

    for (int i = 0; i < p->msgs; i++) {
        char message[128];
        snprintf(message, sizeof(message), "Thread %d: Log message #%d", my_id, i + 1);
        log_write(p->log, message);
        usleep(1000); // 1ms para intercalar
    }

    return NULL;
}

int main(int argc, char **argv) {
    int nthreads = NUM_THREADS_DEFAULT;
    int msgs = MSGS_PER_THREAD;
    if (argc >= 2) nthreads = atoi(argv[1]);
    if (argc >= 3) msgs = atoi(argv[2]);

    logger_t *log = log_init("saida.log");
    if (log == NULL) {
        fprintf(stderr, "Erro ao inicializar o logger.\n");
        return 1;
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * nthreads);
    tparam_t *params = malloc(sizeof(tparam_t) * nthreads);

    for (int i = 0; i < nthreads; i++) {
        params[i].log = log;
        params[i].msgs = msgs;
        pthread_create(&threads[i], NULL, thread_function, &params[i]);
    }

    for (int i = 0; i < nthreads; i++) {
        pthread_join(threads[i], NULL);
    }

    log_destroy(log);
    free(threads);
    free(params);

    printf("Simulação finalizada. Verifique o arquivo 'saida.log'.\n");
    return 0;
}
