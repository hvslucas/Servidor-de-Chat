#include "../include/thread_safe_queue.h"

ThreadSafeQueue* queue_init(int capacity) {
    ThreadSafeQueue *queue = (ThreadSafeQueue*)malloc(sizeof(ThreadSafeQueue));
    queue->data = (void**)malloc(sizeof(void*) * capacity);
    queue->capacity = capacity;
    queue->size = 0;
    queue->front = 0;
    queue->rear = 0;
    
    pthread_mutex_init(&queue->mutex, NULL);
    sem_init(&queue->empty, 0, capacity);
    sem_init(&queue->full, 0, 0);
    
    return queue;
}

void queue_destroy(ThreadSafeQueue *queue) {
    if (!queue) return;
    
    pthread_mutex_destroy(&queue->mutex);
    sem_destroy(&queue->empty);
    sem_destroy(&queue->full);
    free(queue->data);
    free(queue);
}

int queue_put(ThreadSafeQueue *queue, void *item) {
    sem_wait(&queue->empty);
    pthread_mutex_lock(&queue->mutex);
    
    queue->data[queue->rear] = item;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->size++;
    
    pthread_mutex_unlock(&queue->mutex);
    sem_post(&queue->full);
    
    return 0;
}

void* queue_get(ThreadSafeQueue *queue) {
    sem_wait(&queue->full);
    pthread_mutex_lock(&queue->mutex);
    
    void *item = queue->data[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;
    
    pthread_mutex_unlock(&queue->mutex);
    sem_post(&queue->empty);
    
    return item;
}

int queue_size(ThreadSafeQueue *queue) {
    pthread_mutex_lock(&queue->mutex);
    int size = queue->size;
    pthread_mutex_unlock(&queue->mutex);
    return size;
}