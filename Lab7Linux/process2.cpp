#include <iostream>
#include <pthread.h>
#include <vector>
#include <unistd.h>
#include "shared_resources.h"

void* whiteThreadFunction(void* arg) {
    int id = *(int*)arg;
    delete (int*)arg;

    addThreadToQueue(id, "white");

    pthread_mutex_lock(mutex);
    while (currentState != "WHITE") {
        pthread_cond_wait(whiteEvent, mutex);
    }
    pthread_mutex_unlock(mutex);

    sem_wait(semaphore);

    pthread_mutex_lock(outputMutex);
    std::cout << "[White Thread " << id << "] Accessing resource.\n";
    pthread_mutex_unlock(outputMutex);

    pthread_mutex_lock(mutex);
    (*globalVar)++;
    pthread_mutex_unlock(mutex);

    sleep(1); 

    sem_post(semaphore);

    pthread_mutex_lock(outputMutex);
    std::cout << "[White Thread " << id << "] Leaving the resource.\n";
    pthread_mutex_unlock(outputMutex);

    pthread_mutex_lock(mutex);
    (*whiteUsing)--;
    pthread_mutex_unlock(mutex);

    return NULL;
}

int main() {
    initializeSharedMemoryAndSync();

    std::vector<pthread_t> threads;
    for (int i = 0; i < 5; ++i) {
        pthread_t thread;
        int* arg = new int(i);
        pthread_create(&thread, NULL, whiteThreadFunction, arg);
        threads.push_back(thread);
    }

    for (auto& t : threads) {
        pthread_join(t, NULL);
    }

    cleanupSharedMemoryAndSync();
    std::cout << "Process2 completed.\n";

    return 0;
}
