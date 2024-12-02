#include "shared_resources.h"
#include <algorithm>
#include <random>
#include <cstring>
#include <iostream>

QueueElement* sharedQueue = nullptr;
int* queueSize = nullptr;
int* globalVar = nullptr;
int* blackUsing = nullptr;
int* whiteUsing = nullptr;
int* threadsToAdd = nullptr;
int* addedThreads = nullptr;
std::string currentState = "NONE";

HANDLE semaphore = NULL;
HANDLE mutex = NULL;
HANDLE outputMutex = NULL;
HANDLE blackEvent = NULL;
HANDLE whiteEvent = NULL;
HANDLE startProcessingEvent = NULL;

void shuffleQueue() {
    static std::random_device rd;
    static std::mt19937 g(rd());

    WaitForSingleObject(mutex, INFINITE);

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

    ReleaseMutex(mutex);
}

void processQueue(HANDLE whiteEvent, HANDLE blackEvent, HANDLE mutex, HANDLE outputMutex) {
    while (true) {
        WaitForSingleObject(mutex, INFINITE);

        if (*queueSize == 0) {
            ReleaseMutex(mutex);
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
            SetEvent(whiteEvent);
        }
        else if (std::strcmp(currentType, "black") == 0 && *whiteUsing == 0) {
            *blackUsing += batch.size();
            SetEvent(blackEvent);
        }
        else {
            for (const auto& elem : batch) {
                sharedQueue[*queueSize] = elem;
                (*queueSize)++;
            }
            batch.clear();
            ReleaseMutex(mutex);
            Sleep(100); 
            continue;
        }

        ReleaseMutex(mutex);

        WaitForSingleObject(outputMutex, INFINITE);
        for (const auto& elem : batch) {
            std::cout << "[" << elem.type << " Thread " << elem.id << "] is using the resource.\n";
        }
        ReleaseMutex(outputMutex);

        for (const auto& elem : batch) {
            WaitForSingleObject(mutex, INFINITE);
            (*globalVar)++;
            std::cout << "[" << elem.type << " Thread " << elem.id << "] incremented globalVar to " << *globalVar << ".\n";
            ReleaseMutex(mutex);
        }

        WaitForSingleObject(outputMutex, INFINITE);
        for (const auto& elem : batch) {
            std::cout << "[" << elem.type << " Thread " << elem.id << "] left the resource.\n";
        }
        ReleaseMutex(outputMutex);

        WaitForSingleObject(mutex, INFINITE);
        if (std::strcmp(currentType, "white") == 0) {
            *whiteUsing -= batch.size();
            if (*whiteUsing == 0) {
                ResetEvent(whiteEvent);
            }
        }
        else if (std::strcmp(currentType, "black") == 0) {
            *blackUsing -= batch.size();
            if (*blackUsing == 0) {
                ResetEvent(blackEvent);
            }
        }
        ReleaseMutex(mutex);
    }
}

void addThreadToQueue(int id, const char* type) {
    WaitForSingleObject(mutex, INFINITE);

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
            SetEvent(startProcessingEvent);
            std::cout << "All threads added. startProcessingEvent signaled.\n";
        }
    }
    else {
        std::cerr << "Queue is full. Cannot add thread.\n";
    }

    ReleaseMutex(mutex);
}
