package edu.ucv.andra.so.lab7;


import java.util.List;
import java.util.concurrent.CountDownLatch;

public class ProcessController {

    private final List<ColoredThread> whiteThreads;
    private final List<ColoredThread> blackThreads;
    private final ResourceAccess resourceAccess;

    public ProcessController(List<ColoredThread> whiteThreads, List<ColoredThread> blackThreads, ResourceAccess resourceAccess) {
        this.whiteThreads = whiteThreads;
        this.blackThreads = blackThreads;
        this.resourceAccess = resourceAccess;
    }

    public void startAllThreads(CountDownLatch latch) {
        for (ColoredThread thread : whiteThreads) {
            new Thread(() -> {
                try {
                    thread.run();
                } finally {
                    latch.countDown();
                }
            }).start();
        }

        for (ColoredThread thread : blackThreads) {
            new Thread(() -> {
                try {
                    thread.run();
                } finally {
                    latch.countDown();
                }
            }).start();
        }
    }
}