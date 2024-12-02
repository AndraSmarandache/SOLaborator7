#include <windows.h>
#include <iostream>
#include "shared_resources.h"

int main() {
    HANDLE sharedMemory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
        sizeof(int) * 6 + sizeof(QueueElement) * MAX_QUEUE_SIZE, "GlobalSharedMemory");
    if (sharedMemory == NULL) {
        std::cerr << "Failed to create shared memory.\n";
        return 1;
    }

    void* sharedMemoryPtr = MapViewOfFile(sharedMemory, FILE_MAP_ALL_ACCESS, 0, 0,
        sizeof(int) * 6 + sizeof(QueueElement) * MAX_QUEUE_SIZE);
    if (sharedMemoryPtr == NULL) {
        std::cerr << "Failed to map shared memory.\n";
        return 1;
    }

    int* sharedVars = (int*)sharedMemoryPtr;
    globalVar = &sharedVars[0];
    blackUsing = &sharedVars[1];
    whiteUsing = &sharedVars[2];
    threadsToAdd = &sharedVars[3];
    addedThreads = &sharedVars[4];
    queueSize = &sharedVars[5];
    sharedQueue = (QueueElement*)(sharedVars + 6);

    *globalVar = 0;
    *blackUsing = 0;
    *whiteUsing = 0;
    *threadsToAdd = 10;
    *addedThreads = 0;
    *queueSize = 0;

    semaphore = CreateSemaphore(NULL, 3, 3, "GlobalSemaphore");
    mutex = CreateMutex(NULL, FALSE, "GlobalMutex");
    outputMutex = CreateMutex(NULL, FALSE, "GlobalOutputMutex");
    whiteEvent = CreateEvent(NULL, TRUE, FALSE, "WhiteEvent");
    blackEvent = CreateEvent(NULL, TRUE, FALSE, "BlackEvent");
    startProcessingEvent = CreateEvent(NULL, TRUE, FALSE, "StartProcessingEvent");

    if (!semaphore || !mutex || !outputMutex || !whiteEvent || !blackEvent || !startProcessingEvent) {
        std::cerr << "Failed to create synchronization objects.\n";
        return 1;
    }

    currentState = "NONE";

    STARTUPINFO si1 = { sizeof(STARTUPINFO) };
    STARTUPINFO si2 = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi1, pi2;

    if (!CreateProcess(".\\Process1.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si1, &pi1)) {
        std::cerr << "Failed to create Process 1.\n";
        return 1;
    }

    if (!CreateProcess(".\\Process2.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si2, &pi2)) {
        std::cerr << "Failed to create Process 2.\n";
        return 1;
    }

    WaitForSingleObject(startProcessingEvent, INFINITE);

    shuffleQueue();
    processQueue(whiteEvent, blackEvent, mutex, outputMutex);

    WaitForSingleObject(pi1.hProcess, INFINITE);
    WaitForSingleObject(pi2.hProcess, INFINITE);

    UnmapViewOfFile(sharedMemoryPtr);
    CloseHandle(sharedMemory);
    CloseHandle(semaphore);
    CloseHandle(mutex);
    CloseHandle(outputMutex);
    CloseHandle(whiteEvent);
    CloseHandle(blackEvent);
    CloseHandle(startProcessingEvent);
    CloseHandle(pi1.hProcess);
    CloseHandle(pi2.hProcess);

    return 0;
}
