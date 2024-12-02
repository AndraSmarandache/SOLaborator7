#ifndef SHARED_RESOURCES_H
#define SHARED_RESOURCES_H

#include <windows.h>
#include <string>

#define MAX_QUEUE_SIZE 100

struct QueueElement {
    int id;
    char type[10]; 
};

extern QueueElement* sharedQueue;
extern int* queueSize;
extern int* globalVar;
extern int* blackUsing;
extern int* whiteUsing;
extern int* threadsToAdd;
extern int* addedThreads;
extern std::string currentState;

extern HANDLE semaphore;
extern HANDLE mutex;
extern HANDLE outputMutex;
extern HANDLE blackEvent;
extern HANDLE whiteEvent;
extern HANDLE startProcessingEvent;

void shuffleQueue();
void addThreadToQueue(int id, const char* type);
void processQueue(HANDLE whiteEvent, HANDLE blackEvent, HANDLE mutex, HANDLE outputMutex);

#endif // SHARED_RESOURCES_H
