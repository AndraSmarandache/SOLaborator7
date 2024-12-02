#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include "shared_resources.h"

int main() {
    initializeSharedMemoryAndSync();

    *globalVar = 0;
    *blackUsing = 0;
    *whiteUsing = 0;
    *threadsToAdd = 10;
    *addedThreads = 0;
    *queueSize = 0;

    currentState = "NONE";

    pid_t pid1 = fork();
    if (pid1 == 0) {
        execl("./process1", "./process1", NULL);
        perror("execl failed for process1");
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        execl("./process2", "./process2", NULL);
        perror("execl failed for process2");
        exit(1);
    }

    pthread_mutex_lock(mutex);
    while (*addedThreads < *threadsToAdd) {
        pthread_mutex_unlock(mutex);
        usleep(100000); 
        pthread_mutex_lock(mutex);
    }
    pthread_mutex_unlock(mutex);

    std::cout << "All threads added. Starting queue processing...\n";

    shuffleQueue();
    processQueue();

    waitpid(pid1, NULL, 0);
    std::cout << "Process1 completed.\n";

    waitpid(pid2, NULL, 0);
    std::cout << "Process2 completed.\n";

    cleanupSharedMemoryAndSync();

    std::cout << "Queue processing completed. Exiting...\n";

    return 0;
}
