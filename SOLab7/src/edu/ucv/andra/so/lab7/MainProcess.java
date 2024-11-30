package edu.ucv.andra.so.lab7;

import java.io.File;
import java.util.List;
import java.util.concurrent.CountDownLatch;

public class MainProcess {
    public static void main(String[] args) {
        try {
            System.out.println("Current directory: " + System.getProperty("user.dir"));

            CountDownLatch latch = new CountDownLatch(2);

            ProcessBuilder process1Builder = new ProcessBuilder("java", "z.Process1");
            process1Builder.directory(new File("bin"));
            Process process1 = process1Builder.start();

            ProcessBuilder process2Builder = new ProcessBuilder("java", "z.Process2");
            process2Builder.directory(new File("bin"));
            Process process2 = process2Builder.start();

            Process1 p1 = new Process1();
            Process2 p2 = new Process2();

            ResourceAccess resourceAccess = new ResourceAccess();

            List<ColoredThread> process1Threads = p1.createWhiteThreads(resourceAccess);
            List<ColoredThread> process2Threads = p2.createBlackThreads(resourceAccess);

            ProcessController processController = new ProcessController(process1Threads, process2Threads, resourceAccess);

            CountDownLatch threadsLatch = new CountDownLatch(process1Threads.size() + process2Threads.size());

            processController.startAllThreads(threadsLatch);

            new Thread(() -> {
                try {
                    process1.waitFor();
                    latch.countDown();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }).start();

            new Thread(() -> {
                try {
                    process2.waitFor();
                    latch.countDown();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }).start();

            latch.await();
            threadsLatch.await();

            System.out.println("Both processes have finished.");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
