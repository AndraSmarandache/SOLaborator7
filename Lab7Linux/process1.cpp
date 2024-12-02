#include <iostream>
#include <pthread.h>
#include <vector>
#include <unistd.h>
#include "shared_resources.h"

void* blackThreadFunction(void* arg) {
    int id = *(int*)arg;
    delete (int*)arg;

    addThreadToQueue(id, "black");

    pthread_mutex_lock(mutex);
    while (currentState != "BLACK") {
        pthread_cond_wait(blackEvent, mutex);
    }
    pthread_mutex_unlock(mutex);

    sem_wait(semaphore);

    pthread_mutex_lock(outputMutex);
    std::cout << "[Black Thread " << id << "] Accessing resource.\n";
    pthread_mutex_unlock(outputMutex);

    pthread_mutex_lock(mutex);
    (*globalVar)++;
    pthread_mutex_unlock(mutex);

    sleep(1); 

    sem_post(semaphore);

    pthread_mutex_lock(outputMutex);
    std::cout << "[Black Thread " << id << "] Leaving the resource.\n";
    pthread_mutex_unlock(outputMutex);

    pthread_mutex_lock(mutex);
    (*blackUsing)--;
    pthread_mutex_unlock(mutex);

    return NULL;
}

int main() {
    initializeSharedMemoryAndSync();

    std::vector<pthread_t> threads;
    for (int i = 0; i < 5; ++i) {
        pthread_t thread;
        int* arg = new int(i);
        pthread_create(&thread, NULL, blackThreadFunction, arg);
        threads.push_back(thread);
    }

    for (auto& t : threads) {
        pthread_join(t, NULL);
    }

    cleanupSharedMemoryAndSync();
    std::cout << "Process1 completed.\n";

    return 0;
}
