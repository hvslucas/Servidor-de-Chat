#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

typedef struct {
    void **data;
    int capacity;
    int size;
    int front;
    int rear;
    pthread_mutex_t mutex;
    sem_t empty;
    sem_t full;
} ThreadSafeQueue;

ThreadSafeQueue* queue_init(int capacity);
void queue_destroy(ThreadSafeQueue *queue);
int queue_put(ThreadSafeQueue *queue, void *item);
void* queue_get(ThreadSafeQueue *queue);
int queue_size(ThreadSafeQueue *queue);

#endif