#include <stdio.h>
#include <stdlib.h>
#include "eventbuf.h"
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>

// These will be initialized in main() from the command line.
int producer_count;
int consumer_count;
int events_to_produce;
int outsanding_events;
int complete;
struct eventbuf *eb;
sem_t *items;
sem_t *mutex;
sem_t *spaces;


void produce_event(int arg){
    int event = arg;
    sem_wait(spaces);    
    sem_wait(mutex);
    eventbuf_add(eb, event);
    sem_post(mutex);
    sem_post(items);
}

void *event_producer(void *arg){
    int *id = arg;
    for(int x = 0; x <events_to_produce; x++){
        int event = *id * 100 + x;
        produce_event(event);
        printf("P%d: adding event %d\n", *id, event);
    }
    printf("P%d: exiting\n", *id);
    return NULL;
}

void *event_consumer(void *arg){
    int *id = arg;
    while(1){
        sem_wait(items);
        sem_wait(mutex);
        int event = eventbuf_get(eb); 
        sem_post(mutex);
        if (complete == 1 && eventbuf_empty(eb) == 1){
            sem_post(mutex);
            sem_post(spaces);
            sem_post(items);
            break;
        }
        sem_post(spaces);
        printf("C%d: got event %d\n", *id, event);
    }
    printf("C%d: exiting\n", *id);
    return NULL;
}

sem_t *sem_open_temp(const char *name, int value)
{
    sem_t *sem;

    // Create the semaphore
    if ((sem = sem_open(name, O_CREAT, 0600, value)) == SEM_FAILED)
        return SEM_FAILED;

    // Unlink it so it will go away after this process exits
    if (sem_unlink(name) == -1) {
        sem_close(sem);
        return SEM_FAILED;
    }

    return sem;
}

int main(int argc, char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "usage: events producer_count consumer_count events_to_produce outstanding_events\n");
        exit(1);
    }

    producer_count = atoi(argv[1]);
    consumer_count = atoi(argv[2]);
    events_to_produce = atoi(argv[3]);
    outsanding_events = atoi(argv[4]);
    complete = 0;

    eb = eventbuf_create();
    items = sem_open_temp("my_sem_items", 0);
    mutex = sem_open_temp("my_sem_mutex", 1);
    spaces = sem_open_temp("my_sem_spaces", outsanding_events);

    pthread_t *producer_thread = calloc(producer_count, sizeof *producer_thread);
    int *producer_thread_id = calloc(producer_count, sizeof *producer_thread_id);
    for (int i = 0; i < producer_count; i++) {
        producer_thread_id[i] = i;
        pthread_create(producer_thread + i, NULL, event_producer, producer_thread_id + i);
    }

    pthread_t *consumer_thread = calloc(consumer_count, sizeof *consumer_thread);
    int *consumer_thread_id = calloc(consumer_count, sizeof *consumer_thread_id);
    for (int i = 0; i < consumer_count; i++) {
        consumer_thread_id[i] = i;
        pthread_create(consumer_thread + i, NULL, event_consumer, consumer_thread_id + i);
    }

    for (int i = 0; i < producer_count; i++)
        pthread_join(producer_thread[i], NULL);

    complete = 1;
    sem_post(spaces);
    sem_post(items);
    sem_post(mutex);

    for (int i = 0; i < consumer_count; i++)
        pthread_join(consumer_thread[i], NULL);

    eventbuf_free(eb);
}

