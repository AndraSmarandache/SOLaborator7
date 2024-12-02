#include <windows.h>
#include <iostream>
#include <thread>
#include <vector>
#include "shared_resources.h"

void whiteThreadFunction(int id) {
    addThreadToQueue(id, "white");

    WaitForSingleObject(startProcessingEvent, INFINITE);

    WaitForSingleObject(mutex, INFINITE);
    if (*queueSize == 0 || *blackUsing > 0) {
        SetEvent(whiteEvent);
    }
    ReleaseMutex(mutex);
}


void initializeSharedMemoryAndEvents() {
    HANDLE sharedMemory = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "GlobalSharedMemory");
    if (sharedMemory == NULL) {
        std::cerr << "Failed to open shared memory.\n";
        exit(1);
    }

    void* sharedMemoryPtr = MapViewOfFile(sharedMemory, FILE_MAP_ALL_ACCESS, 0, 0,
        sizeof(int) * 6 + sizeof(QueueElement) * MAX_QUEUE_SIZE);
    if (sharedMemoryPtr == NULL) {
        std::cerr << "Failed to map shared memory.\n";
        exit(1);
    }

    int* sharedVars = (int*)sharedMemoryPtr;
    globalVar = &sharedVars[0];
    blackUsing = &sharedVars[1];
    whiteUsing = &sharedVars[2];
    threadsToAdd = &sharedVars[3];
    addedThreads = &sharedVars[4];
    queueSize = &sharedVars[5];
    sharedQueue = (QueueElement*)(sharedVars + 6);

    semaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, "GlobalSemaphore");
    mutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "GlobalMutex");
    outputMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "GlobalOutputMutex");
    blackEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, "BlackEvent");
    whiteEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, "WhiteEvent");
    startProcessingEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, "StartProcessingEvent");
}

void cleanupSharedMemory() {
    UnmapViewOfFile(sharedQueue);
    CloseHandle(semaphore);
    CloseHandle(mutex);
    CloseHandle(outputMutex);
    CloseHandle(blackEvent);
    CloseHandle(whiteEvent);
    CloseHandle(startProcessingEvent);
}

int main() {
    initializeSharedMemoryAndEvents();

    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(whiteThreadFunction, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    cleanupSharedMemory();
    return 0;
}
