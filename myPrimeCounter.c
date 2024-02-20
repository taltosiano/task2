#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <unistd.h>
#include "queue.h" 



bool isPrime(int num) {
    if (num <= 1) return false;
    if (num <= 3) return true;
    if (num % 2 == 0 || num % 3 == 0) return false;
    
    for (int i = 5; i * i <= num; i += 6) {
        if (num % i == 0 || num % (i + 2) == 0) return false;
    }
    
    return true;

}

atomic_bool signals = false;
atomic_int primeCounter = 0;
queue_t q;

void* threadFunction(void* arg) {
 queue_t *q = (queue_t *) arg;

    while (1) {
        pthread_mutex_lock(&q->lock);
        while (q->head == NULL && !atomic_load_explicit(&signals, memory_order_acquire)) {
            pthread_cond_wait(&q->cond, &q->lock);
        }

        if (atomic_load_explicit(&signals, memory_order_acquire) && q->head == NULL) {
            pthread_mutex_unlock(&q->lock);
            break;
        }


        task_t *task = q->head;
        q->head = q->head->next;
        if (q->head == NULL) {
            q->tail = NULL;
        }
        pthread_mutex_unlock(&q->lock);

        int primes = 0;

        for (int i = 0; i < task->count; ++i) {
            if (isPrime(task->numbers[i])) {
                primes++;
            }
        }
        free(task->numbers);

        free(task);
        atomic_fetch_add(&primeCounter, primes);
    }
    return NULL;}

int main(int argc, char *argv[]) {
    int num_threads = NUM_THREADS; 
    int batch_size = MAX_QUEUE_SIZE; 


    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    int *batch = malloc(batch_size * sizeof(int));
    if (threads == NULL || batch == NULL) {
        exit(EXIT_FAILURE);
    }

    int batch_count = 0; // to count numbers in current batch

    init_queue(&q); // initialize queue

    // run worker in each thread
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, threadFunction, (void *) &q) != 0) {
            return 1;
        }
    }

    int num;
    while (scanf("%d", &num) != EOF) {
        batch[batch_count++] = num;
        if (batch_count == batch_size) {
            enqueue(&q, batch, batch_count);
            batch_count = 0;
        }
    }

    if (batch_count > 0) {
        enqueue(&q, batch, batch_count);
    }

    free(batch);

    atomic_store_explicit(&signals, true, memory_order_release);
    pthread_cond_broadcast(&q.cond);
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("%d total primes.\n", primeCounter);

    free(threads);

    return 0;
}