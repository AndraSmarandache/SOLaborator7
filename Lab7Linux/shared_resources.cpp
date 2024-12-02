#include "shared_resources.h"
#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <random>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

QueueElement* sharedQueue = nullptr;
int* queueSize = nullptr;
int* globalVar = nullptr;
int* blackUsing = nullptr;
int* whiteUsing = nullptr;
int* threadsToAdd = nullptr;
int* addedThreads = nullptr;
std::string currentState = "NONE";

sem_t* semaphore = nullptr;
pthread_mutex_t* mutex = nullptr;
pthread_mutex_t* outputMutex = nullptr;
pthread_cond_t* blackEvent = nullptr;
pthread_cond_t* whiteEvent = nullptr;
pthread_cond_t* startProcessingEvent = nullptr;

void initializeSharedMemoryAndSync() {
    int fd = shm_open("/shared_memory", O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open failed");
        exit(1);
    }

    size_t sharedSize = sizeof(int) * 6 + sizeof(QueueElement) * MAX_QUEUE_SIZE +
                        sizeof(sem_t) + sizeof(pthread_mutex_t) * 3 +
                        sizeof(pthread_cond_t) * 3;

    if (ftruncate(fd, sharedSize) == -1) {
        perror("ftruncate failed");
        exit(1);
    }

    void* ptr = mmap(NULL, sharedSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    int* sharedVars = (int*)ptr;
    globalVar = &sharedVars[0];
    blackUsing = &sharedVars[1];
    whiteUsing = &sharedVars[2];
    threadsToAdd = &sharedVars[3];
    addedThreads = &sharedVars[4];
    queueSize = &sharedVars[5];
    sharedQueue = (QueueElement*)(sharedVars + 6);

    void* syncPtr = (void*)(sharedQueue + MAX_QUEUE_SIZE);
    semaphore = (sem_t*)syncPtr;
    syncPtr = (void*)(semaphore + 1);

    mutex = (pthread_mutex_t*)syncPtr;
    syncPtr = (void*)(mutex + 1);

    outputMutex = (pthread_mutex_t*)syncPtr;
    syncPtr = (void*)(outputMutex + 1);

    blackEvent = (pthread_cond_t*)syncPtr;
    syncPtr = (void*)(blackEvent + 1);

    whiteEvent = (pthread_cond_t*)syncPtr;
    syncPtr = (void*)(whiteEvent + 1);

    startProcessingEvent = (pthread_cond_t*)syncPtr;

     if (sem_init(semaphore, 1, 3) == -1) {
        perror("sem_init failed");
        exit(1);
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    if (pthread_mutex_init(mutex, &attr) != 0 ||
        pthread_mutex_init(outputMutex, &attr) != 0) {
        perror("pthread_mutex_init failed");
        exit(1);
    }

    pthread_condattr_t condAttr;
    pthread_condattr_init(&condAttr);
    pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);

    if (pthread_cond_init(blackEvent, &condAttr) != 0 ||
        pthread_cond_init(whiteEvent, &condAttr) != 0 ||
        pthread_cond_init(startProcessingEvent, &condAttr) != 0) {
        perror("pthread_cond_init failed");
        exit(1);
    }
}

void cleanupSharedMemoryAndSync() {
    sem_destroy(semaphore);
    pthread_mutex_destroy(mutex);
    pthread_mutex_destroy(outputMutex);
    pthread_cond_destroy(blackEvent);
    pthread_cond_destroy(whiteEvent);
    pthread_cond_destroy(startProcessingEvent);
    shm_unlink("/shared_memory");
}

void shuffleQueue() {
    static std::random_device rd;
    static std::mt19937 g(rd());

    pthread_mutex_lock(mutex);

    std::cout << "Queue before shuffle:\n";
    for (int i = 0; i < *queueSize; ++i) {
        std::cout << "[" << sharedQueue[i].id << ", " << sharedQueue[i].type << "] ";
    }
    std::cout << "\n";

    std::vector<QueueElement> tempQueue(sharedQueue, sharedQueue + *queueSize);
    std::shuffle(tempQueue.begin(), tempQueue.end(), g);
    std::memcpy(sharedQueue, tempQueue.data(), sizeof(QueueElement) * tempQueue.size());

    std::cout << "Queue after shuffle:\n";
    for (int i = 0; i < *queueSize; ++i) {
        std::cout << "[" << sharedQueue[i].id << ", " << sharedQueue[i].type << "] ";
    }
    std::cout << "\n";

    pthread_mutex_unlock(mutex);
}

void addThreadToQueue(int id, const char* type) {
    pthread_mutex_lock(mutex);

    if (*queueSize < MAX_QUEUE_SIZE) {
        QueueElement elem;
        elem.id = id;
        std::strncpy(elem.type, type, sizeof(elem.type) - 1);
        elem.type[sizeof(elem.type) - 1] = '\0';

        sharedQueue[*queueSize] = elem;
        (*queueSize)++;
        (*addedThreads)++;

        std::cout << "[" << type << " Thread " << id << "] Added to queue.\n";
        std::cout << "Threads to add: " << *threadsToAdd << ", added threads = " << *addedThreads << "\n";

        if (*addedThreads == *threadsToAdd) {
            pthread_cond_broadcast(startProcessingEvent);
        }
    } else {
        std::cerr << "Queue is full. Cannot add thread.\n";
    }

    pthread_mutex_unlock(mutex);
}

void processQueue() {
    while (true) {
        pthread_mutex_lock(mutex);

        if (*queueSize == 0) {
            pthread_mutex_unlock(mutex);
            break;
        }

        std::vector<QueueElement> batch;
        char currentType[10];
        std::strncpy(currentType, sharedQueue[0].type, sizeof(currentType));

        while (*queueSize > 0 && std::strcmp(sharedQueue[0].type, currentType) == 0) {
            batch.push_back(sharedQueue[0]);
            for (int i = 0; i < *queueSize - 1; ++i) {
                sharedQueue[i] = sharedQueue[i + 1];
            }
            (*queueSize)--;
        }

        if (std::strcmp(currentType, "white") == 0 && *blackUsing == 0) {
            *whiteUsing += batch.size();
            currentState = "WHITE";
            pthread_cond_broadcast(whiteEvent);
        } else if (std::strcmp(currentType, "black") == 0 && *whiteUsing == 0) {
            *blackUsing += batch.size();
            currentState = "BLACK";
            pthread_cond_broadcast(blackEvent);
        } else {
            for (const auto& elem : batch) {
                sharedQueue[*queueSize] = elem;
                (*queueSize)++;
            }
            batch.clear();
            pthread_mutex_unlock(mutex);
            usleep(100000);
            continue;
        }

        pthread_mutex_unlock(mutex);

        pthread_mutex_lock(outputMutex);
        for (const auto& elem : batch) {
            std::cout << "[" << elem.type << " Thread " << elem.id << "] is using the resource.\n";
        }
        pthread_mutex_unlock(outputMutex);

        pthread_mutex_lock(mutex);
        for (const auto& elem : batch) {
            (*globalVar)++;
            std::cout << "[" << elem.type << " Thread " << elem.id << "] incremented globalVar to " << *globalVar << ".\n";
        }
        pthread_mutex_unlock(mutex);

        pthread_mutex_lock(outputMutex);
        for (const auto& elem : batch) {
            std::cout << "[" << elem.type << " Thread " << elem.id << "] Leaving the resource.\n";
        }
        pthread_mutex_unlock(outputMutex);

        pthread_mutex_lock(mutex);
        if (std::strcmp(currentType, "white") == 0) {
            *whiteUsing -= batch.size();
        } else if (std::strcmp(currentType, "black") == 0) {
            *blackUsing -= batch.size();
        }
        pthread_mutex_unlock(mutex);
    }

    std::cout << "Queue processing completed.\n";
}
